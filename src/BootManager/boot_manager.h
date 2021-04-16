/*******************************************************************************************************
 * Copyright (c) 2018-2019 Government Technology Agency of Singapore (GovTech)
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

#ifndef BOOT_MANAGER_H
#define BOOT_MANAGER_H

#include <string> 
#include "mbed.h"
#include "persist_config.h"

void PrintHeader(void);
void PrintMenu(void);
bool EnterBootManager(void);
void RunBootManager(void);
void InitAfterLogin(void);
bool GenerateSalt(unsigned char* salt, int salt_len);
bool GetDerivedKeyFromPass(std::string pass, const unsigned char* salt, int salt_len, unsigned char* derived_key, int derived_key_len);
void ChangeBootManagerPass(void);
bool CheckBootManagerPass(std::string pass);
void BootManagerLogin(void);
std::string GetUserInputString(bool is_hidden = false);
void SetDefaultConfig(void);
void ClearClientSslData(void);
void WirelessModuleReset(void);

#endif // BOOT_MANAGER_H