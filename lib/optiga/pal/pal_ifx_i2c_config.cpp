/**
* MIT License
*
* Copyright (c) 2018 Infineon Technologies AG
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE
*
*
* \file
*
* \brief This file implements platform abstraction layer configurations for ifx i2c protocol.
*
* \ingroup  grPAL
* @{
*/

/**********************************************************************************************************************
 * HEADER FILES
 *********************************************************************************************************************/
#include "mbed.h"
extern "C" {
	#include "optiga/pal/pal_gpio.h"
	#include "optiga/pal/pal_i2c.h"
    #include "optiga/ifx_i2c/ifx_i2c_config.h"
}


/*********************************************************************************************************************
 * pal ifx i2c instance
 *********************************************************************************************************************/
I2C i2c_ctx_0(OPTIGA_TRUST_X_I2C_SDA, OPTIGA_TRUST_X_I2C_SCL);
DigitalOut reset(OPTIGA_TRUST_X_RESET);
DigitalOut vdd(PF_3);

/**
 * \brief PAL I2C configuration for OPTIGA. 
 */
pal_i2c_t optiga_pal_i2c_context_0 =
{
    /// Pointer to I2C master platform specific context
    (void*)&i2c_ctx_0,
    /// Slave address
    IFX_I2C_BASE_ADDR,
    /// Upper layer context
    NULL,
    /// Callback event handler
    NULL
};

ifx_i2c_context_t ifx_i2c_context_0 =
{
    /// Slave address
    IFX_I2C_BASE_ADDR,
    /// i2c-master frequency
    OPTIGA_TRUST_X_I2C_FREQ,
    /// IFX-I2C frame size
#if (DL_MAX_FRAME_SIZE >= 0x0115)
    0x0115,
#else
	DL_MAX_FRAME_SIZE,
#endif
    /// Vdd pin
    (pal_gpio_t*)&vdd,
    /// Reset pin
    (pal_gpio_t*)&reset,
    /// optiga pal i2c context
    &optiga_pal_i2c_context_0,
};


/**
* @}
*/

