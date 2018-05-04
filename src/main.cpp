/******************************************************************************
 * filename : main.cpp
 * author   : zhaoyongfei
 * email    : zhaoyongfei@360.cn
 * desc     : 仿造ucs服务端模型搭建
******************************************************************************/

#include <getopt.h>
#include "config.h"
#include "mainloop.h"
#include "process.h"
#include "server.h"
#include "log/logger.h"
#include "log/timegeneration.h"
#include "util/os_util.h"

// app
#include "app/application.h"
#include "app/master.h"
#include "app/worker.h"

int main(int argc, char * argv[])
{
    char ch;
    //配置文件的位置
    bool daemon = false;
    const char *configPath = "./etc/base.conf"; // 默认配置文件
    const char *pidPath = "./proc/ucs.pid";     // 默认的pid文件
    while(-1 != (ch = getopt(argc, argv, "f:dp:th")))
    {
        switch(ch)
        {
        case 'f':   // 手动指定配置文件的位置
            configPath = optarg;
            break;
        case 'd':   // 是否采取守护进程方式运行程序
            daemon = true;
            break;
        case 'p':   // 指定进程文件
            pidPath = optarg;
            break;
        case 't':   // 检测配置文件是否可用
            return 0;
        case 'h':   // 打印帮助信息
            break;
        default:
            break;
        }
    }

    // 解析配置文件
    MConfig::I().loadConfFile(configPath);

    // 初始化日志
    char debug_log_path[256];
    snprintf(debug_log_path, 255, "%s/debug", MConfig::I().log.path.data());
    Util::mkdirs(debug_log_path);
    snprintf(debug_log_path, 255, "%s/debug.master.log", debug_log_path);
    Debugger::I().Init(MConfig::I().log.level, debug_log_path);

    Process proc(argv);
    if(daemon)
        proc.daemon();
    proc.SignalBind();    // 信号绑定

    // 创建服务端控制器
    Server server(&(MConfig::I().base.master_addr), &(MConfig::I().base.worker_addr));

    //创建服务路由
    BaseApp *worker = new WorkerApp;
    worker->Init();
    BaseApp *master = new MasterApp;
    master->Init();

    // 创建工作线程
    switch(server.GenerateWorker(MConfig::I().base.workers))
    {
    case P_MASTER:
        MainLoop::I().Init();
        server.PidFile(pidPath);        //将pid存入文件
        proc.SetProcessTitle("master");
        TG::I().Start(MainLoop::I().GetBase()); // 时间管理
        server.AddApplication(master, MainLoop::I().GetBase(), P_MASTER);
        break;
    case P_SINGLE:
        MainLoop::I().Init();
        server.PidFile(pidPath);        //将pid存入文件
        proc.SetProcessTitle("single");
        server.AddApplication(master, MainLoop::I().GetBase(), P_MASTER);
        server.AddApplication(worker, MainLoop::I().GetBase(), P_WORKER);
        TG::I().Start(MainLoop::I().GetBase()); // 时间管理
        break;
    case P_WORKER:
        MainLoop::I().Init();
        proc.SetProcessTitle("worker");
        server.AddApplication(worker, MainLoop::I().GetBase(), P_WORKER);
        break;
    }

    Debugger::I().log(Debugger::d_pain, "START WORKING...");
    MainLoop::I().Run();
    Debugger::I().log(Debugger::d_pain, "STOP WORKING...");

	return 0;
}
