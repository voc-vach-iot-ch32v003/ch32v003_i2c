#include "ch32v003_i2c.h"

#if defined(__has_include) && __has_include("ch32v003_debug.h") && (I2C_DEBUG_LOG_ENABLE == 1)
#include "ch32v003_debug.h"
#define I2C_LOG(...) printf(__VA_ARGS__)
#else
#define I2C_LOG(...)
#endif

// --- CÁC HÀM NỘI BỘ TIỆN ÍCH DÀNH RIÊNG CHO BIT-BANGING ---
static inline void i2cSoftwareDelay(void)
{
    for (volatile uint32_t i = 0; i < 30; i++);
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

        // 1. Xóa sạch cả 2 bit remap cũ của I2C1 (Bit 1 và Bit 22)
        AFIO->PCFR1 &= ~((1 << 1) | (1 << 22));

        // 2. Tách bit 0 của remapVal đưa vào Bit 1 của thanh ghi (Quản lý Remap 1)
        if (remapVal & 0x01)
        {
            AFIO->PCFR1 |= (1 << 1);
        }
        // 3. Tách bit 1 của remapVal đưa vào Bit 22 của thanh ghi (Quản lý Remap 2 & 3)
        if (remapVal & 0x02)
        {
            AFIO->PCFR1 |= (1 << 22);
        }

        // 4. Cấu hình chân chức năng ngoại vi tương ứng thích ứng theo từng tùy chọn Remap
        if (remapVal == I2C_REMAP_DEFAULT)
        {
            pinMode(MCU_I2C_SDA, AF_OD); // PC1 -> SDA Mặc định
            pinMode(MCU_I2C_SCL, AF_OD); // PC2 -> SCL Mặc định
        }
#if defined(CH32V003_F4P6)
        else if (remapVal == I2C_REMAP_OPTION_1)
        {
            // Remap 1 tương ứng SDA chân PD0, SCL chân PD1 (Lưu ý: PD1 trùng chân nạp SWIO)
            pinMode(MCU_I2C_SDA_REMAP1, AF_OD); // PD0
            pinMode(MCU_I2C_SCL_REMAP1, AF_OD); // PD1
        }
        else if (remapVal == I2C_REMAP_OPTION_2 || remapVal == I2C_REMAP_OPTION_3)
        {
            // Remap 2 & 3 tương ứng SDA chân PC6, SCL chân PC5 (Dành cho gói vỏ lớn như TSSOP20)
            pinMode(MCU_I2C_SDA_REMAP2, AF_OD); // PC6
            pinMode(MCU_I2C_SCL_REMAP2, AF_OD); // PC5
        }
#endif

        // 5. Kích hoạt xung clock nội khối I2C1
        RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

        // 6. Khai báo tần số bus hệ thống (đơn vị MHz) vào khối điều khiển để tính toán thời gian
        const uint32_t prsc = FUNCONF_SYSTEM_CORE_CLOCK / 1000000;
        I2C1->CTLR2 = prsc;

        // 7. Thiết lập hệ số chia tần số (CCR) dựa theo tốc độ clockSpeed đầu vào
        uint32_t ccr = FUNCONF_SYSTEM_CORE_CLOCK / (handle->config.hardware.clockSpeed * 2);
        if (ccr < 4) ccr = 4; // Giới hạn phần cứng tối thiểu cho xung I2C
        I2C1->CKCFGR = ccr;

        // 8. Kích hoạt hoàn toàn khối I2C phần cứng (Peripheral Enable)
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
        uint32_t timeout = I2C_TIMEOUT_COUNT;

        // Nếu chưa làm Master (không phải Restart), kiểm tra xem đường Bus có bận không
        if (!(I2C1->STAR2 & I2C_STAR2_MSL))
            while (I2C1->STAR2 & I2C_STAR2_BUSY)
            {
                if (--timeout == 0)
                {
                    I2C_LOG("[Lỗi I2C HAL] Bus bị kẹt cứng cờ BUSY.\n");
                    return 0;
                }
            }

        // Phát điều kiện START (hoặc RESTART) lên đường Bus
        I2C1->CTLR1 |= I2C_CTLR1_START;
        timeout = I2C_TIMEOUT_COUNT;
        // Chờ phần cứng xác nhận đã phát xong Start Bit (cờ SB)
        while (!(I2C1->STAR1 & I2C_STAR1_SB))
        {
            if (--timeout == 0) return 0;
        }

        // Gửi Byte địa chỉ thiết bị tớ (Dịch trái 1 bit + ghép bit hướng Đọc/Ghi)
        I2C1->DATAR = (slaveAddress << 1) | direction;
        timeout = I2C_TIMEOUT_COUNT;
        // Chờ thiết bị tớ phản hồi ACK (cờ ADDR được bật)
        while (!(I2C1->STAR1 & I2C_STAR1_ADDR))
        {
            if (--timeout == 0)
            {
                I2C_LOG("[Lỗi I2C HAL] NACK địa chỉ 0x%02X.\n", slaveAddress);
                return 0;
            }
        }

        // Đọc thanh ghi STAR2 để xóa cờ ADDR, sẵn sàng cho việc truyền nhận dữ liệu tiếp theo
        (void)I2C1->STAR2;
    }
    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    else
    {
        // Lấy thông tin chân cấu hình SDA và SCL
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
        const uint8_t addrByte = (slaveAddress << 1) | direction;
        return i2cWrite(handle, addrByte);
    }

    return 1; // Trả về 1: Khởi động chu trình START thành công
}

