# 复合动态内存堆使用说明
## 概述说明
本内存堆方案的目的是R128S2的RV核可以同时把代码部署在XIP、SRAM、LPSRAM、HPSRAM上，根据算法对内存速度需求自行分配，最大化体现芯片性能。
而对于R128S3，由于整体是HPSRAM，所以不推荐使用该方案。
## 配置说明
以R128S2的risc-v核同时部署在lpsram和hpsram为例。
1. 先确保其他核，如DSP核或者ARM核，先把内存释放出来，如R128S2中，DSP核默认使用了8M的hpsram，需要先释放部分hpsram内存出来，实现方式请参考《DSP_FreeRTOS_环境_开发指南》中的“LSP文件”章节。
2. 执行mrtos_menuconfig,切换内存堆方案CONFIG_HEAP_MULTIPLE_DYN=y
3. 使能CONFIG_HPSRAM，并编辑HPSRAM的起始地址和长度，例如CONFIG_HPSRAM_START_ADDRESS=0x0c400000， CONFIG_HPSRAM_LENGTH=0x400000；不需要使能CONFIG_LPSRAM，因为ld文件默认定义了RAM = CONFIG_ARCH_START_ADDRESS，这个地址默认是0x8200000，也就是默认内存堆就是LPSRAM。如果修改CONFIG_ARCH_START_ADDRESS为hpsram的地址，则同理不需要使能CONFIG_HPSRAM，而改为使能CONFIG_LPSRAM。其余的SRAM也是一样道理。
4. 修改分区表，添加hpsram的分区，如修改board/r128s2/pro/configs/sys_partition_xip.fex
```
[partition]
    name         = rv-hpsram
    size         = 1800
    downloadfile = "rtos_hriscv.fex"
    user_type    = 0x8000
```
注意在v0.9版本前，默认生成的hpsram分区名称是rtos_hpsram_riscv.fex，但是分区名称不能大于20个字符，会导致烧录失败，所以请更新SDK或者tool文件夹到最新。
5. 修改board/r128s2/pro/configs/image_header.template.cfg，确认hpsram分区的加载地址，注意sram_offs的数值，就是在第3步的CONFIG_HPSRAM_START_ADDRESS数值。
`{"id": "0xa5ce5a70", "bin": "rtos_hriscv.fex",  "attr": S(ATTR_EXE),   "sram_offs": "0xc400000",    "ep": "",                "sign_key" : S(SIGN_NonTFKey)},`
6. 修改board/r128s2/pro/configs/env_nor.cfg，让芯片启动后会主动去load hpsram分区。
`loadparts=arm-lpsram@:rv-lpsram@:rv-hpsram@:dsp-hpsram@:config@0x8000000`
7. 以上配置完成后，就编译烧录即可，需要运行在hpsram中的代码，请自行修改对应的ld文件，如修改lichee/rtos/projects/r128s2/pro_c906/freertos.lds.S，把cedarx部署在hpsram上：
```
    .hpsram_text :
    {
        . = ALIGN(16);
        __hpsram_start__ = .;
        __hpsram_text_start__ = .;
	    *libxrbtc.a: (.text .text.* .rodata .rodata.*)
	    *libxrbtc.o (.text .text.* .rodata .rodata.*)

        *libadecoder.a: (.text .text.* .rodata .rodata.*)
        *libarecoder.a: (.text .text.* .rodata .rodata.*)
        *libaw_mp3dec.a: (.text .text.* .rodata .rodata.*)
        *libaw_aacdec.a: (.text .text.* .rodata .rodata.*)
        *libaw_amrdec.a: (.text .text.* .rodata .rodata.*)
        *libaw_amrenc.a: (.text .text.* .rodata .rodata.*)
        *libaw_flacdec.a: (.text .text.* .rodata .rodata.*)
        *libaw_oggdec.a: (.text .text.* .rodata .rodata.*)
        *libaw_opuscodec.a: (.text .text.* .rodata .rodata.*)
        *libaw_wavdec.a: (.text .text.* .rodata .rodata.*)
        *libcdx_base.a: (.text .text.* .rodata .rodata.*)
        *libmuxer.a: (.text .text.* .rodata .rodata.*)
        *libparser.a: (.text .text.* .rodata .rodata.*)
        *libplayback.a: (.text .text.* .rodata .rodata.*)
        *librecord.a: (.text .text.* .rodata .rodata.*)
        *librtplayer.a: (.text .text.* .rodata .rodata.*)
        *libstream.a: (.text .text.* .rodata .rodata.*)
        *libxplayer.a: (.text .text.* .rodata .rodata.*)
        *libxrecorder.a: (.text .text.* .rodata .rodata.*)
        *libaudiosystem_c906.a: (.text .text.* .rodata .rodata.*)

        *libadecoder.o (.text .text.* .rodata .rodata.*)
        *libarecoder.o (.text .text.* .rodata .rodata.*)
        *libaw_mp3dec.o (.text .text.* .rodata .rodata.*)
        *libaw_aacdec.o (.text .text.* .rodata .rodata.*)
        *libaw_amrdec.o (.text .text.* .rodata .rodata.*)
        *libaw_amrenc.o (.text .text.* .rodata .rodata.*)
        *libaw_flacdec.o (.text .text.* .rodata .rodata.*)
        *libaw_oggdec.o (.text .text.* .rodata .rodata.*)
        *libaw_opuscodec.o (.text .text.* .rodata .rodata.*)
        *libaw_wavdec.o (.text .text.* .rodata .rodata.*)
        *libcdx_base.o (.text .text.* .rodata .rodata.*)
        *libmuxer.o (.text .text.* .rodata .rodata.*)
        *libparser.o (.text .text.* .rodata .rodata.*)
        *libplayback.o (.text .text.* .rodata .rodata.*)
        *librecord.o (.text .text.* .rodata .rodata.*)
        *librtplayer.o (.text .text.* .rodata .rodata.*)
        *libstream.o (.text .text.* .rodata .rodata.*)
        *libxplayer.o (.text .text.* .rodata .rodata.*)
        *libxrecorder.o (.text .text.* .rodata .rodata.*)
        *libaudiosystem_c906.o (.text .text.* .rodata .rodata.*)
        /* MUST not put IRQ handler/callback in .hpsram section */
        . = ALIGN(16);
        __hpsram_text_end__ = .;
    } > HPSRAM
```