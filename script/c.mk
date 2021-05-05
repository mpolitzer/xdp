include script/include.mk

CC      := $(prefix)gcc
LD      := $(prefix)ld
NM      := $(prefix)nm
OBJCOPY := $(prefix)objcopy
OBJDUMP := $(prefix)objdump
SIZE    := $(prefix)size
RM      := rm -f

        ld_e_o = $(CC) $$($1-cflags) $$^ $$($1-ldlibs) -o $$@
  quiet_ld_e_o = @echo "LD: $$@"; $(ld_e_o)
verbose_ld_e_o = @echo $(ld_e_o); $(ld_e_o)

        cc_o_c = $(CC) $$($1-cflags) -o $$@ -c $$<
  quiet_cc_o_c = @echo "CC: $$@"; $(cc_o_c)
verbose_cc_o_c = @echo $(cc_o_c); $(cc_o_c)

        cc_d_c = $(CC) $$($1-cflags) -MM -MT $$(@:.d=.o) $$< > $$@
  quiet_cc_d_c = @echo "DP: $$@"; $(cc_d_c)
verbose_cc_d_c = @echo $(cc_d_c); $(cc_d_c)

        rm_x_x = $(RM) $$($1-out) $$($1-objs-y) $$($1-deps-y)
  quiet_rm_x_x = @echo "RM: $$($1-out), objs, deps"; $(rm_x_x)
verbose_rm_x_x = @echo $(rm_x_x); $(rm_x_x)

define add_exe =
$1: $(O)$$($1-out)
$1-clean: $$($1-out)-clean

$1-objs-y := $$(addprefix $(O),$($1-objs))
$1-deps-y := $$($1-objs-y:.o=.d)
$$($1-out): $$($1-objs-y)
	$$(shell mkdir -p `dirname $$@`)
	$($(print)ld_e_o)
$$($1-objs-y): $$(O)%.o : $$(S)%.c $$(O)%.d
	$$(shell mkdir -p $(O)`dirname $$<`)
	$($(print)cc_o_c)
$$($1-deps-y): $$(O)%.d : $$(S)%.c
	$$(shell mkdir -p $(O)`dirname $$<`)
	$($(print)cc_d_c)
$$($1-out)-clean:
	$($(print)rm_x_x)
-include $$($1-deps-y)
endef

$(foreach out,$(c-rules),$(eval $(call add_exe,$(out))))
all:   $(foreach out,$(c-rules),$($(out)-out))
clean: $(foreach out,$(c-rules),$($(out)-out)-clean)
