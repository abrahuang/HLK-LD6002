/*
连接说明：
LD6002 TX  -> ESP32 LD6002_RX_PIN
LD6002 RX  -> ESP32 LD6002_TX_PIN (仅单向接收时可悬空)
LD6002 GND -> ESP32 GND
LD6002 VCC -> ESP32 3.3V 或 5V (按模块丝印确认)
*/

#include <Arduino.h>
#include "LD6002.h"

// 🔹 引脚与波特率宏定义（修改此处即可适配任意开发板）
#define LD6002_RX_PIN  14
#define LD6002_TX_PIN  15
#define LD6002_BAUD    115200

LD6002 radar(Serial1);

void setup() {
    Serial.begin(115200);
    while (!Serial); // 等待串口终端连接（可选）
    Serial.println("🟢 LD6002 Radar Test Started");

    // 初始化串口1，引脚通过宏传入
    Serial1.begin(LD6002_BAUD, SERIAL_8N1, LD6002_RX_PIN, LD6002_TX_PIN);
}

float lastHeartRate = 0;
float lastBreathRate = 0;
float lastDistance = 0;

void loop() {
    radar.update();

    // 🔹 错误日志监控与打印
    LD6002::ErrorCode err = radar.getLastError();
    if (err != LD6002::ErrorCode::NoError) {
        Serial.printf("⚠️ [Radar Error] %s\n", radar.errorToString(err));
        radar.clearError(); // 处理后清除，避免重复打印
    }

    // 心率
    if (radar.hasNewHeartRate()) {
        float val = radar.getHeartRate();
        if (val > 0 && val != lastHeartRate) {
            Serial.printf("❤️ Heart Rate: %.2f bpm\n", val);
            lastHeartRate = val;
        }
        radar.clearHeartRateFlag();
    }

    // 呼吸率
    if (radar.hasNewBreathRate()) {
        float val = radar.getBreathRate();
        if (val > 0 && val != lastBreathRate) {
            Serial.printf("🌬️ Breath Rate: %.2f bpm\n", val);
            lastBreathRate = val;
        }
        radar.clearBreathRateFlag();
    }

    // 距离
    if (radar.hasNewDistance()) {
        float val = radar.getDistance();
        if (val > 0 && val != lastDistance) {
            Serial.printf("📏 Distance: %.2f cm\n", val);
            lastDistance = val;
        }
        radar.clearDistanceFlag();
    }
}