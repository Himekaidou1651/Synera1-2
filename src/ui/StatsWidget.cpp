/**
 * @file    StatsWidget.cpp
 * @brief   统计报表组件实现
 * @author  
 * @date    2026-06-24
 */

#include "StatsWidget.h"
#include "ChartWidget.h"
#include "../core/Game.h"
#include "../core/Player.h"
#include "../core/Bench.h"
#include "../core/Commondata.h"
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <algorithm>

StatsWidget::StatsWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedSize(STATSWIDGET_WIDTH, STATSWIDGET_HEIGHT);
    setStyleSheet(
        "StatsWidget {"
        "    background-color: rgba(18, 22, 28, 240);"
        "    border: 1px solid rgba(255, 255, 255, 0.12);"
        "    border-radius: 14px;"
        "}"
    );

    setupUI();
}

void StatsWidget::setupUI() {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(PANEL_MARGIN_LR, PANEL_MARGIN_TB, PANEL_MARGIN_LR, PANEL_MARGIN_TB);
    rootLayout->setSpacing(LAYOUT_SPACING_TIGHT);

    // 标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setStyleSheet("background: transparent;");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(SHOP_MARGIN, 2, SHOP_MARGIN, 2);

    titleLabel_ = new QLabel("📊 统计报表", this);
    titleLabel_->setStyleSheet(
        "QLabel {"
        "    font-size: 15px;"
        "    font-weight: bold;"
        "    color: #5dade2;"
        "    background: transparent;"
        "}"
    );
    titleLayout->addWidget(titleLabel_);
    titleLayout->addStretch();

    closeBtn_ = new QPushButton("✕", this);
    closeBtn_->setFixedSize(BUTTON_SIZE_SMALL, BUTTON_SIZE_SMALL);
    closeBtn_->setToolTip("关闭");
    closeBtn_->setStyleSheet(
        "QPushButton {"
        "    color: rgba(255,255,255,0.5);"
        "    border: none;"
        "    background: transparent;"
        "    font-size: 14px;"
        "    border-radius: 14px;"
        "}"
        "QPushButton:hover {"
        "    color: #ffffff;"
        "    background: rgba(255,255,255,0.15);"
        "}"
    );
    titleLayout->addWidget(closeBtn_);

    rootLayout->addWidget(titleBar);

    // ===== 标签页 =====
    QString tabStyle =
        "QTabWidget::pane { border: 1px solid rgba(255,255,255,0.08); border-radius: 6px; background: rgba(0,0,0,0.2); } "
        "QTabBar::tab { "
        "  background-color: rgba(44, 62, 80, 180); color: #95a5a6; "
        "  padding: 6px 14px; font-weight: bold; font-size: 11px; "
        "  border-top-left-radius: 4px; border-top-right-radius: 4px; "
        "  border: 1px solid rgba(255,255,255,0.06); border-bottom: none; "
        "} "
        "QTabBar::tab:selected { "
        "  background-color: rgba(52, 73, 94, 220); color: #5dade2; "
        "  border-bottom: 2px solid #5dade2; "
        "} "
        "QTabBar::tab:hover:!selected { "
        "  background-color: rgba(52, 73, 94, 200); color: #bdc3c7; "
        "}";

    tabWidget_ = new QTabWidget();
    tabWidget_->setStyleSheet(tabStyle);

    // ========= 标签页1: 每局支出 =========
    expenditureTab_ = new QWidget();
    QVBoxLayout *expLayout = new QVBoxLayout(expenditureTab_);
    expLayout->setContentsMargins(6, 6, 6, 6);
    expenditureChart_ = new ChartWidget(expenditureTab_);
    expenditureChart_->setTitle("每局支出");
    expenditureChart_->setLineColor(QColor(231, 76, 60));
    expenditureChart_->setValueSuffix(" 金");
    expLayout->addWidget(expenditureChart_);
    tabWidget_->addTab(expenditureTab_, "💰 支出");

    // ========= 标签页2: 每局收入 =========
    incomeTab_ = new QWidget();
    QVBoxLayout *incLayout = new QVBoxLayout(incomeTab_);
    incLayout->setContentsMargins(6, 6, 6, 6);
    incomeChart_ = new ChartWidget(incomeTab_);
    incomeChart_->setTitle("每局收入");
    incomeChart_->setLineColor(QColor(46, 204, 113));
    incomeChart_->setValueSuffix(" 金");
    incLayout->addWidget(incomeChart_);
    tabWidget_->addTab(incomeTab_, "📈 收入");

    // ========= 标签页3: 每局净收入 =========
    netIncomeTab_ = new QWidget();
    QVBoxLayout *netLayout = new QVBoxLayout(netIncomeTab_);
    netLayout->setContentsMargins(6, 6, 6, 6);
    netIncomeChart_ = new ChartWidget(netIncomeTab_);
    netIncomeChart_->setTitle("每局净收入");
    netIncomeChart_->setLineColor(QColor(241, 196, 15));
    netIncomeChart_->setValueSuffix(" 金");
    netLayout->addWidget(netIncomeChart_);
    tabWidget_->addTab(netIncomeTab_, "⚖ 净收入");

    // ========= 标签页4: 己方单位列表 =========
    unitsTab_ = new QWidget();
    QVBoxLayout *unitsLayout = new QVBoxLayout(unitsTab_);
    unitsLayout->setContentsMargins(6, 6, 6, 6);
    unitsEdit_ = new QTextEdit(unitsTab_);
    unitsEdit_->setReadOnly(true);
    unitsEdit_->setStyleSheet(
        "QTextEdit {"
        "    background-color: rgba(0, 0, 0, 0.3);"
        "    border: 1px solid rgba(255, 255, 255, 0.06);"
        "    border-radius: 6px;"
        "    padding: 6px 8px;"
        "    font-size: 12px;"
        "    color: #ecf0f1;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "}"
        "QScrollBar:vertical {"
        "    background: rgba(255,255,255,0.05);"
        "    width: 8px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(255,255,255,0.15);"
        "    border-radius: 4px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
    );
    unitsLayout->addWidget(unitsEdit_);
    tabWidget_->addTab(unitsTab_, "👥 单位");

    rootLayout->addWidget(tabWidget_, 1);

    // 连接关闭按钮
    connect(closeBtn_, &QPushButton::clicked, this, [this]() {
        hide();
    });
}

