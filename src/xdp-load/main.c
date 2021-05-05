#include<stdio.h>
#include<string.h>

#include<fcntl.h>
#include<linux/if_link.h> /* XDP_FLAGS_SKB_MODE */
#include<net/if.h>        /* if_nametoindex */
#include<signal.h>        /* signal, SIGINT, SIGTERM */
#include<termios.h>
#include<unistd.h>

#include<bpf/bpf.h>
#include<bpf/libbpf.h>

#include<sys/epoll.h>
#include<sys/timerfd.h>
#include<sys/resource.h>

#include<netlink/genl/genl.h>
#include<netlink/genl/ctrl.h>

#include<lua.h>
#include<lauxlib.h>
#include<lualib.h>
#include<genl-echo/echo.h>

#define ENSURE(x, action, ...) \
	do { if (!(x)) { fprintf(stderr, __VA_ARGS__); action; }} while (0)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

struct app_s {
	struct { int fd; } epoll;
	struct { int fd; } timer;
	struct { int fd; } input;
	struct {
		int fd, family;
		struct nl_sock *sk;
	} nl;
	struct {
		int fd, ifi, flags;
		struct bpf_object *o;
	} xdp;
	lua_State *L;
} *g_app; /* NOTE: global because signal */

static int set_nonblock(int fd)
{
	return fcntl(fd, F_SETFD, (fcntl(fd, F_GETFD)|O_NONBLOCK));
}

static int setrlimit_inifinity(void)
{
	struct rlimit rlim_new = {
		.rlim_cur = RLIM_INFINITY,
		.rlim_max = RLIM_INFINITY,
	};

	return setrlimit(RLIMIT_MEMLOCK, &rlim_new);
}

static int app_epoll_ctl(struct app_s *me, int op, int fd, void *ptr)
{
	struct epoll_event e = {
		.events   = op,
		.data.ptr = ptr,
	};
	return epoll_ctl(me->epoll.fd, EPOLL_CTL_ADD, fd, &e);
}

static void app_input_prompt()
{
	printf("> ");
	fflush(stdout);
}

static int echo_req(struct nl_msg *msg, int id, const char *s)
{
	genlmsg_put(msg
	           ,NL_AUTO_PORT
		   ,NL_AUTO_SEQ
		   ,id
		   ,0
	           ,NLM_F_REQUEST
		   ,ECHO_A_MSG
		   ,ECHO_VERSION_NR);
	NLA_PUT_STRING(msg, ECHO_C_ECHO, s);
	return 0;

nla_put_failure:
	free(msg);
	return -1;
}

static void app_input_on_gets(struct app_s *me, char *s, size_t n)
{
#if 1
	//printf("sending: %s\n", s);
	struct nl_msg *msg = nlmsg_alloc();
	if (echo_req(msg, me->nl.family, s) == 0)
		nl_send_auto(me->nl.sk, msg);
#else
	printf("lua:%s\n", s);
	printf("%d\n", luaL_dostring(me->L, s));
#endif
}

static int app_timer_set_s(struct app_s *me, int s)
{
	struct itimerspec tick = {
		.it_interval = {s, 0},
		.it_value    = {s, 0},
	};
	return timerfd_settime(me->timer.fd, 0, &tick, 0);
}

static void app_timer_on_tick(struct app_s *me)
{
	printf(".");
	fflush(stdout);
}

static int app_on_genl_msg(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct genlmsghdr *gnlh = genlmsg_hdr(nlh);
	struct app_s *me = arg;

	switch (gnlh->cmd) {
	case ECHO_C_ECHO: {
		struct nlattr *attrs[ECHO_A_COUNT];
		if (genlmsg_parse(nlh, 0, attrs, ECHO_A_MAX, NULL))
			return NL_SKIP;
		const char *msg = nla_get_string(attrs[ECHO_A_MSG]);
		ENSURE(luaL_dostring(me->L, msg) == LUA_OK
		      ,return NL_SKIP
		      ,"failed to execute Lua call.\n");
		return NL_OK;
	}
	default:
		return NL_SKIP;
	}
}

static void app_fini(struct app_s *me)
{
	bpf_set_link_xdp_fd(me->xdp.ifi, -1, me->xdp.flags);
}

static void unload_bpf()
{
	if (g_app)
		app_fini(g_app);
	putchar('\n');
        exit(0);
}

