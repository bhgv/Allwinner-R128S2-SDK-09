obj-$(CONFIG_PROJECT_R128S2_DEVKIT_C906) += devkit_c906/
obj-$(CONFIG_PROJECT_R128S2_DEVKIT_M33) += devkit_m33/
obj-$(CONFIG_PROJECT_R128S2_DEVKIT_RGB_C906) += devkit_rgb_c906/
obj-$(CONFIG_PROJECT_R128S2_DEVKIT_RGB_M33) += devkit_rgb_m33/
obj-$(CONFIG_PROJECT_R128S2_EVT_C906) += evt_c906/
obj-$(CONFIG_PROJECT_R128S2_EVT_M33) += evt_m33/

subdir-ccflags-y += -I components/common/aw/pm/include

ifeq ($(CONFIG_DRIVERS_XRADIO), y)
subdir-ccflags-y += -I components/common/aw/xradio/include/sdd
subdir-ccflags-y += -I drivers/rtos-hal/include/hal
endif
