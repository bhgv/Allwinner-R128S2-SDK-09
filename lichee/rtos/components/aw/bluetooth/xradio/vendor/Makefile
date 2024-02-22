
BT_VENDOR_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BT_VENDOR_DIRS += $(shell find $(BT_VENDOR_BASE) -type d)

BT_VENDOR_SRCS = $(basename $(foreach dir,$(BT_VENDOR_DIRS),$(wildcard $(dir)/*.[csS])))

BT_VENDOR_OBJS += $(addsuffix .o,$(BT_VENDOR_SRCS))

CFLAGS += -I components/aw/bluetooth/xradio/vendor/

obj-y += $(subst $(BT_VENDOR_BASE)/, , $(BT_VENDOR_OBJS))
