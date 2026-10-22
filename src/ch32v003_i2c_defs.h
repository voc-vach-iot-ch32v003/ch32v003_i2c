/**
 * @file ch32v003_i2c_defs.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa hằng số thanh ghi và macro phụ trợ cho thư viện I2C.
 */

#ifndef CH32V003_I2C_DEFS_H
#define CH32V003_I2C_DEFS_H

// Cho phép người dùng ghi đè cấu hình qua include/sys_config.h của dự án
#if defined(__has_include)
#if __has_include("sys_config.h")
#include "sys_config.h"
#endif
#endif

#define I2C_DIRECTION_WRITE 0 /**< Bit chỉ định hướng Ghi dữ liệu */
#define I2C_DIRECTION_READ  1 /**< Bit chỉ định hướng Đọc dữ liệu */

#ifndef I2C_TIMEOUT_COUNT
#define I2C_TIMEOUT_COUNT   30000 /**< Ngưỡng đếm vòng lặp thoát hiểm ngăn chặn treo mạch (~5ms) */
#endif

#ifndef I2C_SOFTWARE_DELAY_LOOPS
#define I2C_SOFTWARE_DELAY_LOOPS 5 /**< Số vòng chờ mỗi nửa nhịp của I2C phần mềm (5 ≈ 100kHz ở 48MHz, tăng để chậm lại) */
#endif

#ifndef I2C_DEBUG_LOG_ENABLE
#define I2C_DEBUG_LOG_ENABLE 0 /**< 1 = In log lỗi bus/NACK qua printf (cần ch32v003_debug), 0 = Tắt để tiết kiệm Flash */
#endif

#endif // CH32V003_I2C_DEFS_H
