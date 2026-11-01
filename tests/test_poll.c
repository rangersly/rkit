#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#include "mypoll.h"

static int failures = 0;

void ShowTest(int test);

static int cb_count = 0;
static int cb_fd[16];
static short cb_events[16];

static void RecordCb(int fd, short revents, void *user_data) {
    (void)user_data;
    if (cb_count < 16) {
        cb_fd[cb_count] = fd;
        cb_events[cb_count] = revents;
    }
    cb_count++;
}

static void ResetCb(void) { cb_count = 0; }

static int Seen(int fd, short events) {
    int n = cb_count < 16 ? cb_count : 16;
    for (int i = 0; i < n; i++) {
        if (cb_fd[i] == fd && (cb_events[i] & events)) {
            return 1;
        }
    }
    return 0;
}

static int reentry_first = -1;
static int reentry_second = -1;

static void ReentryCb(int fd, short revents, void *user_data) {
    (void)revents;
    (void)user_data;
    if (cb_count < 16) {
        cb_fd[cb_count] = fd;
        cb_events[cb_count] = POLLIN;
    }
    cb_count++;
    if (fd == reentry_first) {
        PollerDel(reentry_second, POLLIN);
    }
}

static void DestroyCb(int fd, short revents, void *user_data) {
    (void)fd;
    (void)revents;
    (void)user_data;
    cb_count++;
    PollerDestory();
}

