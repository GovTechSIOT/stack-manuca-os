/*******************************************************************************************************
 * Copyright (c) 2018-2020 Government Technology Agency of Singapore (GovTech)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 *
 * See the License for the specific language governing permissions and limitations under the License.
 *******************************************************************************************************/
 
#ifndef THREADS_H
#define THREADS_H
 
#include "decada_manager.h"

/* behavior_coordinator_thread.cpp */
void behavior_coordinator_thread(void);

/* communications_thread.cpp */
void subscription_manager_thread(DecadaManager* decada_ptr);
void communications_controller_thread(void);

/* event_manager_thread.cpp */
void event_manager_thread(void);
 
/* sensor_thread.cpp */
void sensor_thread(void);
 
#endif  // THREADS_H 