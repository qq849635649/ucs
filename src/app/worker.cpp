#include "worker.h"

#include <iostream>
using namespace std;

#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>

WorkerApp::WorkerApp()
{}

WorkerApp::~WorkerApp()
{}

void WorkerApp::Init()
{
    handlers_.insert(std::make_pair(string("/health"), &HealthCheckCallback));
}

// 函数功能: 健康检查窗口
void WorkerApp::HealthCheckCallback(struct evhttp_request * req, void * arg)
{
    pid_t pid =  getpid();
    char buf[100] = { 0 };
    int ret = snprintf(buf, 100, "%s-worker-%d", TG::I().GetTime(), pid);
    evbuffer_add(req->output_buffer, buf, ret);
    evhttp_send_reply(req, 200, "OK", NULL);
}
