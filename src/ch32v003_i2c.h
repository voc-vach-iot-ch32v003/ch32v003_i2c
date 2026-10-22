/**
 * @file ch32v003_i2c.h
 * @author Vọc Vạch IoT
 * @brief Giao diện lập trình API của thư viện điều khiển I2C đa năng cho CH32V003.
 */

#ifndef CH32V003_I2C_H
#define CH32V003_I2C_H

#include "ch32fun.h"
#include "ch32v003_i2c_defs.h"
#include "ch32v003_i2c_types.h"

/**
 * @brief Khởi tạo hệ thống giao tiếp I2C dựa trên cấu hình người dùng cung cấp.
 * @details Chế độ phần cứng: tự cấp clock, reset khối I2C1, remap chân và tính hệ số chia
 *          (Fast Mode 400kHz tự bật bit F/S với tỉ lệ Tlow/Thigh = 2).
 * @param handle Con trỏ quản lý cấu trúc dữ liệu I2C_DriverHandle_t.
 * @return uint8_t Trả về 1 nếu khởi tạo thành công, 0 nếu cấu hình lỗi.
 */
uint8_t i2cInit(const I2C_DriverHandle_t* handle);

/**
 * @brief Phát điều kiện START (hoặc Repeated START) và gửi địa chỉ thiết bị tớ.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param slaveAddress Địa chỉ 7-bit của thiết bị tớ mục tiêu.
 * @param direction Hướng truyền nhận dữ liệu (I2C_DIRECTION_WRITE hoặc I2C_DIRECTION_READ).
 * @return uint8_t Trả về 1 nếu thiết bị phản hồi ACK, 0 nếu lỗi Timeout hoặc NACK.
 * @note Khi trả về 0, thư viện đã tự phát STOP để giải phóng Bus.
 */
uint8_t i2cStart(const I2C_DriverHandle_t* handle, uint8_t slaveAddress, uint8_t direction);

/**
 * @brief Truyền một byte dữ liệu đơn lẻ lên đường Bus I2C.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param data Giá trị byte dữ liệu cần truyền đi.
 * @return uint8_t Trả về 1 nếu truyền thành công và nhận ACK, ngược lại trả về 0.
 */
uint8_t i2cWrite(const I2C_DriverHandle_t* handle, uint8_t data);

/**
 * @brief Nhận một byte dữ liệu đơn lẻ từ thiết bị phản hồi.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param ackOption Quyết định gửi tín hiệu ACK (1) giữ chuỗi hay NACK (0) ngắt chuỗi nhận.
 * @return uint8_t Giá trị byte dữ liệu đọc về từ Bus.
 * @note Chế độ phần cứng: khi ackOption = 0, STOP được tự phát ngay sau byte này.
 */
uint8_t i2cRead(const I2C_DriverHandle_t* handle, uint8_t ackOption);

/**
 * @brief Phát lệnh điều kiện STOP giải phóng trạng thái rảnh cho đường Bus.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 */
void i2cStop(const I2C_DriverHandle_t* handle);

/**
 * @brief Ghi chuỗi mảng dữ liệu liên tiếp (Burst Write) bắt đầu từ thanh ghi chỉ định.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param slaveAddress Địa chỉ 7-bit của thiết bị tớ mục tiêu.
 * @param startRegister Mã thanh ghi đích bắt đầu ghi.
 * @param data Mảng dữ liệu cần ghi.
 * @param length Số lượng byte cần ghi (cho phép 0 để chỉ ghi địa chỉ thanh ghi).
 * @return uint8_t Trả về 1 nếu mọi byte đều nhận ACK, ngược lại là 0.
 */
uint8_t i2cWriteBurst(const I2C_DriverHandle_t* handle, uint8_t slaveAddress,
                      uint8_t startRegister, const uint8_t* data, uint8_t length);

/**
 * @brief Đọc chuỗi mảng dữ liệu liên tiếp (Burst Read) từ thanh ghi chỉ định của thiết bị tớ.
 * @param handle Con trỏ quản lý Driver ngữ cảnh.
 * @param slaveAddress Địa chỉ 7-bit của thiết bị tớ mục tiêu.
 * @param startRegister Mã thanh ghi đích bắt đầu thực hiện chuỗi đọc liên tục.
 * @param buffer Mảng con trỏ lưu trữ chuỗi dữ liệu đầu ra đổ về.
 * @param length Số lượng byte mong muốn đọc về.
 * @return uint8_t Trả về 1 nếu toàn bộ tiến trình đọc hoàn thành tốt, ngược lại là 0.
 */
uint8_t i2cReadBurst(const I2C_DriverHandle_t* handle, uint8_t slaveAddress,
                     uint8_t startRegister, uint8_t* buffer, uint8_t length);

#endif // CH32V003_I2C_H
