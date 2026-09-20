/**
 * @file ch32v003_i2c_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa cấu trúc dữ liệu và enum cấu hình hệ thống I2C cho CH32V003.
 * @version 1.0.0
 * @date 2026-09-17
 */

#ifndef CH32V003_I2C_TYPES_H
#define CH32V003_I2C_TYPES_H

#include "ch32v003_gpio.h"

/**
 * @brief Danh mục phân loại chế độ hoạt động của Driver I2C.
 */
typedef enum
{
    I2C_MODE_HARDWARE = 0, /**< Sử dụng khối ngoại vi chuyên dụng I2C1 */
    I2C_MODE_SOFTWARE = 1 /**< Sử dụng phần mềm giả lập tín hiệu Bit-banging */
} I2C_DriverMode_t;

/**
 * @brief Tùy chọn cấu hình Remap chân vật lý cho I2C Hardware (Dựa trên AFIO_PCFR1).
 * @details Tự động ẩn các cấu hình chân remap nâng cao nếu chip sử dụng là gói vỏ hẹp SOP8 (J4M6).
 */
typedef enum
{
    I2C_REMAP_DEFAULT = 0, /**< SDA = PC1, SCL = PC2 - Mặc định (Tương thích tốt với mọi gói chân J4M6 / A4M6 / F4P6) */

#if defined(CH32V003_F4P6)
    I2C_REMAP_OPTION_1 = 1, /**< SDA = PD0, SCL = PD1 - Yêu cầu cấu hình cho gói vỏ F4P6 (SWIO) */
    I2C_REMAP_OPTION_2 = 2, /**< SDA = PC6, SCL = PC5 - Dành riêng cho thiết kế gói vỏ TSSOP20 / F4P6 */
    I2C_REMAP_OPTION_3 = 3 /**< SDA = PC6, SCL = PC5 - Cấu hình ánh xạ trùng lặp trên F4P6 */
#endif
} I2C_HardwareRemap_t;

/**
 * @brief Tốc độ truyền nhận của Bus I2C.
 */
typedef enum
{
    I2C_SPEED_STANDARD = 100000, /**< Chế độ tiêu chuẩn Standard Mode 100kHz */
    I2C_SPEED_FAST = 400000 /**< Chế độ tốc độ cao Fast Mode 400kHz */
} I2C_BusSpeed_t;

/**
 * @brief Cấu hình chi tiết cho tầng I2C Hardware.
 */
typedef struct
{
    I2C_HardwareRemap_t remapOption; /**< Chọn cấu hình ánh xạ chân vật lý */
    I2C_BusSpeed_t clockSpeed; /**< Chọn tốc độ hoạt động của Bus */
} I2C_HardwareConfiguration_t;

/**
 * @brief Cấu hình linh hoạt chân vật lý cho tầng I2C Software.
 */
typedef struct
{
    uint8_t sdaPin; /**< Chân truyền dữ liệu vật lý (Lấy từ sơ đồ MCU_Pin_t) */
    uint8_t sclPin; /**< Chân cấp xung nhịp vật lý (Lấy từ sơ đồ MCU_Pin_t) */
} I2C_SoftwareConfiguration_t;

/**
 * @brief Cấu trúc quản lý ngữ cảnh Driver I2C tổng thể.
 */
typedef struct
{
    I2C_DriverMode_t driverMode; /**< Chế độ Driver hoạt động (Hardware/Software) */
    union
    {
        I2C_HardwareConfiguration_t hardware; /**< Cấu hình phần cứng chuyên dụng */
        I2C_SoftwareConfiguration_t software; /**< Cấu hình phần mềm Bit-banging */
    } config;
} I2C_DriverHandle_t;

#endif // CH32V003_I2C_TYPES_H
