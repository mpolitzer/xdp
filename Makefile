all: # default
-include .config.mk

#-------------------------------------------------------------------------------
fetch-rules := $(kver)

$(kver)-url  := https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-$(kver).tar.xz
$(kver)-dst  := linux-$(kver).tar.xz
$(kver)-out  := work/linux-$(kver)
#-------------------------------------------------------------------------------
c-rules         := xdp-load

xdp-load-out    := work/rootfs/usr/bin/xdp-load
xdp-load-objs   := src/xdp-load/main.o
xdp-load-pkgs   := libbpf libelf lua5.3 libnl-genl-3.0
xdp-load-cflags := `pkg-config --with-path=/usr/lib64/pkgconfig --cflags --cflags $(xdp-load-pkgs)` -ggdb -O2 -Wall -pedantic -Isrc
xdp-load-ldlibs := `pkg-config --with-path=/usr/lib64/pkgconfig --cflags --libs $(xdp-load-pkgs)`   -ggdb -O2
#-------------------------------------------------------------------------------
bpf-rules       := xdp-pass xdp-drop

xdp-pass-out    := $(rootfs)/usr/libexec/xdp-pass.o
xdp-pass-objs   := src/bpf-progs/pass.o
xdp-pass-cflags := -Isrc

xdp-drop-out    := $(rootfs)/usr/libexec/xdp-drop.o
xdp-drop-objs   := src/bpf-progs/drop.o
xdp-drop-cflags := -Isrc
#-------------------------------------------------------------------------------
kmod-rules      := genl-echo

genl-echo-out   := $(rootfs)
genl-echo-mpath := src/genl-echo
genl-echo-kpath := $(kpath)
#-------------------------------------------------------------------------------
buildr:
	@script/cmd buildr_run sh -lc 'make $(cmd)'
buildr-prepare: $(kver)
	@cp script/qemu.config $(kpath)/.config
	@make -j$(nproc) -C $(kpath) \
	  olddefconfig scripts \
	  INSTALL_HDR_PATH=$(buildfs) headers_install \
	  modules_prepare \
	  $(xdplua-flags)
	@make -C $(kpath)/tools/lib/bpf/ \
	  LP64=0 prefix=$(buildfs) \
	  install install_headers \
	  $(xdplua-flags)
buildr-rootfs: $(kver)
	@cp script/qemu.config $(kpath)/.config
	@mkdir -p $(rootfs)/boot
	@make -j8 -C $(kpath) \
	  olddefconfig scripts bzImage vmlinux modules \
	  INSTALL_PATH=$(rootfs)/boot install \
	  INSTALL_MOD_PATH=$(rootfs) INSTALL_MOD_STRIP=1 modules_install \
	  $(xdplua-flags)
#-------------------------------------------------------------------------------
$(initrd): $(xdp-pass-out) $(xdp-drop-out) $(xdp-load-out)
	@sh script/cmd initrd_run $(initrd)
#-------------------------------------------------------------------------------
qemu: $(image) $(initrd)
	@sh script/cmd qemu_run $(image) $(initrd)
#-------------------------------------------------------------------------------
include script/c.mk
include script/bpf.mk
include script/kmod.mk
include script/fetch.mk

.PHONY: qemu buildr
