/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      UART driver definition.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include "pal_irq.h"
#include "hal_interrupt.h"
#include "irqs-sun20iw2p1.h"
#include "hal_status.h"
#include "errno.h"

static int32_t PalGetIrq(pal_irq_type_t type)
{
	int32_t irq = -EINVAL;

	switch (type) {
	case PAL_IRQ_TYPE_LL:
		irq = BLE_LL_IRQn;
		break;
	case PAL_IRQ_TYPE_BB:
		irq = BTC_BB_IRQn;
		break;
	case PAL_IRQ_TYPE_WLANCOEX:
		irq = BTCOEX_IRQn;
		break;
	case PAL_IRQ_TYPE_DBG:
		irq = BTC_DBG_IRQn;
		break;
	case PAL_IRQ_TYPE_SLPTMR:
		irq = BTC_SLPTMR_IRQn;
		break;
	default:
		break;
	}
	return irq;
}

int32_t PalIrqRegister(pal_irq_type_t type, PalIrqHandler_t handler)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return -EINVAL;
	}
	hal_request_irq(irq, (hal_irq_handler_t)handler, NULL, NULL);
	hal_nvic_irq_set_priority(irq, CONFIG_ARCH_ARM_ARMV8M_IRQ_DEFAULT_PRIORITY);
	hal_interrupt_clear_pending(irq);
	hal_enable_irq(irq);
	return 0;
}

int32_t PalIrqUnregister(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return -EINVAL;
	}
	hal_disable_irq(irq);
	hal_interrupt_clear_pending(irq);
	return 0;
}

void PalIrqClearPending(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return ;
	}
	hal_interrupt_clear_pending(irq);
}

uint32_t PalIrqGetNest(void)
{
	return hal_interrupt_get_nest();
}

void PalIrqDisable(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return ;
	}
	hal_disable_irq(irq);
}

int32_t PalIrqEnable(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return -EINVAL;
	}
	if (hal_enable_irq(irq) != HAL_OK) {
		return -EPERM;
	}
	return 0;
}

