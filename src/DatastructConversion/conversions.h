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
#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#include <string>
#include <cstring>
#include <ctime>
#include <stdint.h>

char* StringToChar(const std::string& str);
std::string IntToHex(uint32_t i);
std::string IntToString(int v);
std::string MsPaddingIntToString(int v);
char* DoubleToChar(char* str, double v, int decimalDigits);
std::string CharToHex(unsigned char* arr, int len);
void HexToChar(std::string str, unsigned char* arr);
int StringToInt(const std::string& str);
std::string TimeToString(const std::time_t time);
std::time_t StringToTime(const std::string& str);
std::string ToUpperCase (std::string s);
std::string ToLowerCase (std::string s);
double StringToDouble (std::string s);

#endif  // CONVERSIONS_H