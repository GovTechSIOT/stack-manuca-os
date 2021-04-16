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

#ifndef PERSIST_CONFIG_H
#define PERSIST_CONFIG_H

#include <string>

/** PersistConfig struct.
 *  @brief  Plain-Old-Data structure for holding system configuration data
 *
 *  Example:
 *  @code{.cpp}
 *  #include "persist_config.h"
 *
 *  int main() 
 *  {
 *      PersistConfig config;
 *      config.dummy_int = 200;
 *      config.dummy_str = "test string";
 *  }
 *  @endcode
 */
struct PersistConfig
{
    // Test variables
    int dummy_int;
    std::string dummy_str;   
};

struct BootManagerPass 
{
    std::string derived_key;
    std::string salt;
};

#endif // PERSIST_CONFIG_H