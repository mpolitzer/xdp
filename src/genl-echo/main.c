#include<net/genetlink.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include"echo.h"

int echo_msg(struct sk_buff *skb, struct genl_info *info);

static struct nla_policy echo_policy[ECHO_A_MAX + 1] = {
	[ECHO_A_MSG] = { .type = NLA_NUL_STRING },
};

struct genl_ops echo_ops[] = {
	{
		.cmd = ECHO_C_ECHO,
		.doit = echo_msg,
		.policy = echo_policy,
	}
};

static struct genl_family echo_family = {
	.name    = ECHO_NAME,
	.version = ECHO_VERSION_NR,
	.maxattr = ECHO_A_MAX,
	.netnsok = true,
	.module  = THIS_MODULE,
	.ops     = echo_ops,
	.n_ops   = ARRAY_SIZE(echo_ops),
	.parallel_ops = false,
};

int echo_msg(struct sk_buff *req, struct genl_info *info) {
	struct sk_buff *rep = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	struct nlattr *na = info->attrs[ECHO_A_MSG];
	void *genl = genlmsg_put(rep, 0, info->snd_seq, &echo_family, 0, ECHO_C_ECHO);

	nla_put_string(rep, ECHO_A_MSG, nla_data(na));
	genlmsg_end(rep, genl);

	return genlmsg_reply(rep, info);
}

static int __init echo_init(void) {
	if (genl_register_family(&echo_family)) {
		return -EINVAL;
	}
	return 0;
}

static void __exit echo_exit(void) {
	genl_unregister_family(&echo_family);
}

module_init(echo_init);
module_exit(echo_exit);

MODULE_LICENSE("MIT");
