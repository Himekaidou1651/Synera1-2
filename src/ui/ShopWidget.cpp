/**
 * @file    ShopWidget.cpp
 * @brief   商店界面组件实现
 * @author  
 * @date    2026-06-24
 */

#include "ShopWidget.h"
#include "../core/Game.h"
#include "../core/Player.h"
#include "../core/Unit.h"
#include "../core/Shop.h"
#include <QPainter>
#include <QIcon>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

ShopWidget::ShopWidget(std::shared_ptr<Game> game, QWidget *parent)
    : QWidget(parent), game_(game) {
    setStyleSheet("ShopWidget { background-color: #34495e; border: 2px solid #2c3e50; border-radius: 8px; padding: 12px; }");
    loadAssets();
    setupUI();
    updateDisplay();
}

ShopWidget::~ShopWidget() {
}

void ShopWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(LAYOUT_SPACING_TIGHT);
    mainLayout->setContentsMargins(SHOP_MARGIN, SHOP_MARGIN, SHOP_MARGIN, SHOP_MARGIN);
    
    // --- 创建标签页 ---
    tabWidget_ = new QTabWidget();
    tabWidget_->setStyleSheet(
        "QTabWidget::pane { border: none; background: transparent; } "
        "QTabBar::tab { "
        "  background-color: #2c3e50; color: #95a5a6; "
        "  padding: 6px 16px; font-weight: bold; font-size: 12px; "
        "  border-top-left-radius: 4px; border-top-right-radius: 4px; "
        "  border: 1px solid #34495e; border-bottom: none; "
        "} "
        "QTabBar::tab:selected { "
        "  background-color: #34495e; color: #ecf0f1; "
        "  border-bottom: 2px solid #3498db; "
        "} "
        "QTabBar::tab:hover:!selected { "
        "  background-color: #3d566e; color: #bdc3c7; "
        "}"
    );
    
    // ======== 招募页 ========
    recruitTab_ = new QWidget();
    QVBoxLayout *recruitLayout = new QVBoxLayout(recruitTab_);
    recruitLayout->setSpacing(RECRUIT_SPACING);
    recruitLayout->setContentsMargins(SHOP_TAB_MARGIN, SHOP_MARGIN, SHOP_TAB_MARGIN, SHOP_TAB_MARGIN);
    
    // 标题和金币
    QHBoxLayout *headerLayout = new QHBoxLayout();
    shopTitleLabel_ = new QLabel("🛍 招募区");
    shopTitleLabel_->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; color: #ecf0f1; letter-spacing: 1px; }");
    headerLayout->addWidget(shopTitleLabel_);
    headerLayout->addStretch();
    goldDisplayLabel_ = new QLabel("💰 0");
    goldDisplayLabel_->setStyleSheet("QLabel { font-size: 12px; color: #f39c12; font-weight: bold; background-color: #2c3e50; padding: 3px 6px; border-radius: 3px; }");
    headerLayout->addWidget(goldDisplayLabel_);
    recruitLayout->addLayout(headerLayout);
    
    // 控制按钮行（图标按钮）
    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(LAYOUT_SPACING_SHOP);
    
    refreshBtn_ = new QPushButton("🔄");
    refreshBtn_->setFixedSize(SHOPBTN_W, SHOPBTN_H);
    refreshBtn_->setFocusPolicy(Qt::TabFocus);
    refreshBtn_->setToolTip("刷新商店（花费1金币）");
    refreshBtn_->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: 2px solid #2980b9; "
        "border-radius: 5px; font-size: 18px; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
        "QPushButton:disabled { background-color: #95a5a6; border: 2px solid #7f8c8d; color: #bdc3c7; }"
    );
    connect(refreshBtn_, &QPushButton::clicked, this, &ShopWidget::onRefreshShop);
    controlLayout->addWidget(refreshBtn_);
    
    upgradePopBtn_ = new QPushButton("👥");
    upgradePopBtn_->setFixedSize(SHOPBTN_W, SHOPBTN_H);
    upgradePopBtn_->setFocusPolicy(Qt::TabFocus);
    upgradePopBtn_->setToolTip("升级人口上限");
    upgradePopBtn_->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: 2px solid #1e8449; "
        "border-radius: 5px; font-size: 18px; } "
        "QPushButton:hover { background-color: #1e8449; } "
        "QPushButton:pressed { background-color: #145a32; }"
        "QPushButton:disabled { background-color: #95a5a6; border: 2px solid #7f8c8d; color: #bdc3c7; }"
    );
    connect(upgradePopBtn_, &QPushButton::clicked, this, &ShopWidget::onUpgradePopulation);
    controlLayout->addWidget(upgradePopBtn_);
    
    controlLayout->addStretch();
    recruitLayout->addLayout(controlLayout);
    
    // 商店槽位网格
    shopLayout_ = new QGridLayout();
    shopLayout_->setSpacing(LAYOUT_SPACING_SHOP);
    
    const int slotCount = game_ ? game_->getShop().getSlotCount() : 5;
    for (int i = 0; i < slotCount; ++i) {
        QPushButton *slotBtn = new QPushButton();
        slotBtn->setMinimumSize(90, 100);
        slotBtn->setMaximumSize(90, 100);
        slotBtn->setToolTip("点击招募 / 拖放到棋盘或备战区");

        QString slotStyle =
            "QPushButton { "
            "background-color: #34495e; "
            "color: #ecf0f1; "
            "border: 3px solid #95a5a6; "
            "border-radius: 6px; "
            "font-size: 10px; "
            "text-align: top left; "
            "padding: 4px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #2c3e50; "
            "border: 3px solid #3498db; "
            "} "
            "QPushButton:pressed { "
            "background-color: #1a252f; "
            "border: 3px solid #2980b9; "
            "}";

        if (!pixmapCache_["ui/shop_slot"].isNull() && !assetRootPath_.isEmpty()) {
            QString slotPath = QUrl::fromLocalFile(QDir(assetRootPath_).filePath("ui/shop_slot.svg")).toString();
            slotStyle = QString(
                "QPushButton { "
                "border-image: url(%1) 0 0 0 0 stretch stretch; "
                "color: #ecf0f1; "
                "border: 3px solid #95a5a6; "
                "border-radius: 6px; "
                "font-size: 10px; "
                "text-align: top left; "
                "padding: 4px; "
                "font-weight: bold; "
                "} "
                "QPushButton:hover { "
                "background-color: rgba(44, 62, 80, 0.8); "
                "border: 3px solid #3498db; "
                "} "
                "QPushButton:pressed { "
                "background-color: rgba(26, 37, 47, 0.9); "
                "border: 3px solid #2980b9; "
                "}")
                .arg(slotPath);
        }

        slotBtn->setFocusPolicy(Qt::TabFocus);
        slotBtn->setStyleSheet(slotStyle);
        
        connect(slotBtn, &QPushButton::clicked, this, [this, i]() { onUnitClicked(i); });
        slotBtn->installEventFilter(this);
        slotBtn->setAttribute(Qt::WA_Hover);
        
        shopSlots_.push_back(slotBtn);
        shopLayout_->addWidget(slotBtn, 0, i);
    }
    
    recruitLayout->addLayout(shopLayout_);
    recruitLayout->addStretch();
    
    // ======== 装备页 ========
    equipTab_ = new QWidget();
    QVBoxLayout *equipLayout = new QVBoxLayout(equipTab_);
    equipLayout->setSpacing(LAYOUT_SPACING_SHOP);
    equipLayout->setContentsMargins(SHOP_TAB_MARGIN, SHOP_MARGIN, SHOP_TAB_MARGIN, SHOP_TAB_MARGIN);
    
    // ===== P2: 装备购买区（移到这里） =====
    QLabel *equipShopLabel = new QLabel("🎯 装备商店（刷新时生成）");
    equipShopLabel->setStyleSheet("QLabel { font-size: 11px; color: #f39c12; font-weight: bold; padding: 2px 0; }");
    equipLayout->addWidget(equipShopLabel);
    
    QHBoxLayout *equipShopLayout2 = new QHBoxLayout();
    equipShopLayout2->setSpacing(6);
    for (int i = 0; i < 3; ++i) {
        QPushButton *btn = new QPushButton("空");
        btn->setMinimumSize(85, 85);
        btn->setMaximumSize(85, 85);
        btn->setFocusPolicy(Qt::TabFocus);
        btn->setStyleSheet(
            "QPushButton { background-color: #2c3e50; color: #bdc3c7; border: 2px solid #7f8c8d; "
            "border-radius: 5px; font-size: 9px; text-align: center; } "
            "QPushButton:hover { border: 2px solid #f39c12; color: #f39c12; } "
            "QPushButton:disabled { background-color: #1a252f; color: #5a6a7a; }"
        );
        int idx = i;
        connect(btn, &QPushButton::clicked, this, [this, idx]() { onBuyEquipSlot(idx); });
        equipBuySlots_.push_back(btn);
        equipShopLayout2->addWidget(btn);
    }
    equipShopLayout2->addStretch();
    equipLayout->addLayout(equipShopLayout2);
    
    // 装备控制按钮行
    QHBoxLayout *equipControlLayout = new QHBoxLayout();
    equipControlLayout->setSpacing(LAYOUT_SPACING_SHOP);

    applyEquipBtn_ = new QPushButton("⚔");
    applyEquipBtn_->setFixedSize(SHOPBTN_W, SHOPBTN_H);
    applyEquipBtn_->setFocusPolicy(Qt::TabFocus);
    applyEquipBtn_->setToolTip("将暂存装备穿戴到备战区单位");
    applyEquipBtn_->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: 2px solid #1e8449; "
        "border-radius: 5px; font-size: 18px; } "
        "QPushButton:hover { background-color: #1e8449; } "
        "QPushButton:pressed { background-color: #186a3b; }"
        "QPushButton:disabled { background-color: #95a5a6; border: 2px solid #7f8c8d; color: #bdc3c7; }"
    );
    connect(applyEquipBtn_, &QPushButton::clicked, this, &ShopWidget::onApplyEquipment);
    equipControlLayout->addWidget(applyEquipBtn_);

    // 装备页中也保留一个合成按钮（方便装备后直接合成）
    combineInEquipBtn_ = new QPushButton("⚡");
    combineInEquipBtn_->setFixedSize(SHOPBTN_W, SHOPBTN_H);
    combineInEquipBtn_->setFocusPolicy(Qt::TabFocus);
    combineInEquipBtn_->setToolTip("合成三星单位（三合一）");
    combineInEquipBtn_->setStyleSheet(
        "QPushButton { background-color: #d35400; color: white; border: 2px solid #a84300; "
        "border-radius: 5px; font-size: 18px; } "
        "QPushButton:hover { background-color: #a84300; } "
        "QPushButton:pressed { background-color: #7a3600; }"
        "QPushButton:disabled { background-color: #95a5a6; border: 2px solid #7f8c8d; color: #bdc3c7; }"
    );
    connect(combineInEquipBtn_, &QPushButton::clicked, this, &ShopWidget::onAutoCombine);
    equipControlLayout->addWidget(combineInEquipBtn_);

    // P2-2: 装备合成按钮
    QPushButton *combineEquipBtn = new QPushButton("🔧");
    combineEquipBtn->setFixedSize(SHOPBTN_W, SHOPBTN_H);
    combineEquipBtn->setFocusPolicy(Qt::TabFocus);
    combineEquipBtn->setToolTip("三件同星同类型装备→一件更高星装备");
    combineEquipBtn->setStyleSheet(
        "QPushButton { background-color: #e67e22; color: white; border: 2px solid #d35400; "
        "border-radius: 5px; font-size: 18px; } "
        "QPushButton:hover { background-color: #d35400; } "
        "QPushButton:disabled { background-color: #95a5a6; border: 2px solid #7f8c8d; color: #bdc3c7; }"
    );
    connect(combineEquipBtn, &QPushButton::clicked, this, &ShopWidget::onCombineEquipment);
    equipControlLayout->addWidget(combineEquipBtn);

    // P2-3: 装备售卖按钮
    QPushButton *sellEquipBtn = new QPushButton("💰");
    sellEquipBtn->setFixedSize(SHOPBTN_W, SHOPBTN_H);
    sellEquipBtn->setFocusPolicy(Qt::TabFocus);
    sellEquipBtn->setToolTip("出售选中装备（1星=1金, 2星=3金, 3星=6金）");
    sellEquipBtn->setStyleSheet(
        "QPushButton { background-color: #c0392b; color: white; border: 2px solid #a93226; "
        "border-radius: 5px; font-size: 18px; } "
        "QPushButton:hover { background-color: #a93226; } "
        "QPushButton:disabled { background-color: #95a5a6; border: 2px solid #7f8c8d; color: #bdc3c7; }"
    );
    connect(sellEquipBtn, &QPushButton::clicked, this, &ShopWidget::onSellEquipment);
    equipControlLayout->addWidget(sellEquipBtn);

    // 装备页刷新按钮（与招募页功能相同）
    QPushButton *equipRefreshBtn = new QPushButton("🔄");
    equipRefreshBtn->setFixedSize(SHOPBTN_W, SHOPBTN_H);
    equipRefreshBtn->setFocusPolicy(Qt::TabFocus);
    equipRefreshBtn->setToolTip("刷新商店（花费1金币）");
    equipRefreshBtn->setStyleSheet(
        "QPushButton { background-color: #2980b9; color: white; border: 2px solid #2471a3; "
        "border-radius: 5px; font-size: 18px; } "
        "QPushButton:hover { background-color: #2471a3; } "
        "QPushButton:disabled { background-color: #95a5a6; border: 2px solid #7f8c8d; color: #bdc3c7; }"
    );
    connect(equipRefreshBtn, &QPushButton::clicked, this, &ShopWidget::onRefreshShop);
    equipControlLayout->addWidget(equipRefreshBtn);

    equipControlLayout->addStretch();
    equipLayout->addLayout(equipControlLayout);

    // 暂存装备列表
    droppedEquipLabel_ = new QLabel("暂存装备：无");
    droppedEquipLabel_->setStyleSheet("QLabel { font-size: 11px; color: #ecf0f1; padding: 4px; } ");
    droppedEquipLabel_->setWordWrap(true);
    equipLayout->addWidget(droppedEquipLabel_);

    equipLayout->addStretch();

    // 添加标签页
    tabWidget_->addTab(recruitTab_, "📋 招募");
    tabWidget_->addTab(equipTab_, "🎯 装备");
    
    // P3-3: 装备图鉴页
    collectionTab_ = new QWidget();
    QVBoxLayout *colLayout = new QVBoxLayout(collectionTab_);
    colLayout->setContentsMargins(6, 4, 6, 4);
    QLabel *colLabel = new QLabel("📖 装备图鉴");
    colLabel->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #f39c12; }");
    colLayout->addWidget(colLabel);
    
    auto allTemplates = GetEquipmentTemplates();
    std::set<std::string> shown;
    for (const auto& t : allTemplates) {
        if (shown.count(t.name)) continue;
        shown.insert(t.name);
        QString text = QString::fromUtf8("· %1 ★%2 [%3] ATK+%4~%5 HP+%6~%7")
            .arg(QString::fromStdString(t.name))
            .arg(t.starLevel)
            .arg(QString::fromStdString(Unit::EquipmentTypeToString(t.type)))
            .arg(t.atkMin).arg(t.atkMax)
            .arg(t.hpMin).arg(t.hpMax);
        if (!t.restriction.empty())
            text += QString::fromUtf8(" [%1限定]").arg(QString::fromStdString(t.restriction));
        if (!t.effect.empty())
            text += QString::fromUtf8(" ✨%1").arg(QString::fromStdString(t.effect));
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet("QLabel { font-size: 9px; color: #bdc3c7; padding: 1px; }");
        colLayout->addWidget(lbl);
    }
    colLayout->addStretch();
    tabWidget_->addTab(collectionTab_, "📖 图鉴");

    mainLayout->addWidget(tabWidget_);

    // 状态提示（放在标签页外部，始终可见）
    actionStatusLabel_ = new QLabel("");
    actionStatusLabel_->setStyleSheet("QLabel { font-size: 11px; color: #f1c40f; padding: 2px 4px; }");
    mainLayout->addWidget(actionStatusLabel_);
}

