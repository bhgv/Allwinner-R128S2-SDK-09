#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hal_atomic.h>
#include <console.h>
#ifdef CONFIG_DRIVER_SYSCONFIG
#include <script.h>
#include <hal_cfg.h>
#endif

#include <pm_base.h>
#include <pm_adapt.h>
#include <pm_debug.h>
#include <pm_wakesrc.h>
#include <pm_rpcfunc.h>
#include <pm_init.h>
#include <pm_devops.h>
#include <pm_wakecnt.h>
#include <pm_subsys.h>

#define PM_WS_IRQ_COLLECTOR	PM_WS_AFFINITY_M33

#define pm_wakesrc_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_wakesrc, node)

#define WAKESRC_SYS_CONFIG	"wakeup-source"

static SemaphoreHandle_t pm_wakesrc_mutex = NULL;
static hal_spinlock_t pm_wakesrc_lock;
static struct list_head pm_wakesrc_register_list = LIST_HEAD_INIT(pm_wakesrc_register_list);
/* irq in the sub core will be taken over by the main pm core */
static int sub_core_irq[PM_WAKEUP_SRC_MAX] = {INVAL_IRQ_NUM};

static char *type_name[] = {
	"ALWAYS",
	"MIGHT",
	"SOFT",
};

int pm_wakesrc_init(void)
{
	pm_wakesrc_mutex = xSemaphoreCreateMutex();
	hal_spin_lock_init(&pm_wakesrc_lock);

	return 0;
}

struct pm_wakesrc *pm_wakesrc_find_registered_by_irq(int irq)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;

	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		if (ws->irq == irq)
			return ws;
	}

	return NULL;
}

int pm_wakesrc_is_registered(struct pm_wakesrc *ws)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *lws = NULL;

	list_for_each_safe(node, list_save, list) {
		lws = pm_wakesrc_containerof(node);
		if (lws == ws)
			return 1;
	}

	return 0;
}

int pm_wakesrc_type_check_num(wakesrc_type_t type)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;
	int num_wakesrc = 0;

	if (!wakesrc_type_valid(type))
		return -EINVAL;

	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		if (ws->enabled && (ws->type == type))
			num_wakesrc++;
	}

	return num_wakesrc;
}

struct pm_wakesrc *pm_wakesrc_register(const int irq, const char *name, const unsigned int type)
{
	int i = 0;
	struct pm_wakesrc *ws = NULL;
	struct pm_wakesrc_settled *ws_settled = NULL;

	if (!name || !wakesrc_type_valid(type) || ((0 > irq) && (irq > PM_SOFT_WAKEUP_IRQ_BASE))) {
		pm_err("%s(%d): Invalid paras\n", __func__, __LINE__);
		return NULL;
	}

#ifdef CONFIG_ARCH_DSP
	/* RINTC_IRQ_MASK */
	if ((type == PM_WAKESRC_SOFT_WAKEUP) && (irq < -65000)) {
		pm_err("%s(%d): Invalid irq for soft wakeup\n", __func__, __LINE__);
		return NULL;
	}
#endif

	if (type == PM_WAKESRC_SOFT_WAKEUP) {
		if (irq > PM_SOFT_WAKEUP_IRQ_BASE) {
			pm_err("%s(%d): Invalid irq for soft wakeup\n", __func__, __LINE__);
			return NULL;
		}
	} else {
		for (i = PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
			ws_settled = pm_wakesrc_get_by_id(i);
			if (ws_settled->irq == irq)
				break;
		}
		if (i == PM_WAKEUP_SRC_MAX) {
			pm_err("%s(%d): get settled wakesrc failed\n", __func__, __LINE__);
			return NULL;
		}
	}

	if (pm_wakesrc_find_registered_by_irq(irq)) {
		pm_err("%s(%d): irq %d is already registered\n", __func__, __LINE__, irq);
		return NULL;
	}


	if (pdTRUE != xSemaphoreTake(pm_wakesrc_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_wakesrc_mutex);
		pm_err("%s(%d): register take semaphore failed\n", __func__, __LINE__);
		return NULL;
	}

