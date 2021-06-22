<a href="https://govtechsiot.github.io/decada-embedded-example-mbedos/"><img src="https://codedocs.xyz/doxygen/doxygen.svg"/></a>

## Introduction
`decada-embedded-example-mbedos`, previously known as stack-manuca-os, is a functional real-time operating system example using [MbedOS](https://os.mbed.com/) for embedded software developers to reference and build upon. This example showcases how one can publish sensor measure points to the government's IoT infrastructure via DECADA Cloud.



## Development Team
* Lau Lee Hong (lau\_lee\_hong@tech.gov.sg)
* Yap Zi Qi    (yap\_zi\_qi@tech.gov.sg)
* Goh Kok Boon (goh\_kok\_boon@tech.gov.sg)
* Lee Tze Han



## Requirements for Development
* Development Machine running either Windows, MacOS or Ubuntu 18.04
* An Mbed OS supported target (see https://os.mbed.com/platforms/)
  * Source code is tested using a STM32F767ZI
  * Most devices running an ARM Cortex-M4 and ARM Cortex-M7 should work as well
* ESP32 WiFi Module
* Infineon Optiga Trust X (*optional*)
* External MCU Programmer (*optional*; for devices that do not have an onboard programmer, like a STLINK-v3)



## Quick Start
 * Set up the development environment (https://siot.gov.sg/starter-kit/set-up-your-software-env/) 
 * Clone the repository onto local disk: 
    `git clone --recurse-submodules https://github.com/GovTechSIOT/decada-embedded-example-mbedos.git`
 * Toggle operating modes in `mbed_app.json`
 * Edit configurations in `mbed_app.json` (if necessary)
    * `use-secure-element` - Uses the Infineon Optiga Trust X as the Secure Element in this example
 * Copy the compiled binary file (.bin) into your Mbed device via a programmer



### Hardware Setup
This repository is tested on the STM32F767ZI DK with peripherals configured as:
* **ESP32**
  * Pins configurable in `mbed_app.json` (`wifi-tx`, `wifi-rx`, `wifi-en`)
* **Infineon Optiga Trust X**
  * Pins configurable in `lib/optiga/mbed_lib.json`

Other hardware targets supported by MbedOS can utilize the repository by changing the targeted board in the compilation process.



## Variants
Besides MbedOS, we provide embedded source code example to connect to DECADA Cloud using [Zephyr](https://www.zephyrproject.org/) as well.
`decada-embedded-example-zephyr`: https://github.com/GovTechSIOT/decada-embedded-example-zephyr



## Documentation and References
* Singapore Government Tech Stack - Sensors & IoT: https://www.siot.gov.sg/core-products/decada-embedded/
* Technical Instruction Manual for GovTech's SIOT Starter Kit: https://siot.gov.sg/starter-kit/
* GovTech's Custom STM32F767ZI Development Kit (MANUCA DK) User Manual: https://siot.gov.sg/files/MANUCA_User_Manual_V1.pdf



## License and Contributions
The software is provided under the Apache-2.0 license (see https://www.siot.gov.sg/starter-kit/terms-and-conditions/). Contributions to this project repository are accepted under the same license. Please see [CONTRIBUTING.md](CONTRIBUTING.md) for more information. 
