/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      irq driver definition.
 *
 *  Copyright (c) 2018 ARM Ltd. All Rights Reserved.
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

#ifndef _PAL_IRQ_H
#define _PAL_IRQ_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/
typedef enum {
	PAL_IRQ_TYPE_BASE = 0,
	PAL_IRQ_TYPE_LL = PAL_IRQ_TYPE_BASE,
	PAL_IRQ_TYPE_BB,
	PAL_IRQ_TYPE_WLANCOEX,
	PAL_IRQ_TYPE_DBG,
	PAL_IRQ_TYPE_SLPTMR,
	PAL_IRQ_TYPE_MAX,
} pal_irq_type_t;

typedef int (*PalIrqHandler_t)(void);

int32_t PalIrqRegister(pal_irq_type_t type, PalIrqHandler_t handler);
int32_t PalIrqUnregister(pal_irq_type_t type);
void PalIrqClearPending(pal_irq_type_t type);
uint32_t PalIrqGetNest(void);
void PalIrqDisable(pal_irq_type_t type);
int32_t PalIrqEnable(pal_irq_type_t type);

#ifdef __cplusplus
};
#endif

#endif /* _PAL_IRQ_H */