	ws = malloc(sizeof(struct pm_wakesrc));
	if (!ws) {
		pm_err("%s(%d): register alloc failed\n", __func__, __LINE__);
		return NULL;
	}
	if (type == PM_WAKESRC_SOFT_WAKEUP) {
		ws->id = PM_SOFT_WAKESRC_MAJOR_ID + irq;
	} else if (ws_settled) {
		ws->id = ws_settled->id;
	}
#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	ws->affinity = PM_WS_AFFINITY_M33;
#elif CONFIG_ARCH_RISCV_C906
	ws->affinity = PM_WS_AFFINITY_RISCV;
#elif CONFIG_ARCH_DSP
	ws->affinity = PM_WS_AFFINITY_DSP;
#endif
	ws->irq = irq;
	ws->enabled = 0;
	snprintf(ws->name, PM_WAKESRC_NAME_LENTH, "%s", name);
	ws->cnt.event_cnt = 0;
	ws->cnt.active_cnt = 0;
	ws->cnt.relax_cnt = 0;
	ws->cnt.wakeup_cnt = 0;
	hal_spin_lock_init(&ws->lock);
	ws->type = type;
	list_add_tail(&ws->node, &pm_wakesrc_register_list);

	xSemaphoreGive(pm_wakesrc_mutex);

	return ws;
}

int pm_wakesrc_unregister(struct pm_wakesrc *ws)
{
	struct pm_wakesrc **pws = NULL;

	if (!pm_wakesrc_is_registered(ws)) {
		pm_err("%s(%d): wakesrc(%p) hasn't registered\n", __func__, __LINE__, ws);
		return -EINVAL;
	}

	if (ws->enabled) {
		pm_warn("%s(%d): irq %d remain enabled while unregister, try to release irq...\n", __func__, __LINE__, ws->irq);
		if (pm_clear_wakeirq(ws)) {
			pm_err("%s(%d): try to release irq %d failed. pm_wakesrc unregisters failed\n", __func__, __LINE__, ws->irq);
			return -EFAULT;
		} else
			pm_warn("%s(%d): irq %d disabled down\n", __func__, __LINE__, ws->irq);
	}

	if (pdTRUE != xSemaphoreTake(pm_wakesrc_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_wakesrc_mutex);
		return -EBUSY;
	}

	list_del(&ws->node);
	ws->id = 0;
	ws->affinity = 0;
	ws->irq = 0;
	ws->enabled = 0;
	memset(ws->name, 0, sizeof(char) * PM_WAKESRC_NAME_LENTH);
	ws->cnt.event_cnt = 0;
	ws->cnt.active_cnt = 0;
	ws->cnt.relax_cnt = 0;
	ws->cnt.wakeup_cnt = 0;
	hal_spin_lock_deinit(&ws->lock);
	ws->type = 0;

	free(ws);
	pws = &ws;
	*pws = NULL;

	xSemaphoreGive(pm_wakesrc_mutex);

	return 0;
}

int pm_always_wakeup(void)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;
	int might_wakeup = 0;

	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		if (ws->type == PM_WAKESRC_MIGHT_WAKEUP)
			might_wakeup++;
	}

	return !(might_wakeup);
}

int pm_set_wakeirq(struct pm_wakesrc *ws)
{
	int ret = 0;
	int i;
	int irq;
	struct pm_wakesrc_settled *ws_set;
	wakesrc_affinity_t affinity;

	if (!pm_wakesrc_is_registered(ws)) {
		pm_err("fail to set wakeirq, wakesrc(%p) is not registered\n", ws);
		return -ENODEV;
	}

	if (ws->enabled) {
		pm_err("fail to set wakeirq. wakesrc(%p) has been enabled\n", ws);
		return -EEXIST;
	}

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	affinity = PM_WS_AFFINITY_M33;
#endif

#ifdef CONFIG_ARCH_RISCV_C906
	affinity = PM_WS_AFFINITY_RISCV;
#endif

#ifdef CONFIG_ARCH_DSP
	affinity = PM_WS_AFFINITY_DSP;
#endif
	if (ws->type == PM_WAKESRC_SOFT_WAKEUP) {
		ws->enabled = 1;
		return ret;
	}

	irq = ws->irq;
	for (i = PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
		ws_set = pm_wakesrc_get_by_id(i);
		if (ws_set->irq == irq)
			break;
	}

	if (i == PM_WAKEUP_SRC_MAX) {
		pm_err("wakeirq enable failed, irq %d invalid\n", irq);
		return -EINVAL;
	}

	ret = pm_set_wakesrc(ws_set->id, affinity, 1);
	if (ret)
		pm_err("set wakeirq failed, irq: %d, wakesrc settled: %s\n",ws_set->irq, ws_set->name);

	if (ws)
		ws->enabled = 1;

	return ret;
}

