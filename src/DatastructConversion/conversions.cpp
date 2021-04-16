/**
 * @defgroup conversions Data Structure Conversions
 * @{
 */

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include "conversions.h"

/**
 *  @brief  Converts a C++ string to C-string; Use free() to release mem after copying the content.
 *  @author Lau Lee Hong
 *  @param  str C++ string
 *  @return Pointer to a null-terminated array of character
 */
char* StringToChar(const std::string& str) 
{
    char* buffer = (char*)std::malloc(std::strlen(str.c_str()) + 1);
    if(buffer != NULL)
    {
        std::strcpy(buffer, str.c_str());
    }
    
    return buffer; 
}

/**
 *  @brief  Converts an int-type to string-type hex.
 *  @author Lau Lee Hong
 *  @param  i Integer to be converted to hex string
 *  @return Hexadecimal of string format
 */
std::string IntToHex(uint32_t i)
{
  std::stringstream stream;
  stream << std::hex << i;
  return stream.str();
}

/**
 *  @brief  Converts an int-type to string.
 *  @author Lau Lee Hong
 *  @param  i (int)integer to be converted (std::string)integer
 *  @return Integer of string format
 */
std::string IntToString(int v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

/**
 *  @brief  Converts an int-type with milliseconds padding to string.
 *  @author Goh Kok Boon
 *  @param  i (int)integer to be converted (std::string)integer
 *  @return Integer of string format with milliseconds padding
 */
std::string MsPaddingIntToString(int v)
{
    std::ostringstream oss;
    oss << v;
    std::string time_ms = oss.str() + "000";
    return time_ms;
}

/**
 *  @brief  Converts an double-type to a C-string with variable number of decimal places.
 *  @author Lau Lee Hong
 *  @param  str             buffer to store result
 *  @param  v               value
 *  @param  decimal_digits  number of decimal places
 *  @return Null-terminated charcter array of double value with user-defined number of decimal places
 */
char* DoubleToChar(char* str, double v, int decimal_digits)
{
    int i = 1;
    int int_part, fract_part;
    int len;
    char *ptr;

    /* Prepare decimal digits multiplicator */
    for (; decimal_digits != 0; i *= 10, decimal_digits--);

    /* Calculate integer & fractional parts */
    int_part = (int)v;
    if (v < 0)
    {
        v = -v;
    }
    fract_part = (int)((v - (double)(int)v) * i);

    /* Fill in integer part */
    std::sprintf(str, "%i.", int_part);

    /* Prepare fill in of fractional part */
    len = std::strlen(str);
    ptr = &str[len];

    /* Fill in leading fractional zeros */
    for (i /= 10; i > 1; i /= 10, ptr++)
    {
        if (fract_part >= i)
        {
            break;
        }
        *ptr = '0';
    }

    /* Fill in (rest of) fractional part */
    std::sprintf(ptr, "%i", fract_part);

    return str;
}

/**
 *  @brief  Converts an array of char to a hex string
 *  @author Lee Tze Han
 *  @param  arr Array of unsigned char
 *  @param  len Length of arr
 *  @return Hex string representation
 */
std::string CharToHex(unsigned char* arr, int len)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (int i = 0; i < len; i++)
    {
        ss << std::setw(2) << static_cast<unsigned>(arr[i]);
    }

    return ss.str();
}

/**
 *  @brief  Converts a C++ hex string to an array of char
 *  @author Lee Tze Han
 *  @param  str C++ hex string (left padded with 0)
 *  @param  arr unsigned char array to store output
 *  @return Hex string representation
 *  @note   arr should already allocated
 */
void HexToChar(std::string str, unsigned char* arr)
{
    for (int i = 0; i < str.length() / 2; i++)
    {
        arr[i] = std::stoi(str.substr(2*i, 2), nullptr, 16);
    }
}

/**
 *  @brief  Converts a C++ string to an integer
 *  @author Lee Tze Han
 *  @param  str C++ string
 *  @return Integer from string representation
 */
int StringToInt(const std::string& str)
{
    std::istringstream iss(str);
    int v;
    iss >> v;
    return v;   
}

/**
 *  @brief  Converts a time_t object (from NTPClient) to a C++ string
 *  @author Lee Tze Han
 *  @param  time time_t representing current time
 *  @return String representation of the underlying typedef'd uint32_t
 */
std::string TimeToString(const std::time_t time)
{
    std::ostringstream oss;
    oss << time;
    return oss.str();   
}

/**
 *  @brief  Converts a C++ string to time_t object
 *  @author Lee Tze Han
 *  @param  str C++ string
 *  @return time_t from string representation
 */
std::time_t StringToTime(const std::string& str)
{
    std::istringstream iss(str);
    std::time_t time;
    iss >> time;
    return time;    
}

/**
 *  @brief  Converts lowercases in a C++ string to uppercase
 *  @author Lau Lee Hong
 *  @param  s C++ string
 *  @return Original string with lowercase alphabets uppercased
 */
std::string ToUpperCase (std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

/**
 *  @brief  Converts uppercases in a C++ string to lowercase
 *  @author Lau Lee Hong
 *  @param  s C++ string
 *  @return Original string with upper alphabets lowercased
 */
std::string ToLowerCase (std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

/**
 *  @brief  Converts string holding data of a double to a double-type
 *  @author Lau Lee Hong
 *  @param  s C++ string containing data of a double
 *  @return Double-typed data of input
 */
double StringToDouble (std::string s)
{
    return std::stod(s);
}

/** @}*/