uint8_t i2cWrite(const I2C_DriverHandle_t* handle, uint8_t data)
{
    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        uint32_t timeout = I2C_TIMEOUT_COUNT;
        // Chờ thanh ghi đệm dữ liệu trống (cờ TXE - Transmit Buffer Empty)
        while (!(I2C1->STAR1 & I2C_STAR1_TXE))
        {
            if (--timeout == 0) return 0;
        }
        // Ghi dữ liệu vào thanh ghi dữ liệu để phần cứng tự động gửi đi
        I2C1->DATAR = data;

        timeout = I2C_TIMEOUT_COUNT;
        // Chờ dữ liệu truyền xong hoàn toàn trên đường Bus (cờ BTF - Byte Transfer Finished)
        while (!(I2C1->STAR1 & I2C_STAR1_BTF))
        {
            if (--timeout == 0) return 0;
        }
        return 1; // Truyền phần cứng thành công
    }

    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    const uint8_t sda = handle->config.software.sdaPin;
    const uint8_t scl = handle->config.software.sclPin;

    // Vòng lặp gửi lần lượt 8 bit dữ liệu (từ bit cao MSB đến bit thấp LSB)
    for (uint8_t i = 0; i < 8; i++)
    {
        // Kiểm tra bit cao nhất, nếu là 1 thì xuất HIGH, ngược lại xuất LOW
        digitalWrite(sda, (data & 0x80) ? HIGH : LOW);
        data <<= 1; // Dịch trái 1 bit để chuẩn bị cho lượt tiếp theo
        i2cSoftwareDelay();

        // Tạo một xung Clock (Kéo SCL lên CAO rồi hạ xuống THẤP) để thiết bị tớ đọc bit
        digitalWrite(scl, HIGH);
        i2cSoftwareDelay();
        digitalWrite(scl, LOW);
    }

    // Trả chân SDA về trạng thái rảnh (HIGH) để chuẩn bị nhận phản hồi ACK từ chip tớ
    digitalWrite(sda, HIGH);
    i2cSoftwareDelay();
    digitalWrite(scl, HIGH); // Bật SCL lên CAO để làm nhịp kiểm tra ACK
    i2cSoftwareDelay();

    uint8_t ack = 1;
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    // Đọc chân SDA: Nếu chip tớ kéo SDA xuống LOW nghĩa là nó đã nhận được (ACK)
    while (digitalRead(sda) == HIGH)
    {
        if (--timeout == 0)
        {
            ack = 0; // Hết thời gian chờ mà SDA vẫn HIGH -> Bị lỗi NACK
            break;
        }
    }

    // Kết thúc chu trình: Kéo SCL xuống THẤP để khóa Bus lại, sẵn sàng cho byte kế tiếp
    digitalWrite(scl, LOW);
    i2cSoftwareDelay();

    return ack; // Trả về 1 nếu nhận được ACK, 0 nếu bị NACK/Timeout
}

uint8_t i2cRead(const I2C_DriverHandle_t* handle, const uint8_t ackOption)
{
    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        // Nếu đây là byte cuối cùng muốn nhận (ackOption == 0)
        if (!ackOption)
        {
            I2C1->CTLR1 &= ~I2C_CTLR1_ACK; // 1. Tắt gửi ACK để báo hiệu dừng cho chip tớ
            I2C1->CTLR1 |= I2C_CTLR1_STOP; // 2. Đặt cờ phát STOP ngay để giải phóng bus sau byte này
        }
        // Nếu vẫn muốn đọc tiếp các byte sau (ackOption == 1)
        else
        {
            I2C1->CTLR1 |= I2C_CTLR1_ACK; // Bật gửi ACK để chip tớ tiếp tục truyền byte tiếp theo
        }

        uint32_t timeout = I2C_TIMEOUT_COUNT;
        // Chờ cho đến khi thanh ghi nhận có dữ liệu đổ về (cờ RXNE - Receive Not Empty)
        while (!(I2C1->STAR1 & I2C_STAR1_RXNE))
        {
            if (--timeout == 0) return 0;
        }

        return I2C1->DATAR; // Đọc dữ liệu ra từ thanh ghi phần cứng và trả về
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

        byte <<= 1; // Dịch trái để chuẩn bị nhận bit tiếp theo
        if (digitalRead(sda) == HIGH) byte |= 0x01; // Nếu bus ở mức CAO thì ghi nhận bit 1

        digitalWrite(scl, LOW); // Hạ xung Clock xuống mức THẤP kết thúc 1 bit
    }

    // Phát tín hiệu phản hồi (ACK/NACK): Nhận tiếp thì kéo LOW (ACK), dừng lại thì thả HIGH (NACK)
    digitalWrite(sda, ackOption ? LOW : HIGH);
    i2cSoftwareDelay();

    // Tạo thêm một xung Clock độc lập để chip tớ đọc trạng thái ACK/NACK vừa phát
    digitalWrite(scl, HIGH);
    i2cSoftwareDelay();
    digitalWrite(scl, LOW);
    i2cSoftwareDelay();

    return byte; // Trả về byte dữ liệu hoàn chỉnh đã đọc được
}