void StatsWidget::updateStats(const Game* game) {
    if (!game) return;
    updateExpenditureTab(game);
    updateIncomeTab(game);
    updateNetIncomeTab(game);
    updateUnitsTab(game);
}

void StatsWidget::updateExpenditureTab(const Game* game) {
    const auto& log = game->getBattleLog();
    std::vector<ChartWidget::DataPoint> data;
    for (const auto& entry : log) {
        int spent = entry.getGoldSpent();
        if (spent < 0) spent = 0; // 避免负数
        data.push_back({entry.round, static_cast<double>(spent)});
    }
    expenditureChart_->setData(data);
}

void StatsWidget::updateIncomeTab(const Game* game) {
    const auto& log = game->getBattleLog();
    std::vector<ChartWidget::DataPoint> data;
    for (const auto& entry : log) {
        data.push_back({entry.round, static_cast<double>(entry.totalGoldEarned)});
    }
    incomeChart_->setData(data);
}

void StatsWidget::updateNetIncomeTab(const Game* game) {
    const auto& log = game->getBattleLog();
    std::vector<ChartWidget::DataPoint> data;
    for (const auto& entry : log) {
        data.push_back({entry.round, static_cast<double>(entry.getNetIncome())});
    }
    netIncomeChart_->setData(data);
}

