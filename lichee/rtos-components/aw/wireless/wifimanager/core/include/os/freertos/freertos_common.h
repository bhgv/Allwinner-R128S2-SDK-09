/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FREERTOS_COMMON_H
#define _FREERTOS_COMMON_H

#if __cplusplus
extern "C" {
#endif
#include "errno.h"
uint8_t char2uint8(char* trs);
void uint8tochar(char *mac_addr_char, uint8_t *mac_addr_uint8);

#if __cplusplus
};  // extern "C"
#endif

#endif  // _FREERTOS_COMMON_H