/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      System PM implementation.
 *
 *  Copyright (c) 2020-2021 Xradio, Inc.
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

#ifndef PAL_SYS_PM_H
#define PAL_SYS_PM_H

#include "pal_os.h"

typedef enum {
	PAL_WAKELOCK_ID_BASE = 0,
	PAL_WAKELOCK_ID_BTC = PAL_WAKELOCK_ID_BASE,
	PAL_WAKELOCK_ID_LTASK,
	PAL_WAKELOCK_ID_HCITX,
	PAL_WAKELOCK_ID_HCIRX,
	PAL_WAKELOCK_ID_BTC_BB,
	PAL_WAKELOCK_ID_MAX,
} pal_wakelock_id_t;

typedef enum {
	PAL_WAKESRC_ID_BASE = 0,
	PAL_WAKESRC_ID_SLPTMR = PAL_WAKESRC_ID_BASE,
	PAL_WAKESRC_ID_MAX,
} pal_wakesrc_id_t;

typedef enum {
	PAL_PM_MODE_BASE = 0,
	PAL_PM_MODE_ON = PAL_PM_MODE_BASE,
	PAL_PM_MODE_SLEEP,
	PAL_PM_MODE_STANDBY,
	PAL_PM_MODE_HIBERNATION,
	PAL_PM_MODE_MAX,
} pal_suspend_mode_t;

typedef struct {
	uint32_t suspend_noirq_interval;
	uint32_t enter_standby_interval;
	uint32_t exit_standby_interval;
	uint32_t resume_noirq_interval;
} pal_pm_interval_t;

typedef void *pal_task_handle_t;

int32_t PalPmDevopsRegister(void);
int32_t PalPmDevopsUnregister(void);
int32_t PalPmWakelockAcquire(pal_wakelock_id_t wl);
int32_t PalPmWakelockRelease(pal_wakelock_id_t wl);
int32_t PalPmWakelockGetRefer(pal_wakelock_id_t wl);
void PalPmWakesrcAcquire(pal_wakesrc_id_t ws);
void PalPmWakesrcReleaseSleepy(pal_wakesrc_id_t ws);
void PalPmWakesrcReleaseWakeup(pal_wakesrc_id_t ws);
int32_t PalPmTaskRegister(pal_task_handle_t handle);
int32_t PalPmTaskUnregister(pal_task_handle_t handle);
uint32_t PalPmIsHostReady(void);
uint32_t PalPmGetInterval(pal_pm_interval_t *interval);
void PalPmClearInterval(void);

#endif /*PAL_SYS_PM_H*/
