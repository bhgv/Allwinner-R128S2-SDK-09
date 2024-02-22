
/*
* Copyright (c) 2021-2027 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __TWI_SUN20IW2_H__
#define __TWI_SUN20IW2_H__

#if defined(CONFIG_ARCH_RISCV_C906)
#define SUNXI_IRQ_TWI0	57
#define SUNXI_IRQ_TWI1	58
/* not used */
#define SUNXI_IRQ_TWI2 (0)
#define SUNXI_IRQ_TWI3 (0)
#elif defined(CONFIG_ARCH_DSP)
#define SUNXI_IRQ_TWI0 (RINTC_IRQ_MASK | 42)
#define SUNXI_IRQ_TWI1 (RINTC_IRQ_MASK | 43)
/* not used */
#define SUNXI_IRQ_TWI2 (0)
#define SUNXI_IRQ_TWI3 (0)
#else
#define SUNXI_IRQ_TWI0	41
#define SUNXI_IRQ_TWI1	42
/* not used */
#define SUNXI_IRQ_TWI2 (0)
#define SUNXI_IRQ_TWI3 (0)
#endif

/* the base address of TWI*/
#define SUNXI_TWI0_PBASE 0x040049000
#define TWI0_SCK GPIOA(12)
#define TWI0_SDA GPIOA(13)
#define TWI0_PIN_MUXSEL 3

#define SUNXI_TWI1_PBASE 0x040049400
#define TWI1_SCK GPIOB(0)
#define TWI1_SDA GPIOB(1)
#define TWI1_PIN_MUXSEL 3

#define SUNXI_TWI2_PBASE 0x02502800
#define TWI2_SCK GPIOH(5)
#define TWI2_SDA GPIOH(6)
#define TWI2_PIN_MUXSEL 4

#define SUNXI_TWI3_PBASE 0x02502c00
#define TWI3_SCK GPIOE(6)
#define TWI3_SDA GPIOE(7)
#define TWI3_PIN_MUXSEL 4

#define TWI_DISABLE_PIN_MUXSEL 15
#define TWI_PULL_STATE 1
#define TWI_DRIVE_STATE 0

#define SUNXI_CLK_TWI(x)	CLK_BUS_TWI##x
#define SUNXI_CLK_RST_TWI(x)	RST_TWI##x

#endif /* __TWI_SUN20IW2_H__ */

