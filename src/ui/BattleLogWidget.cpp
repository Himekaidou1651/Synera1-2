/**
 * @file    BattleLogWidget.cpp
 * @brief   战斗日志窗口组件实现
 * @author  
 * @date    2026-06-24
 */

#include "BattleLogWidget.h"
#include "../core/Game.h"
#include "../core/Commondata.h"
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

BattleLogWidget::BattleLogWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedSize(BATTLELOG_WIDTH, BATTLELOG_HEIGHT);
    setStyleSheet(
        "BattleLogWidget {"
        "    background-color: rgba(18, 22, 28, 240);"
        "    border: 1px solid rgba(255, 255, 255, 0.12);"
        "    border-radius: 14px;"
        "}"
    );

    setupUI();
}

void BattleLogWidget::setupUI() {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(PANEL_MARGIN_LR, PANEL_MARGIN_TB, PANEL_MARGIN_LR, PANEL_MARGIN_TB);
    rootLayout->setSpacing(LAYOUT_SPACING_TIGHT);

    // 标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setStyleSheet("background: transparent;");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(SHOP_MARGIN, 2, SHOP_MARGIN, 2);

    titleLabel_ = new QLabel("📜 战斗日志", this);
    titleLabel_->setStyleSheet(
        "QLabel {"
        "    font-size: 15px;"
        "    font-weight: bold;"
        "    color: #f1c40f;"
        "    background: transparent;"
        "}"
    );
    titleLayout->addWidget(titleLabel_);
    titleLayout->addStretch();

    // 清空按钮
    QPushButton *clearBtn = new QPushButton("🗑", this);
    clearBtn->setFixedSize(BUTTON_SIZE_SMALL, BUTTON_SIZE_SMALL);
    clearBtn->setToolTip("清空日志");
    clearBtn->setStyleSheet(
        "QPushButton {"
        "    color: rgba(255,255,255,0.5);"
        "    border: none;"
        "    background: transparent;"
        "    font-size: 14px;"
        "    border-radius: 14px;"
        "}"
        "QPushButton:hover {"
        "    color: #e74c3c;"
        "    background: rgba(255,255,255,0.1);"
        "}"
    );
    titleLayout->addWidget(clearBtn);

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

    // 战斗日志文本框
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setStyleSheet(
        "QTextEdit {"
        "    background-color: rgba(0, 0, 0, 0.3);"
        "    border: 1px solid rgba(255, 255, 255, 0.06);"
        "    border-radius: 8px;"
        "    padding: 8px 10px;"
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
    rootLayout->addWidget(logEdit_, 1);

    // 连接关闭按钮
    connect(closeBtn_, &QPushButton::clicked, this, [this]() {
        hide();
        if (parentWidget()) {
            parentWidget()->setProperty("logVisible", false);
        }
    });

    // 连接清空按钮（发射信号，由 MainWindow 处理清空游戏数据）
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        logEdit_->clear();
        emit clearLogRequested();
    });
}