void ShopWidget::updateDisplay() {
    hoveredSlotIndex_ = -1;  // 每次刷新时重置悬停状态
    updateGoldDisplay();
    updateEquipmentDisplay();
    if (game_) {
        const int slotCount = game_->getShop().getSlotCount();
        for (int i = 0; i < slotCount; ++i) {
            drawShopItem(i);
        }
        // P0-1: 刷新装备购买按钮
        auto& equipSlots = game_->getShop().getEquipmentSlots();
        for (int i = 0; i < static_cast<int>(equipBuySlots_.size()); ++i) {
            if (i < static_cast<int>(equipSlots.size()) && !equipSlots[i].sold) {
                const auto& es = equipSlots[i];
                QString typeStr = QString::fromStdString(Unit::EquipmentTypeToString(es.eq.type));
                QString text = QString("%1\n%2 ★%3\n💰 %4金")
                    .arg(typeStr)
                    .arg(QString::fromStdString(es.eq.name))
                    .arg(es.eq.starLevel)
                    .arg(es.price);
                equipBuySlots_[i]->setText(text);
                equipBuySlots_[i]->setEnabled(true);
                equipBuySlots_[i]->setToolTip(
                    QString::fromUtf8("%1 ATK+%2 HP+%3%4")
                        .arg(QString::fromStdString(es.eq.name))
                        .arg(es.eq.atkBonus).arg(es.eq.hpBonus)
                        .arg(es.eq.restriction.empty() ? QString() : QString::fromUtf8(" [%1限定]").arg(QString::fromStdString(es.eq.restriction)))
                );
            } else {
                equipBuySlots_[i]->setText("空");
                equipBuySlots_[i]->setEnabled(false);
                equipBuySlots_[i]->setToolTip("");
            }
        }
    }
}

