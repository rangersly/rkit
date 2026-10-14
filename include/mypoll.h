#ifndef POLL_MYPOLL_H_
#define POLL_MYPOLL_H_

#include <poll.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// 回调函数格式定义
typedef void (*poller_cb)(int fd, short revents, void *user_data);

/*
 * PollerCreate : 创建poller监听池
 * max_fds : 申请的最大监听fd数量,建议根据服务要求进行设置,不建议超过 1024
 * Return :
 * 0  ->  正常退出
 * -1 ->  内存空间申请失败
 */
int PollerCreate(nfds_t max_fds);

/*
 * PollerDestory : 注销poller监听池,释放空间,可再次创建
 */
void PollerDestory(void);

/*
 * PollerAdd : 向poller中添加监听fd
 *  fd : 监听的文件描述符
 *  events : 监听的事件(POLLIN可读 | POLLOUT可写)
 *  user_data : 需要保存的用户数据,连接数据等
 * 同一个fd允许重复添加,以events区分;但完全相同的(fd, events)只允许存在一项
 * Return :
 *  -1 -> poller监听池未创建
 *  -2 -> poller监听池已满
 *  -3 -> 已存在相同的(fd, events)项
 */
int PollerAdd(int fd, short events, void *user_data);

/*
 * PollerDel : 删除监听池中(fd, events)匹配的项,不保证保持原顺序
 *  fd : 要删除的监听文件描述符
 *  events : 要删除项监听的events,须与添加时一致
 * Return :
 *  0  -> 正常退出
 *  -1 -> 监听池未创建
 *  -2 -> 未找到需要删除的内容
 */
int PollerDel(int fd, short events);

/*
 * PollerRun : 处理监听池中的连接
 *  timeout_ms : 等待事件的时间,-1为无限等待,单位是ms
 *  callback : 用户自定义的处理函数
 *  err_fd : 出错fd的输出参数,可为NULL,仅当返回1时被写入
 * Return :
 *  0  -> 正常退出
 *  -1 -> 监听池未创建或callback为空
 *  -2 -> poll函数内部错误
 *  1  -> 存在出错fd,已自动移出监听池并通过*err_fd返回,需手动关闭
 */
int PollerRun(int timeout_ms, poller_cb callback, int *err_fd);

#ifdef __cplusplus
}
#endif

#endif
