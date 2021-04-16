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
#include <string>
#include "mbed_config.h"
#include "mbed.h"
#include "mbed_trace.h"
#include "rtos.h"
#include "global_params.h"
#include "threads.h"
#include "boot_manager.h"
#include "device_uid.h"
#include "persist_store.h"

#if (MBED_MAJOR_VERSION != 6 || MBED_MINOR_VERSION != 5 || MBED_PATCH_VERSION != 0)
#error "MBed OS version is not targeted 6.5.0"
#endif  // mbed-os version check

#define TRACE_GROUP "Undefined"

/* Global System Parameters */
const std::string device_uuid = GetDeviceUid();

/* Mutex & Mailbox Declarations */
#if !MBED_TEST_MODE
Mutex stdio_mutex;
#endif
Mutex mqtt_mutex;
Mail<llp_sensor_mail_t, 256> llp_sensor_mail_box;
Mail<comms_upstream_mail_t, 256> comms_upstream_mail_box;
Mail<service_response_mail_t, 256> service_response_mail_box;
Mail<mqtt_arrived_mail_t, 128> mqtt_arrived_mail_box;
Mail<sensor_control_mail_t, 64> sensor_control_mail_box;

/* RTOS Main Threads Initialization */
Thread thread_1 (osPriorityNormal, OS_STACK_SIZE*8, NULL, "CommunicationsControllerThread");
Thread thread_2 (osPriorityNormal, OS_STACK_SIZE, NULL, "SensorThread");
Thread thread_3 (osPriorityNormal, OS_STACK_SIZE, NULL, "BehaviorCoordinatorThread");
Thread thread_4 (osPriorityNormal, OS_STACK_SIZE, NULL, "EventManagerThread");

/* Event flags */
EventFlags event_flags;

/* Watchdog Timer */
const uint32_t wd_timeout_ms = 20000;
Watchdog &watchdog = Watchdog::get_instance();

#if !MBED_TEST_MODE
int main()
{   
    /* Wait for hardware signals to stabilize */
    ThisThread::sleep_for(1s);

    WirelessModuleReset();

    mbed_trace_init();

    if (EnterBootManager())
    {
        RunBootManager();
    }
    else
    {
        thread_1.start(communications_controller_thread);
        thread_2.start(sensor_thread);
        thread_3.start(behavior_coordinator_thread);
        thread_4.start(event_manager_thread);
        
        watchdog.start(wd_timeout_ms);
        
        ThisThread::sleep_for(rtos::Kernel::wait_for_u32_forever);
    }
}
#endif
