ifeq ($(CONFIG_KERNEL_FREERTOS), y)

ifeq ($(CONFIG_DRIVERS_NAND_FLASH), y)
subdir-ccflags-y += -I drivers/rtos-hal/hal/source/nand_flash/lib/include/
subdir-ccflags-y += -I drivers/rtos-hal/hal/source/nand_flash/include/

OBJECTS += nand_init.o
OBJECTS += nand_debug.o

OBJECTS += src/nand_osal_for_rtos.o
OBJECTS += src/nand_format.o
OBJECTS += src/nand_phy.o
OBJECTS += src/nand_scan.o
OBJECTS += src/nand_global.o
OBJECTS += src/uboot_tail_spinand.o
OBJECTS += src/nand_id.o
OBJECTS += src/nand_physic_interface_spinand.o
OBJECTS += src/spic_op.o
OBJECTS += src/nand_ecc_op.o
OBJECTS += src/spi_nand2.o
OBJECTS += src/build_nand_partition.o
OBJECTS += src/build_phy_partition.o
OBJECTS += src/nand_info_init.o
OBJECTS += src/nand_physic_interface.o
OBJECTS += src/nand_secure_storage.o
OBJECTS += src/nand_ota.o
OBJECTS += src/uboot_tail.o

#Required when compiling NFTL (for owners with flash driver permissions only)
ifeq ($(CONFIG_NAND_FLASH_NFTL), y)
	OBJECTS += lib/nftl/nftl_build.o
	OBJECTS += lib/nftl/nftl_cache.o
	OBJECTS += lib/nftl/nftl_common.o
	OBJECTS += lib/nftl/nftl_crc.o
	OBJECTS += lib/nftl/nftl_gc.o
	OBJECTS += lib/nftl/nftl_hexdump.o
	OBJECTS += lib/nftl/nftl_init.o
	OBJECTS += lib/nftl/nftl_list.o
	OBJECTS += lib/nftl/nftl_logic.o
	OBJECTS += lib/nftl/nftl_physic.o
	OBJECTS += lib/nftl/nftl_smart.o
	OBJECTS += lib/nftl/nftl_start.o
endif

obj-y += $(OBJECTS)

ifeq ($(CONFIG_ARCH_RISCV), y)
	ifndef CONFIG_NAND_FLASH_NFTL
		obj-$(CONFIG_ARCH_SUN20IW2) += libfreertos-nand-sun20iw2-c906.o
	endif
else ifeq ($(CONFIG_ARCH_ARM),y)
	ifndef CONFIG_NAND_FLASH_NFTL
		obj-$(CONFIG_ARCH_SUN20IW2) += libfreertos-nand-sun20iw2-m33.o
	endif
endif

endif
endif
