# 🔌 ch32v003_i2c

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Supported-orange.svg)
![Framework](https://img.shields.io/badge/Framework-ch32v003fun-green.svg)
![MCU](https://img.shields.io/badge/MCU-CH32V003-red.svg)

Thư viện I2C Master cho vi điều khiển CH32V003, dùng chung một API cho **I2C phần cứng** (khối I2C1, có Remap chân) và **I2C phần mềm** (bit-banging trên 2 chân GPIO bất kỳ). Mỗi bus được quản lý qua một Handle `I2C_DriverHandle_t`, nên một chương trình có thể dùng nhiều bus cùng lúc.

---

## 🚀 Tính năng nổi bật

- **Hai chế độ, một API:** đổi `driverMode` giữa `I2C_MODE_HARDWARE` và `I2C_MODE_SOFTWARE` mà không phải sửa code đọc/ghi.
- **Standard 100kHz / Fast 400kHz đúng chuẩn:** Fast Mode tự bật bit F/S với tỉ lệ Tlow/Thigh = 2. Tần số đo thật trên bàn test là 97.7kHz và 381kHz (sườn lên qua trở kéo 4.7kΩ làm chậm nhẹ).
- **Không kẹt bus khi thiết bị không trả lời:** gặp NACK hoặc hết thời gian chờ, thư viện tự xóa cờ lỗi và phát STOP. Giao dịch kế tiếp vẫn chạy bình thường (VD: quét địa chỉ, cảm biến bị rút ra).
- **Đọc/ghi liên tiếp nhiều thanh ghi** bằng `i2cReadBurst()` / `i2cWriteBurst()`, xử lý đúng các trường hợp đặc biệt khi đọc 1 hoặc 2 byte.
- **I2C phần mềm khoảng 100kHz** ở 48MHz, chỉnh được bằng `I2C_SOFTWARE_DELAY_LOOPS`.
- **Remap chân phần cứng** theo gói vỏ: PC1/PC2 dùng được trên cả J4M6, A4M6, F4P6. F4P6 có thêm PD0/PD1 và PC6/PC5.

---

## 📦 Cài đặt (Installation)

```ini
lib_deps =
    https://github.com/voc-vach-iot-ch32v003/ch32v003_i2c.git
```

Thư viện tự kéo theo `ch32v003_gpio` và `ch32v003_debug`.

---

## ⚙️ Cấu hình môi trường

Khai báo gói vỏ chip trong `platformio.ini` để thư viện chọn đúng sơ đồ chân và các tùy chọn Remap:

```ini
build_flags = -DCH32V003_F4P6 ; hoặc -DCH32V003_A4M6 / -DCH32V003_J4M6
```

Các macro sau có giá trị mặc định, ghi đè được trong `include/sys_config.h` của dự án:

| Macro | Mặc định | Ý nghĩa |
| :-- | :-- | :-- |
| `I2C_TIMEOUT_COUNT` | `30000` | Số vòng chờ tối đa một cờ phần cứng (~5ms) trước khi bỏ giao dịch. |
| `I2C_SOFTWARE_DELAY_LOOPS` | `5` | Số vòng chờ mỗi nửa nhịp của I2C phần mềm (5 ≈ 100kHz, tăng để chậm lại cho dây dài). |
| `I2C_DEBUG_LOG_ENABLE` | `0` | `1` = in log lỗi bus/NACK qua `printf`. Để `0` sẽ tiết kiệm Flash. |

Bus I2C cần **trở kéo lên** (thường 4.7kΩ lên 3.3V) trên cả SDA và SCL. Đa số module cảm biến đã có sẵn.

---

## 📝 Code mẫu sử dụng (Quick Start)

Đọc mã nhận dạng (Chip ID) của cảm biến BMI160 rồi bật LED nếu đúng:

```c
#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_i2c.h"

#define LED_PIN        MCU_PIN3 // Chân vật lý số 3 (J4M6: PA2, A4M6: PC3, F4P6: PD6)
#define BMI160_ADDRESS 0x68
#define BMI160_CHIP_ID 0xD1

// I2C phần cứng trên chân mặc định PC1 (SDA) / PC2 (SCL), tốc độ 400kHz
static const I2C_DriverHandle_t i2cBus = {
    .driverMode = I2C_MODE_HARDWARE,
    .config.hardware = {
        .remapOption = I2C_REMAP_DEFAULT,
        .clockSpeed = I2C_SPEED_FAST,
    },
};

static void setup(void)
{
    pinMode(LED_PIN, OUTPUT);
    i2cInit(&i2cBus);
}

static void loop(void)
{
    uint8_t chipId = 0;

    // Đọc 1 byte tại thanh ghi 0x00. Trả về 0 nếu cảm biến không trả lời (bus không bị kẹt)
    const uint8_t found = i2cReadBurst(&i2cBus, BMI160_ADDRESS, 0x00, &chipId, 1);
    digitalWrite(LED_PIN, (found && chipId == BMI160_CHIP_ID) ? HIGH : LOW);

    delayMs(500);
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Khoảng chờ an toàn tránh brick chip

    setup();
    while (1)
    {
        loop();
    }
}
```

Muốn dùng I2C phần mềm trên 2 chân bất kỳ, chỉ cần đổi Handle:

```c
static const I2C_DriverHandle_t i2cSoftware = {
    .driverMode = I2C_MODE_SOFTWARE,
    .config.software = {
        .sdaPin = MCU_PIN5, // Chân vật lý bất kỳ, tránh chân SWIO (PD1)
        .sclPin = MCU_PIN6,
    },
};
```

---

## 📁 Danh sách Ví dụ mẫu (Examples)

Thư viện giao tiếp nền: chưa có thư mục `examples/`. Các thư viện cảm biến (VD: `ch32v003_bmi160`) là ví dụ sử dụng thực tế.

---

## 📑 Tra cứu API (API Reference)

### Khởi tạo

| Hàm xử lý | Tham số | Giá trị trả về | Mô tả |
| :-- | :-- | :-- | :-- |
| `i2cInit(handle)` | `const I2C_DriverHandle_t* handle` | `uint8_t` | Cấu hình chân và tốc độ. Phần cứng: reset khối I2C1, Remap chân, tính hệ số chia. Trả `1` nếu thành công, `0` nếu `handle` rỗng. |

### Giao dịch trọn gói (khuyên dùng)

| Hàm xử lý | Tham số | Giá trị trả về | Mô tả |
| :-- | :-- | :-- | :-- |
| `i2cWriteBurst(handle, slaveAddress, startRegister, data, length)` | `uint8_t slaveAddress`, `uint8_t startRegister`, `const uint8_t* data`, `uint8_t length` | `uint8_t` | START, ghi địa chỉ thanh ghi rồi `length` byte, STOP. `length = 0` chỉ ghi địa chỉ thanh ghi. Trả `1` nếu mọi byte nhận ACK. |
| `i2cReadBurst(handle, slaveAddress, startRegister, buffer, length)` | `uint8_t slaveAddress`, `uint8_t startRegister`, `uint8_t* buffer`, `uint8_t length` | `uint8_t` | Ghi địa chỉ thanh ghi, Repeated START, đọc `length` byte (byte cuối NACK), STOP. Trả `1` nếu thành công. |

### Thao tác từng bước

| Hàm xử lý | Tham số | Giá trị trả về | Mô tả |
| :-- | :-- | :-- | :-- |
| `i2cStart(handle, slaveAddress, direction)` | `uint8_t slaveAddress`, `uint8_t direction` | `uint8_t` | START (hoặc Repeated START) + địa chỉ 7-bit với `I2C_DIRECTION_WRITE` / `I2C_DIRECTION_READ`. Trả `0` nếu NACK/hết giờ, lúc đó STOP đã được phát. |
| `i2cWrite(handle, data)` | `uint8_t data` | `uint8_t` | Ghi 1 byte, trả `1` nếu nhận ACK. |
| `i2cRead(handle, ackOption)` | `uint8_t ackOption` | `uint8_t` | Đọc 1 byte. `1` = ACK (đọc tiếp), `0` = NACK (byte cuối). Phần cứng: `0` tự phát STOP sau byte này. |
| `i2cStop(handle)` | Không | `void` | Phát STOP. Gọi thừa sau `i2cRead(handle, 0)` vẫn an toàn. |

### Kiểu dữ liệu

| Kiểu | Giá trị | Mô tả |
| :-- | :-- | :-- |
| `I2C_DriverMode_t` | `I2C_MODE_HARDWARE`, `I2C_MODE_SOFTWARE` | Chế độ của Handle. |
| `I2C_BusSpeed_t` | `I2C_SPEED_STANDARD` (100kHz), `I2C_SPEED_FAST` (400kHz) | Tốc độ I2C phần cứng. |
| `I2C_HardwareRemap_t` | `I2C_REMAP_DEFAULT` (PC1/PC2), F4P6: `I2C_REMAP_OPTION_1` (PD0/PD1), `I2C_REMAP_OPTION_2/3` (PC6/PC5) | Chân SDA/SCL của I2C phần cứng. Remap 1 dùng PD1 = SWIO: cần `setSwioMode(SWIO_MODE_GPIO)`. |
| `I2C_DriverHandle_t` | `driverMode` + `config.hardware { remapOption, clockSpeed }` hoặc `config.software { sdaPin, sclPin }` | Handle của một bus. |

---

## 📄 Giấy phép (License) & Tác giả

- **Tác giả:** [Vọc Vạch IoT](https://github.com/voc-vach-iot)
- **Giấy phép:** Phát hành theo giấy phép [MIT License](LICENSE).
