#ifndef POLL_MYPOLL_H_
#define POLL_MYPOLL_H_

#include <poll.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @file mypoll.h
 * @brief 基于 poll(2) 的轻量级事件监听池封装。
 *
 * 提供一个进程级的全局事件循环：用 PollerAdd 注册待监听的 (fd, events)
 * 组合，用 PollerRun 阻塞等待并按回调分发就绪事件，用 PollerDel 注销单个
 * 监听项，用 PollerDestory 释放全部资源。
 *
 * @warning 本库使用全局静态状态，同一进程内只存在一个监听池，且非线程安全。
 *          多线程或多监听池场景请自行加锁或改用有状态句柄式实现。
 */

/**
 * @brief 事件就绪回调函数类型。
 *
 * @param[in] fd        触发事件的监听文件描述符。
 * @param[in] revents   就绪事件位掩码，取值可能为 POLLIN、POLLOUT、
 *                      POLLERR、POLLHUP、POLLNVAL 等的按位或。
 * @param[in] user_data 注册该 (fd, events) 时传入的用户指针，可为 NULL。
 */
typedef void (*poller_cb)(int fd, short revents, void *user_data);

/**
 * @brief 创建（或重建）全局监听池。
 *
 * @param[in] max_fds 允许注册的最大 (fd, events) 项数，建议不超过 1024；
 *                    传 0 视为非法参数。
 *
 * @return 错误码。
 * @retval 0 创建成功。
 * @retval -1 max_fds 为 0，或内存申请失败。
 *
 * @note 若监听池已存在，会先释放旧池再按新容量重建。
 * @see PollerDestory
 */
int PollerCreate(nfds_t max_fds);

/**
 * @brief 注销全局监听池并释放其占用的内存。
 *
 * @note 可重复调用；调用后可用 PollerCreate 重新创建。
 * @warning 仅释放监听池自身资源，不会关闭任何已注册的 fd，
 *          相关 fd 需由调用方自行关闭。
 * @see PollerCreate
 */
void PollerDestory(void);

/**
 * @brief 向监听池注册一个待监听的 (fd, events) 组合。
 *
 * @param[in] fd        监听的文件描述符。
 * @param[in] events    监听的事件，为 POLLIN、POLLOUT 的按位或。
 * @param[in] user_data 用户指针，事件触发时原样回传给回调，可为 NULL。
 *
 * @return 错误码。
 * @retval  0 注册成功。
 * @retval -1 监听池未创建。
 * @retval -2 监听池已满，已达 PollerCreate 指定的 max_fds。
 * @retval -3 已存在完全相同的 (fd, events) 项。
 *
 * @note 同一个 fd 可用不同的 events 重复注册；但 (fd, events) 组合必须唯一。
 * @see PollerDel
 */
int PollerAdd(int fd, short events, void *user_data);

/**
 * @brief 注销监听池中与 (fd, events) 完全匹配的项。
 *
 * @param[in] fd     要注销的文件描述符。
 * @param[in] events 注册该项时使用的 events，必须完全一致。
 *
 * @return 错误码。
 * @retval  0 注销成功。
 * @retval -1 监听池未创建。
 * @retval -2 未找到匹配的 (fd, events) 项。
 *
 * @note 删除采用末尾元素填补，不保证剩余项的顺序。
 * @see PollerAdd
 */
int PollerDel(int fd, short events);

/**
 * @brief 等待并分发监听池中已就绪的事件。
 *
 * 阻塞至多 timeout_ms，然后对本轮就绪的每个 (fd, events) 调用 callback。
 * 本轮就绪事件在调用前已快照，回调执行期间可安全地调用 PollerAdd /
 * PollerDel，不会破坏本轮遍历或丢失已就绪事件。
 *
 * @param[in]  timeout_ms 等待超时（毫秒），-1 表示无限等待。
 * @param[in]  callback   事件就绪回调，不可为 NULL。
 * @param[out] err_fd     出错 fd 的输出参数，可为 NULL；仅在返回 1 时写入。
 *
 * @return 处理结果。
 * @retval  0 正常返回，可能处理了若干事件，也可能超时。
 * @retval -1 监听池未创建，或 callback 为 NULL。
 * @retval -2 poll(2) 调用出错，具体原因打印到 stderr。
 * @retval  1 发现出错的 fd（revents 含 POLLERR/POLLHUP/POLLNVAL）：该
 *            项已自动移出监听池，出错 fd 通过 *err_fd 返回，调用方需自行关闭。
 * @retval -3 回调执行期间监听池被 PollerDestory 销毁或被 PollerCreate 重建：
 *            本轮剩余就绪项不再分发，立即返回。
 *
 * @warning 出错项会被自动移出监听池；若同一 fd 注册了多项，仅移除出错的该项。
 * @note 当 revents 同时包含普通事件与错误标志时，先回调再返回错误。
 * @note 回调内可安全调用 PollerAdd / PollerDel；若在回调内销毁（或重建）监听池，
 *       剩余分发会中止并返回 -3，调用方应据此停止使用并视需要重新 PollerCreate。
 */
int PollerRun(int timeout_ms, poller_cb callback, int *err_fd);

#ifdef __cplusplus
}
#endif

#endif
