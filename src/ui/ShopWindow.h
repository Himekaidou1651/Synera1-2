/**
 * @file    ShopWindow.h
 * @brief   商店弹窗对话框，包装 ShopWidget 并提供键盘操作
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QDialog>
#include <memory>

class Game;
class ShopWidget;
class QPushButton;
class QKeyEvent;

/**
 * @class   ShopWindow
 * @brief   商店弹窗对话框，提供独立的商店操作界面
 */
class ShopWindow : public QDialog {
    Q_OBJECT

public:
    ShopWindow(std::shared_ptr<Game> game, QWidget *parent = nullptr);
    ~ShopWindow();

    void updateDisplay();
    ShopWidget* shopWidget() const { return shopWidget_; }

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUI();

    std::shared_ptr<Game> game_;
    ShopWidget *shopWidget_ = nullptr;
    QPushButton *closeBtn_ = nullptr;
};
