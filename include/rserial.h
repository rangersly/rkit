#ifndef RKIT_SERIAL_H_
#define RKIT_SERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @file rserial.h
 * @brief 串口打开与配置的封装。
 *
 * SerialOpen 以非阻塞方式打开串口设备,配置为原始模式下的 8N1、无流控,
 * 并丢弃打开前可能残留的输入数据。成功返回的文件描述符可直接配合 rio.h
 * 的 IoRead / IoWrite 以及 mypoll.h 的监听池使用。
 */

/**
 * @brief 打开并配置串口,成功返回文件描述符。
 *
 * 打开标志为 O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC,随后通过 termios
 * 配置为原始模式、8 数据位、无校验、1 停止位、无软硬件流控,并设置输入
 * 与输出波特率。
 *
 * @param[in] path 串口设备路径,如 "/dev/ttyUSB0"、"/dev/ttyS0";不可为 NULL。
 * @param[in] baud 整数波特率,取值 9600、19200、38400、57600、115200;
 *                 必须与对端设备一致。
 *
 * @return 打开结果。
 * @retval >=0 配置完成的串口文件描述符。
 * @retval  -1 打开、参数校验或配置失败,errno 保留真实原因。
 *
 * @note path 为 NULL 或 baud 不在支持列表时以 EINVAL 失败返回;
 *       失败路径会关闭已打开的 fd。
 * @note fd 为非阻塞,O_NONBLOCK 已设置,读写可能返回 EAGAIN/EWOULDBLOCK。
 * @warning 本函数不负责关闭返回的 fd,调用方需自行 close。
 */
int SerialOpen(const char *path, int baud);

#ifdef __cplusplus
}
#endif

#endif