void ShopWidget::refreshShop() {
    if (!game_) {
        updateDisplay();
        return;
    }
    
    // 委托给 Shop::refresh 处理金币扣除和单位生成
    Shop& shop = game_->getShop();
    Player& player = game_->getPlayer();
    int currentRound = game_->getCurrentRound();
    
    shop.refresh(Owner::PlayerCtrl, currentRound, player, 1);
    
    updateDisplay();
}

void ShopWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    auto bgIt = pixmapCache_.find("ui/shop_background");
    if (bgIt != pixmapCache_.end() && !bgIt->second.isNull()) {
        painter.drawPixmap(rect(), bgIt->second.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        QWidget::paintEvent(event);
    }
}

bool ShopWidget::eventFilter(QObject *obj, QEvent *event) {
    // 装备 QLabel：按下延迟启动 QDrag（不能在事件处理中直接 exec）
    QLabel *label = qobject_cast<QLabel*>(obj);
    if (label && label->property("equipId").isValid()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                int equipId = label->property("equipId").toInt();
                QTimer::singleShot(0, this, [this, equipId]() {
                    QDrag *drag = new QDrag(this);
                    QMimeData *mime = new QMimeData();
                    mime->setData("application/x-synera-equip", QByteArray::number(equipId));
                    drag->setMimeData(mime);
                    drag->exec(Qt::CopyAction);
                });
                return true;
            }
        }
        return false;
    }
    
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
        QPushButton *button = qobject_cast<QPushButton *>(obj);
        if (button) {
            int index = -1;
            for (int i = 0; i < static_cast<int>(shopSlots_.size()); ++i) {
                if (shopSlots_[i] == button) {
                    index = i;
                    break;
                }
            }
            if (index >= 0) {
                if (event->type() == QEvent::Enter) {
                    int prev = hoveredSlotIndex_;
                    hoveredSlotIndex_ = index;
                    if (prev >= 0 && prev != index) {
                        drawShopItem(prev);
                    }
                } else {
                    if (hoveredSlotIndex_ == index) {
                        hoveredSlotIndex_ = -1;
                    }
                }
                drawShopItem(index);
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ShopWidget::drawShopItem(int index) {
    if (!game_) return;
    
    const int slotCount = game_->getShop().getSlotCount();
    if (index < 0 || index >= slotCount) {
        return;
    }
    QPushButton *button = shopSlots_[index];
    if (!button) {
        return;
    }

    bool hovered = (index == hoveredSlotIndex_);
    const auto& shopUnits = game_->getShop().getUnits();
    
    if (index < static_cast<int>(shopUnits.size()) && shopUnits[index]) {
        auto unit = shopUnits[index];
        QString titleText = QString::fromStdString(unit->displayName());

        // 移除旧的名字标签
        QLabel *oldLabel = button->findChild<QLabel*>("nameLabel");
        if (oldLabel) delete oldLabel;

        if (hovered) {
            button->setText(QString("%1\n❤%2 ⚔%3")
                                .arg(titleText)
                                .arg(unit->getHP())
                                .arg(unit->getATK()));
        } else {
            button->setText("");
            // 用独立的 QLabel 在右上角显示名字，与图标左对齐互不干扰
            QLabel *nameLabel = new QLabel(titleText, button);
            nameLabel->setObjectName("nameLabel");
            nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            nameLabel->setStyleSheet(
                "QLabel { color: #ecf0f1; font-size: 10px; font-weight: bold; "
                "background: transparent; padding: 0px; }");
            nameLabel->adjustSize();
            nameLabel->move(button->width() - nameLabel->width() - 4, 2);
            nameLabel->show();
        }

        button->setToolTip(QString("%1\n★ %2 星\n❤ HP: %3\n⚔ ATK: %4\n💰 2 金币")
                                   .arg(titleText)
                                   .arg(unit->getStarLevel())
                                   .arg(unit->getHP())
                                   .arg(unit->getATK()));
        auto icon = getUnitPixmap(unit);
        if (!icon.isNull()) {
            button->setIcon(QIcon(icon));
            button->setIconSize(QSize(56, 56));
        } else {
            button->setIcon(QIcon());
        }
    } else {
        // 移除旧的名字标签
        QLabel *oldLabel = button->findChild<QLabel*>("nameLabel");
        if (oldLabel) delete oldLabel;
        button->setText("空槽");
        button->setToolTip("当前槽位为空");
        button->setIcon(QIcon());
    }
}

void ShopWidget::updateGoldDisplay() {
    if (game_) {
        const Player& player = game_->getPlayer();
        goldDisplayLabel_->setText(QString("💰 %1").arg(player.getGold()));
    }
}

void ShopWidget::onRefreshShop() {
    if (!game_) return;
    
    Shop& shop = game_->getShop();
    Player& player = game_->getPlayer();
    int currentRound = game_->getCurrentRound();
    
    if (shop.refresh(Owner::PlayerCtrl, currentRound, player, 1)) {
        actionStatusLabel_->setText("商店已刷新！");
    } else {
        actionStatusLabel_->setText("金币不足，无法刷新商店！");
    }
    
    updateDisplay();
}

void ShopWidget::doRefreshShop() {
    onRefreshShop();
}

void ShopWidget::doBuySelected(int index) {
    if (!game_) return;
    const int slotCount = game_->getShop().getSlotCount();
    if (index >= 0 && index < slotCount) {
        onUnitClicked(index);
        return;
    }
    // 默认找第一个有单位的槽位
    const auto& shopUnits = game_->getShop().getUnits();
    for (int i = 0; i < slotCount; ++i) {
        if (i < static_cast<int>(shopUnits.size()) && shopUnits[i]) {
            onUnitClicked(i);
            return;
        }
    }
    actionStatusLabel_->setText("商店没有可购买的单位！");
}

int ShopWidget::getSlotCount() const {
    return game_ ? game_->getShop().getSlotCount() : 5;
}

QPoint ShopWidget::getSlotScreenPos(int index) const {
    if (index < 0 || index >= static_cast<int>(shopSlots_.size()) || !shopSlots_[index]) {
        return QPoint();
    }
    // 将槽位按钮的全局坐标返回（按钮中心）
    QPushButton *btn = shopSlots_[index];
    QPoint center = btn->mapToGlobal(QPoint(btn->width() / 2, btn->height() / 2));
    return center;
}

void ShopWidget::onUnitClicked(int index) {
    if (!game_) return;
    hoveredSlotIndex_ = -1;  // 点击时清除悬停，避免弹窗后状态残留
    // 发射购买请求信号，由 MainWindow 处理确认弹窗和动画
    emit purchaseRequested(index);
}

void ShopWidget::executePurchase(int index) {
    if (!game_) return;
    
    Shop& shop = game_->getShop();
    Player& player = game_->getPlayer();
    
    if (shop.buyUnit(index, player, 2)) {
        actionStatusLabel_->setText("购买成功！");
    } else {
        actionStatusLabel_->setText("购买失败：金币不足、人口已满或备战区已满！");
    }
    
    updateDisplay();
}

void ShopWidget::onUpgradePopulation() {
    if (!game_) return;
    
    Player& player = game_->getPlayer();
    if (Shop::upgradePopulation(player, 5)) {
        actionStatusLabel_->setText("人口上限已提升！");
    } else {
        actionStatusLabel_->setText("金币不足，无法升级人口！");
    }
    
    updateDisplay();
}
// ===== P2-2: 装备合成（同名同星三合一） =====
void ShopWidget::onCombineEquipment() {
    if (!game_) return;
    Player& player = game_->getPlayer();
    
    // 按名称和星级分组：(name, star) -> 装备ID列表
    std::map<std::pair<std::string, int>, std::vector<int>> groups;
    
    // 1. 搜索暂存区装备
    auto stored = player.getStoredEquipments();
    for (auto* eq : stored) {
        if (eq->ownerUnitId == 0) {
            groups[{eq->name, eq->starLevel}].push_back(eq->id);
        }
    }
    
    // 2. 搜索备战区单位身上的装备
    auto& bench = player.getBench();
    for (int i = 0; i < BENCH_SIZE; ++i) {
        auto unit = bench.getUnit(i);
        if (!unit) continue;
        for (int s = 0; s < 4; ++s) {
            Equipment* eq = unit->GetEquipmentSlot(s);
            if (eq) {
                groups[{eq->name, eq->starLevel}].push_back(eq->id);
            }
        }
    }
    
    // 3. 搜索棋盘上玩家单位的装备
    auto& board = game_->getBoard();
    auto boardUnits = board.getAllUnits();
    for (auto& [x, y, unit] : boardUnits) {
        if (!unit || unit->getOwner() != Owner::PlayerCtrl) continue;
        for (int s = 0; s < 4; ++s) {
            Equipment* eq = unit->GetEquipmentSlot(s);
            if (eq) {
                groups[{eq->name, eq->starLevel}].push_back(eq->id);
            }
        }
    }
    
    // 找第一个有至少3件的组
    for (auto& [key, ids] : groups) {
        if (ids.size() >= 3) {
            std::string msg;
            if (Shop::combineEquipment(player, ids[0], ids[1], ids[2], msg)) {
                actionStatusLabel_->setText(QString::fromStdString(msg));
            } else {
                actionStatusLabel_->setText(QString::fromStdString(msg));
            }
            updateDisplay();
            return;
        }
    }
    actionStatusLabel_->setText("没有三件同名同星级的装备可合成");
    updateDisplay();
}

// ===== P2-3: 装备售卖 =====
void ShopWidget::onSellEquipment() {
    if (!game_) return;
    Player& player = game_->getPlayer();
    auto stored = player.getStoredEquipments();
    
    // 售卖第一件暂存装备
    for (auto* eq : stored) {
        if (eq->ownerUnitId == 0) {
            int price = Shop::sellEquipment(player, eq->id);
            if (price > 0) {
                actionStatusLabel_->setText(
                    QString::fromUtf8("售出 %1，获得 %2 金币")
                        .arg(QString::fromStdString(eq->name))
                        .arg(price));
            }
            updateDisplay();
            return;
        }
    }
    actionStatusLabel_->setText("没有可售卖的暂存装备");
    updateDisplay();
}

// ===== P0-1: 装备商店购买 =====
void ShopWidget::onBuyEquipSlot(int index) {
    if (!game_) return;
    auto& equipSlots = game_->getShop().getEquipmentSlots();
    if (index < 0 || index >= static_cast<int>(equipSlots.size())) return;
    if (equipSlots[index].sold) {
        actionStatusLabel_->setText("该装备已售出！");
        return;
    }
    Player& player = game_->getPlayer();
    int price = equipSlots[index].price;
    if (player.getGold() < price) {
        actionStatusLabel_->setText(QString("金币不足！需要 %1 金币").arg(price));
        return;
    }
    if (game_->getShop().buyEquipment(index, player)) {
        const auto& eq = equipSlots[index].eq;
        actionStatusLabel_->setText(
            QString::fromUtf8("购买了 %1 ！已存入暂存区")
                .arg(QString::fromStdString(eq.name)));
    }
    updateDisplay();
}

void ShopWidget::doBuyEquipment(int index) {
    onBuyEquipSlot(index);
}

void ShopWidget::onApplyEquipment() {
    if (!game_) return;
    
    Player& player = game_->getPlayer();
    auto& bench = player.getBench();
    auto stored = player.getStoredEquipments();
    
    if (stored.empty()) {
        actionStatusLabel_->setText("没有暂存装备可穿戴！");
        updateDisplay();
        return;
    }
    
    int equippedCount = 0;
    int failedCount = 0;
    
        // 遍历所有暂存装备，尝试装备到备战区单位
    for (auto* eq : stored) {
        if (eq->ownerUnitId != 0) continue; // 跳过已装备的
        
        int slot = eq->type;
        std::string restriction = eq->restriction;
        bool equipped = false;
        
        for (int i = 0; i < BENCH_SIZE && !equipped; ++i) {
            auto unit = bench.getUnit(i);
            if (!unit) continue;
            int actualSlot = slot;
            if ((slot == 2 || slot == 3) && unit->GetEquipmentSlot(2)) actualSlot = 3;
            if (unit->GetEquipmentSlot(actualSlot)) continue;
            if (!restriction.empty() && restriction != Unit::ClassToString(unit->GetClassType())) continue;
            if (unit->Equip(eq, actualSlot)) {
                // 使用 unitId 绑定装备
                player.equipToUnit(eq->id, unit->getUnitId());
                equipped = true;
                equippedCount++;
            }
        }
        
        if (!equipped) {
            failedCount++;
        }
    }
    
    // 更新显示
    auto remaining = player.getStoredEquipments();
    if (remaining.empty()) {
        droppedEquipLabel_->setText("暂存装备：无");
    } else {
        QStringList equipList;
        for (auto* eq : remaining) {
            equipList << QString::fromUtf8("【%1】%2 ATK+%3 HP+%4")
                .arg(QString::fromStdString(Unit::EquipmentTypeToString(eq->type)))
                .arg(QString::fromStdString(eq->name))
                .arg(eq->atkBonus)
                .arg(eq->hpBonus);
        }
        droppedEquipLabel_->setText(QString::fromUtf8("暂存装备(%1件)：\n%2")
            .arg(remaining.size())
            .arg(equipList.join("\n")));
    }
    
    std::ostringstream ss;
    ss << "装备完成：成功" << equippedCount << " 件，失败 " << failedCount << " 件（无合适目标或槽位已满）";
    actionStatusLabel_->setText(QString::fromStdString(ss.str()));
    updateDisplay();
}

void ShopWidget::onAutoCombine() {
    if (!game_) return;
    
    Player& player = game_->getPlayer();
    std::string message;
    Shop::autoCombine(player, message);
    actionStatusLabel_->setText(QString::fromStdString(message));
    
    updateDisplay();
}

void ShopWidget::updateEquipmentDisplay() {
    if (!droppedEquipLabel_ || !game_) return;
    
    Player& player = game_->getPlayer();
    auto stored = player.getStoredEquipments();
    
    // 清除旧的装备标签
    for (auto* btn : equipItemBtns_) {
        if (btn) {
            equipTab_->layout()->removeWidget(btn);
            delete btn;
        }
    }
    equipItemBtns_.clear();
    // 也清除旧的 QLabel（通过遍历子控件）
    QList<QLabel*> oldLabels = equipTab_->findChildren<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto* lbl : oldLabels) {
        if (lbl->property("equipId").isValid()) {
            equipTab_->layout()->removeWidget(lbl);
            delete lbl;
        }
    }
    
    if (stored.empty()) {
        droppedEquipLabel_->setText("暂存装备：无");
        applyEquipBtn_->setEnabled(false);
    } else {
        droppedEquipLabel_->setText(QString::fromUtf8("暂存装备(%1件) - 可拖拽到单位上装备：").arg(stored.size()));
        applyEquipBtn_->setEnabled(true);
        
        // P0-4: 为每件装备创建可拖拽按钮
        QVBoxLayout *equipLayout = qobject_cast<QVBoxLayout*>(equipTab_->layout());
        if (equipLayout) {
            // 找到 stretch 之前插入（在 droppedEquipLabel_ 之后）
            int insertIdx = equipLayout->indexOf(droppedEquipLabel_) + 1;
            for (auto* eq : stored) {
                QLabel *lbl = new QLabel();
                QString lblText = QString::fromUtf8("【%1】%2 ★%3\nATK+%4 HP+%5")
                    .arg(QString::fromStdString(Unit::EquipmentTypeToString(eq->type)))
                    .arg(QString::fromStdString(eq->name))
                    .arg(eq->starLevel)
                    .arg(eq->atkBonus).arg(eq->hpBonus);
                if (!eq->restriction.empty()) {
                    lblText += QString::fromUtf8("\n[%1限定]").arg(QString::fromStdString(eq->restriction));
                }
                lbl->setText(lblText);
                lbl->setMinimumHeight(40);
                lbl->setMaximumHeight(50);
                lbl->setStyleSheet(
                    "QLabel { background-color: #2c3e50; color: #ecf0f1; border: 2px solid #7f8c8d; "
                    "border-radius: 5px; font-size: 9px; padding: 4px; } "
                    "QLabel:hover { border: 2px solid #3498db; background-color: #34495e; }"
                );
                lbl->setMouseTracking(true);
                lbl->setProperty("equipId", eq->id);
                lbl->installEventFilter(this);
                
                equipItemBtns_.push_back(nullptr);  // 占位，保持 cleanup 兼容
                equipLayout->insertWidget(insertIdx++, lbl);
            }
        }
    }
}

void ShopWidget::loadAssets() {
    assetRootPath_ = findAssetsRoot();
    if (assetRootPath_.isEmpty()) {
        return;
    }
    pixmapCache_["ui/shop_background"] = loadPixmapFromAssets("ui/shop_background.svg");
    pixmapCache_["ui/shop_slot"] = loadPixmapFromAssets("ui/shop_slot.svg");
}

QString ShopWidget::findAssetsRoot() const {
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

QPixmap ShopWidget::loadPixmapFromAssets(const QString &relativePath) const {
    if (assetRootPath_.isEmpty()) {
        return QPixmap();
    }

    QString fullPath = QDir(assetRootPath_).filePath(relativePath);
    if (!QFileInfo::exists(fullPath)) {
        return QPixmap();
    }

    QPixmap pixmap(fullPath);
    return pixmap.isNull() ? QPixmap() : pixmap;
}

QPixmap ShopWidget::getUnitPixmap(std::shared_ptr<Unit> unit) {
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
        pixmap = loadPixmapFromAssets("units/default.svg");
    }

    pixmapCache_[cacheKey] = pixmap;
    return pixmap;
}