void BattleLogWidget::updateLog(const Game* game) {
    if (!game || !logEdit_) return;

    const auto& log = game->getBattleLog();
    const auto& aiLog = game->getAiBehaviorLog();
    bool showAiInfo = game->getShowAiInfo();

    QString html;
    html += "<style>"
            "  .log-entry { margin-bottom: 8px; padding: 6px 8px; border-radius: 6px; }"
            "  .log-win { background-color: rgba(39, 174, 96, 0.2); border-left: 3px solid #27ae60; }"
            "  .log-loss { background-color: rgba(231, 76, 60, 0.2); border-left: 3px solid #e74c3c; }"
            "  .log-round { font-weight: bold; color: #f1c40f; font-size: 13px; }"
            "  .log-detail { font-size: 11px; color: rgba(255,255,255,0.7); margin-top: 3px; line-height: 1.4; }"
            "  .log-ai { font-size: 11px; color: rgba(231, 76, 60, 0.85); margin-top: 2px; padding: 2px 6px; }"
            "  .log-ai-title { font-weight: bold; color: #e74c3c; font-size: 12px; margin-top: 6px; }"
            "  .log-sep { border: none; border-top: 1px solid rgba(255,255,255,0.06); margin: 2px 0; }"
            "  .log-ai-sep { border: none; border-top: 1px dashed rgba(231, 76, 60, 0.3); margin: 4px 0; }"
            "</style>";

    // 显示战斗记录
    for (int i = 0; i < static_cast<int>(log.size()); ++i) {
        const auto& entry = log[i];
        QString cls = entry.playerWon ? "log-win" : "log-loss";
        QString icon = entry.playerWon ? "🏆" : "💀";

        html += QString("<div class='log-entry %1'>").arg(cls);
        html += QString("<div class='log-round'>%1 回合 %2</div>").arg(icon).arg(entry.round);
        html += QString("<div class='log-detail'>%1</div>").arg(QString::fromStdString(entry.summary));
        html += "</div>";

        if (i < static_cast<int>(log.size()) - 1) {
            html += "<hr class='log-sep'>";
        }
    }

    // 显示 AI 行为日志
    if (showAiInfo && !aiLog.empty()) {
        html += "<hr class='log-ai-sep'>";
        html += "<div class='log-ai-title'>🤖 敌方 AI 行为</div>";
        
        // 只显示最近的 30 条日志
        int startIdx = std::max(0, static_cast<int>(aiLog.size()) - 30);
        for (int i = startIdx; i < static_cast<int>(aiLog.size()); ++i) {
            QString entryText = QString::fromStdString(aiLog[i]);
            // 用不同颜色标记不同类型
            QString color = "rgba(231, 76, 60, 0.85)";
            if (entryText.startsWith("===")) {
                color = "#f1c40f";
            } else if (entryText.contains(QString::fromUtf8("\xe8\xb4\xad\xe4\xb9\xb0\xe4\xba\x86"))) { // "购买了"
                color = "#27ae60";
            } else if (entryText.contains(QString::fromUtf8("\xe6\x94\xbb\xe5\x87\xbb"))) { // "攻击"
                color = "#e74c3c";
            } else if (entryText.contains(QString::fromUtf8("\xe7\xa7\xbb\xe5\x8a\xa8"))) { // "移动"
                color = "#3498db";
            } else if (entryText.contains(QString::fromUtf8("\xe5\x8d\x87\xe7\xba\xa7"))) { // "升级"
                color = "#9b59b6";
            }
            html += QString("<div class='log-ai' style='color: %1;'>▸ %2</div>").arg(color, entryText.toHtmlEscaped());
        }
    }

    if (log.empty() && !showAiInfo) {
        html += "<div style='color: rgba(255,255,255,0.4); text-align: center; padding: 30px; font-size: 13px;'>"
                "暂无战斗记录<br><span style='font-size: 11px;'>开始战斗后将在此显示详细日志</span></div>";
    } else if (log.empty() && showAiInfo && aiLog.empty()) {
        html += "<div style='color: rgba(255,255,255,0.4); text-align: center; padding: 30px; font-size: 13px;'>"
                "暂无战斗记录<br><span style='font-size: 11px;'>等待敌方 AI 行动...</span></div>";
    }

    logEdit_->setHtml(html);

    // 自动滚动到底部
    QScrollBar* scrollBar = logEdit_->verticalScrollBar();
    if (scrollBar) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void BattleLogWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging_ = true;
        dragStartPos_ = event->globalPosition().toPoint() - parentWidget()->mapToGlobal(pos());
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void BattleLogWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging_ && (event->buttons() & Qt::LeftButton)) {
        QPoint newPos = event->globalPosition().toPoint() - dragStartPos_;
        // 限制在父窗口范围内
        if (parentWidget()) {
            QRect parentRect = parentWidget()->rect();
            newPos.setX(qBound(0, newPos.x(), parentRect.width() - width()));
            newPos.setY(qBound(0, newPos.y(), parentRect.height() - height()));
        }
        move(newPos);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void BattleLogWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging_ = false;
        event->accept();
    }
    QWidget::mouseReleaseEvent(event);
}
