#include "ch32v003_i2c.h"

#if (I2C_DEBUG_LOG_ENABLE == 1) && defined(__has_include) && __has_include("ch32v003_debug.h")
#include "ch32v003_debug.h"
#define I2C_LOG(...) printf(__VA_ARGS__)
#else
#define I2C_LOG(...)
#endif

// --- CÁC HÀM NỘI BỘ TIỆN ÍCH DÀNH RIÊNG CHO I2C PHẦN CỨNG ---

/**
 * @brief Chờ một cờ trong STAR1 bật lên, thoát sớm nếu thiết bị tớ phản hồi NACK (cờ AF).
 * @return 1 nếu cờ đã bật, 0 nếu NACK hoặc hết thời gian chờ.
 */
static uint8_t i2cHardwareWaitFlag(const uint16_t flag)
{
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    while (!(I2C1->STAR1 & flag))
    {
        if ((I2C1->STAR1 & I2C_STAR1_AF) || --timeout == 0)
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Hủy giao dịch đang dở: xóa cờ NACK và phát STOP để thiết bị tớ nhả Bus.
 * @details Nếu bỏ qua bước này, cờ AF và trạng thái Master bị treo khiến mọi giao dịch sau đều lỗi.
 */
static void i2cHardwareAbort(void)
{
    I2C1->STAR1 &= ~I2C_STAR1_AF;

    // Đã nhả Bus (VD: i2cRead(0) tự phát STOP): đặt STOP lúc này sẽ treo lơ lửng và chặn START kế tiếp
    if (!(I2C1->STAR2 & I2C_STAR2_MSL)) return;
    I2C1->CTLR1 |= I2C_CTLR1_STOP;

    // Chờ phần cứng phát xong STOP (bit tự xóa) trước khi cho phép START mới
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    while ((I2C1->CTLR1 & I2C_CTLR1_STOP) && --timeout)
    {
    }
}

/**
 * @brief Phát START + địa chỉ ở chế độ phần cứng, không xóa cờ ADDR (để hàm gọi tự quyết định thứ tự ACK).
 */
static uint8_t i2cHardwareStartAddress(const uint8_t slaveAddress, const uint8_t direction)
{
    // Nếu chưa làm Master (không phải Restart), kiểm tra xem đường Bus có bận không
    if (!(I2C1->STAR2 & I2C_STAR2_MSL))
    {
        uint32_t timeout = I2C_TIMEOUT_COUNT;
        while (I2C1->STAR2 & I2C_STAR2_BUSY)
        {
            if (--timeout == 0)
            {
                I2C_LOG("[Lỗi I2C HAL] Bus bị kẹt cứng cờ BUSY.\n");
                return 0;
            }
        }
    }

    // Phát điều kiện START (hoặc RESTART) và chờ phần cứng xác nhận (cờ SB)
    I2C1->CTLR1 |= I2C_CTLR1_START;
    if (!i2cHardwareWaitFlag(I2C_STAR1_SB))
    {
        i2cHardwareAbort();
        return 0;
    }

    // Gửi Byte địa chỉ thiết bị tớ (Dịch trái 1 bit + ghép bit hướng Đọc/Ghi) và chờ ACK (cờ ADDR)
    I2C1->DATAR = (slaveAddress << 1) | direction;
    if (!i2cHardwareWaitFlag(I2C_STAR1_ADDR))
    {
        I2C_LOG("[Lỗi I2C HAL] NACK địa chỉ 0x%02X.\n", slaveAddress);
        i2cHardwareAbort();
        return 0;
    }
    return 1;
}

// --- CÁC HÀM NỘI BỘ TIỆN ÍCH DÀNH RIÊNG CHO BIT-BANGING ---
static inline void i2cSoftwareDelay(void)
{
    for (volatile uint32_t i = 0; i < I2C_SOFTWARE_DELAY_LOOPS; i++);
}

// --- THỰC THI HÀM KHỞI TẠO HỆ THỐNG ---
uint8_t i2cInit(const I2C_DriverHandle_t* handle)
{
    if (handle == NULL) return 0;

    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        // Kích hoạt xung clock cho khối AFIO trước tiên để mở khóa thanh ghi PCFR1
        RCC->APB2PCENR |= RCC_APB2Periph_AFIO;

        const uint8_t remapVal = handle->config.hardware.remapOption;

        // 1. Ghi 2 bit remap của I2C1: bit 0 của remapVal -> Bit 1 (Remap 1), bit 1 -> Bit 22 (Remap 2 & 3)
        AFIO->PCFR1 &= ~((1UL << 1) | (1UL << 22));
        if (remapVal & 0x01)
        {
            AFIO->PCFR1 |= (1UL << 1);
        }
        if (remapVal & 0x02)
        {
            AFIO->PCFR1 |= (1UL << 22);
        }

        // 2. Cấu hình chân chức năng ngoại vi tương ứng thích ứng theo từng tùy chọn Remap
        if (remapVal == I2C_REMAP_DEFAULT)
        {
            pinMode(MCU_I2C_SDA, AF_OD); // PC1 -> SDA Mặc định
            pinMode(MCU_I2C_SCL, AF_OD); // PC2 -> SCL Mặc định
        }
#if defined(CH32V003_F4P6)
        else if (remapVal == I2C_REMAP_OPTION_1)
        {
            // Remap 1: SDA chân PD0, SCL chân PD1 (Lưu ý: PD1 trùng chân nạp SWIO, cần setSwioMode(SWIO_MODE_GPIO))
            pinMode(MCU_I2C_SDA_REMAP1, AF_OD); // PD0
            pinMode(MCU_I2C_SCL_REMAP1, AF_OD); // PD1
        }
        else
        {
            // Remap 2 & 3: SDA chân PC6, SCL chân PC5 (chỉ có trên TSSOP20)
            pinMode(MCU_I2C_SDA_REMAP2, AF_OD); // PC6
            pinMode(MCU_I2C_SCL_REMAP2, AF_OD); // PC5
        }
#endif

        // 3. Cấp clock và reset khối I2C1 để thoát mọi trạng thái kẹt từ lần chạy trước
        RCC->APB1PCENR |= RCC_APB1Periph_I2C1;
        RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
        RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

        // 4. Khai báo tần số bus hệ thống (MHz) - chỉ ghi được khi PE = 0
        I2C1->CTLR1 &= ~I2C_CTLR1_PE;
        I2C1->CTLR2 = FUNCONF_SYSTEM_CORE_CLOCK / 1000000;

        // 5. Hệ số chia: Standard Tlow = Thigh (CCR = F/2f), Fast Tlow = 2*Thigh (CCR = F/3f) kèm bit F/S
        const uint32_t speed = handle->config.hardware.clockSpeed;
        uint32_t ccr;
        if (speed > I2C_SPEED_STANDARD)
        {
            ccr = FUNCONF_SYSTEM_CORE_CLOCK / (speed * 3);
            if (ccr < 1) ccr = 1;
            ccr |= I2C_CKCFGR_FS;
        }
        else
        {
            ccr = FUNCONF_SYSTEM_CORE_CLOCK / (speed * 2);
            if (ccr < 4) ccr = 4; // Giới hạn phần cứng tối thiểu ở Standard Mode
        }
        I2C1->CKCFGR = ccr;

        // 6. Kích hoạt hoàn toàn khối I2C phần cứng (Peripheral Enable)
        I2C1->CTLR1 |= I2C_CTLR1_PE;
    }
    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    else
    {
        // Khởi tạo các chân IO bất kỳ ở chế độ Output Open-Drain để kéo dải bus
        pinMode(handle->config.software.sdaPin, OUTPUT_OD);
        pinMode(handle->config.software.sclPin, OUTPUT_OD);

        // Đưa đường truyền về trạng thái rảnh (Cả hai dây SDA và SCL đều ở mức CAO)
        digitalWrite(handle->config.software.sdaPin, HIGH);
        digitalWrite(handle->config.software.sclPin, HIGH);
        i2cSoftwareDelay();
    }

    return 1; // Khởi tạo thành công
}

uint8_t i2cStart(const I2C_DriverHandle_t* handle, const uint8_t slaveAddress, const uint8_t direction)
{
    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        if (!i2cHardwareStartAddress(slaveAddress, direction))
        {
            return 0;
        }
        // Đọc thanh ghi STAR2 để xóa cờ ADDR, sẵn sàng cho việc truyền nhận dữ liệu tiếp theo
        (void)I2C1->STAR2;
        return 1;
    }

    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    const uint8_t sda = handle->config.software.sdaPin;
    const uint8_t scl = handle->config.software.sclPin;

    // Tạo chuỗi xung START bằng tay: Kéo SDA xuống mức THẤP trong khi SCL vẫn ở mức CAO
    digitalWrite(sda, HIGH);
    digitalWrite(scl, HIGH);
    i2cSoftwareDelay();
    digitalWrite(sda, LOW);
    i2cSoftwareDelay();
    digitalWrite(scl, LOW);
    i2cSoftwareDelay();

    // Gộp địa chỉ với hướng truyền và gửi byte này đi qua hàm I2C phần mềm
    if (!i2cWrite(handle, (slaveAddress << 1) | direction))
    {
        I2C_LOG("[Lỗi I2C SW] NACK địa chỉ 0x%02X.\n", slaveAddress);
        i2cStop(handle);
        return 0;
    }
    return 1;
}

