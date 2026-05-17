#pragma once
#include <Arduino.h>

class LD6002 {
public:
    enum class FrameType : uint16_t {
        HeartRate = 0x0A15,
        BreathRate = 0x0A14,
        Distance = 0x0A16,
        Unknown = 0xFFFF
    };

    // 🔹 新增：错误状态枚举
    enum class ErrorCode : uint8_t {
        NoError = 0,
        ChecksumHeaderFail,   // 头部校验失败
        ChecksumPayloadFail,  // 载荷校验失败
        BufferOverflow,       // 帧长超出缓冲区限制
        InvalidFrameType,     // 未知帧类型
        UnknownError
    };

    explicit LD6002(HardwareSerial &serial);

    void update();
    
    // 🔹 新增：错误日志 API
    ErrorCode getLastError() const;
    void clearError();
    static const char* errorToString(ErrorCode err);

    // 数据访问接口（保持不变）
    bool hasNewHeartRate() const;
    bool hasNewBreathRate() const;
    bool hasNewDistance() const;
    float getHeartRate() const;
    float getBreathRate() const;
    float getDistance() const;
    void clearHeartRateFlag();
    void clearBreathRateFlag();
    void clearDistanceFlag();

private:
    static constexpr uint8_t MAX_FRAME_LEN = 17;
    HardwareSerial &serial;
    uint8_t frame[MAX_FRAME_LEN];
    uint8_t pos = 0;
    bool syncing = false;
    uint16_t expectedFrameLen = 0; // 改为 uint16_t 防止长度计算溢出

    float heartRate = 0;
    float breathRate = 0;
    float distance = 0;
    bool newHeartRate = false;
    bool newBreathRate = false;
    bool newDistance = false;

    ErrorCode lastError = ErrorCode::NoError;
    void setError(ErrorCode err) { lastError = err; }

    // 🔹 新增：显式字节序转换，摆脱 memcpy 隐式依赖
    float bytesToFloat(const uint8_t *data) const;
    uint32_t bytesToUInt32(const uint8_t *data) const;
    
    uint8_t calcXorInverse(const uint8_t *data, int len) const;
    FrameType getFrameType(const uint8_t *frame) const;
    void parseFrame(const uint8_t *frame);
    void printHex(const uint8_t *data, int len) const;
};