void StatsWidget::updateUnitsTab(const Game* game) {
    if (!unitsEdit_) return;

    const auto& player = game->getPlayer();
    const auto& bench = player.getBench();
    const auto& board = game->getBoard();

    QString html;
    html += "<style>"
            "  .u-section { margin-bottom: 10px; }"
            "  .u-title { font-size: 13px; font-weight: bold; color: #5dade2; margin-bottom: 4px; }"
            "  .u-card { background: rgba(255,255,255,0.06); border-radius: 6px; padding: 6px 8px; margin-bottom: 4px; }"
            "  .u-name { color: #ecf0f1; font-weight: bold; font-size: 12px; }"
            "  .u-star { color: #f1c40f; font-size: 11px; }"
            "  .u-info { color: rgba(255,255,255,0.6); font-size: 11px; }"
            "  .u-hp { color: #e74c3c; }"
            "  .u-atk { color: #e67e22; }"
            "  .u-def { color: #3498db; }"
            "  .u-empty { color: rgba(255,255,255,0.3); text-align: center; padding: 20px; font-size: 12px; }"
            "  .u-sep { border: none; border-top: 1px solid rgba(255,255,255,0.06); margin: 6px 0; }"
            "</style>";

    // 棋盘上的单位
    html += "<div class='u-section'>";
    html += "<div class='u-title'>⚔ 棋盘单位</div>";
    auto boardUnits = board.getUnitsForOwner(Owner::PlayerCtrl);
    bool hasBoardUnits = false;
    for (const auto& unit : boardUnits) {
        if (unit) {
            hasBoardUnits = true;
            QString stars;
            for (int s = 0; s < unit->getStarLevel(); ++s) stars += "⭐";
            html += "<div class='u-card'>";
            html += QString("<div><span class='u-name'>%1</span> <span class='u-star'>%2</span></div>")
                .arg(QString::fromStdString(unit->debugName())).arg(stars);
            html += QString("<div class='u-info'>"
                "<span class='u-hp'>❤ %1</span> | "
                "<span class='u-atk'>⚔ %2</span> | "
                "⭐ %3</div>")
                .arg(unit->getHP()).arg(unit->getATK())
                .arg(unit->getStarLevel());
            html += "</div>";
        }
    }
    if (!hasBoardUnits) {
        html += "<div class='u-empty'>棋盘上没有单位</div>";
    }
    html += "</div>";

    html += "<hr class='u-sep'>";

    // 备战区单位
    html += "<div class='u-section'>";
    html += "<div class='u-title'>🪑 备战区单位</div>";
    bool hasBenchUnits = false;
    for (int i = 0; i < BENCH_SIZE; ++i) {
        auto unit = bench.getUnit(i);
        if (unit) {
            hasBenchUnits = true;
            QString stars;
            for (int s = 0; s < unit->getStarLevel(); ++s) stars += "⭐";
            html += "<div class='u-card'>";
            html += QString("<div><span class='u-name'>%1</span> <span class='u-star'>%2</span></div>")
                .arg(QString::fromStdString(unit->debugName())).arg(stars);
            html += QString("<div class='u-info'>"
                "<span class='u-hp'>❤ %1</span> | "
                "<span class='u-atk'>⚔ %2</span> | "
                "⭐ %3</div>")
                .arg(unit->getHP()).arg(unit->getATK())
                .arg(unit->getStarLevel());
            html += "</div>";
        }
    }
    if (!hasBenchUnits) {
        html += "<div class='u-empty'>备战区没有单位</div>";
    }
    html += "</div>";

    unitsEdit_->setHtml(html);
}

// ===== 拖拽支持 =====

void StatsWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging_ = true;
        dragStartPos_ = event->globalPosition().toPoint() - mapToParent(QPoint(0, 0));
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void StatsWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging_) {
        move(event->globalPosition().toPoint() - dragStartPos_);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void StatsWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging_ = false;
        event->accept();
    }
    QWidget::mouseReleaseEvent(event);
}
