include script/include.mk

CC      := $(prefix)clang
RM      := rm -f

        cc_o_c = $(CC) $$($1-cflags) --target=bpf -c $$< -o $$@
  quiet_cc_o_c = @echo "CC: $$@"; $(cc_o_c)
verbose_cc_o_c = @echo $(cc_o_c); $(cc_o_c)

        cc_d_c = $(CC) $$($1-cflags) --target=bpf -MM -MT $$(@:.d=.o) $$< > $$@
  quiet_cc_d_c = @echo "DP: $$@"; $(cc_d_c)
verbose_cc_d_c = @echo $(cc_d_c); $(cc_d_c)

        rm_x_x = $(RM) $$($1-out) $$($1-objs-y) $$($1-deps-y)
  quiet_rm_x_x = @echo "RM: $$($1-out), objs, deps"; $(rm_x_x)
verbose_rm_x_x = @echo $(rm_x_x); $(rm_x_x)

define add_bpf =
$1: $$($1-out)
$1-clean: $$($1-out)-clean

$1-objs-y := $$(addprefix $(O),$($1-objs))
$1-deps-y := $$($1-objs-y:.o=.d)
$$($1-out): $$($1-objs-y)
	$$(shell mkdir -p `dirname $$@`)
	$($(print)cp_x_x)
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

$(foreach out,$(bpf-rules),$(eval $(call add_bpf,$(out))))
all:   $(foreach out,$(bpf-rules),$($(out)-out))
clean: $(foreach out,$(bpf-rules),$($(out)-out)-clean)
