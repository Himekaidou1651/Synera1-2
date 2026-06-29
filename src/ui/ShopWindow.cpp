/**
 * @file    ShopWindow.cpp
 * @brief   商店弹窗对话框实现
 * @author  
 * @date    2026-06-24
 */

#include "ShopWindow.h"
#include "ShopWidget.h"
#include "../core/Game.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QDir>

static QPixmap loadAssetPixmapSW(const QString &relativePath) {
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 4; ++i) {
        QString candidate = dir.filePath(relativePath);
        QPixmap pixmap(candidate);
        if (!pixmap.isNull()) return pixmap;
        dir.cdUp();
    }
    return QPixmap();
}

ShopWindow::ShopWindow(std::shared_ptr<Game> game, QWidget *parent)
    : QDialog(parent), game_(game)
{
    setWindowTitle(QString::fromUtf8("商店"));
    setMinimumSize(480, 520);
    resize(540, 620);
    
    // 去掉默认标题栏的问号按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    setupUI();
}

ShopWindow::~ShopWindow() {
}

void ShopWindow::setupUI() {
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(8);
    
    // 标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(QString::fromUtf8("🏪 商店"));
    titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #ecf0f1; }");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
    // 关闭按钮
    closeBtn_ = new QPushButton(QString::fromUtf8("✕ 关闭"));
    closeBtn_->setFixedHeight(36);
    closeBtn_->setStyleSheet(
        "QPushButton { "
        "    background-color: #7f8c8d; "
        "    color: white; "
        "    border: 2px solid #6c7a7d; "
        "    border-radius: 6px; "
        "    padding: 6px 16px; "
        "    font-size: 13px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #95a5a6; } "
        "QPushButton:pressed { background-color: #6c7a7d; }"
    );
    connect(closeBtn_, &QPushButton::clicked, this, &QDialog::accept);
    titleLayout->addWidget(closeBtn_);
    mainLayout->addLayout(titleLayout);
    
    // 商店组件
    shopWidget_ = new ShopWidget(game_, this);
    mainLayout->addWidget(shopWidget_, 1);
    
    // 底部快捷键提示
    QLabel *hintLabel = new QLabel(QString::fromUtf8("快捷键: R=刷新  B=购买  Esc=关闭"));
    hintLabel->setStyleSheet("QLabel { font-size: 11px; color: #95a5a6; padding: 4px 0px; }");
    hintLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(hintLabel);
    
    // 对话框整体样式（深色背景，匹配原商店风格）
    setStyleSheet(
        "ShopWindow { "
        "    background-color: #2c3e50; "
        "    border: 2px solid #1a252f; "
        "    border-radius: 10px; "
        "}"
    );
}

void ShopWindow::updateDisplay() {
    if (shopWidget_) {
        shopWidget_->updateDisplay();
    }
}

void ShopWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        accept();
        return;
    }
    if (event->key() == Qt::Key_R && shopWidget_) {
        shopWidget_->doRefreshShop();
        updateDisplay();
        return;
    }
    if (event->key() == Qt::Key_B && shopWidget_) {
        shopWidget_->doBuySelected();
        updateDisplay();
        return;
    }
    QDialog::keyPressEvent(event);
}
