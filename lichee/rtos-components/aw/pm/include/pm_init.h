#ifndef __PM_INIT_H__
#define __PM_INIT_H__

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
void pm_clear_wupio_cfg(void);
#endif

#ifdef CONFIG_ARCH_RISCV_C906
void pm_riscv_clear_boot_flag(void);
#endif

int pm_syscore_init(void);
int pm_devops_init(void);
int pm_wakecnt_init(void);
int pm_wakesrc_init(void);
int pm_init(int argc, char **argv);

#endif
