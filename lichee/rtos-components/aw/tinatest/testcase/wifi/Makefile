ifeq ($(CONFIG_DRIVERS_XRADIO), y)

obj-$(CONFIG_TESTCASE_WIFION) += tt-wifion.o
obj-$(CONFIG_TESTCASE_WIFIOFF) += tt-wifioff.o
obj-$(CONFIG_TESTCASE_WIFISCAN) += tt-wifiscan.o
obj-$(CONFIG_TESTCASE_WIFICON) += tt-wificonn.o
obj-$(CONFIG_TESTCASE_WIFIRM) += tt-wifidisconn.o

CFLAGS += -I components/common/aw/wireless/wifimanager/core/include
CFLAGS += -I components/common/thirdparty/network/lwip/src/include
CFLAGS += -I drivers/drv/wireless/xradio/adapter
CFLAGS += -I drivers/drv/wireless/xradio/os/include

endif
