# Overview

This repository organizes a 'test lab' to build and run xdp programs.
It creates 3 machines based on alpine:
- a `xdp` virtual machine (qemu)
- a `server` (podman)
- a `client` (podman)

Connected by two bridges:
- br-wan (server, xdp)
- br-lan (xdp, client)

With this we can guarantee that all traffic from `client` goes through the
`xdp` machine. Tests can be designed from `client` to `server` or from `client`
to the `internet`.

```
+--------+  br-wan  +-----+  br-lan  +--------+
| server |<----+--->| xdp |<-------->| client |
+--------+     |    +-----+          +--------+
               |
               | iptables (masquerade)
               |
+----------+   |
| internet |<--+
+----------+
```

# eXpress Data Path (XDP)

[Express Data Path](https://en.wikipedia.org/wiki/Express_Data_Path) is a linux
infrastructure to execute programs on packets before they are handled by the
regular TCP/IP network stack. This allows for earlier drops and redirects of
specific packets resulting in a higher network troughput.

# Application

A sample application is provided to interact with the eBPF programs. It installs
the eBPF programs in XDP, setups:
- loads a eBPF program 
- epoll loop with a hook interactive command line
- generic netlink sample to communicate with the included kernel module
- timerfd (for periodic interactions of ebpf maps)
- lua (to trigger dynamic userspace functionality from kernel)

# Kernel Module

A sample module is provided to interact with the generic [Netlink](https://en.wikipedia.org/wiki/Netlink)
from the kernel side, setups:
- echo request
- echo reply

# eBPF programs

A pair sample programs to `pass` and to `drop` packages is also provided.

# Build and Run

This step will build a kernel, module, application and an initramfs based on
alpine linux. You will have a `kernel` and an `rootfs.img` with everything
ready to run in qemu.

- details of the kernel can be found in `config.mk` and `script/qemu.config`.
- details of the initramfs image can be found in `script/cmd`, function `initrd_init`.

```sh
make buildr cmd=all
make buildr cmd=buildr-rootfs
sudo sh script/network_init
```

- Only make qemu is necessary after the first setup, it should rebuild the
application, kernel modules and image as necessary.

```sh
make qemu
```

- To only rebuild the application, you can issue the command:

```sh
make buildr cmd=all
```

## tmux

A tmux script is provided as an automated way to run the test environment. It
creates a pane for each of the `qemu`, `client` and `server` machines in the
following layout:

```sh
./script/cmd tmux_run
```

```
+---------------+
| $    | xdp    |
|      +--------+
|      | server |
|      +--------+
|      | client |
+---------------+
```
