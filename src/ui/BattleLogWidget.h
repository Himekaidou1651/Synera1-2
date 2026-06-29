/**
 * @file    BattleLogWidget.h
 * @brief   战斗日志窗口组件，显示战斗过程文字记录
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollBar>
#include <memory>

class Game;

/**
 * @class   BattleLogWidget
 * @brief   可拖拽的战斗日志悬浮窗口，实时显示战斗事件
 */
class BattleLogWidget : public QWidget {
    Q_OBJECT

public:
    explicit BattleLogWidget(QWidget *parent = nullptr);
    ~BattleLogWidget() = default;

    /**
     * @brief  根据当前游戏状态更新日志内容
     * @param  game 游戏实例指针
     */
    void updateLog(const class Game* game);

signals:
    void clearLogRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupUI();

    QTextEdit *logEdit_ = nullptr;
    QLabel *titleLabel_ = nullptr;
    QPushButton *closeBtn_ = nullptr;

    // ========== 拖拽状态 ==========
    bool isDragging_ = false;
    QPoint dragStartPos_;
};
