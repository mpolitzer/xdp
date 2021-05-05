ifeq ($(O),)
  O=$(PWD)/
endif

CP      := cp

        cp_x_x = $(CP) $$^ $$@
  quiet_cp_x_x = @echo "CP: $$^ $$@"; $(cp_x_x)
verbose_cp_x_x = @echo $(cp_x_x); $(cp_x_x)

