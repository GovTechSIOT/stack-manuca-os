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

#ifndef PERSIST_STORE_H
#define PERSIST_STORE_H

#include "mbed.h"
#include "persist_config.h"

void WriteConfig(const PersistConfig& pconf);
void WriteSystemTime(const time_t time);
void WriteSwVer(const std::string sw_ver);
void WriteInitFlag(const std::string flag);
void WriteWifiSsid(const std::string ssid);
void WriteWifiPass(const std::string pass);
void WriteCycleInterval(const std::string interval);
void WriteClientCertificate(const std::string cert);
void WriteClientCertificateSerialNumber(const std::string cert_sn);
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 0)
void WriteClientPrivateKey(const std::string private_key);
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

PersistConfig ReadConfig(void);
time_t ReadSystemTime(void);
std::string ReadSwVer(void);
std::string ReadInitFlag(void);
std::string ReadWifiSsid(void);
std::string ReadWifiPass(void);
std::string ReadCycleInterval(void);
std::string ReadClientCertificate(void);
std::string ReadClientCertificateSerialNumber(void);
#if defined(MBED_CONF_APP_USE_SECURE_ELEMENT) && (MBED_CONF_APP_USE_SECURE_ELEMENT == 0)
std::string ReadClientPrivateKey(void);
#endif  // MBED_CONF_APP_USE_SECURE_ELEMENT

#endif // PERSIST_STORE_H