/*
 * ===========================================================================================
 *
 *       Filename:  barrier.h
 *
 *    Description:  barrier impl. of riscv
 *
 *        Version:  Melis3.0
 *         Create:  2020-07-08 14:35:53
 *       Revision:  none
 *       Compiler:  GCC:version 7.2.1 20170904 (release),ARM/embedded-7-branch revision 255204
 *
 *         Author:  caozilong@allwinnertech.com
 *   Organization:  BU1-PSW
 *  Last Modified:  2020-09-28 11:12:43
 *
 * ===========================================================================================
 */
#ifndef _ASM_RISCV_BARRIER_H
#define _ASM_RISCV_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()           __asm__ __volatile__ ("nop")

#define RISCV_FENCE(p, s) \
    __asm__ __volatile__ ("fence " #p "," #s : : : "memory")

/* These barriers need to enforce ordering on both devices or memory. */
#define mb()            RISCV_FENCE(iorw,iorw)
#define rmb()           RISCV_FENCE(ir,ir)
#define wmb()           RISCV_FENCE(ow,ow)

/* These barriers do not need to enforce ordering on devices, just memory. */
#define __smp_mb()      RISCV_FENCE(rw,rw)
#define __smp_rmb()     RISCV_FENCE(r,r)
#define __smp_wmb()     RISCV_FENCE(w,w)

#if defined(CONFIG_ARCH_RISCV_C906)
#define dmb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
} while(0)

#define dsb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ ("sync");\
} while(0)

#define isb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ ("sync.i");\
} while(0)
#elif defined(CONFIG_ARCH_RISCV_E906)
#define dmb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
} while(0)

#define dsb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ (".insn r 0xb, 0, 0, x0, x0, x24");\
} while(0)

#define isb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ (".insn r 0xb, 0, 0, x0, x0, x26");\
} while(0)
#endif


#define soft_break(...) do { __asm__ __volatile__("ebreak" ::: "memory", "cc"); } while(0)
#endif

#endif
