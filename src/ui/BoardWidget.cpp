/**
 * @file    BoardWidget.cpp
 * @brief   棋盘渲染组件实现
 * @author  
 * @date    2026-06-24
 */

#include "BoardWidget.h"
#include "FontConfig.h"
#include "../core/Game.h"
#include "../core/Board.h"
#include "../core/Unit.h"
#include "../core/Bench.h"
#include "../core/Player.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QSvgRenderer>
#include <QDir>
#include <QStringList>
#include <QUrl>
#include <QRegularExpression>
#include <cmath>
#include <set>
#include <map>

BoardWidget::BoardWidget(std::shared_ptr<Game> game, QWidget *parent)
    : QWidget(parent), game_(game) {
    setMinimumSize(BOARD_MIN_WIDTH, BOARD_MIN_HEIGHT);  // 动态尺寸，由MainWindow的resizeEvent调整
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAcceptDrops(true);
    setStyleSheet("BoardWidget { background-color: #1a252f; border-radius: 8px; border: 2px solid #34495e; }");
    loadAssets();

    // 创建合成按钮（位于备战区右侧）
    combineBtn_ = new QPushButton("⚡ 合成", this);
    combineBtn_->setFixedSize(COMBINE_BTN_W, COMBINE_BTN_H);
    combineBtn_->setFocusPolicy(Qt::TabFocus);
    combineBtn_->setToolTip("合成三星单位（三合一）");
    combineBtn_->setStyleSheet(
        "QPushButton { "
        "background-color: #d35400; color: white; border: 2px solid #a84300; "
        "border-radius: 4px; font-weight: bold; font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #a84300; } "
        "QPushButton:pressed { background-color: #7a3600; }"
    );
    connect(combineBtn_, &QPushButton::clicked, this, &BoardWidget::combineRequested);

    // 倍速按钮
    speedBtn_ = new QPushButton("▶ 1x", this);
    speedBtn_->setFixedSize(COMBINE_BTN_W, COMBINE_BTN_H);
    speedBtn_->setFocusPolicy(Qt::TabFocus);
    speedBtn_->setToolTip("切换战斗倍速 (1x → 2x → 4x → 1x)");
    speedBtn_->setStyleSheet(
        "QPushButton { "
        "background-color: #2980b9; color: white; border: 2px solid #2471a3; "
        "border-radius: 4px; font-weight: bold; font-size: 10px; "
        "} "
        "QPushButton:hover { background-color: #2471a3; } "
        "QPushButton:pressed { background-color: #1a5276; }"
    );
    connect(speedBtn_, &QPushButton::clicked, this, [this]() {
        if (game_) {
            int spd = game_->cycleCombatSpeed();
            QString label = QString("▶ %1x").arg(spd);
            speedBtn_->setText(label);
        }
    });

    // 羁绊面板已移动到 MainWindow 右侧信息区
}

BoardWidget::~BoardWidget() {
}

void BoardWidget::updateDisplay() {
    // 每次刷新前，重新计算并展示羁绊（对玩家的单位）
    if (game_) {
        std::vector<Unit*> units;
        const Board &board = game_->getBoard();
        auto boardUnits = board.getUnitsForOwner(Owner::PlayerCtrl);
        for (auto &su : boardUnits) if (su) units.push_back(su.get());
        const Bench &bench = game_->getPlayer().getBench();
        for (int i = 0; i < BENCH_SIZE; ++i) {
            auto su = bench.getUnit(i);
            if (su) units.push_back(su.get());
        }
        auto activated = Unit::CheckTraits(units);
        activeTraits_.clear();
        for (auto &s : activated) activeTraits_.push_back(QString::fromStdString(s));
    }
    update(); // 触发重绘
}

void BoardWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景图或默认背景
    auto boardBgIt = pixmapCache_.find("ui/background");
    if (boardBgIt == pixmapCache_.end() || boardBgIt->second.isNull()) {
        painter.fillRect(rect(), QColor(44, 62, 80));
    } else {
        painter.drawPixmap(rect(), boardBgIt->second.scaled(rect().size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }

    drawBoard(&painter);
    drawBench(&painter);
    drawUnits(&painter);
    
    // P3-4: 装备掉落闪烁覆盖层
    if (equipDropAlpha_ > 0.01f) {
        painter.fillRect(rect(), QColor(241, 196, 15, static_cast<int>(equipDropAlpha_ * 60)));
    }
    
    // 拖拽时的视觉反馈（在单位之上绘制）
    if (isDragging_) {
        drawDropZoneBoundaries(&painter);
        drawPlacementPreview(&painter);
        drawDragGhost(&painter);
    }

    // 点击选中单位时显示详情面板
    if (hasSelection_ && !isDragging_) {
        drawInfoPanel(&painter);
    }

    // 大招全屏文字覆盖层
    if (ultimateOverlayAlpha_ > 0.005f) {
        // 背景半透明遮罩（最底层）
        painter.fillRect(rect(), QColor(0, 0, 0, static_cast<int>(ultimateOverlayAlpha_ * 80)));

        // 大招特效SVG
        if (!ultiSvgPath_.isEmpty() && ultiSvgDelay_ <= 0.0f) {
            QString full = QDir(assetRootPath_).filePath(ultiSvgPath_);
            QFile f(full);
            if (f.open(QIODevice::ReadOnly)) {
                QByteArray data = f.readAll();
                f.close();
                QSvgRenderer sr(data);
                if (sr.isValid()) {
                    QSize base = sr.defaultSize();
                    if (base.isEmpty()) base = QSize(128, 128);
                    double s = static_cast<double>(getCellSize() / 3) / std::max(base.width(), base.height());
                    QSize sz(static_cast<int>(base.width() * s), static_cast<int>(base.height() * s));
                    QPixmap pm(sz);
                    pm.fill(Qt::transparent);
                    QPainter pp(&pm);
                    sr.render(&pp);
                    pp.end();
                    painter.setOpacity(0.5 + ultimateOverlayAlpha_ * 0.5);
                    painter.drawPixmap(static_cast<int>(ultiSvgPos_.x() - sz.width() / 2),
                                       static_cast<int>(ultiSvgPos_.y() - sz.height() / 2), pm);
                    painter.setOpacity(1.0);
                }
            }
        }

        // 大招文字（最顶层）
        float scale = 0.5f + ultimateOverlayAlpha_ * 0.5f;  // 0.5→1.0 缩放
        painter.setPen(QColor(255, 215, 0, static_cast<int>(ultimateOverlayAlpha_ * 255)));
        QFont ultFont = appFont(static_cast<int>(42 * scale), true);
        painter.setFont(ultFont);
        painter.drawText(rect(), Qt::AlignCenter, ultimateOverlayText_);
        // 副标题
        painter.setPen(QColor(255, 255, 255, static_cast<int>(ultimateOverlayAlpha_ * 180)));
        painter.setFont(appFont(static_cast<int>(16 * scale), false));
        painter.drawText(rect().adjusted(0, 50, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                         QString::fromUtf8("★ 大 招 ★"));
    }
}

void BoardWidget::drawBoard(QPainter *painter) {
    // 设置高质量渲染
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    // 绘制行列标识
        painter->setPen(Qt::white);
        painter->setFont(appFont(10, true));
    
        // 列标识 (A-H) - 与棋盘格子对齐
        for (int y = 0; y < BOARD_N; ++y) {
            int x_pos = getBoardMargin() + y * getCellSize() + getCellSize() / 2 - 5;
            int y_pos = getBoardMargin() - 12;
            char col_label = 'A' + y;
            painter->drawText(x_pos - 5, y_pos, 10, 14, Qt::AlignCenter, QString(col_label));
        }
    
        // 行标识 (1-8)
        for (int x = 0; x < BOARD_M; ++x) {
            int x_pos = getBoardMargin() - 12;
            int y_pos = getBoardMargin() + x * getCellSize() + getCellSize() / 2 - 5;
            painter->drawText(x_pos, y_pos, 10, 14, Qt::AlignCenter, QString::number(x + 1));
        }
    
        const auto boardCellIt = pixmapCache_.find("ui/board_cell");
        const bool hasBoardCellBg = (boardCellIt != pixmapCache_.end() && !boardCellIt->second.isNull());

        // 绘制棋盘网格
        for (int x = 0; x < BOARD_M; ++x) {
            for (int y = 0; y < BOARD_N; ++y) {
                QRect cellRect = getCellRect(x, y);
            
                if (hasBoardCellBg) {
                    painter->drawPixmap(cellRect, boardCellIt->second.scaled(cellRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                } else {
                    // 交替填充背景颜色 - 增加对比度
                    if ((x + y) % 2 == 0) {
                        painter->fillRect(cellRect, QColor(65, 90, 110));  // 较亮的颜色
                    } else {
                        painter->fillRect(cellRect, QColor(45, 65, 85));   // 较暗的颜色
                    }
                }
            
                // 绘制边框 - 使用更明显的颜色和粗度
                painter->setPen(QPen(QColor(80, 100, 120), 1));
                painter->drawRect(cellRect);
            
                // 键盘选中状态 - 金色边框（优先级高于高亮）
                if (hasSelection_ && !selInBench_ && selX_ == x && selY_ == y) {
                    painter->fillRect(cellRect, QColor(241, 196, 15, 80));
                    painter->setPen(QPen(QColor(241, 196, 15), 3));
                    painter->drawRect(cellRect);
                }
                // 鼠标高亮选中格子
                else if (!highlightInBench_ && highlightX_ == x && highlightY_ == y) {
                    painter->fillRect(cellRect, QColor(52, 152, 219, 150));
                    painter->setPen(QPen(QColor(52, 152, 219), 2));
                    painter->drawRect(cellRect);
                }
                // 拖动时：攻击范围高亮（橙红色，区别于当前格子的蓝色）
                else if (isDragging_ && draggedUnit_ && !dropPreviewInBench_ && dropPreviewValid_) {
                    bool inRange = false;
                    const auto& rangeMap = draggedUnit_->getNormalAttackRangeMap();
                    for (const auto& offset : rangeMap) {
                        int rx = dropPreviewX_ + offset.first;
                        int ry = dropPreviewY_ + offset.second;
                        if (rx == x && ry == y && (rx != dropPreviewX_ || ry != dropPreviewY_)) {
                            inRange = true;
                            break;
                        }
                    }
                    if (inRange) {
                        painter->fillRect(cellRect, QColor(230, 126, 34, 100));
                        painter->setPen(QPen(QColor(230, 126, 34, 180), 2));
                        painter->drawRect(cellRect);
                    }
                }
            }
        }
    
        // 绘制玩家半场/敌方半场分割线 - 与棋盘格子完全对齐
        int midY = BOARD_M / 2 * getCellSize() + getBoardMargin();
    
        // 分隔线 - 从棋盘左边缘到右边缘
        painter->setPen(QPen(QColor(230, 126, 34), 3));
        painter->drawLine(getBoardMargin(), midY, getBoardMargin() + BOARD_N * getCellSize(), midY);
    
        // 添加半透明背景区分 - 与棋盘格子完全对齐
        painter->setPen(Qt::NoPen);
        painter->fillRect(QRect(getBoardMargin(), getBoardMargin(), BOARD_N * getCellSize(), BOARD_M / 2 * getCellSize()), 
                          QColor(231, 76, 60, 10));   // 轻红色，表示敌方区域
        painter->fillRect(QRect(getBoardMargin(), midY, BOARD_N * getCellSize(), BOARD_M / 2 * getCellSize()), 
                          QColor(46, 204, 113, 10));  // 轻绿色，表示玩家区域
    
        // 标注 - 与棋盘格子对齐
        painter->setPen(Qt::white);
        painter->setFont(appFont(10, true));
        painter->drawText(getBoardMargin() + 8, getBoardMargin() + 18, "敌方");
        painter->drawText(getBoardMargin() + 8, midY + 18, "玩家");

                // 羁绊面板已移动到 MainWindow 右侧信息区显示
}

void BoardWidget::drawBench(QPainter *painter) {
    // 绘制备战区背景
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    QRect benchAreaRect(getBoardMargin(), benchY, BOARD_N * getCellSize(), getBenchHeight());
    painter->fillRect(benchAreaRect, QColor(39, 49, 69));
    painter->setPen(QPen(QColor(100, 100, 100), 1));
    painter->drawRect(benchAreaRect);
    
    const auto benchCellIt = pixmapCache_.find("ui/bench_cell");
    const bool hasBenchCellBg = (benchCellIt != pixmapCache_.end() && !benchCellIt->second.isNull());
    
    // 绘制备战区格子
    painter->setPen(QPen(QColor(80, 80, 80), 1));
    const int BENCH_SLOT_WIDTH = BOARD_N * getCellSize() / BENCH_SIZE;
    
    for (int i = 0; i < BENCH_SIZE; ++i) {
        QRect slotRect(
            getBoardMargin() + i * BENCH_SLOT_WIDTH,
            benchY,
            BENCH_SLOT_WIDTH,
            getBenchHeight()
        );
        
        if (hasBenchCellBg) {
            painter->drawPixmap(slotRect, benchCellIt->second.scaled(slotRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            // 填充格子
            if (i % 2 == 0) {
                painter->fillRect(slotRect, QColor(52, 73, 94));
            }
        }
        
        painter->drawRect(slotRect);
        
        // 键盘选中状态 - 金色边框
        if (hasSelection_ && selInBench_ && selX_ == i) {
            painter->fillRect(slotRect, QColor(241, 196, 15, 60));
            painter->setPen(QPen(QColor(241, 196, 15), 3));
            painter->drawRect(slotRect);
            painter->setPen(QPen(QColor(80, 80, 80), 1));
        }
        // 鼠标高亮选中槽位
        else if (highlightInBench_ && highlightX_ == i) {
            painter->fillRect(slotRect, QColor(41, 128, 185, 100));
            painter->setPen(QPen(QColor(41, 128, 185), 2));
            painter->drawRect(slotRect);
            painter->setPen(QPen(QColor(80, 80, 80), 1));
        }
    }
    
    // 标注 - 添加人口占用提示
    painter->setPen(Qt::white);
    painter->setFont(appFont(10, true));
    painter->drawText(getBoardMargin() + 5, benchY + 18, "备战区");
}

void BoardWidget::drawUnits(QPainter *painter) {
    if (!game_) return;

    const Board &board = game_->getBoard();
    
    // 收集正在动画中的单位，避免重复绘制
    std::set<std::shared_ptr<Unit>> animatingUnits;
    for (const auto& anim : moveAnims_) {
        if (anim.active && anim.unit) {
            animatingUnits.insert(anim.unit);
        }
    }
    
    for (int x = 0; x < BOARD_M; ++x) {
        for (int y = 0; y < BOARD_N; ++y) {
            auto unit = board.getUnit(x, y);
            if (unit && animatingUnits.find(unit) == animatingUnits.end()) {
                drawUnit(painter, x, y, unit);
            }
        }
    }

    const Bench &bench = game_->getPlayer().getBench();
    for (int i = 0; i < BENCH_SIZE; ++i) {
        auto unit = bench.getUnit(i);
        if (unit) {
            drawBenchUnit(painter, i, unit);
        }
    }
    
    // 在静态单位之上绘制动画
    drawMoveAnimations(painter);
    drawDamageAnimations(painter);
    drawDeathAnimations(painter);
}

void BoardWidget::drawUnit(QPainter *painter, int x, int y, std::shared_ptr<Unit> unit) {
    if (!unit) return;

    QRect cellRect = getCellRect(x, y);
    QPixmap pixmap = getUnitPixmap(unit);

    // 使用更大的sprite区域（利用增大的格子）
    if (!pixmap.isNull()) {
        QRect spriteRect = cellRect.adjusted(8, 22, -8, -22);
        QPixmap scaledPixmap = pixmap.scaled(spriteRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPoint spriteTopLeft(
            spriteRect.left() + (spriteRect.width() - scaledPixmap.width()) / 2,
            spriteRect.top() + (spriteRect.height() - scaledPixmap.height()) / 2
        );
        painter->drawPixmap(spriteTopLeft, scaledPixmap);
        
        // 为敌方单位添加红色边框标记
        if (unit->getOwner() == Owner::EnemyCtrl) {
            painter->setPen(QPen(QColor(231, 76, 60), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(spriteRect);
        }
    } else {
        QColor unitColor = unit->getOwner() == Owner::PlayerCtrl ? QColor(46, 204, 113) : QColor(231, 76, 60);
        painter->setBrush(unitColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(cellRect.center(), getCellSize() / 2 - 8, getCellSize() / 2 - 8);
        
        // 为敌方单位添加边框
        if (unit->getOwner() == Owner::EnemyCtrl) {
            painter->setPen(QPen(QColor(255, 100, 100), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(cellRect.center(), getCellSize() / 2 - 8, getCellSize() / 2 - 8);
        }
    }

    // 绘制生命条
    drawHealthBar(painter, cellRect, unit);

    // 绘制法力条
    drawManaBar(painter, cellRect, unit);

    // 绘制单位信息（简化版，详细属性在Tooltip中）
    drawUnitInfo(painter, cellRect, unit);
}

void BoardWidget::drawUnitAtPosition(QPainter *painter, QPointF pos, std::shared_ptr<Unit> unit) {
    if (!unit) return;
    
    // 基于位置计算格子矩形
    QRect cellRect(
        static_cast<int>(pos.x() - getCellSize() / 2),
        static_cast<int>(pos.y() - getCellSize() / 2),
        getCellSize(),
        getCellSize()
    );
    
    QPixmap pixmap = getUnitPixmap(unit);
    
    if (!pixmap.isNull()) {
        QRect spriteRect = cellRect.adjusted(8, 22, -8, -22);
        QPixmap scaledPixmap = pixmap.scaled(spriteRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPoint spriteTopLeft(
            spriteRect.left() + (spriteRect.width() - scaledPixmap.width()) / 2,
            spriteRect.top() + (spriteRect.height() - scaledPixmap.height()) / 2
        );
        painter->drawPixmap(spriteTopLeft, scaledPixmap);
        
        if (unit->getOwner() == Owner::EnemyCtrl) {
            painter->setPen(QPen(QColor(231, 76, 60), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(spriteRect);
        }
    } else {
        QColor unitColor = unit->getOwner() == Owner::PlayerCtrl ? QColor(46, 204, 113) : QColor(231, 76, 60);
        painter->setBrush(unitColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(cellRect.center(), getCellSize() / 2 - 8, getCellSize() / 2 - 8);
        
        if (unit->getOwner() == Owner::EnemyCtrl) {
            painter->setPen(QPen(QColor(255, 100, 100), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(cellRect.center(), getCellSize() / 2 - 8, getCellSize() / 2 - 8);
        }
    }
    
    drawHealthBar(painter, cellRect, unit);
    drawManaBar(painter, cellRect, unit);
    drawUnitInfo(painter, cellRect, unit);
}

void BoardWidget::drawHealthBar(QPainter *painter, const QRect &cellRect, std::shared_ptr<Unit> unit) {
    int maxHP = unit->getMaxHP();
    int currentHP = unit->getHP();
    
    // 生命条位置 - 使用图标❤代替文字
    QRect healthBarRect(cellRect.left() + 2, cellRect.bottom() - 10, getCellSize() - 4, 6);
    painter->fillRect(healthBarRect, QColor(60, 60, 60, 200)); // 深色背景
    
    // 生命条 - 根据血量百分比变色
    float hpPercent = maxHP > 0 ? static_cast<float>(currentHP) / maxHP : 0.0f;
    QColor hpColor;
    if (hpPercent > 0.6f) hpColor = QColor(46, 204, 113);      // 绿色
    else if (hpPercent > 0.3f) hpColor = QColor(241, 196, 15);  // 黄色
    else hpColor = QColor(231, 76, 60);                          // 红色
    
    QRect healthRect(healthBarRect.left() + 1, healthBarRect.top() + 1, 
                     static_cast<int>((healthBarRect.width() - 2) * hpPercent), healthBarRect.height() - 2);
    painter->fillRect(healthRect, hpColor);
}

void BoardWidget::drawManaBar(QPainter *painter, const QRect &cellRect, std::shared_ptr<Unit> unit) {
    int maxMana = unit->getMaxMana();
    int currentMana = unit->getMana();
    
    // 法力条位置 - 在生命条下方
    QRect manaBarRect(cellRect.left() + 2, cellRect.bottom() - 4, getCellSize() - 4, 3);
    painter->fillRect(manaBarRect, QColor(40, 40, 60, 200)); // 深色背景
    
    // 法力条 - 蓝色渐变效果
    float manaPercent = maxMana > 0 ? static_cast<float>(currentMana) / maxMana : 0;
    QRect manaRect(manaBarRect.left() + 1, manaBarRect.top() + 1, 
                   static_cast<int>((manaBarRect.width() - 2) * manaPercent), manaBarRect.height() - 2);
    painter->fillRect(manaRect, QColor(52, 152, 219)); // 蓝色
}

void BoardWidget::drawUnitInfo(QPainter *painter, const QRect &cellRect, std::shared_ptr<Unit> unit) {
    // 简化显示：只显示星级和简短图标信息
    // 详细属性通过 Tooltip 展示
    
    // 左上角：星级（★）使用图标
    QRect starRect(cellRect.left() + 2, cellRect.top() + 2, 28, 18);
    painter->setPen(Qt::NoPen);
    painter->fillRect(starRect, QColor(0, 0, 0, 150));
    painter->setPen(QColor(255, 215, 0)); // 金色
    painter->setFont(appFont(10, true));
    QString starText = QString("%1").arg(QString(unit->getStarLevel(), QChar(0x2605)));
    painter->drawText(starRect, Qt::AlignCenter, starText);
    
    // 右上角：阵营标记（用圆点颜色表示）
    QColor ownerColor = unit->getOwner() == Owner::PlayerCtrl ? QColor(46, 204, 113) : QColor(231, 76, 60);
    painter->setBrush(ownerColor);
    painter->setPen(QPen(QColor(255, 255, 255, 100), 1));
    painter->drawEllipse(cellRect.right() - 16, cellRect.top() + 4, 12, 12);
    
    // 特殊标志（背刺等）
    if (unit->HasBackstab()) {
        painter->setPen(QColor(255, 215, 0));
        painter->setFont(appFont(9, true));
        painter->drawText(cellRect.left() + 2, cellRect.top() + 20, QString::fromUtf8("\xe8\x83\x8c\xe5\x88\xba"));
    }

    // === AI 意图图标显示（仅敌方单位） ===
    if (game_ && game_->getShowAiInfo() && unit->getOwner() == Owner::EnemyCtrl) {
        const auto& intentMap = game_->getAiIntentMap();
        auto it = intentMap.find(unit);
        if (it != intentMap.end()) {
            const auto& intent = it->second;
            QString iconText;
            QString bgColor;
            
            if (intent.action == "移动") {
                iconText = "🚶";
                bgColor = "rgba(52, 152, 219, 0.8)";  // 蓝色
            } else if (intent.action == "攻击") {
                iconText = "🎯";
                bgColor = "rgba(231, 76, 60, 0.8)";   // 红色
            } else {
                iconText = "⏸";
                bgColor = "rgba(149, 165, 166, 0.8)"; // 灰色
            }
            
            // 在单位头顶绘制意图图标
            QRect intentRect(cellRect.left() + 2, cellRect.top() - 8, 24, 20);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0, 0, 0, 180));
            painter->drawRoundedRect(intentRect, 4, 4);
            painter->setPen(Qt::white);
            painter->setFont(QFont("Segoe UI Emoji", 12, QFont::Bold));
            painter->drawText(intentRect, Qt::AlignCenter, iconText);
            
            // 如果有目标位置，画出指示线
            if (intent.targetX >= 0 && intent.targetY >= 0) {
                QPointF targetCenter = getCellCenter(intent.targetX, intent.targetY);
                QPointF unitCenter = cellRect.center();
                
                painter->setPen(QPen(QColor(255, 255, 100, 120), 2, Qt::DashLine));
                painter->drawLine(unitCenter, targetCenter);
                
                // 在目标位置画一个小标记
                painter->setBrush(QColor(255, 255, 100, 60));
                painter->setPen(QPen(QColor(255, 255, 100, 150), 2));
                QRectF targetMark(targetCenter.x() - 6, targetCenter.y() - 6, 12, 12);
                painter->drawEllipse(targetMark);
            }
        }
    }
}

QRect BoardWidget::getCellRect(int x, int y) const {
    // x = 行(row), y = 列(column)
    return QRect(
        getBoardMargin() + y * getCellSize(),
        getBoardMargin() + x * getCellSize(),
        getCellSize(),
        getCellSize()
    );
}

QPointF BoardWidget::getCellCenter(int x, int y) const {
    QRect r = getCellRect(x, y);
    return QPointF(r.center().x(), r.center().y());
}

QRect BoardWidget::getBenchSlotRect(int index) const {
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    const int BENCH_SLOT_WIDTH = BOARD_N * getCellSize() / BENCH_SIZE;
    return QRect(
        getBoardMargin() + index * BENCH_SLOT_WIDTH,
        benchY,
        BENCH_SLOT_WIDTH,
        getBenchHeight()
    );
}

std::pair<int, int> BoardWidget::screenToBoard(int sx, int sy) const {
    int x = (sy - getBoardMargin()) / getCellSize();
    int y = (sx - getBoardMargin()) / getCellSize();
    return {x, y};
}

std::pair<int, int> BoardWidget::screenToBench(int sx, int sy) const {
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    int slotWidth = BOARD_N * getCellSize() / BENCH_SIZE;
    
    if (sy < benchY || sy > benchY + getBenchHeight()) {
        return {-1, -1};
    }
    
    int index = (sx - getBoardMargin()) / slotWidth;
    return {index, 0};
}
void BoardWidget::drawBenchUnit(QPainter *painter, int index, std::shared_ptr<Unit> unit) {
    if (!unit) return;

    QRect slotRect = getBenchSlotRect(index);
    QRect infoRect(slotRect.left() + 10, slotRect.top() + 6, slotRect.width() - 20, 16);
    QRect contentRect = slotRect.adjusted(10, 24, -10, -20);
    QPixmap pixmap = getUnitPixmap(unit);

    painter->setPen(Qt::NoPen);
    painter->fillRect(infoRect, QColor(0, 0, 0, 150));
    painter->setPen(Qt::white);
    painter->setFont(appFont(9, true));
    // 使用图标替代文字，与棋盘格子风格一致
        painter->drawText(infoRect, Qt::AlignCenter,
                          QString("%1★ %2%3 %4%5")
                              .arg(unit->getStarLevel())
                              .arg(QString::fromUtf8("\xe2\x9d\xa4"))  // ❤
                              .arg(unit->getHP())
                              .arg(QString::fromUtf8("\xe2\x9a\x94"))  // ⚔
                              .arg(unit->getATK()));

    if (!pixmap.isNull()) {
        QPixmap scaledPixmap = pixmap.scaled(contentRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPoint spriteTopLeft(
            contentRect.left() + (contentRect.width() - scaledPixmap.width()) / 2,
            contentRect.top() + (contentRect.height() - scaledPixmap.height()) / 2
        );
        painter->drawPixmap(spriteTopLeft, scaledPixmap);
    } else {
        QColor unitColor = unit->getOwner() == Owner::PlayerCtrl ? QColor(46, 204, 113) : QColor(231, 76, 60);
        painter->setBrush(unitColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(contentRect.center(), contentRect.width() / 2, contentRect.height() / 2);
    }

    painter->setPen(Qt::white);
    painter->setFont(appFont(9, true));
    painter->drawText(slotRect.adjusted(4, slotRect.height() - 18, -4, -4), Qt::AlignLeft | Qt::AlignBottom,
                      QString("%1").arg(QString::fromStdString(unit->displayName())));
}
void BoardWidget::mousePressEvent(QMouseEvent *event) {
    int sx = event->pos().x();
    int sy = event->pos().y();
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    
    // 信息面板装备卸载按钮
    if (hasSelection_ && !infoEquipRects_.empty()) {
        for (auto& [rect, slot] : infoEquipRects_) {
            if (rect.contains(sx, sy)) {
                std::shared_ptr<Unit> unit;
                if (selInBench_) {
                    unit = game_->getPlayer().getBench().getUnit(selX_);
                } else {
                    unit = game_->getBoard().getUnit(selX_, selY_);
                }
                if (unit) {
                    Equipment* eq = unit->GetEquipmentSlot(slot);
                    if (eq) {
                        game_->getPlayer().unequipFromUnit(eq->id);
                        unit->Unequip(slot);
                        soundEffect_[0]->play();
                        update();
                    }
                }
                return;
            }
        }
    }
    
    // 处理拖拽中的放下操作
    if (isDragging_) {
        if (sy >= benchY && sy <= benchY + getBenchHeight()) {
            auto [index, _] = screenToBench(sx, sy);
            if (index >= 0 && index < BENCH_SIZE) {
                handleDrop(index, 0, true);
                isDragging_ = false;
                draggedUnit_.reset();
                hasSelection_ = false;
                soundEffect_[0]->play();
                update();
                return;
            }
        } else {
            auto [x, y] = screenToBoard(sx, sy);
            if (x >= 0 && x < BOARD_M && y >= 0 && y < BOARD_N) {
                handleDrop(x, y, false);
                isDragging_ = false;
                draggedUnit_.reset();
                hasSelection_ = false;
                soundEffect_[0]->play();
                update();
                return;
            }
        }
        // 拖拽中点击无效区域：取消拖拽
        isDragging_ = false;
        draggedUnit_.reset();
        hasSelection_ = false;
        update();
        return;
    }
    
    if (sy >= benchY && sy <= benchY + getBenchHeight()) {
        // 点击备战区
        auto [index, _] = screenToBench(sx, sy);
        if (index >= 0 && index < BENCH_SIZE) {
            const Bench &bench = game_->getPlayer().getBench();
            auto unit = bench.getUnit(index);
            if (unit) {
                // 如果已选中该单位且不是备战区，或已选中不同单位：切换选中
                if (hasSelection_ && selInBench_ && selX_ == index) {
                    // 点击同一个已选中单位：取消选中
                    hasSelection_ = false;
                    update();
                    return;
                }
                if (hasSelection_ && !selInBench_) {
                    // 有棋盘选中单位，点击备战区单位：切换到选中备战区单位
                    hasSelection_ = true;
                    selInBench_ = true;
                    selX_ = index;
                    selY_ = 0;
                    highlightX_ = index;
                    highlightInBench_ = true;
                    soundEffect_[0]->play();
                    draggedUnit_ = unit;
                    startDrag(index, 0, true);
                    isDragging_ = true;
                    dragCurrentPos_ = event->pos();
                    update();
                    return;
                }
                // 新选中备战区单位
                hasSelection_ = true;
                selInBench_ = true;
                selX_ = index;
                selY_ = 0;
                highlightX_ = index;
                highlightInBench_ = true;
                soundEffect_[0]->play();
                // 按住可拖拽
                draggedUnit_ = unit;
                startDrag(index, 0, true);
                isDragging_ = true;
                dragCurrentPos_ = event->pos();
                update();
                return;
            } else {
                // 点击空备战区槽位：如果有选中单位则移过来，否则取消选中
                if (hasSelection_ && selInBench_) {
                    game_->moveBenchUnit(selX_, index);
                    hasSelection_ = false;
                    soundEffect_[0]->play();
                    update();
                    return;
                }
                if (hasSelection_ && !selInBench_) {
                    game_->moveBoardToBench(selX_, selY_, index);
                    hasSelection_ = false;
                    soundEffect_[0]->play();
                    update();
                    return;
                }
                hasSelection_ = false;
                update();
                return;
            }
        }
    } else {
        // 点击棋盘
        auto [x, y] = screenToBoard(sx, sy);
        if (x >= 0 && x < BOARD_M && y >= 0 && y < BOARD_N) {
            const Board &board = game_->getBoard();
            auto unit = board.getUnit(x, y);
            if (unit) {
                // 点击棋盘上的单位
                if (hasSelection_ && !selInBench_ && selX_ == x && selY_ == y) {
                    // 点击同一个已选中单位：取消选中
                    hasSelection_ = false;
                    update();
                    return;
                }
                if (hasSelection_ && selInBench_) {
                    // 备战区有选中单位，点击棋盘：放到棋盘
                    game_->dragUnitFromBenchToBoard(selX_, x, y);
                    hasSelection_ = false;
                    soundEffect_[0]->play();
                    update();
                    return;
                }
                if (hasSelection_ && !selInBench_ && (selX_ != x || selY_ != y)) {
                    // 点击不同的棋盘单位：切换选中为该单位
                    hasSelection_ = true;
                    selInBench_ = false;
                    selX_ = x;
                    selY_ = y;
                    highlightX_ = x;
                    highlightY_ = y;
                    highlightInBench_ = false;
                    soundEffect_[0]->play();
                    // 按住可拖拽
                    draggedUnit_ = unit;
                    startDrag(x, y, false);
                    isDragging_ = true;
                    dragCurrentPos_ = event->pos();
                    update();
                    return;
                }
                // 新选中棋盘单位
                hasSelection_ = true;
                selInBench_ = false;
                selX_ = x;
                selY_ = y;
                highlightX_ = x;
                highlightY_ = y;
                highlightInBench_ = false;
                soundEffect_[0]->play();
                // 按住可拖拽
                draggedUnit_ = unit;
                startDrag(x, y, false);
                isDragging_ = true;
                dragCurrentPos_ = event->pos();
                update();
                return;
            } else {
                // 点击空格子
                if (hasSelection_ && selInBench_) {
                    // 备战区有选中单位：放到棋盘空格
                    game_->dragUnitFromBenchToBoard(selX_, x, y);
                    hasSelection_ = false;
                    soundEffect_[0]->play();
                    update();
                    return;
                }
                if (hasSelection_ && !selInBench_) {
                    // 棋盘有选中单位：移动到空格
                    game_->dragUnitOnBoard(selX_, selY_, x, y);
                    hasSelection_ = false;
                    soundEffect_[0]->play();
                    update();
                    return;
                }
                // 点击无单位空格：取消选中
                hasSelection_ = false;
                update();
                return;
            }
        }
    }
    // 点击棋盘外区域：取消选中
    hasSelection_ = false;
    update();
}

void BoardWidget::mouseMoveEvent(QMouseEvent *event) {
    int sx = event->pos().x();
    int sy = event->pos().y();
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    
    // 更新拖拽幽灵位置
    if (isDragging_) {
        dragCurrentPos_ = event->pos();
    }
    
    // 计算放置预览
    if (isDragging_ && draggedUnit_) {
        if (sy >= benchY && sy <= benchY + getBenchHeight()) {
            auto [index, _] = screenToBench(sx, sy);
            dropPreviewInBench_ = true;
            dropPreviewX_ = index;
            dropPreviewY_ = 0;
            dropPreviewValid_ = isValidDropTarget(index, 0, true);
            highlightInBench_ = true;
            highlightX_ = index;
        } else {
            auto [x, y] = screenToBoard(sx, sy);
            dropPreviewInBench_ = false;
            dropPreviewX_ = x;
            dropPreviewY_ = y;
            dropPreviewValid_ = isValidDropTarget(x, y, false);
            highlightInBench_ = false;
            highlightX_ = x;
            highlightY_ = y;
        }
    } else {
        // 非拖拽状态：普通高亮
        if (sy >= benchY && sy <= benchY + getBenchHeight()) {
            auto [index, _] = screenToBench(sx, sy);
            highlightInBench_ = true;
            highlightX_ = index;
            dropPreviewX_ = -1;
        } else {
            auto [x, y] = screenToBoard(sx, sy);
            highlightInBench_ = false;
            highlightX_ = x;
            highlightY_ = y;
            dropPreviewX_ = -1;
        }
    }
    
    update();
}

void BoardWidget::mouseReleaseEvent(QMouseEvent *event) {
    int sx = event->pos().x();
    int sy = event->pos().y();
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    
    // 仅当有实际拖拽移动（非纯点击）才执行放下操作
    // mousePressEvent 已处理纯点击的选中/移动逻辑
    if (isDragging_ && dragStartX_ >= 0 && dragStartY_ >= 0) {
        // 计算鼠标是否移动了足够距离（> 8px 才算拖拽）
        QPoint startPos;
        if (dragFromBench_) {
            startPos = getBenchSlotRect(dragStartX_).center();
        } else {
            startPos = getCellCenter(dragStartX_, dragStartY_).toPoint();
        }
        QPoint delta = event->pos() - startPos;
        
        if (delta.manhattanLength() > 8) {
            if (sy >= benchY && sy <= benchY + getBenchHeight()) {
                auto [index, _] = screenToBench(sx, sy);
                if (index >= 0 && index < BENCH_SIZE) {
                    if (dragFromBench_) {
                        if (!game_->moveBenchUnit(dragStartX_, index)) {
                            game_->swapBenchUnits(dragStartX_, index);
                        }
                    } else {
                        game_->moveBoardToBench(dragStartX_, dragStartY_, index);
                    }
                    soundEffect_[0]->play();
                }
            } else {
                auto [x, y] = screenToBoard(sx, sy);
                if (x >= 0 && x < BOARD_M && y >= 0 && y < BOARD_N) {
                    if (dragFromBench_) {
                        game_->dragUnitFromBenchToBoard(dragStartX_, x, y);
                    } else {
                        game_->dragUnitOnBoard(dragStartX_, dragStartY_, x, y);
                    }
                    soundEffect_[0]->play();
                }
            }
        }
    }
    
    // 清除拖拽状态（保留选中状态以便继续通过键盘操作）
    isDragging_ = false;
    dragStartX_ = -1;
    dragStartY_ = -1;
    dragFromBench_ = false;
    draggedUnit_.reset();
    dropPreviewX_ = -1;
    dropPreviewY_ = -1;
    dropPreviewValid_ = false;
    update();
}

void BoardWidget::keyPressEvent(QKeyEvent *event) {
    const int BOARD_COLS = BOARD_N;
    const int BOARD_ROWS = BOARD_M;

    // 方向键：移动选中焦点
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down ||
        event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {

        if (!hasSelection_) {
            // 首次按方向键：选中棋盘中央（玩家半场顶部）
            hasSelection_ = true;
            selInBench_ = false;
            selX_ = BOARD_ROWS / 2;
            selY_ = BOARD_COLS / 2;
        } else {
            // 移动选中焦点
            int dx = 0, dy = 0;
            if (event->key() == Qt::Key_Up) dx = -1;
            else if (event->key() == Qt::Key_Down) dx = 1;
            else if (event->key() == Qt::Key_Left) dy = -1;
            else if (event->key() == Qt::Key_Right) dy = 1;

            if (selInBench_) {
                // 在备战区：左右移动
                int newIdx = selX_ + dy;
                if (newIdx >= 0 && newIdx < BENCH_SIZE) {
                    selX_ = newIdx;
                }
                // 按上方向键从备战区切换到棋盘
                if (dx < 0) {
                    selInBench_ = false;
                    selX_ = BOARD_ROWS - 1;
                    selY_ = qBound(0, selX_, BOARD_COLS - 1);
                }
            } else {
                int newX = selX_ + dx;
                int newY = selY_ + dy;
                if (newX >= 0 && newX < BOARD_ROWS && newY >= 0 && newY < BOARD_COLS) {
                    selX_ = newX;
                    selY_ = newY;
                }
                // 按下方向键从棋盘切换到备战区
                if (dx > 0 && newX >= BOARD_ROWS) {
                    selInBench_ = true;
                    selX_ = 0; // 备战区第一个槽位
                }
            }
        }
        // 同步高亮到选中位置
        highlightX_ = selX_;
        highlightY_ = selY_;
        highlightInBench_ = selInBench_;
        update();
        event->accept();
        return;
    }

    // Enter: 在选中位置执行点击操作（选中/拖拽）
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!hasSelection_) {
            // 无选中时按Enter，将焦点给到结束回合按钮
            QWidget::keyPressEvent(event);
            return;
        }

        if (selInBench_) {
            // 在备战区：尝试选中/拿起备战区单位
            const Bench &bench = game_->getPlayer().getBench();
            auto unit = bench.getUnit(selX_);
            if (unit) {
                if (isDragging_) {
                    // 拖拽中：放下到目标备战区槽位（交换或移动）
                    if (dragFromBench_) {
                        game_->moveBenchUnit(dragStartX_, selX_);
                    } else {
                        game_->moveBoardToBench(dragStartX_, dragStartY_, selX_);
                    }
                    isDragging_ = false;
                    draggedUnit_.reset();
                    dragStartX_ = dragStartY_ = -1;
                } else {
                    // 拿起备战区单位
                    draggedUnit_ = unit;
                    startDrag(selX_, 0, true);
                    isDragging_ = true;
                    dragCurrentPos_ = QPoint(width() / 2, height() / 2);
                }
            } else if (isDragging_) {
                // 拖拽中按Enter：放下到空备战区槽位
                if (dragFromBench_) {
                    game_->moveBenchUnit(dragStartX_, selX_);
                } else {
                    game_->moveBoardToBench(dragStartX_, dragStartY_, selX_);
                }
                isDragging_ = false;
                draggedUnit_.reset();
                dragStartX_ = dragStartY_ = -1;
            }
        } else {
            // 在棋盘上：尝试选中/拿起棋盘单位
            const Board &board = game_->getBoard();
            auto unit = board.getUnit(selX_, selY_);
            if (unit) {
                if (isDragging_) {
                    // 拖拽中：放入目标棋盘格子（交换）
                    if (dragFromBench_) {
                        game_->dragUnitFromBenchToBoard(dragStartX_, selX_, selY_);
                    } else {
                        game_->dragUnitOnBoard(dragStartX_, dragStartY_, selX_, selY_);
                    }
                    isDragging_ = false;
                    draggedUnit_.reset();
                    dragStartX_ = dragStartY_ = -1;
                } else {
                    // 拿起棋盘单位
                    draggedUnit_ = unit;
                    startDrag(selX_, selY_, false);
                    isDragging_ = true;
                    dragCurrentPos_ = QPoint(width() / 2, height() / 2);
                }
            } else if (isDragging_) {
                // 拖拽中按Enter：放下到空棋盘格
                handleKeyboardDrop();
            }
        }
        update();
        event->accept();
        return;
    }

    // Esc: 取消选中和拖拽
    if (event->key() == Qt::Key_Escape) {
        hasSelection_ = false;
        selX_ = selY_ = -1;
        selInBench_ = false;
        isDragging_ = false;
        draggedUnit_.reset();
        dragStartX_ = dragStartY_ = -1;
        dropPreviewX_ = dropPreviewY_ = -1;
        update();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

// 键盘放下单位辅助方法
void BoardWidget::handleKeyboardDrop() {
    if (!isDragging_ || !draggedUnit_ || dragStartX_ < 0) return;

    if (dragFromBench_) {
        // 从备战区拖出：放到当前选中的棋盘格子
        int targetX = selX_;
        int targetY = selY_;
        if (targetX < 0 || targetY < 0 || selInBench_) {
            // 如果没有有效的棋盘选中位置，放到棋盘中央（玩家半场）
            targetX = BOARD_M - 1;
            targetY = BOARD_N / 2;
        }
        game_->dragUnitFromBenchToBoard(dragStartX_, targetX, targetY);
    } else {
        // 从棋盘拖出：放到当前选中的备战区槽位
        int targetSlot = (selInBench_ && selX_ >= 0) ? selX_ : 0;
        game_->moveBoardToBench(dragStartX_, dragStartY_, targetSlot);
    }
    isDragging_ = false;
    draggedUnit_.reset();
    dragStartX_ = dragStartY_ = -1;
    hasSelection_ = false;
}

void BoardWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-unit") ||
        event->mimeData()->hasFormat("application/x-synera-equip")) {
        event->acceptProposedAction();  // 先放行进入，具体校验在 dragMoveEvent
    }
}

void BoardWidget::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-synera-equip")) {
        if (canEquipAtCursor(event->position().toPoint(), event->mimeData())) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
    } else if (event->mimeData()->hasFormat("application/x-unit")) {
        event->acceptProposedAction();
    }
}

bool BoardWidget::canEquipAtCursor(QPoint pos, const QMimeData *mime) const {
    if (!game_ || !mime) return false;
    
    int equipId = mime->data("application/x-synera-equip").toInt();
    Equipment* eq = game_->getPlayer().getEquipmentById(equipId);
    if (!eq) return false;
    
    int sx = pos.x();
    int sy = pos.y();
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    
    std::shared_ptr<Unit> targetUnit;
    
    if (sy >= benchY && sy <= benchY + getBenchHeight()) {
        auto [idx, _] = const_cast<BoardWidget*>(this)->screenToBench(sx, sy);
        if (idx >= 0 && idx < BENCH_SIZE) {
            targetUnit = game_->getPlayer().getBench().getUnit(idx);
        }
    } else {
        auto [bx, by] = const_cast<BoardWidget*>(this)->screenToBoard(sx, sy);
        if (bx >= BOARD_M / 2 && bx < BOARD_M && by >= 0 && by < BOARD_N) {
            targetUnit = game_->getBoard().getUnit(bx, by);
        }
    }
    
    if (!targetUnit || targetUnit->getOwner() != Owner::PlayerCtrl) return false;
    
    // 职业限制检查
    if (!eq->restriction.empty() && eq->restriction != Unit::ClassToString(targetUnit->GetClassType()))
        return false;
    
    // 槽位检查：该类型槽位是否已被占用
    int slot = eq->type;
    if ((slot == 2 || slot == 3) && targetUnit->GetEquipmentSlot(2) && targetUnit->GetEquipmentSlot(3))
        return false;  // 两个饰品槽都满了
    if (slot < 2 && targetUnit->GetEquipmentSlot(slot))
        return false;  // 武器/防具槽已占用
    
    return true;
}

void BoardWidget::dropEvent(QDropEvent *event) {
    // ===== P0-4: 装备拖放处理 =====
    if (event->mimeData()->hasFormat("application/x-synera-equip") && game_) {
        int equipId = event->mimeData()->data("application/x-synera-equip").toInt();
        Equipment* eq = game_->getPlayer().getEquipmentById(equipId);
        if (!eq) return;
        
        int sx = event->position().toPoint().x();
        int sy = event->position().toPoint().y();
        int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
        
        std::shared_ptr<Unit> targetUnit;
        int targetBenchIdx = -1;
        int targetBoardX = -1, targetBoardY = -1;
        
        if (sy >= benchY && sy <= benchY + getBenchHeight()) {
            // 备战区
            auto [idx, _] = screenToBench(sx, sy);
            if (idx >= 0 && idx < BENCH_SIZE) {
                targetUnit = game_->getPlayer().getBench().getUnit(idx);
                targetBenchIdx = idx;
            }
        } else {
            // 棋盘（仅玩家半场）
            auto [bx, by] = screenToBoard(sx, sy);
            if (bx >= BOARD_M / 2 && bx < BOARD_M && by >= 0 && by < BOARD_N) {
                targetUnit = game_->getBoard().getUnit(bx, by);
                targetBoardX = bx;
                targetBoardY = by;
            }
        }
        
        if (targetUnit && targetUnit->getOwner() == Owner::PlayerCtrl) {
            if (!eq->restriction.empty() && eq->restriction != Unit::ClassToString(targetUnit->GetClassType())) {
                event->ignore();
                return;
            }
            int slot = eq->type;
            if ((slot == 2 || slot == 3) && targetUnit->GetEquipmentSlot(2)) slot = 3;
            if (slot >= 0 && slot < 4 && !targetUnit->GetEquipmentSlot(slot)) {
                if (targetUnit->Equip(eq, slot)) {
                    game_->getPlayer().equipToUnit(equipId, targetUnit->getUnitId());
                    event->acceptProposedAction();
                    update();
                    return;
                }
            }
        }
        event->ignore();
        return;
    }
    
    if (event->mimeData()->hasFormat("application/x-unit")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void BoardWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // 重新计算布局尺寸
    recalcLayout();
    
    // 更新合成按钮位置（位于备战区右侧边缘）
    if (combineBtn_) {
        int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
        int boardRight = getBoardMargin() + BOARD_N * getCellSize();
        int btnX = boardRight + 6;  // 棋盘右侧外
        int btnY = benchY + (getBenchHeight() - combineBtn_->height()) / 2;
        combineBtn_->move(btnX, btnY);
        combineBtn_->raise();
        speedBtn_->move(btnX, btnY - speedBtn_->height() - 4);  // 合成按钮上方
        speedBtn_->raise();
    }
    
    update();
}

void BoardWidget::startDrag(int fromX, int fromY, bool isBench) {
    dragStartX_ = fromX;
    dragStartY_ = fromY;
    dragFromBench_ = isBench;
}

void BoardWidget::handleDrop(int toX, int toY, bool toBench) {
    if (!game_) {
        return;
    }
    if (toBench) {
        if (dragFromBench_) {
            game_->moveBenchUnit(dragStartX_, toX);
        } else {
            game_->moveBoardToBench(dragStartX_, dragStartY_, toX);
        }
    } else {
        if (dragFromBench_) {
            game_->dragUnitFromBenchToBoard(dragStartX_, toX, toY);
        } else {
            game_->dragUnitOnBoard(dragStartX_, dragStartY_, toX, toY);
        }
    }
}

void BoardWidget::handleIllegalPlacement(int fromX, int fromY, int toX, int toY) {
    (void)fromX;
    (void)fromY;
    (void)toX;
    (void)toY;
}

// ============ 动画系统实现 ============

void BoardWidget::startUnitMoveAnim(int fromX, int fromY, int toX, int toY) {
    if (!game_) return;
    
    // 获取移动的单位
    auto unit = game_->getBoard().getUnit(toX, toY);
    if (!unit) {
        // 可能单位已经移动到目标位置，尝试从旧位置获取
        unit = game_->getBoard().getUnit(fromX, fromY);
    }
    if (!unit) return;
    
    // 创建新的移动动画
    UnitMoveAnim anim;
    anim.fromX = fromX;
    anim.fromY = fromY;
    anim.toX = toX;
    anim.toY = toY;
    anim.progress = 0.0f;
    anim.active = true;
    anim.unit = unit;
    moveAnims_.push_back(anim);
}

void BoardWidget::startDamageAnim(int x, int y, int damage, bool isCrit) {
    DamageAnim anim;
    anim.pixelPos = getCellCenter(x, y);
    anim.damage = damage;
    anim.progress = 0.0f;
    anim.active = true;
    anim.isCrit = isCrit;
    damageAnims_.push_back(anim);
}

void BoardWidget::startDeathAnim(int x, int y) {
    DeathAnim anim;
    anim.pixelPos = getCellCenter(x, y);
    anim.progress = 0.0f;
    anim.active = true;
    deathAnims_.push_back(anim);
}

// P3-4: 装备掉落闪烁动画
void BoardWidget::startEquipDropAnim() {
    equipDropAlpha_ = 1.0f;
}

void BoardWidget::startUltimateOverlay(const QString& text, const QString& charType, int unitX, int unitY) {
    ultimateOverlayText_ = text;
    ultimateOverlayAlpha_ = 1.0f;
    ultiSvgDelay_ = 0.0f;
    ultiSvgPos_ = getCellCenter(unitX, unitY);
    QMap<QString, QString> m;
    m["Reimu"] = "ulti/reimu/阴阳玉.svg";
    m["Marisa"] = "ulti/marisa/光柱.svg";
    m["Koishi"] = "ulti/koishi/恋之瞳.svg";
    m["Youmu"] = "ulti/youmu/楼观.svg";
    m["Alice"] = "ulti/alice/人形.svg";
    m["Aya"] = "ulti/aya/风符.svg";
    m["Hatate"] = "ulti/hatate/闪光.svg";
    m["Sakuya"] = "ulti/sakuya/飞刀.svg";
    m["Sanae"] = "ulti/sanae/五角星.svg";
    ultiSvgPath_ = m.value(charType, "");
}

void BoardWidget::clearAnimations() {
    moveAnims_.clear();
    damageAnims_.clear();
    deathAnims_.clear();
    equipDropAlpha_ = 0.0f;
    ultimateOverlayAlpha_ = 0.0f;
    ultiSvgDelay_ = 0.0f;
    ultimateOverlayText_.clear();
    ultiSvgPath_.clear();
}

void BoardWidget::updateAnimations(float deltaTime) {
    const float MOVE_SPEED = 4.0f;     // 移动动画速度
    const float DAMAGE_SPEED = 2.0f;   // 伤害数字动画速度
    const float DEATH_SPEED = 6.0f;    // 死亡动画速度（~166ms）
    
    // 更新移动动画
    for (auto& anim : moveAnims_) {
        if (!anim.active) continue;
        anim.progress += deltaTime * MOVE_SPEED;
        if (anim.progress >= 1.0f) {
            anim.progress = 1.0f;
            anim.active = false;
        }
    }
    
    // 清理完成的移动动画
    moveAnims_.erase(
        std::remove_if(moveAnims_.begin(), moveAnims_.end(),
            [](const UnitMoveAnim& a) { return !a.active && a.progress >= 1.0f; }),
        moveAnims_.end()
    );
    
    // 更新伤害数字动画
    for (auto& anim : damageAnims_) {
        if (!anim.active) continue;
        anim.progress += deltaTime * DAMAGE_SPEED;
        if (anim.progress >= 1.0f) {
            anim.active = false;
        }
    }
    
    // 清理完成的伤害动画
    damageAnims_.erase(
        std::remove_if(damageAnims_.begin(), damageAnims_.end(),
            [](const DamageAnim& a) { return !a.active; }),
        damageAnims_.end()
    );
    
    // 更新死亡动画
    for (auto& anim : deathAnims_) {
        if (!anim.active) continue;
        anim.progress += deltaTime * DEATH_SPEED;
        if (anim.progress >= 1.0f) {
            anim.active = false;
        }
    }
    
    // 清理完成的死亡动画
    deathAnims_.erase(
        std::remove_if(deathAnims_.begin(), deathAnims_.end(),
            [](const DeathAnim& a) { return !a.active; }),
        deathAnims_.end()
    );
    
    // P3-4: 装备掉落闪烁衰减
    if (equipDropAlpha_ > 0.01f) {
        equipDropAlpha_ *= 0.92f;  // 指数衰减
        if (equipDropAlpha_ < 0.01f) equipDropAlpha_ = 0.0f;
    }

    // 大招全屏文字衰减（快速淡出）
    if (ultimateOverlayAlpha_ > 0.005f) {
        ultimateOverlayAlpha_ *= 0.88f;
        if (ultiSvgDelay_ > 0.0f) ultiSvgDelay_ -= deltaTime;
        if (ultimateOverlayAlpha_ < 0.005f) {
            ultimateOverlayAlpha_ = 0.0f;
            ultimateOverlayText_.clear();
            ultiSvgPath_.clear();
        }
    }
}

bool BoardWidget::hasActiveAnimations() const {
    return !moveAnims_.empty() || !damageAnims_.empty() || !deathAnims_.empty()
        || equipDropAlpha_ > 0.01f || ultimateOverlayAlpha_ > 0.005f;
}

void BoardWidget::drawMoveAnimations(QPainter *painter) {
    for (const auto& anim : moveAnims_) {
        if (!anim.active) continue;
        
        // 计算插值位置
        QPointF fromPos = getCellCenter(anim.fromX, anim.fromY);
        QPointF toPos = getCellCenter(anim.toX, anim.toY);
        
        // 使用缓动函数让动画更平滑
        float t = anim.progress;
        // easeInOutQuad: t < 0.5 ? 2*t*t : 1 - pow(-2*t + 2, 2) / 2
        float easedT;
        if (t < 0.5f) {
            easedT = 2.0f * t * t;
        } else {
            easedT = 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        }
        
        QPointF currentPos(
            fromPos.x() + (toPos.x() - fromPos.x()) * easedT,
            fromPos.y() + (toPos.y() - fromPos.y()) * easedT
        );
        
        // 绘制单位在插值位置
        if (anim.unit) {
            drawUnitAtPosition(painter, currentPos, anim.unit);
        }
    }
}

void BoardWidget::drawDamageAnimations(QPainter *painter) {
    for (const auto& anim : damageAnims_) {
        if (!anim.active) continue;
        
        // 伤害数字向上飘动并淡出
        float alpha = 1.0f - anim.progress;
        int offsetY = static_cast<int>(-50.0f * anim.progress);
        
        QPointF pos = anim.pixelPos;
        pos.setY(pos.y() + offsetY);
        
        // 设置字体和颜色
        QFont font = appFont(anim.isCrit ? 22 : 16, true);
        painter->setFont(font);
        
        // 根据暴击选择颜色
        QColor textColor = anim.isCrit ? QColor(255, 68, 68, static_cast<int>(alpha * 255)) : 
                                        QColor(255, 255, 255, static_cast<int>(alpha * 255));
        
        // 为暴击添加金色边框
        if (anim.isCrit) {
            painter->setPen(QColor(255, 215, 0, static_cast<int>(alpha * 255)));
        } else {
            painter->setPen(textColor);
        }
        
        painter->drawText(
            QRectF(pos.x() - 50, pos.y() - 20, 100, 40),
            Qt::AlignCenter,
            QString::number(anim.damage)
        );
        
        // 为暴击添加额外装饰
        if (anim.isCrit) {
            painter->setPen(QPen(QColor(255, 215, 0, static_cast<int>(alpha * 150)), 2));
            painter->drawLine(
                QPointF(pos.x() - 30, pos.y() - 10),
                QPointF(pos.x() + 30, pos.y() - 10)
            );
        }
    }
}

void BoardWidget::drawDeathAnimations(QPainter *painter) {
    for (const auto& anim : deathAnims_) {
        if (!anim.active) continue;
        
        float t = anim.progress;
        float scale = 1.0f + t * 0.5f;  // 扩散效果
        float alpha = 1.0f - t;          // 淡出效果
        
        // 绘制扩散圆圈
        int radius = static_cast<int>(getCellSize() / 2 * scale);
        painter->setPen(QPen(QColor(231, 76, 60, static_cast<int>(alpha * 200)), 3));
        painter->setBrush(QColor(231, 76, 60, static_cast<int>(alpha * 50)));
        painter->drawEllipse(anim.pixelPos, radius, radius);
        
        // 绘制内部小圆圈
        painter->setPen(QPen(QColor(255, 200, 100, static_cast<int>(alpha * 150)), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(anim.pixelPos, radius * 0.6f, radius * 0.6f);
    }
}

// ============ 动态尺寸计算 ============

int BoardWidget::getCellSize() const {
    // 根据widget宽度计算单元格大小，确保棋盘和备战区完整显示
    int availableWidth = width() - 2 * BASE_BOARD_MARGIN - CELL_RIGHT_RESERVE;  // 预留右侧羁绊面板空间
    int availableHeight = height() - 2 * BASE_BOARD_MARGIN - BENCH_MARGIN - BASE_CELL_SIZE;  // 预留备战区
    
    int cellByWidth = availableWidth / BOARD_N;
    int cellByHeight = availableHeight / (BOARD_M + 1);  // +1 for bench
    
    // 取较小值，并限制在合理范围内
    int cellSize = qMin(cellByWidth, cellByHeight);
    cellSize = qBound(CELL_SIZE_MIN, cellSize, CELL_SIZE_MAX);  // 限制单元格尺寸范围
    return cellSize;
}

int BoardWidget::getBoardMargin() const {
    // 边距基于widget尺寸动态计算，但保持最小边距
    int margin = qMin(width(), height()) / 30;
    return qBound(MARGIN_MIN, margin, MARGIN_MAX);
}

int BoardWidget::getBenchHeight() const {
    return getCellSize();
}

void BoardWidget::recalcLayout() {
    // 动态尺寸方法在绘制时实时计算，无需缓存
    // 此方法仅用于触发布局更新
}

void BoardWidget::loadAssets() {
    assetRootPath_ = findAssetsRoot();
    if (assetRootPath_.isEmpty()) {
        return;
    }

    pixmapCache_["ui/background"] = loadPixmapFromAssets("ui/background.png");
    if (pixmapCache_["ui/background"].isNull()) {
        pixmapCache_["ui/background"] = loadPixmapFromAssets("ui/background.jpg");
    }
    if (pixmapCache_["ui/background"].isNull()) {
        pixmapCache_["ui/background"] = loadPixmapFromAssets("ui/background.svg");
    }
    pixmapCache_["ui/board_cell"] = loadPixmapFromAssets("ui/board_cell.svg");
    pixmapCache_["ui/bench_cell"] = loadPixmapFromAssets("ui/bench_cell.svg");

    if (!assetRootPath_.isEmpty()) {
        QDir soundsDir(QDir(assetRootPath_).filePath("sounds"));
        for (const QString& wavFile : soundsDir.entryList({"*.wav"}, QDir::Files)) {
            auto effect = std::make_unique<QSoundEffect>();
            effect->setSource(QUrl::fromLocalFile(soundsDir.filePath(wavFile)));
            effect->setVolume(0.9f);
            soundEffect_.push_back(std::move(effect));
        }
    }
}

QString BoardWidget::findAssetsRoot() const {
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (dir.exists("assets")) {
            return dir.filePath("assets");
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QString();
}

QPixmap BoardWidget::loadPixmapFromAssets(const QString &relativePath) const {
    if (assetRootPath_.isEmpty()) return QPixmap();
    QString fullPath = QDir(assetRootPath_).filePath(relativePath);
    if (!QFileInfo::exists(fullPath)) return QPixmap();

    QPixmap pixmap(fullPath);
    if (!pixmap.isNull()) return pixmap;

    // QPixmap 直接加载失败（如 SVG），尝试 QSvgRenderer
    if (relativePath.endsWith(".svg", Qt::CaseInsensitive)) {
        QSvgRenderer renderer(fullPath);
        if (renderer.isValid()) {
            QSize sz = renderer.defaultSize();
            if (sz.isEmpty()) sz = QSize(128, 128);
            QPixmap svgPixmap(sz);
            svgPixmap.fill(Qt::transparent);
            QPainter p(&svgPixmap);
            renderer.render(&p);
            p.end();
            return svgPixmap;
        }
    }
    return QPixmap();
}

QPixmap BoardWidget::getUnitPixmap(std::shared_ptr<Unit> unit) {
    if (!unit) {
        return QPixmap();
    }

    QString spriteName = QString::fromStdString(unit->getSpriteName());
    QString cacheKey = QString("unit/%1").arg(spriteName);
    auto it = pixmapCache_.find(cacheKey);
    if (it != pixmapCache_.end()) {
        return it->second;
    }

    QStringList suffixes = {"png", "jpg", "jpeg", "svg"};
    QPixmap pixmap;
    for (const auto &suffix : suffixes) {
        pixmap = loadPixmapFromAssets(QString("units/%1.%2").arg(spriteName, suffix));
        if (!pixmap.isNull()) {
            break;
        }
    }

    if (pixmap.isNull()) {
        pixmap = loadPixmapFromAssets("units/default.png");
    }

    pixmapCache_[cacheKey] = pixmap;
    return pixmap;
}

// ============ 拖拽视觉反馈实现 ============

void BoardWidget::drawInfoPanel(QPainter *painter) {
    if (!game_ || !hasSelection_) return;

    // 获取选中的单位
    std::shared_ptr<Unit> unit;
    QRect cellRect;
    if (selInBench_) {
        unit = game_->getPlayer().getBench().getUnit(selX_);
        if (!unit) return;
        cellRect = getBenchSlotRect(selX_);
    } else {
        unit = game_->getBoard().getUnit(selX_, selY_);
        if (!unit) return;
        cellRect = getCellRect(selX_, selY_);
    }

    // 面板参数
    const int panelWidth = 200;
    const int panelMargin = 8;
    const int lineHeight = 18;
    const int barHeight = 14;
    
    // 面板位置：在单位格子右侧，如果超出窗口则放左侧
    int panelX = cellRect.right() + panelMargin;
    int panelY = cellRect.top();
    if (panelX + panelWidth > width()) {
        panelX = cellRect.left() - panelWidth - panelMargin;
    }
    if (panelY + 300 > height()) {
        panelY = height() - 310;
    }
    if (panelY < 0) panelY = 4;

    QRect panelRect(panelX, panelY, panelWidth, 280);
    
    // 半透明背景
    painter->setPen(QPen(QColor(255, 255, 255, 80), 1));
    painter->setBrush(QColor(20, 25, 35, 220));
    painter->drawRoundedRect(panelRect, 6, 6);

    int y = panelY + 8;
    int xL = panelX + 10;
    int xR = panelX + panelWidth - 10;
    int contentW = panelWidth - 20;

    // 标题：名称 + 星级
    painter->setPen(QColor(255, 215, 0));
    painter->setFont(appFont(12, true));
    painter->drawText(xL, y, contentW, 16, Qt::AlignLeft,
        QString::fromStdString(unit->displayName()) + QString("  ★%1").arg(unit->getStarLevel()));
    y += 20;

    // —— HP 条 ——
    painter->setPen(QColor(200, 200, 200));
    painter->setFont(appFont(9, false));
    painter->drawText(xL, y, QString("HP  %1 / %2").arg(unit->getHP()).arg(unit->getMaxHP()));
    y += 12;
    float hpRatio = unit->getMaxHP() > 0 ? (float)unit->getHP() / unit->getMaxHP() : 0;
    QColor hpColor = hpRatio > 0.6f ? QColor(46, 204, 113) : (hpRatio > 0.3f ? QColor(241, 196, 15) : QColor(231, 76, 60));
    painter->fillRect(xL, y, contentW, barHeight, QColor(40, 40, 40));
    painter->fillRect(xL + 1, y + 1, (int)((contentW - 2) * hpRatio), barHeight - 2, hpColor);
    y += barHeight + 6;

    // —— ATK 条 ——
    painter->setPen(QColor(200, 200, 200));
    painter->drawText(xL, y, QString("ATK  %1").arg(unit->getATK()));
    y += 12;
    // ATK 条：相对最大显示（假设 maxATK ≈ maxHP/10 作为参考）
    int refAtk = std::max(1, unit->getATK() + 20);
    float atkRatio = std::min(1.0f, (float)unit->getATK() / refAtk);
    painter->fillRect(xL, y, contentW, barHeight, QColor(40, 40, 40));
    painter->fillRect(xL + 1, y + 1, (int)((contentW - 2) * atkRatio), barHeight - 2, QColor(231, 76, 60));
    y += barHeight + 6;

    // —— Mana 条 ——
    painter->setPen(QColor(200, 200, 200));
    painter->drawText(xL, y, QString("Mana  %1 / %2").arg(unit->getMana()).arg(unit->getMaxMana()));
    y += 12;
    float manaRatio = unit->getMaxMana() > 0 ? (float)unit->getMana() / unit->getMaxMana() : 0;
    painter->fillRect(xL, y, contentW, barHeight, QColor(40, 40, 40));
    painter->fillRect(xL + 1, y + 1, (int)((contentW - 2) * manaRatio), barHeight - 2, QColor(52, 152, 219));
    y += barHeight + 8;

    // —— 专属武器 ——
    painter->setPen(QColor(180, 180, 180));
    painter->setFont(appFont(8, false));
    painter->drawText(xL, y, contentW, 14, Qt::AlignLeft,
        QString::fromUtf8("🔱 专属: %1").arg(QString::fromStdString(unit->getEquipmentName())));
    y += 16;

    // —— 装备列表 ——
    painter->setPen(QColor(255, 215, 0));
    painter->setFont(appFont(9, true));
    painter->drawText(xL, y, QString::fromUtf8("🎒 装备"));
    y += 14;

    infoEquipRects_.clear();
    const QString slotNames[] = {"武器", "防具", "饰品1", "饰品2"};
    bool hasAnyEquip = false;
    for (int i = 0; i < 4; ++i) {
        auto eq = unit->GetEquipmentSlot(i);
        if (eq) {
            hasAnyEquip = true;
            painter->setPen(QColor(200, 200, 200));
            painter->setFont(appFont(8, false));
            QString eqText = QString("[%1] %2 ★%3 ATK+%4 HP+%5")
                .arg(slotNames[i])
                .arg(QString::fromStdString(eq->name))
                .arg(eq->starLevel)
                .arg(eq->atkBonus).arg(eq->hpBonus);
            painter->drawText(xL + 6, y, contentW - 22, 14, Qt::AlignLeft, eqText);
            
            // 卸载按钮 [×]
            QRect btnRect(xL + contentW - 14, y - 2, 14, 14);
            infoEquipRects_.push_back({btnRect, i});
            painter->setPen(QPen(QColor(231, 76, 60), 1));
            painter->setBrush(QColor(231, 76, 60, 60));
            painter->drawRoundedRect(btnRect, 2, 2);
            painter->setPen(QColor(255, 255, 255));
            painter->setFont(appFont(8, true));
            painter->drawText(btnRect, Qt::AlignCenter, QString::fromUtf8("×"));
            
            y += 14;
        }
    }
    if (!hasAnyEquip) {
        painter->setPen(QColor(120, 120, 120));
        painter->setFont(appFont(8, false));
        painter->drawText(xL + 6, y, "（无装备）");
        y += 14;
    }
    y += 4;

    // —— 羁绊/光环效果 ——
    const auto& traits = activeTraits_;
    if (!traits.empty()) {
        painter->setPen(QColor(255, 215, 0));
        painter->setFont(appFont(9, true));
        painter->drawText(xL, y, QString::fromUtf8("✨ 激活羁绊"));
        y += 14;

        painter->setPen(QColor(160, 200, 160));
        painter->setFont(appFont(8, false));
        QString unitName = QString::fromStdString(unit->displayName());
        Race unitRace = unit->GetRace();
        ClassType unitClass = unit->GetClassType();
        for (const auto& trait : traits) {
            QStringList lines = trait.split('\n');
            QString firstLine = lines.isEmpty() ? QString() : lines[0].trimmed();
            // 装备套装条目只显示当前单位的
            if (firstLine.contains(QString::fromUtf8("】—"))) {
                if (!firstLine.endsWith(unitName)) continue;
            } else {
                // 种族/职业羁绊：检查当前单位是否属于该羁绊
                if (firstLine.startsWith(QString::fromUtf8("【人】")) && unitRace != Race::Ningen) continue;
                if (firstLine.startsWith(QString::fromUtf8("【妖】")) && unitRace != Race::Youkai) continue;
                if (firstLine.startsWith(QString::fromUtf8("【巫女】")) && unitClass != ClassType::Miko) continue;
                if (firstLine.startsWith(QString::fromUtf8("【魔法使】")) && unitClass != ClassType::Mahoushi) continue;
                if (firstLine.startsWith(QString::fromUtf8("【女仆】")) && unitClass != ClassType::Maid) continue;
                if (firstLine.startsWith(QString::fromUtf8("【记者】")) && unitClass != ClassType::Journalist) continue;
            }
            for (const QString& line : lines) {
                if (line.trimmed().isEmpty()) continue;
                painter->drawText(xL + 6, y, contentW - 6, 14, Qt::AlignLeft, line.trimmed());
                y += 14;
            }
        }
    }
}

bool BoardWidget::isValidDropTarget(int x, int y, bool isBench) const {
    if (!game_ || !draggedUnit_) return false;
    
    if (isBench) {
        // 备战区：索引有效即可
        return (x >= 0 && x < BENCH_SIZE);
    } else {
        // 棋盘：必须在玩家半场（下半部分）
        if (x < 0 || x >= BOARD_M || y < 0 || y >= BOARD_N) return false;
        // 玩家半场：第4行到第7行 (索引3-7)
        if (x < BOARD_M / 2) return false;
        // 如果目标格子已有单位，允许交换
        return true;
    }
}

void BoardWidget::drawDragGhost(QPainter *painter) {
    if (!isDragging_ || !draggedUnit_) return;
    
    QPixmap pixmap = getUnitPixmap(draggedUnit_);
    
    // 在鼠标位置绘制半透明单位
    painter->save();
    painter->setOpacity(0.6);
    
    int ghostSize = getCellSize();
    QRect ghostRect(
        dragCurrentPos_.x() - ghostSize / 2,
        dragCurrentPos_.y() - ghostSize / 2,
        ghostSize,
        ghostSize
    );
    
    if (!pixmap.isNull()) {
        // 有精灵图：绘制半透明精灵
        QRect spriteRect = ghostRect.adjusted(8, 22, -8, -22);
        QPixmap scaledPixmap = pixmap.scaled(spriteRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPoint spriteTopLeft(
            spriteRect.left() + (spriteRect.width() - scaledPixmap.width()) / 2,
            spriteRect.top() + (spriteRect.height() - scaledPixmap.height()) / 2
        );
        painter->drawPixmap(spriteTopLeft, scaledPixmap);
        
        // 绘制白色边框突出幽灵
        painter->setOpacity(0.3);
        painter->setPen(QPen(QColor(255, 255, 255), 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(spriteRect);
    } else {
        // 无精灵图：绘制半透明圆形
        QColor ghostColor = draggedUnit_->getOwner() == Owner::PlayerCtrl ?
            QColor(46, 204, 113, 150) : QColor(231, 76, 60, 150);
        painter->setBrush(ghostColor);
        painter->setPen(QPen(QColor(255, 255, 255, 100), 2, Qt::DashLine));
        painter->drawEllipse(ghostRect.center(), ghostSize / 2 - 8, ghostSize / 2 - 8);
    }
    
    // 绘制拖拽源标记（小箭头指示来源）
    painter->setOpacity(0.8);
    painter->setPen(QPen(QColor(255, 255, 255), 2));
    painter->setBrush(Qt::NoBrush);
    QPointF fromCenter;
    if (dragFromBench_) {
        QRect fromRect = getBenchSlotRect(dragStartX_);
        fromCenter = fromRect.center();
    } else {
        fromCenter = getCellCenter(dragStartX_, dragStartY_);
    }
    // 绘制从源位置到鼠标位置的虚线
    painter->setPen(QPen(QColor(255, 255, 255, 80), 1, Qt::DashLine));
    painter->drawLine(fromCenter, dragCurrentPos_);
    
    painter->restore();
}

void BoardWidget::drawPlacementPreview(QPainter *painter) {
    if (!isDragging_ || dropPreviewX_ < 0) return;
    
    QRect previewRect;
    if (dropPreviewInBench_) {
        previewRect = getBenchSlotRect(dropPreviewX_);
    } else {
        if (dropPreviewX_ >= BOARD_M || dropPreviewY_ >= BOARD_N) return;
        previewRect = getCellRect(dropPreviewX_, dropPreviewY_);
    }
    
    painter->save();
    
    if (dropPreviewValid_) {
        // 有效放置：绿色高亮
        painter->setBrush(QColor(46, 204, 113, 60));
        painter->setPen(QPen(QColor(46, 204, 113), 3));
        
        // 绘制角落标记（四角括号效果）
        int cornerLen = 12;
        int cx = previewRect.center().x();
        int cy = previewRect.center().y();
        int left = previewRect.left();
        int right = previewRect.right();
        int top = previewRect.top();
        int bottom = previewRect.bottom();
        
        QPen cornerPen(QColor(46, 204, 113), 3);
        painter->setPen(cornerPen);
        painter->setBrush(Qt::NoBrush);
        // 左上角
        painter->drawLine(left, top + cornerLen, left, top);
        painter->drawLine(left, top, left + cornerLen, top);
        // 右上角
        painter->drawLine(right - cornerLen, top, right, top);
        painter->drawLine(right, top, right, top + cornerLen);
        // 左下角
        painter->drawLine(left, bottom - cornerLen, left, bottom);
        painter->drawLine(left, bottom, left + cornerLen, bottom);
        // 右下角
        painter->drawLine(right - cornerLen, bottom, right, bottom);
        painter->drawLine(right, bottom, right, bottom - cornerLen);
        
        // 填充半透明底色
        painter->setBrush(QColor(46, 204, 113, 30));
        painter->setPen(Qt::NoPen);
        painter->drawRect(previewRect);
    } else {
        // 无效放置：红色高亮
        painter->setBrush(QColor(231, 76, 60, 40));
        painter->setPen(QPen(QColor(231, 76, 60), 2, Qt::DashLine));
        painter->drawRect(previewRect);
        
        // 绘制X标记
        painter->setPen(QPen(QColor(231, 76, 60, 150), 3));
        int margin = 8;
        painter->drawLine(previewRect.left() + margin, previewRect.top() + margin,
                         previewRect.right() - margin, previewRect.bottom() - margin);
        painter->drawLine(previewRect.right() - margin, previewRect.top() + margin,
                         previewRect.left() + margin, previewRect.bottom() - margin);
    }
    
    painter->restore();
}

void BoardWidget::drawDropZoneBoundaries(QPainter *painter) {
    if (!isDragging_) return;
    
    painter->save();
    painter->setOpacity(0.3);
    
    // 棋盘边界（玩家半场 - 可放置区域）
    int midY = BOARD_M / 2 * getCellSize() + getBoardMargin();
    QRect playerBoardRect(
        getBoardMargin(),
        midY,
        BOARD_N * getCellSize(),
        (BOARD_M / 2) * getCellSize()
    );
    painter->setPen(QPen(QColor(46, 204, 113), 2, Qt::DashLine));
    painter->setBrush(QColor(46, 204, 113, 15));
    painter->drawRect(playerBoardRect);
    
    // 备战区边界
    int benchY = getBoardMargin() + BOARD_M * getCellSize() + BENCH_MARGIN;
    QRect benchBoundary(
        getBoardMargin(),
        benchY,
        BOARD_N * getCellSize(),
        getBenchHeight()
    );
    painter->setPen(QPen(QColor(52, 152, 219), 2, Qt::DashLine));
    painter->setBrush(QColor(52, 152, 219, 15));
    painter->drawRect(benchBoundary);
    
    // 添加文字提示
    painter->setOpacity(0.6);
    painter->setPen(QColor(46, 204, 113));
    painter->setFont(appFont(9, true));
    painter->drawText(playerBoardRect.adjusted(8, 8, -8, -8),
                     Qt::AlignLeft | Qt::AlignTop,
                     "▼ 可放置区域");
    
    painter->setPen(QColor(52, 152, 219));
    painter->drawText(benchBoundary.adjusted(8, 8, -8, -8),
                     Qt::AlignLeft | Qt::AlignTop,
                     "▼ 备战区");
    
    painter->restore();
}
