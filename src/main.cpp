/**
 * @file    main.cpp
 * @brief   应用程序入口，初始化字体、创建主窗口并启动事件循环
 * @author  
 * @date    2026-06-24
 */

#include <QApplication>
#include "ui/MainWindow.h"
#include "ui/FontConfig.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 初始化全局字体配置
    setupAppFont();
    
    // 创建主窗口（包含开场动画和主菜单）
    MainWindow *mainWindow = new MainWindow();
    mainWindow->initialize();  // 开场动画在 initialize 中自动播放
    mainWindow->show();
    
    return app.exec();
}