uint8_t i2cWrite(const I2C_DriverHandle_t* handle, uint8_t data)
{
    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        // Chờ thanh ghi đệm trống (TXE), ghi dữ liệu rồi chờ truyền xong hoàn toàn (BTF)
        if (!i2cHardwareWaitFlag(I2C_STAR1_TXE))
        {
            return 0;
        }
        I2C1->DATAR = data;
        return i2cHardwareWaitFlag(I2C_STAR1_BTF);
    }

    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    const uint8_t sda = handle->config.software.sdaPin;
    const uint8_t scl = handle->config.software.sclPin;

    // Vòng lặp gửi lần lượt 8 bit dữ liệu (từ bit cao MSB đến bit thấp LSB)
    for (uint8_t i = 0; i < 8; i++)
    {
        digitalWrite(sda, (data & 0x80) ? HIGH : LOW);
        data <<= 1;
        i2cSoftwareDelay();

        // Tạo một xung Clock (Kéo SCL lên CAO rồi hạ xuống THẤP) để thiết bị tớ đọc bit
        digitalWrite(scl, HIGH);
        i2cSoftwareDelay();
        digitalWrite(scl, LOW);
    }

    // Thả SDA (HIGH) rồi tạo xung clock thứ 9 để lấy mẫu ACK từ chip tớ
    digitalWrite(sda, HIGH);
    i2cSoftwareDelay();
    digitalWrite(scl, HIGH);
    i2cSoftwareDelay();

    // Chip tớ kéo SDA xuống LOW trong lúc SCL ở mức CAO nghĩa là ACK
    const uint8_t ack = (digitalRead(sda) == LOW);

    // Kết thúc chu trình: Kéo SCL xuống THẤP để khóa Bus lại, sẵn sàng cho byte kế tiếp
    digitalWrite(scl, LOW);
    i2cSoftwareDelay();

    return ack; // Trả về 1 nếu nhận được ACK, 0 nếu bị NACK
}