static int app_init(struct app_s *me, const char *ifi_name, const char *prog_path)
{
	memset(me, 0, sizeof(*me));
	me->epoll.fd = epoll_create1(0);

	// timer
	me->timer.fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
	ENSURE
	  (set_nonblock(me->timer.fd) == 0
	  ,return -1
	  ,"failed to set timerfd to nonblocking\n");
	ENSURE
	  (app_epoll_ctl(me, EPOLLIN, me->timer.fd, &me->timer) == 0
	  ,return -1
	  ,"failed to initialze timer event\n");
	ENSURE
	  (app_timer_set_s(me, 1) == 0
	  ,return -1
	  ,"failed to initialize timer\n");

	// input
	me->input.fd = STDIN_FILENO;
	ENSURE
	  (set_nonblock(me->input.fd) == 0
	  ,return -1
	  ,"failed to set stdin to nonblocking\n");
	ENSURE
	  (app_epoll_ctl(me, EPOLLIN, me->input.fd, &me->input) == 0
	  ,return -1
	  ,"failed to initialze events\n");

	// nl
	me->nl.sk = nl_socket_alloc();
	genl_connect(me->nl.sk);
	me->nl.fd = nl_socket_get_fd(me->nl.sk);
	me->nl.family = genl_ctrl_resolve(me->nl.sk, ECHO_NAME);
	nl_socket_set_nonblocking(me->nl.sk);
	nl_socket_modify_cb(me->nl.sk
	                   ,NL_CB_MSG_IN
			   ,NL_CB_CUSTOM
			   ,&app_on_genl_msg
			   ,me);
	ENSURE
	  (app_epoll_ctl(me, EPOLLIN, me->nl.fd, &me->nl) == 0
	  ,return -1
	  ,"failed to initialize netlink\n");

	// xdp
	signal(SIGINT,  unload_bpf);
	signal(SIGTERM, unload_bpf);

	ENSURE
	  ((me->xdp.ifi = if_nametoindex(ifi_name)) != 0
	  ,return -1
	  ,"failed to obtain `%s' index\n" , ifi_name);

	struct bpf_prog_load_attr attr = {
		.prog_type = BPF_PROG_TYPE_XDP,
		.file      = prog_path,
	};
	ENSURE
	  (setrlimit_inifinity() == 0
	  ,return -1
	  ,"Failed to increase RLIMIT_MEMLOCK limit\n");
	ENSURE
	  (bpf_prog_load_xattr(&attr, &me->xdp.o, &me->xdp.fd) == 0
	  ,return -1
	  ,"bpf_prog_load_xattr failed\n");
	ENSURE
	  (bpf_set_link_xdp_fd(me->xdp.ifi, me->xdp.fd, me->xdp.flags) == 0
	  ,return -1
	  ,"bpf_prog_link_xdp_fd failed\n");

	me->L = luaL_newstate();
	luaL_openlibs(me->L);

	g_app = me; /* NOTE: signal needs to access this */
	return 0;
}

static int app_loop(struct app_s *me)
{
	struct epoll_event es[8];
	int n = epoll_wait(me->epoll.fd, es, ARRAY_SIZE(es), -1);
	for (int i=0; i<n; ++i) {
		struct epoll_event *e = &es[i];
		if (e->data.ptr == &me->input) {
			size_t n;
			char line[1024];
			if (fgets(line, sizeof(line), stdin) && (n = strlen(line))) {
				line[--n] = 0;

				/* commands */
				if (strcmp(line, "exit") == 0)
					return -1;
				app_input_on_gets(me, line, n);
			}
		}
		if (e->data.ptr == &me->timer) {
			uint64_t cnt;
			read(me->timer.fd, &cnt, sizeof(cnt));
			app_timer_on_tick(me);
		}
		if (e->data.ptr == &me->nl) {
			nl_recvmsgs_default(me->nl.sk);
		}
	}
	return 0;
}

int main(int argc, char *const argv[])
{
	struct app_s app;
	const char *ifi_name = argc>1? argv[1] : "eth1",
	           *bpf_prog = argc>2? argv[2] : "bin/main.o";

	if (app_init(&app, ifi_name, bpf_prog))
		return -1;

	app_input_prompt();
	while (app_loop(&app) == 0)
		;

	app_fini(&app);
	return 0;
}
