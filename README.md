removed GPIO pull-up resistors 
increased maximum frame size to 64 
### 🔑 核心改进说明

| 改进项 | 实现方式 | 优势 |
|:---|:---|:---|
| **🛡️ 错误日志系统** | 新增 `ErrorCode` 枚举 + `getLastError()` + `errorToString()` | 解析失败不再“静默丢弃”，可精准定位是校验头/载荷出错、缓冲区溢出还是未知帧，极大方便硬件调试与产线良率分析。 |
| **🔄 跨平台字节序** | 弃用隐式 `memcpy`，改用显式位移拼装 `bytesToUInt32`，并提供 `LD6002_BIG_ENDIAN` 宏开关 | 彻底解决 AVR/ESP32/ARM/RISC-V 等不同架构主机因大小端差异导致的浮点数解析乱码问题。默认适配 ESP32（小端序）。 |
| **📌 引脚宏定义** | `#define LD6002_RX_PIN 14` 等宏集中管理，传入 `Serial1.begin()` | 解耦硬件依赖，切换开发板（如 ESP32-S3、NodeMCU、Wemos）只需改头文件宏，符合工程化规范。 |
| **🔢 类型安全加固** | `expectedFrameLen` 改为 `uint16_t`，增加 `static_assert` 校验 IEEE 754 兼容性 | 防止协议升级导致帧长计算溢出；编译期拦截非标准浮点架构的潜在风险。 |

# 📡 HLK-LD6002 - Arduino Library
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/phuongnamzz/HLK-LD6002)](https://github.com/phuongnamzz/HLK-LD6002/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/phuongnamzz/HLK-LD6002)](https://github.com/phuongnamzz/HLK-LD6002/network)

![Platforms](https://img.shields.io/badge/Platforms-ESP32%20%7C%20AVR%20%7C%20STM32-green)
![Version](https://img.shields.io/github/v/release/phuongnamzz/HLK-LD6002?color=brightgreen)
![Arduino Library](https://img.shields.io/badge/Arduino-Library-yellow)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Library-orange)

Arduino library for **HLK-LD6002** — a radar module for **respiratory and heart rate detection**.

📁 [📄 Official Documents & Driver Files](https://drive.google.com/drive/folders/1oMJYYOvhXx9uJD9SCkCsjMzLqdOg7AqR)

---

## 🧭 Module Types

### 2D & 3D Models:
![Example pinout module](https://raw.githubusercontent.com/phuongnamzz/HLK-LD6002/main/resources/pinout_module.png)

---

## 🧬 Overview

The **HLK-LD6002** is a radar sensing module based on the **ADT6101P** chip, featuring:

- Integrated 57–64 GHz RF transceiver
- 2T2R PCB microstrip antenna
- ARM® Cortex®-M3 core
- 1MB Flash, radar signal processing
- FMCW radar for precise measurement of human respiratory and heart rate in real time

---

## ⚠️ Important Notes

> 🔋 **Power Requirements**:
>
> - Input: **3.2–3.4V**
> - Ripple: ≤ **50mV**
> - Current: ≥ **1A**
> - DCDC switching frequency: **≥ 2MHz**

> 📡 **Detection Considerations**:
>
> - Measurement depends on **RCS** and **environmental factors**
> - Occasional measurement errors are **normal**
> - Measurement supports **one individual only**
> - Must be in **resting state** for detection

---

## 🧷 Pin Description

![Pin description](https://raw.githubusercontent.com/phuongnamzz/HLK-LD6002/main/resources/pin_description.png)

### 🔌 MCU Connection Note:
- Pull **P19** low at power-on.
- Example connection with MCU:

![Connection diagram](https://raw.githubusercontent.com/phuongnamzz/HLK-LD6002/main/resources/connection%20diagram.png)

---

## 📟 Serial Protocols

### 💓 Heart Rate Protocol
![Heart rate protocol](https://raw.githubusercontent.com/phuongnamzz/HLK-LD6002/main/resources/heart_protocol.png)

### 🌬️ Breath Rate Protocol
![Breath rate protocol](https://raw.githubusercontent.com/phuongnamzz/HLK-LD6002/main/resources/breath_protocol.png)

### 📏 Target Distance Protocol
![Target distance protocol](https://raw.githubusercontent.com/phuongnamzz/HLK-LD6002/main/resources/target_distance_protocol.png)

---

## 🔗 ESP32 Connection Table

| 🆔 No | 📟 LD6002 Pin | ⚙️ Function         | 📲 ESP32 Pin   |
|:----:|:-------------:|:------------------:|:-------------:|
|  1   | **3V3**       | ⚡ Power Input      | **3V3**        |
|  2   | **GND**       | 🛑 Ground           | **GND**        |
|  3   | **P19**       | 🔁 Boot1 (Pull Low) | **GND**        |
|  4   | **TX2**       | 📤 GPIO20 (TX)      | *Not Connected* |
|  5   | **AIO1**      | 🎯 Analog IO        | *Not Connected* |
|  6   | **SCL0**      | 🔄 GPIO7 (I2C SCL)  | *Not Connected* |
|  7   | **TX0**       | 📤 Serial TX        | **GPIO16**     |
|  8   | **RX0**       | 📥 Serial RX        | **GPIO17**     |

---

## 📦 Example Code (ESP32)

```cpp
#include <Arduino.h>
#include "LD6002.h"
LD6002 radar(Serial1);

void setup()
{
  Serial.begin(115200);
  Serial1.begin(1382400, SERIAL_8N1, 16, 17);
  // 🔔some modules come with 115200 baudrate
}

float lastHeartRate = 0;
float lastBreathRate = 0;
float lastDistance = 0;
void loop()
{
  radar.update();

  if (radar.hasNewHeartRate())
  {
   float heartRateMain = radar.getHeartRate();
   if ((heartRateMain != lastHeartRate) && (heartRateMain > 0))
   {
    Serial.printf("Heart Rate: %.2f bpm\n", heartRateMain);
   }
   lastHeartRate = heartRateMain;
   radar.clearHeartRateFlag();
  }

  if (radar.hasNewBreathRate())
  {
   float breathRateMain = radar.getBreathRate();
   if ((breathRateMain != lastBreathRate) && (breathRateMain > 0))
   {
    Serial.printf("Breath Rate: %.2f bpm\n", breathRateMain);
   }
   lastBreathRate = breathRateMain;
   radar.clearBreathRateFlag();
  }

  if (radar.hasNewDistance())
  {
   float distanceMain = radar.getDistance();
   if ((distanceMain != lastDistance) && (distanceMain > 0))
   {
    Serial.printf("Distance: %.2f cm\n", distanceMain);
   }
   lastDistance = distanceMain;
   radar.clearDistanceFlag();
  }
}

```
✅ Result:

```
Heart Rate: 81.00 bpm
Breath Rate: 25.00 bpm
Heart Rate: 78.00 bpm
Breath Rate: 24.00 bpm
Distance: 38.08 cm
Heart Rate: 78.00 bpm
Breath Rate: 27.00 bpm
Heart Rate: 81.00 bpm
Heart Rate: 76.00 bpm
Heart Rate: 78.00 bpm
Breath Rate: 23.00 bpm
Heart Rate: 76.00 bpm
Heart Rate: 75.00 bpm
```
