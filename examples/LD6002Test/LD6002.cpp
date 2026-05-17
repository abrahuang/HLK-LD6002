#include "LD6002.h"

LD6002::LD6002(HardwareSerial &serial) : serial(serial) {}

void LD6002::update() {
    while (serial.available()) {
        uint8_t byte = serial.read();

        if (!syncing) {
            if (byte == 0x01) {
                frame[0] = byte;
                pos = 1;
                syncing = true;
                expectedFrameLen = 0;
            }
        } else {
            if (pos < MAX_FRAME_LEN) {
                frame[pos++] = byte;

                if (pos == 7) {
                    uint16_t dataLen = (frame[3] << 8) | frame[4];
                    expectedFrameLen = 8 + dataLen + 1;
                    if (expectedFrameLen > MAX_FRAME_LEN) {
                        setError(ErrorCode::BufferOverflow);
                        syncing = false;
                        pos = 0;
                        expectedFrameLen = 0;
                    }
                }

                if (expectedFrameLen > 0 && pos >= expectedFrameLen) {
                    parseFrame(frame);
                    syncing = false;
                    pos = 0;
                    expectedFrameLen = 0;
                }
            } else {
                setError(ErrorCode::BufferOverflow);
                syncing = false;
                pos = 0;
                expectedFrameLen = 0;
            }
        }
    }
}

ErrorCode LD6002::getLastError() const { return lastError; }
void LD6002::clearError() { lastError = ErrorCode::NoError; }

const char* LD6002::errorToString(ErrorCode err) {
    switch (err) {
        case ErrorCode::NoError: return "No Error";
        case ErrorCode::ChecksumHeaderFail: return "Header Checksum Fail";
        case ErrorCode::ChecksumPayloadFail: return "Payload Checksum Fail";
        case ErrorCode::BufferOverflow: return "Buffer Overflow / Frame Too Long";
        case ErrorCode::InvalidFrameType: return "Unknown Frame Type";
        default: return "Unknown Error";
    }
}

// 🔹 跨平台字节序处理：显式拼装，不依赖主机端默认字节序
// 默认假设雷达输出为 Little-Endian（ESP32/AVR/ARM 常见）。
// 若你的模块协议指定为大端序，请在编译时添加宏：#define LD6002_BIG_ENDIAN
uint32_t LD6002::bytesToUInt32(const uint8_t *data) const {
#ifdef LD6002_BIG_ENDIAN
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8)  |
            static_cast<uint32_t>(data[3]);
#else
    return (static_cast<uint32_t>(data[3]) << 24) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[1]) << 8)  |
            static_cast<uint32_t>(data[0]);
#endif
}

float LD6002::bytesToFloat(const uint8_t *data) const {
    uint32_t u32 = bytesToUInt32(data);
    float f;
    static_assert(sizeof(float) == sizeof(uint32_t), "Float must be 32-bit IEEE 754");
    memcpy(&f, &u32, sizeof(float));
    return f;
}

uint8_t LD6002::calcXorInverse(const uint8_t *data, int len) const {
    uint8_t cksum = 0;
    for (int i = 0; i < len; i++) cksum ^= data[i];
    return ~cksum;
}

LD6002::FrameType LD6002::getFrameType(const uint8_t *frame) const {
    uint16_t type = (frame[5] << 8) | frame[6];
    switch (type) {
        case 0x0A15: return FrameType::HeartRate;
        case 0x0A14: return FrameType::BreathRate;
        case 0x0A16: return FrameType::Distance;
        default:     return FrameType::Unknown;
    }
}

void LD6002::parseFrame(const uint8_t *frame) {
    if (calcXorInverse(frame, 7) != frame[7]) {
        setError(ErrorCode::ChecksumHeaderFail);
        return;
    }

    FrameType type = getFrameType(frame);
    switch (type) {
        case FrameType::HeartRate:
            if (calcXorInverse(frame + 8, 4) != frame[12]) {
                setError(ErrorCode::ChecksumPayloadFail);
                return;
            }
            heartRate = bytesToFloat(&frame[8]);
            newHeartRate = true;
            break;
        case FrameType::BreathRate:
            if (calcXorInverse(frame + 8, 4) != frame[12]) {
                setError(ErrorCode::ChecksumPayloadFail);
                return;
            }
            breathRate = bytesToFloat(&frame[8]);
            newBreathRate = true;
            break;
        case FrameType::Distance:
            if (calcXorInverse(frame + 8, 8) != frame[16]) {
                setError(ErrorCode::ChecksumPayloadFail);
                return;
            }
            uint32_t flag = bytesToUInt32(&frame[8]);
            if (flag == 1) {
                distance = bytesToFloat(&frame[12]);
                newDistance = true;
            }
            break;
        default:
            setError(ErrorCode::InvalidFrameType);
            break;
    }
    
    // 成功解析后自动清除历史错误（若需保留错误直到手动处理，可注释此行）
    if (lastError != ErrorCode::NoError) clearError();
}

// Getter/Flag 清理函数保持不变（略，与原逻辑一致）
bool LD6002::hasNewHeartRate() const { return newHeartRate; }
bool LD6002::hasNewBreathRate() const { return newBreathRate; }
bool LD6002::hasNewDistance() const { return newDistance; }
float LD6002::getHeartRate() const { return heartRate; }
float LD6002::getBreathRate() const { return breathRate; }
float LD6002::getDistance() const { return distance; }
void LD6002::clearHeartRateFlag() { newHeartRate = false; }
void LD6002::clearBreathRateFlag() { newBreathRate = false; }
void LD6002::clearDistanceFlag() { newDistance = false; }

void LD6002::printHex(const uint8_t *data, int len) const {
    for (int i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
}