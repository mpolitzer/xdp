        make_m_x = $$(MAKE) -sC $$($1-kpath) M=$O$$($1-mpath) src=$(PWD)/$$($1-mpath) INSTALL_MOD_PATH=$$($1-out) INSTALL_MOD_STRIP=1 modules modules_install
  quiet_make_m_x = @echo "MK: $$($(1)-mpath)"; $(make_m_x)
verbose_make_m_x = @echo $(make_m_x); $(make_m_x)

        make_c_x = $$(MAKE) -sC $$($1-kpath) M=$O$$($1-mpath) src=$(PWD)/$$($1-mpath) clean
  quiet_make_c_x = @echo "MK: $$@"; $(make_c_x)
verbose_make_c_x = @echo $(make_c_x); $(make_c_x)

define add_kmod =
$1: $$($1-out)
$1-clean: $$($1-out)-clean
$$($1-out):
	$$(shell mkdir -p $O$$($1-mpath))
	@touch $O$$($1-mpath)/Makefile # hack, but breaks without it.
	$($(print)make_m_x)
$$($1-out)-clean:
	$($(print)make_c_x)
	@rm -f $O$$($1-mpath)/Makefile
.PHONY: $$($1-out) $$($1-out)-clean
endef

$(foreach out,$(kmod-rules),$(eval $(call add_kmod,$(out))))
all:   $(foreach out,$(kmod-rules),$($(out)-out))
clean: $(foreach out,$(kmod-rules),$($(out)-out)-clean)