uint8_t i2cRead(const I2C_DriverHandle_t* handle, const uint8_t ackOption)
{
    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        if (!ackOption)
        {
            I2C1->CTLR1 &= ~I2C_CTLR1_ACK; // Byte cuối: tắt ACK để báo dừng cho chip tớ
            I2C1->CTLR1 |= I2C_CTLR1_STOP; // Đặt cờ phát STOP ngay sau byte này
        }
        else
        {
            I2C1->CTLR1 |= I2C_CTLR1_ACK; // Bật ACK để chip tớ tiếp tục truyền byte kế tiếp
        }

        // Chờ cho đến khi thanh ghi nhận có dữ liệu đổ về (cờ RXNE - Receive Not Empty)
        if (!i2cHardwareWaitFlag(I2C_STAR1_RXNE))
        {
            return 0;
        }
        return I2C1->DATAR;
    }

    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    const uint8_t sda = handle->config.software.sdaPin;
    const uint8_t scl = handle->config.software.sclPin;
    uint8_t byte = 0;

    // Thả chân SDA lên HIGH để chip tớ có thể kéo xuống nhằm truyền dữ liệu
    digitalWrite(sda, HIGH);

    // Vòng lặp đọc lần lượt 8 bit dữ liệu từ bus
    for (uint8_t i = 0; i < 8; i++)
    {
        i2cSoftwareDelay();
        digitalWrite(scl, HIGH); // Tạo xung Clock mức CAO để chốt dữ liệu trên đường bus
        i2cSoftwareDelay();

        byte <<= 1;
        if (digitalRead(sda) == HIGH) byte |= 0x01;

        digitalWrite(scl, LOW);
    }

    // Phát tín hiệu phản hồi: Nhận tiếp thì kéo LOW (ACK), dừng lại thì thả HIGH (NACK)
    digitalWrite(sda, ackOption ? LOW : HIGH);
    i2cSoftwareDelay();
    digitalWrite(scl, HIGH);
    i2cSoftwareDelay();
    digitalWrite(scl, LOW);
    i2cSoftwareDelay();

    return byte;
}

