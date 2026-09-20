# 🔌 ch32v003_i2c

Thư viện Driver tinh gọn hỗ trợ song song **I2C Hardware** (hỗ trợ Remap chân linh hoạt) và **I2C Software (Bit-banging)** dành cho vi điều khiển **CH32V003**. Thư viện hoạt động trên nền framework **ch32v003fun**, tối ưu hóa bộ nhớ, hỗ trợ chống treo bus với Timeout và cung cấp API đọc ghi liên tục (Burst Read) tiện lợi.

---

## 🚀 Tính năng nổi bật

- **Hỗ trợ chế độ Kép (Dual Mode):** Chuyển đổi linh hoạt giữa I2C phần cứng (`I2C_MODE_HARDWARE`) và I2C phần mềm bit-banging (`I2C_MODE_SOFTWARE`) thông qua cấu trúc quản lý `I2C_DriverHandle_t`.
- **Ánh xạ chân Remap phần cứng linh hoạt:** Tự động hỗ trợ các tùy chọn Remap chân cho I2C1 qua thanh ghi `AFIO_PCFR1` (`I2C_REMAP_DEFAULT`, `I2C_REMAP_OPTION_1`, `I2C_REMAP_OPTION_2/3`), tương thích với mọi gói vỏ (`SOP8`, `SOP16`, `TSSOP20`).
- **Bit-banging tùy biến chân tự do:** Với chế độ Software, cho phép định nghĩa bất kỳ chân GPIO nào làm `SDA` và `SCL` thông qua hằng số sơ đồ chân `MCU_Pin_t`.
- **Cơ chế chống treo bus (Timeout Guard):** Tích hợp ngưỡng đếm `I2C_TIMEOUT_COUNT` giúp ngăn chặn hiện tượng treo chương trình khi thiết bị tớ (Slave) gặp sự cố hoặc mất kết nối.
- **Tích hợp Burst Read:** Hàm `i2cReadBurst()` hỗ trợ đọc nhanh chuỗi mảng dữ liệu từ các thanh ghi liên tiếp của thiết bị tớ (như cảm biến MPU6050, RTC, EEPROM...).

---

## 📑 Cấu trúc hàm (API Reference)

### 1. API Khởi tạo và Điều khiển Bus I2C

| Hàm xử lý                                                               | Tham số                                                                                                                  | Giá trị trả về | Mô tả                                                                                                                                                         |
| ----------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ | -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`i2cInit(handle)`**                                                   | `const I2C_DriverHandle_t* handle`                                                                                       | `uint8_t`      | Khởi tạo cấu hình I2C (Hardware/Software, tốc độ clock, remap chân, cấu hình GPIO) theo tham số. Trả về `1` nếu thành công, `0` nếu lỗi.                      |
| **`i2cStart(handle, slaveAddress, direction)`**                         | `const I2C_DriverHandle_t* handle`, `uint8_t slaveAddress`, `uint8_t direction`                                          | `uint8_t`      | Phát điều kiện START/RESTART kèm địa chỉ 7-bit và hướng truyền (`I2C_DIRECTION_WRITE` / `I2C_DIRECTION_READ`). Trả về `1` khi nhận ACK, `0` nếu NACK/Timeout. |
| **`i2cWrite(handle, data)`**                                            | `const I2C_DriverHandle_t* handle`, `uint8_t data`                                                                       | `uint8_t`      | Truyền 1 byte dữ liệu lên đường Bus I2C. Trả về `1` khi thành công nhận ACK, `0` nếu lỗi.                                                                     |
| **`i2cRead(handle, ackOption)`**                                        | `const I2C_DriverHandle_t* handle`, `uint8_t ackOption`                                                                  | `uint8_t`      | Nhận 1 byte dữ liệu từ thiết bị. Truyền `1` để phát ACK tiếp tục đọc, `0` để phát NACK dừng đọc.                                                              |
| **`i2cStop(handle)`**                                                   | `const I2C_DriverHandle_t* handle`                                                                                       | `void`         | Phát điều kiện STOP để giải phóng trạng thái rảnh cho đường Bus I2C.                                                                                          |
| **`i2cReadBurst(handle, slaveAddress, startRegister, buffer, length)`** | `const I2C_DriverHandle_t* handle`, `uint8_t slaveAddress`, `uint8_t startRegister`, `uint8_t* buffer`, `uint8_t length` | `uint8_t`      | Đọc liên tục một mảng byte dữ liệu từ thanh ghi chỉ định `startRegister`. Trả về `1` khi toàn bộ tiến trình thành công, ngược lại là `0`.                     |

---

## ⚙️ Cấu hình môi trường (`platformio.ini`)

Khai báo phụ thuộc thư viện và cờ cấu hình gói vỏ chip tương ứng trong `platformio.ini`:

```ini
[env:ch32v003f4p6]
platform = ch32v
board = ch32v003f4p6
framework = ch32v003fun

; Khai báo gói vỏ để mở rộng tính năng Remap (Ví dụ: TSSOP20 / F4P6)
build_flags =
    -DCH32V003_F4P6
    -I .

; Thư viện phụ thuộc tự động kéo về từ Git
lib_deps =
    https://github.com/voc-vach-iot-ch32v003/ch32v003_gpio.git
    https://github.com/voc-vach-iot-ch32v003/ch32v003_debug.git

```

---

## 📝 Code mẫu sử dụng

```c
#include "ch32fun.h"
#include "ch32v003_i2c.h"

// 1. Cấu hình I2C Phần cứng (Hardware I2C) - Mặc định PC1 (SDA), PC2 (SCL)
I2C_DriverHandle_t i2cHardware = {
    .driverMode = I2C_MODE_HARDWARE,
    .config.hardware = {
        .remapOption = I2C_REMAP_DEFAULT,
        .clockSpeed = I2C_SPEED_FAST // 400kHz
    }
};

// 2. Cấu hình I2C Phần mềm (Software Bit-banging) trên chân GPIO tùy chọn
I2C_DriverHandle_t i2cSoftware = {
    .driverMode = I2C_MODE_SOFTWARE,
    .config.software = {
        .sdaPin = MCU_PIN3, // Chân vật lý tùy chọn
        .sclPin = MCU_PIN4
    }
};

void setup() {
    // Khởi tạo Driver I2C phần cứng
    i2cInit(&i2cHardware);
}

void loop() {
    uint8_t sensorAddr = 0x68; // Địa chỉ I2C của cảm biến (ví dụ MPU6050)
    uint8_t dataBuffer[6];

    // Đọc liên tiếp 6 byte dữ liệu từ thanh ghi 0x3B
    if (i2cReadBurst(&i2cHardware, sensorAddr, 0x3B, dataBuffer, 6)) {
        // Xử lý dữ liệu nhận về thành công...
    }

    delayMs(100);
}

int main() {
    SystemInit();
    setup();
    while (1) {
        loop();
    }
}

```
