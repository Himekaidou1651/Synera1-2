/**
 * @file    ShopWidget.h
 * @brief   商店界面组件，提供单位购买、装备购买/合成/售卖及招募管理
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <memory>
#include <vector>
#include <map>

class Game;
class Unit;
struct Equipment;
class QPaintEvent;
class QEvent;

/**
 * @class   ShopWidget
 * @brief   商店界面组件，包含招募、装备和图鉴三个标签页
 */
class ShopWidget : public QWidget {
    Q_OBJECT

public:
    ShopWidget(std::shared_ptr<Game> game, QWidget *parent = nullptr);
    ~ShopWidget();

    void updateDisplay();

    // ========== 键盘快捷键公共接口 ==========
    void doRefreshShop();       ///< R键：刷新商店
    void doBuySelected(int index = -1); ///< B键：购买单位（默认购买第一个可购买单位）
    void doBuyEquipment(int index = 0);  ///< E键：购买装备
    int getSlotCount() const;   ///< 获取槽位数
    QPoint getSlotScreenPos(int index) const;  ///< 获取槽位在屏幕上的位置（用于飞行动画）

signals:
    void purchaseRequested(int index);  ///< 购买请求信号，由 MainWindow 处理确认

public slots:
    void onAutoCombine();
    void executePurchase(int index);    ///< 由 MainWindow 确认后调用执行购买

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onRefreshShop();
    void onUnitClicked(int index);
    void onUpgradePopulation();
    void onBuyEquipSlot(int index);
    void onApplyEquipment();
    void onCombineEquipment();   ///< 装备合成
    void onSellEquipment();      ///< 装备售卖

private:
    void setupUI();
    void loadAssets();
    QString findAssetsRoot() const;
    QPixmap loadPixmapFromAssets(const QString &relativePath) const;
    QPixmap getUnitPixmap(std::shared_ptr<Unit> unit);
    void drawShopItem(int index);
    void updateGoldDisplay();
    void updateEquipmentDisplay();
    void refreshShop();

    std::shared_ptr<Game> game_;

    // 标签页
    QTabWidget *tabWidget_;
    QWidget *recruitTab_;
    QWidget *equipTab_;
    QWidget *collectionTab_;  ///< 装备图鉴

    // UI 组件 - 招募页
    QLabel *shopTitleLabel_;
    QLabel *goldDisplayLabel_;
    QPushButton *refreshBtn_;
    QPushButton *upgradePopBtn_;
    std::vector<QPushButton *> shopSlots_;
    std::vector<QPushButton *> equipBuySlots_;  ///< 装备购买按钮
    std::vector<QPushButton *> equipItemBtns_;  ///< 可拖拽的装备物品控件
    QGridLayout *shopLayout_;

    // UI 组件 - 装备页
    QPushButton *applyEquipBtn_;
    QLabel *droppedEquipLabel_;
    QPushButton *combineInEquipBtn_;

    // 通用
    QLabel *actionStatusLabel_;

    QString assetRootPath_;
    std::map<QString, QPixmap> pixmapCache_;

    // 当前选中要装备的装备（用于装备单位操作）
    std::unique_ptr<Equipment> pendingEquipment_;

    // 当前鼠标悬停的槽位索引（-1 表示无悬停）
    int hoveredSlotIndex_ = -1;
};
