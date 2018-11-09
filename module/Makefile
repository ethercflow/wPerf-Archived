CONFIG_MODULE_SG=n
obj-m := trace_wperf_events.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
WARN_FLAGS += -Wall

CFLAGS_trace_wperf_events.o := -I$(src)

build:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: build clean
