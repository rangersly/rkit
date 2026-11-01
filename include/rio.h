#ifndef RKIT_IO_H_
#define RKIT_IO_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @file rio.h
 * @brief 非阻塞 read(2) / write(2) 的薄封装。
 *
 * 只做单次系统调用,并在内部对 EINTR 自动重试。封装不修改 fd 的阻塞模式,
 * 也不保证写完或读满:调用方需自行把 fd 设为非阻塞,并根据返回值处理部分
 * 读写与 EAGAIN/EWOULDBLOCK。
 *
 * @warning 传入阻塞 fd 时,本模块的函数会阻塞,与非阻塞语义不符。
 */

/**
 * @brief 从 fd 读取数据,单次 read(2),遇到 EINTR 自动重试。
 *
 * @param[in]  fd  待读取的文件描述符。
 * @param[out] buf 接收数据的缓冲区。
 * @param[in]  len 缓冲区长度。
 *
 * @return 读取结果。
 * @retval >0 实际读到的字节数。
 * @retval  0 对端已关闭(EOF)。
 * @retval -1 读取失败或当前不可读,errno 保留原值;当 errno 为 EAGAIN 或
 *            EWOULDBLOCK 时表示非阻塞 fd 暂时无数据可读。
 *
 * @note 不做循环读满;一次调用最多返回 len 字节。
 * @see IoWrite
 */
ssize_t IoRead(int fd, void *buf, size_t len);

/**
 * @brief 向 fd 写入数据,单次 write(2),遇到 EINTR 自动重试。
 *
 * @param[in] fd  待写入的文件描述符。
 * @param[in] buf 待写入的数据。
 * @param[in] len 待写入的字节数。
 *
 * @return 写入结果。
 * @retval >0 实际写入的字节数,可能小于 len。
 * @retval -1 写入失败或当前不可写,errno 保留原值;当 errno 为 EAGAIN 或
 *            EWOULDBLOCK 时表示非阻塞 fd 暂时不可写。
 *
 * @note 不做 write-all;调用方需自行处理部分写入。
 * @see IoRead
 */
ssize_t IoWrite(int fd, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
