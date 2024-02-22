obj-$(CONFIG_COMPONENTS_USB_GADGET_MTPD) += mtpd.o \
					MtpCommon.o \
					MtpDataBase.o \
					MtpObjectInfo.o \
					MtpPacket.o \
					MtpProperty.o \
					MtpServer.o \
					MtpStorage.o \
					Disk/Dir.o \
					Disk/Disk.o \
					Disk/DiskDebug.o \
					Disk/File.o \
					mtp_update_tools.o

ifeq ($(CONFIG_COMPONENTS_USB_GADGET_MTPD), y)
CFLAGS += -I components/common/aw/usb/gadget/mtpd/ \
	  -I components/common/aw/usb/gadget/mtpd/Disk/ \
	  -I drivers/rtos-hal/hal/source/usb/udc
endif
