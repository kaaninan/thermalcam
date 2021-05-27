# Project Summary

[![Generic badge](https://img.shields.io/badge/status-prototype-brightgreen.svg)](https://shields.io/)

**Project name:** sensorkit-thermalcam

**Project description:** ThermalCam Sensor Kit

## Components:

| Component | Module | Protocol | Links |
|------|------|------|------|
|Microcontroller| STM32L432KC | - |[Module Datasheet](https://www.st.com/resource/en/data_brief/nucleo-l432kc.pdf) / [Datasheet](https://www.st.com/resource/en/datasheet/stm32l432kc.pdf)
|Time-of-Flight| VL53L1X | I2C |[Module Datasheet](https://cdn.ozdisan.com/ETicaret_Dosya/615686_1419050.pdf) / [Datasheet](https://www.st.com/resource/en/datasheet/vl53l1x.pdf)
|Thermal Camera| AMG8833 | I2C |[Datasheet](https://cdn.sparkfun.com/assets/4/1/c/0/1/Grid-EYE_Datasheet.pdf)
|Face Detection| K210 | UART |[Product Link](https://www.seeedstudio.com/Sipeed-M1n-Module-AI-Development-Kit-based-on-K210-p-4491.html)
| Status Led | *WS2812B?* | - | -

## Output:

| Protocol | Module | Status |
|------|------|------|
| I2C | - | *waiting* |
| RS485 | [MAX485](https://datasheets.maximintegrated.com/en/ds/MAX1487-MAX491.pdf) | *waiting* |

## Kit Features:

- WIP


## Limitations:

- ToF and AMG8833 don't work on same I2C port for some reason.