<div align="center">
  <img src="https://raw.githubusercontent.com/GovTechSIOT/stack-manuca-os/master/doc/manuca_os_logo_800_600.png">
</div>

## Introduction
MANUCA OS is a functional real-time operating system for sensor node developers to swiftly build upon using the MANUCA DK, and publish sensor measure points to the government cloud via DECADA Cloud.

For more information, visit https://siot.gov.sg/tech-stack/manuca/overview/

---
## Development Team
* Lau Lee Hong (lau\_lee\_hong@tech.gov.sg)
* Yap Zi Qi    (yap\_zi\_qi@tech.gov.sg)
* Goh Kok Boon (goh\_kok\_boon@tech.gov.sg)

---
## System Requirements for Development
* Windows, MacOS or Ubuntu 18.04

---
## Quick Start
 * Set up the development environment (https://siot.gov.sg/starter-kit/set-up-your-software/) 
 * Clone the repository onto local disk: 
    `git clone --recurse-submodules https://github.com/GovTechSIOT/stack-manuca-os.git`
 * Toggle operating modes in mbed_app.json
 * [Temporary due to DECADA Cloud migration] Disable SSL Verification by changing mbed-os/features/netsocket/TLSSocketWrapper.cpp line 550 to `mbedtls_ssl_conf_authmode(get_ssl_config(), MBEDTLS_SSL_VERIFY_NONE);`
 * Compile the binary (https://siot.gov.sg/starter-kit/build-and-flash/)
 * Copy the binary file (.bin) into the MANUCA DK via programmer (eg. stlink v3).
 
---
## Documentation and References
* Technical Instruction Manual: https://siot.gov.sg/starter-kit/introduction/
* MANUCA DK User Manual: https://siot.gov.sg/files/MANUCA_User_Manual_V1.pdf

---
## License and Contributions
The software is provided under the Apache-2.0 license (see https://siot.gov.sg/starter-kit/licensing/). Contributions to this project repository are accepted under the same license. Please see [contributing.md](CONTRIBUTING.md) for more information. 
