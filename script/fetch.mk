WGET := wget
TAR  := tar

        wget_o = $(WGET) -O $(O)$$($1-dst) $$($1-url)
  quiet_wget_o = @echo "WGET: $$@"; $(wget_o)
verbose_wget_o = @echo $(wget_o); $(wget_o)

        tar_o  = $(TAR) --strip-components 1 -xvf $(O)$$($1-dst) -C $$($1-out)
  quiet_tar_o  = @echo "TAR: $$@"; $(tar_o)
verbose_tar_o  = @echo $(tar_o); $(tar_o)

define add_fetch =
$1: $$($1-out)
$(O)$$($1-dst):
	$$(shell mkdir -p `dirname $$@`)
	$($(print)wget_o)
$$($1-out): $(O)$$($1-dst)
	$$(shell mkdir -p $$($1-out))
	$($(print)tar_o)
endef

$(foreach out,$(fetch-rules),$(eval $(call add_fetch,$(out))))
all:   $(foreach out,$(fetch-rules),$($(out)-out))
#clean: $(foreach out,$(fetch-rules),$($(out)-out)-clean)
