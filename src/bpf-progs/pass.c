#if 0
BS=-Wno-zero-length-array 
clang -ggdb -std=gnu11 -Wall -pedantic $BS -O2 -target bpf -c $0 -o ${0%%.c}.o
exit $?
#endif
/*
 * load:
 * - ip link set dev eth1 xdpgeneric obj main_kern.o sec prog
 * unload:
 * - ip link set dev eth1 xdpgeneric off
 */

#include<linux/bpf.h>
//#include<bpf/bpf_helpers.h>
//#include<bpf/bpf_endian.h>

#include<linux/icmp.h>
#include<linux/ip.h>
#include<linux/tcp.h>
#include<linux/udp.h>

__attribute__((section("prog")))
int firewall(struct xdp_md *ctx)
{
	return XDP_PASS;
}