void i2cStop(const I2C_DriverHandle_t* handle)
{
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        i2cHardwareAbort();
        return;
    }

    const uint8_t sda = handle->config.software.sdaPin;
    const uint8_t scl = handle->config.software.sclPin;

    digitalWrite(sda, LOW);
    i2cSoftwareDelay();
    digitalWrite(scl, HIGH);
    i2cSoftwareDelay();
    digitalWrite(sda, HIGH);
    i2cSoftwareDelay();
}

uint8_t i2cWriteBurst(const I2C_DriverHandle_t* handle, const uint8_t slaveAddress, const uint8_t startRegister,
                      const uint8_t* data, const uint8_t length)
{
    if (length > 0 && data == NULL) return 0;

    if (!i2cStart(handle, slaveAddress, I2C_DIRECTION_WRITE)) return 0;

    uint8_t ok = i2cWrite(handle, startRegister);
    for (uint8_t i = 0; ok && i < length; i++)
    {
        ok = i2cWrite(handle, data[i]);
    }

    i2cStop(handle);
    return ok;
}

uint8_t i2cReadBurst(const I2C_DriverHandle_t* handle, const uint8_t slaveAddress, const uint8_t startRegister,
                     uint8_t* buffer, const uint8_t length)
{
    // Kiểm tra tính hợp lệ của tham số đầu vào (chiều dài > 0 và con trỏ lưu dữ liệu không rỗng)
    if (length == 0 || buffer == NULL) return 0;

    // 1. Phát START và ghi địa chỉ thanh ghi đích muốn bắt đầu đọc
    if (!i2cStart(handle, slaveAddress, I2C_DIRECTION_WRITE)) return 0;
    if (!i2cWrite(handle, startRegister))
    {
        i2cStop(handle);
        return 0;
    }

    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        // 2. Repeated START + địa chỉ kèm bit ĐỌC (giữ Bus, chưa xóa cờ ADDR)
        if (!i2cHardwareStartAddress(slaveAddress, I2C_DIRECTION_READ)) return 0;

        // 3. Thứ tự bắt buộc: chốt ACK/NACK TRƯỚC khi xóa ADDR (đọc STAR2) để byte đầu nhận đúng phản hồi
        if (length == 1)
        {
            I2C1->CTLR1 &= ~I2C_CTLR1_ACK;
            (void)I2C1->STAR2;
            I2C1->CTLR1 |= I2C_CTLR1_STOP;
        }
        else
        {
            I2C1->CTLR1 |= I2C_CTLR1_ACK;
            (void)I2C1->STAR2;
        }

        // 4. Vòng lặp thu thập chuỗi dữ liệu đổ vào mảng buffer
        for (uint8_t i = 0; i < length; i++)
        {
            // Byte cuối cùng (khi length > 1): tắt ACK và hẹn STOP ngay khi byte trước vừa được đọc
            if (length > 1 && i == length - 1)
            {
                I2C1->CTLR1 &= ~I2C_CTLR1_ACK;
                I2C1->CTLR1 |= I2C_CTLR1_STOP;
            }

            if (!i2cHardwareWaitFlag(I2C_STAR1_RXNE))
            {
                i2cHardwareAbort();
                return 0;
            }
            buffer[i] = I2C1->DATAR;
        }
        return 1;
    }

    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    // Phát Repeated START bằng phần mềm để đổi sang chế độ nhận dữ liệu
    if (!i2cStart(handle, slaveAddress, I2C_DIRECTION_READ)) return 0;

    // Các byte trước gửi ACK (1), byte cuối gửi NACK (0)
    for (uint8_t i = 0; i < length; i++)
    {
        buffer[i] = i2cRead(handle, i < (length - 1));
    }

    i2cStop(handle);
    return 1;
}