int main(void) {
    printf("==================\n");
    printf("===Test running===\n");
    printf("==================\n\n");

    int p[2];
    int q[2];
    int err;

    printf("---生命周期与非法调用---\n");
    printf("Create(0) 拒绝零容量 : ");
    ShowTest(PollerCreate(0) == -1);
    printf("未创建时 Add : ");
    ShowTest(PollerAdd(0, POLLIN, NULL) == -1);
    printf("未创建时 Del : ");
    ShowTest(PollerDel(0, POLLIN) == -1);
    printf("未创建时 Run : ");
    ShowTest(PollerRun(0, RecordCb, NULL) == -1);
    printf("Create(16) 成功 : ");
    ShowTest(PollerCreate(16) == 0);

    printf("\n---Add 重复fd与capacity---\n");
    pipe(p);
    printf("Add(fd, POLLIN) : ");
    ShowTest(PollerAdd(p[0], POLLIN, (void *)0x1) == 0);
    printf("Add(同fd, POLLOUT) 允许 : ");
    ShowTest(PollerAdd(p[0], POLLOUT, (void *)0x2) == 0);
    printf("Add(完全相同fd,POLLIN) 拒绝 : ");
    ShowTest(PollerAdd(p[0], POLLIN, (void *)0x3) == -3);
    printf("Add(完全相同fd,POLLOUT) 拒绝 : ");
    ShowTest(PollerAdd(p[0], POLLOUT, (void *)0x4) == -3);
    PollerDestory();

    PollerCreate(2);
    pipe(p);
    PollerAdd(p[0], POLLIN, NULL);
    PollerAdd(p[1], POLLIN, NULL);
    printf("超出capacity Add : ");
    ShowTest(PollerAdd(p[0], POLLOUT, NULL) == -2);
    PollerDestory();

    printf("\n---Del 按(fd, events)区分---\n");
    PollerCreate(16);
    pipe(p);
    pipe(q);
    PollerAdd(p[0], POLLIN, (void *)0x1);
    PollerAdd(p[0], POLLOUT, (void *)0x2);
    PollerAdd(q[0], POLLIN, (void *)0x3);
    printf("Del(p0, POLLOUT) : ");
    ShowTest(PollerDel(p[0], POLLOUT) == 0);
    printf("重复 Del(p0, POLLOUT) : ");
    ShowTest(PollerDel(p[0], POLLOUT) == -2);
    ResetCb();
    write(p[1], "x", 1);
    PollerRun(0, RecordCb, &err);
    printf("删除POLLOUT后POLLIN项仍在 : ");
    ShowTest(Seen(p[0], POLLIN) && !Seen(p[0], POLLOUT));
    printf("Del(q0, POLLIN) : ");
    ShowTest(PollerDel(q[0], POLLIN) == 0);
    printf("Del(p0, POLLIN) : ");
    ShowTest(PollerDel(p[0], POLLIN) == 0);
    printf("空池 Del : ");
    ShowTest(PollerDel(p[0], POLLIN) == -2);
    PollerDestory();

    printf("\n---Run 事件分发与超时---\n");
    PollerCreate(16);
    pipe(p);
    pipe(q);
    PollerAdd(p[0], POLLIN, NULL);
    PollerAdd(q[0], POLLIN, NULL);
    write(p[1], "x", 1);
    write(q[1], "y", 1);
    ResetCb();
    printf("Run 返回0 : ");
    ShowTest(PollerRun(0, RecordCb, &err) == 0);
    printf("回调命中两个fd : ");
    ShowTest(cb_count == 2 && Seen(p[0], POLLIN) && Seen(q[0], POLLIN));
    PollerDestory();

    PollerCreate(16);
    int r[2];
    pipe(r);
    PollerAdd(r[0], POLLIN, NULL);
    ResetCb();
    printf("无事件时超时返回0 : ");
    ShowTest(PollerRun(10, RecordCb, &err) == 0 && cb_count == 0);
    PollerDestory();

    printf("\n---Run 回调重入(快照分发)---\n");
    PollerCreate(16);
    pipe(p);
    pipe(q);
    reentry_first = p[0];
    reentry_second = q[0];
    PollerAdd(p[0], POLLIN, NULL);
    PollerAdd(q[0], POLLIN, NULL);
    write(p[1], "x", 1);
    write(q[1], "y", 1);
    ResetCb();
    PollerRun(0, ReentryCb, &err);
    printf("回调内Del其它fd后本轮不丢事件 : ");
    ShowTest(cb_count == 2);
    PollerDestory();

    printf("\n---Run 回调内Destory(重入销毁)---\n");
    PollerCreate(16);
    pipe(p);
    pipe(q);
    PollerAdd(p[0], POLLIN, NULL);
    PollerAdd(q[0], POLLIN, NULL);
    write(p[1], "x", 1);
    write(q[1], "y", 1);
    ResetCb();
    err = 0;
    printf("回调内Destory返回-3且不崩溃 : ");
    ShowTest(PollerRun(0, DestroyCb, &err) == -3);
    printf("Destory后剩余就绪项不再分发 : ");
    ShowTest(cb_count == 1);
    err = 0;
    printf("Destory后再次Run失败 : ");
    ShowTest(PollerRun(0, DestroyCb, &err) == -1);
    PollerDestory();

    printf("\n---Run 出错fd回传---\n");
    PollerCreate(16);
    int t[2];
    pipe(t);
    int bad = t[0];
    close(bad);
    close(t[1]);
    PollerAdd(bad, POLLIN, NULL);
    err = -1;
    printf("Run 返回1且err_fd正确 : ");
    ShowTest(PollerRun(0, RecordCb, &err) == 1 && err == bad);
    printf("出错fd已移出监听池 : ");
    ShowTest(PollerDel(bad, POLLIN) == -2);
    printf("err_fd 可为NULL : ");
    PollerAdd(bad, POLLIN, NULL);
    ShowTest(PollerRun(0, RecordCb, NULL) == 1);
    PollerDestory();

    printf("\n---Destory 后可重建---\n");
    printf("重建并Add成功 : ");
    ShowTest(PollerCreate(4) == 0 && PollerAdd(1, POLLIN, NULL) == 0);
    PollerDestory();

    printf("\n==================\n");
    if (failures == 0) {
        printf("\e[32mALL TESTS PASSED\e[0m\n");
    } else {
        printf("\e[31m%d TEST(S) FAILED\e[0m\n", failures);
    }
    printf("==================\n");

    return failures == 0 ? 0 : 1;
}

void ShowTest(int test) {
    if (test) {
        printf("\e[32mOK!\e[0m\n");
    } else {
        printf("\e[31mERROR!\e[0m\n");
        failures++;
    }
}
