/**
 * @file    StatsWidget.h
 * @brief   统计报表组件，展示金币收支折线图与单位数据
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <memory>

class Game;
class ChartWidget;

/**
 * @class   StatsWidget
 * @brief   可拖拽的统计报表悬浮窗，包含收支趋势图和单位信息
 */
class StatsWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatsWidget(QWidget *parent = nullptr);
    ~StatsWidget() = default;

    /**
     * @brief  根据当前游戏状态更新统计内容
     * @param  game 游戏实例指针
     */
    void updateStats(const class Game* game);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupUI();
    void updateExpenditureTab(const Game* game);
    void updateIncomeTab(const Game* game);
    void updateNetIncomeTab(const Game* game);
    void updateUnitsTab(const Game* game);

    QTabWidget *tabWidget_ = nullptr;

    // 四个标签页
    QWidget *expenditureTab_ = nullptr;
    QWidget *incomeTab_ = nullptr;
    QWidget *netIncomeTab_ = nullptr;
    QWidget *unitsTab_ = nullptr;

    // 折线图
    ChartWidget *expenditureChart_ = nullptr;
    ChartWidget *incomeChart_ = nullptr;
    ChartWidget *netIncomeChart_ = nullptr;

    // 单位列表
    QTextEdit *unitsEdit_ = nullptr;

    // 标题和关闭
    QLabel *titleLabel_ = nullptr;
    QPushButton *closeBtn_ = nullptr;

    // 拖拽状态
    bool isDragging_ = false;
    QPoint dragStartPos_;
};