int pm_clear_wakeirq(struct pm_wakesrc *ws)
{
	int ret = 0;
	int i;
	int irq;
	struct pm_wakesrc_settled *ws_set;
	wakesrc_affinity_t affinity;

	if (!pm_wakesrc_is_registered(ws) || !ws->enabled) {
		pm_err("clear wakeirq failed, wakesrc(%p) is not enabled\n", ws);
		return -ENODEV;
	}

	if (pm_wakesrc_is_active(ws)) {
		pm_err("clear wakeirq failed, wakesrc(%p) is active\n", ws);
		 return -EBUSY;
	}

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	affinity = PM_WS_AFFINITY_M33;
#endif

#ifdef CONFIG_ARCH_RISCV_C906
	affinity = PM_WS_AFFINITY_RISCV;
#endif

#ifdef CONFIG_ARCH_DSP
	affinity = PM_WS_AFFINITY_DSP;
#endif

	if (ws->type == PM_WAKESRC_SOFT_WAKEUP) {
		ws->enabled = 0;
		return ret;
	}

	irq = ws->irq;
	for (i = PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
		ws_set = pm_wakesrc_get_by_id(i);
		if (ws_set->irq == irq)
			break;
	}

	if (i == PM_WAKEUP_SRC_MAX) {
		pm_err("wakeirq_enable find pm_wakesrc_settled failed, irq %d invalid\n", irq);
		return -EINVAL;
	}

	ret = pm_set_wakesrc(ws_set->id, affinity, 0);
	if (ret)
		pm_err("clear wakeirq failed, irq: %d, wakesrc settled: %s\n",ws_set->irq, ws_set->name);

	if (ws)
		ws->enabled = 0;

	return ret;
}

int pm_wakesrc_is_active(struct pm_wakesrc *ws)
{
	int ret = 0;

	if (!pm_wakesrc_is_registered(ws) || !ws->enabled)
		return -ENODEV;

	hal_spin_lock(&ws->lock);
	if (ws->cnt.active_cnt != ws->cnt.relax_cnt)
		ret = 1;
	hal_spin_unlock(&ws->lock);

	return ret;
}

int pm_wakesrc_is_disabled(struct pm_wakesrc *ws)
{
	if (!pm_wakesrc_is_registered(ws))
		return -ENODEV;

	if (ws->enabled)
		return 0;
	else
		return 1;
}

void pm_wakesrc_update_irq_in_sub_core(int id, int irq)
{
	if (id < PM_WAKEUP_SRC_MAX)
		sub_core_irq[id] = irq;
}

int pm_wakesrc_irq_in_sub_core(int irq)
{
	int i;

	if (irq == INVAL_IRQ_NUM)
		return 0;

	for (i = 0; i < PM_WAKEUP_SRC_MAX; i++) {
		if (irq == sub_core_irq[i])
			return 1;
	}

	return 0;
}

int pm_wakesrc_soft_wakeup(struct pm_wakesrc *ws, wakesrc_action_t action, int keep_ws_enabled)
{
	wakesrc_affinity_t affinity;

	if (!wakesrc_action_valid(action) || !pm_wakesrc_is_registered(ws) || !ws->enabled)
		return -EINVAL;

	if (action == PM_WAKESRC_ACTION_WAKEUP_SYSTEM) {
		pm_stay_awake(ws);
		pm_relax(ws, PM_RELAX_WAKEUP);
	} else {
		pm_stay_awake(ws);
		pm_relax(ws, PM_RELAX_SLEEPY);
	}

	if (!keep_ws_enabled)
		pm_clear_wakeirq(ws);

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	affinity = PM_WS_AFFINITY_M33;
#endif

#ifdef CONFIG_ARCH_RISCV_C906
	affinity = PM_WS_AFFINITY_RISCV;
#endif

#ifdef CONFIG_ARCH_DSP
	affinity = PM_WS_AFFINITY_DSP;
#endif

	return pm_subsys_soft_wakeup(affinity, ws->irq, action);
}

static int pm_wakesrc_list_info(int argc, char **argv)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;

	printf("wakesrc     id        irq      type     event_cnt   active_cnt   relax_cnt   wakeup_cnt   enabled\n");
	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		printf("%-10s  %08x  %-7d  %-10s  %-10ld  %-10ld  %-10ld  %-10ld  %-10d\n",
			ws->name, ws->id, ws->irq, type_name[ws->type],
			ws->cnt.event_cnt, ws->cnt.active_cnt, ws->cnt.relax_cnt, ws->cnt.wakeup_cnt,
			ws->enabled);
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(pm_wakesrc_list_info, pm_list_wakesrc, pm tools)

int pm_wakesrc_is_inexistent(char *name)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
	int ret;
	int32_t val = 0;

	ret = hal_cfg_get_keyvalue(WAKESRC_SYS_CONFIG, name, &val, 1);
	if (!ret) {
		if (val == PM_WAKESRC_HEREON)
			return 0;
		else
			return 1;
	} else {
		pm_err("get wakesrc config [%s] failed, return %d\n", name, ret);
		return 1;
	}
#endif /* CONFIG_DRIVER_SYSCONFIG */

	return 1;
}
