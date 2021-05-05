#ifndef ECHO_H
#define ECHO_H

#define ECHO_NAME "ECHO"
enum {
	ECHO_A_UNSPEC,
	ECHO_A_MSG,
	ECHO_A_COUNT,
};
#define ECHO_A_MAX (ECHO_A_COUNT - 1)

enum {
	ECHO_C_UNSPEC,
	ECHO_C_ECHO,
	ECHO_C_COUNT,
};
#define ECHO_C_MAX (ECHO_C_COUNT - 1)

#define ECHO_VERSION_NR 1

#endif