void i2cStop(const I2C_DriverHandle_t* handle)
{
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        I2C1->CTLR1 |= I2C_CTLR1_STOP;
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

uint8_t i2cReadBurst(const I2C_DriverHandle_t* handle, const uint8_t slaveAddress, const uint8_t startRegister,
                     uint8_t* buffer,
                     const uint8_t length)
{
    // Kiểm tra tính hợp lệ của tham số đầu vào (chiều dài > 0 và con trỏ lưu dữ liệu không rỗng)
    if (length == 0 || buffer == NULL) return 0;

    // --- TRƯỜNG HỢP 1: SỬ DỤNG I2C PHẦN CỨNG ---
    if (handle->driverMode == I2C_MODE_HARDWARE)
    {
        // 1. Phát START và ghi địa chỉ thanh ghi đích muốn bắt đầu đọc
        if (!i2cStart(handle, slaveAddress, I2C_DIRECTION_WRITE)) return 0;
        i2cWrite(handle, startRegister);

        // 2. Phát tín hiệu Repeated START (không giải phóng Bus để chuyển sang chế độ đọc)
        I2C1->CTLR1 |= I2C_CTLR1_START;
        uint32_t timeout = I2C_TIMEOUT_COUNT;
        while (!(I2C1->STAR1 & I2C_STAR1_SB)) { if (--timeout == 0) return 0; }

        // 3. Gửi lại địa chỉ thiết bị tớ kèm bit chọn chế độ ĐỌC (I2C_DIRECTION_READ)
        I2C1->DATAR = (slaveAddress << 1) | I2C_DIRECTION_READ;
        timeout = I2C_TIMEOUT_COUNT;
        while (!(I2C1->STAR1 & I2C_STAR1_ADDR)) { if (--timeout == 0) return 0; }

        // Bật phản hồi ACK để báo thiết bị tớ gửi tiếp mảng dữ liệu, sau đó xóa cờ ADDR
        I2C1->CTLR1 |= I2C_CTLR1_ACK;
        (void)I2C1->STAR2;

        // 4. Vòng lặp thu thập chuỗi dữ liệu đổ vào mảng buffer
        for (uint8_t i = 0; i < length; i++)
        {
            // Nếu là byte cuối cùng trong chuỗi: Tắt ACK và phát STOP ngay để chốt hạ Bus sau byte này
            if (i == length - 1)
            {
                I2C1->CTLR1 &= ~I2C_CTLR1_ACK;
                I2C1->CTLR1 |= I2C_CTLR1_STOP;
            }

            // Chờ cho đến khi bộ đệm nhận nhận đủ 1 byte dữ liệu (cờ RXNE bật)
            timeout = I2C_TIMEOUT_COUNT;
            while (!(I2C1->STAR1 & I2C_STAR1_RXNE)) { if (--timeout == 0) return 0; }

            // Đọc dữ liệu cất vào mảng
            buffer[i] = I2C1->DATAR;
        }
    }
    // --- TRƯỜNG HỢP 2: SỬ DỤNG I2C PHẦN MỀM (BIT-BANGING) ---
    else
    {
        // Ghi địa chỉ thanh ghi bắt đầu muốn đọc bằng hàm I2C phần mềm
        if (!i2cStart(handle, slaveAddress, I2C_DIRECTION_WRITE)) return 0;
        i2cWrite(handle, startRegister);

        // Phát Repeated START bằng phần mềm để đổi sang chế độ nhận dữ liệu
        if (!i2cStart(handle, slaveAddress, I2C_DIRECTION_READ)) return 0;

        // Vòng lặp gọi hàm đọc từng byte: Các byte trước gửi ACK (1), byte cuối gửi NACK (0)
        for (uint8_t i = 0; i < length; i++)
        {
            buffer[i] = i2cRead(handle, i < (length - 1));
        }

        // Phát tín hiệu STOP phần mềm để kết thúc chu trình và giải phóng Bus
        i2cStop(handle);
    }

    return 1; // Đọc chuỗi mảng thành công
}
