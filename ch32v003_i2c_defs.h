/**
 * @file ch32v003_i2c_defs.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa hằng số thanh ghi và macro phụ trợ cho thư viện I2C.
 * @version 1.0.0
 * @date 2026-09-17
 */

#ifndef CH32V003_I2C_DEFS_H
#define CH32V003_I2C_DEFS_H

#define I2C_DIRECTION_WRITE 0 /**< Bit chỉ định hướng Ghi dữ liệu */
#define I2C_DIRECTION_READ  1 /**< Bit chỉ định hướng Đọc dữ liệu */

#define I2C_TIMEOUT_COUNT   30000 /**< Ngưỡng đếm thoát hiểm ngăn chặn treo mạch */

#endif // CH32V003_I2C_DEFS_H

