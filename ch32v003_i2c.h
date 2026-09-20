/**
 * @file ch32v003_i2c.h
 * @author Vọc Vạch IoT
 * @brief Giao diện lập trình API của thư viện điều khiển I2C đa năng cho
 * CH32V003.
 * @version 1.0.0
 * @date 2026-09-17
 */

#ifndef CH32V003_I2C_H
#define CH32V003_I2C_H

#include "ch32fun.h"
#include "ch32v003_i2c_defs.h"
#include "ch32v003_i2c_types.h"

/**
 * @brief Khởi tạo hệ thống giao tiếp I2C dựa trên cấu hình người dùng cung cấp.
 * @param handle Con trỏ quản lý cấu trúc dữ liệu I2C_DriverHandle_t.
 * @return uint8_t Trả về 1 nếu khởi tạo thành công, 0 nếu cấu hình lỗi.
 */
uint8_t i2cInit(const I2C_DriverHandle_t* handle);

/**
 * @brief Phát điều kiện START lên đường Bus vật lý.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param slaveAddress Địa chỉ 7-bit của thiết bị tớ mục tiêu.
 * @param direction Hướng truyền nhận dữ liệu (I2C_DIRECTION_WRITE hoặc
 * I2C_DIRECTION_READ).
 * @return uint8_t Trả về 1 nếu thiết bị phản hồi ACK, 0 nếu bị lỗi Timeout hoặc
 * NACK.
 */
uint8_t i2cStart(const I2C_DriverHandle_t* handle, uint8_t slaveAddress,
                 uint8_t direction);

/**
 * @brief Truyền một byte dữ liệu đơn lẻ lên đường Bus I2C.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param data Giá trị byte dữ liệu cần truyền đi.
 * @return uint8_t Trả về 1 nếu truyền thành công và nhận ACK, ngược lại trả về
 * 0.
 */
uint8_t i2cWrite(const I2C_DriverHandle_t* handle, uint8_t data);

/**
 * @brief Nhận một byte dữ liệu đơn lẻ từ thiết bị phản hồi.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param ackOption Quyết định gửi tín hiệu ACK (1) giữ chuỗi hay NACK (0) ngắt
 * chuỗi nhận.
 * @return uint8_t Giá trị byte dữ liệu đọc về từ Bus.
 */
uint8_t i2cRead(const I2C_DriverHandle_t* handle, uint8_t ackOption);

/**
 * @brief Phát lệnh điều kiện STOP giải phóng trạng thái rảnh cho đường Bus.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 */
void i2cStop(const I2C_DriverHandle_t* handle);

/**
 * @brief Đọc chuỗi mảng dữ liệu liên tiếp (Burst Read) từ thanh ghi chỉ định
 * của thiết bị tớ.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param slaveAddress Địa chỉ 7-bit của thiết bị tớ mục tiêu.
 * @param startRegister Mã thanh ghi đích bắt đầu thực hiện chuỗi đọc liên tục.
 * @param buffer Mảng con trỏ lưu trữ chuỗi dữ liệu đầu ra đổ về.
 * @param length Số lượng byte mong muốn đọc về.
 * @return uint8_t Trả về 1 nếu toàn bộ tiến trình đọc hoàn thành tốt, ngược lại
 * là 0.
 */
uint8_t i2cReadBurst(const I2C_DriverHandle_t* handle, uint8_t slaveAddress,
                     uint8_t startRegister, uint8_t* buffer, uint8_t length);

#endif // CH32V003_I2C_H#include "ch32v003_i2c.h"
