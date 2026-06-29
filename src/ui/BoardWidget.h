/**
 * @file    BoardWidget.h
 * @brief   棋盘渲染组件，处理格子绘制、单位拖拽与战斗动画
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QRect>
#include <QPixmap>
#include <QSoundEffect>
#include <QMimeData>
#include <memory>
#include <optional>
#include <map>
#include <QScrollArea>
#include <QLabel>
#include <QPointF>
#include <QPushButton>
#include "../core/Commondata.h"

class Game;
class Unit;

/**
 * @brief 单位移动动画数据
 */
struct UnitMoveAnim {
    int fromX, fromY;
    int toX, toY;
    float progress;
    bool active;
    std::shared_ptr<Unit> unit;
    
    UnitMoveAnim() : fromX(-1), fromY(-1), toX(-1), toY(-1), progress(0.0f), active(false) {}
};

/**
 * @brief 伤害数字动画数据
 */
struct DamageAnim {
    QPointF pixelPos;
    int damage;
    float progress;
    bool active;
    bool isCrit;
    
    DamageAnim() : damage(0), progress(0.0f), active(false), isCrit(false) {}
};

/**
 * @brief 死亡动画数据
 */
struct DeathAnim {
    QPointF pixelPos;
    float progress;
    bool active;
    
    DeathAnim() : progress(0.0f), active(false) {}
};

/**
 * @class   BoardWidget
 * @brief   棋盘与备战区的可视化组件，处理渲染、拖拽交互与战斗动画
 */
class BoardWidget : public QWidget {
    Q_OBJECT

public:
    BoardWidget(std::shared_ptr<Game> game, QWidget *parent = nullptr);
    ~BoardWidget();

    void updateDisplay();
    
    // ========== 动画接口（由 MainWindow 的战斗循环调用） ==========
    void startUnitMoveAnim(int fromX, int fromY, int toX, int toY);
    void startDamageAnim(int x, int y, int damage, bool isCrit = false);
    void startDeathAnim(int x, int y);
    void startEquipDropAnim();  ///< 装备掉落闪烁动画
    void startUltimateOverlay(const QString& text, const QString& charType, int unitX, int unitY);
    void clearAnimations();
    void updateAnimations(float deltaTime);
    bool hasActiveAnimations() const;

    ///< 获取当前激活的羁绊信息
    const std::vector<QString>& getActiveTraits() const { return activeTraits_; }

signals:
    ///< 合成按钮点击信号，由 MainWindow 连接到 ShopWidget
    void combineRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // ========== 绘制方法 ==========
    void drawBoard(QPainter *painter);
    void drawBench(QPainter *painter);
    void drawUnits(QPainter *painter);
    void drawUnit(QPainter *painter, int x, int y, std::shared_ptr<Unit> unit);
    void drawUnitAtPosition(QPainter *painter, QPointF pos, std::shared_ptr<Unit> unit);
    void drawHealthBar(QPainter *painter, const QRect &cellRect, std::shared_ptr<Unit> unit);
    void drawManaBar(QPainter *painter, const QRect &cellRect, std::shared_ptr<Unit> unit);
    void drawUnitInfo(QPainter *painter, const QRect &cellRect, std::shared_ptr<Unit> unit);
    void drawInfoPanel(QPainter *painter);

    // ========== 坐标转换 ==========
    QRect getCellRect(int x, int y) const;
    QPointF getCellCenter(int x, int y) const;
    std::pair<int, int> screenToBoard(int sx, int sy) const;
    std::pair<int, int> screenToBench(int sx, int sy) const;
    
    ///< 检查装备拖放到光标位置的目标单位是否可装备（含职业/槽位校验）
    bool canEquipAtCursor(QPoint pos, const QMimeData *mime) const;
    // ========== 交互方法 ==========
    void startDrag(int fromX, int fromY, bool isBench = false);
    void handleDrop(int toX, int toY, bool toBench = false);
    void handleIllegalPlacement(int fromX, int fromY, int toX, int toY);
    void handleKeyboardDrop();

    // ========== 资产加载 ==========
    void loadAssets();
    QString findAssetsRoot() const;
    QPixmap loadPixmapFromAssets(const QString &relativePath) const;
    QPixmap getUnitPixmap(std::shared_ptr<Unit> unit);
    QRect getBenchSlotRect(int index) const;
    void drawBenchUnit(QPainter *painter, int index, std::shared_ptr<Unit> unit);
    
    // ========== 动画绘制方法 ==========
    void drawMoveAnimations(QPainter *painter);
    void drawDamageAnimations(QPainter *painter);
    void drawDeathAnimations(QPainter *painter);

    // ========== 游戏指针 ==========
    std::shared_ptr<Game> game_;

    // ========== UI 布局参数（动态计算） ==========
    static constexpr int BASE_CELL_SIZE = BOARD_BASE_CELL_SIZE;  ///< 基础参考尺寸
    static constexpr int BASE_BOARD_MARGIN = BOARD_BASE_MARGIN;
    static constexpr int BENCH_MARGIN = BOARD_BENCH_MARGIN;  ///< 备战区与棋盘之间的间距
    
    // ========== 动态计算的方法 ==========
    int getCellSize() const;
    int getBoardMargin() const;
    int getBenchHeight() const;
    
    ///< 重新计算所有尺寸（在 resizeEvent 中调用）
    void recalcLayout();

    // ========== 拖拽状态 ==========
    int dragStartX_ = -1;
    int dragStartY_ = -1;
    bool dragFromBench_ = false;
    std::shared_ptr<Unit> draggedUnit_;
    bool isDragging_ = false;           ///< 是否正在拖拽
    QPoint dragCurrentPos_;             ///< 拖拽时的鼠标当前位置
    int dropPreviewX_ = -1;             ///< 放置预览的格子 X
    int dropPreviewY_ = -1;             ///< 放置预览的格子 Y
    bool dropPreviewInBench_ = false;   ///< 放置预览是否在备战区
    bool dropPreviewValid_ = false;     ///< 放置预览是否有效

    // ========== 拖拽绘制方法 ==========
    void drawDragGhost(QPainter *painter);
    void drawPlacementPreview(QPainter *painter);
    void drawDropZoneBoundaries(QPainter *painter);
    bool isValidDropTarget(int x, int y, bool isBench) const;

    // ========== 资产资源缓存 ==========
    QString assetRootPath_;
    std::map<QString, QPixmap> pixmapCache_;
    std::vector<std::unique_ptr<QSoundEffect>> soundEffect_;

    // 合成按钮
    QPushButton *combineBtn_ = nullptr;
    // ========== 倍速按钮 ==========
    QPushButton *speedBtn_ = nullptr;

    ///< 当前激活的羁绊/光环字符串（在 updateDisplay 中更新）
    std::vector<QString> activeTraits_;

    ///< 信息面板中装备卸载按钮区域（rect + 槽位号）
    std::vector<std::pair<QRect, int>> infoEquipRects_;

    // ========== 高亮状态 ==========
    int highlightX_ = -1;
    int highlightY_ = -1;
    bool highlightInBench_ = false;
    
    // ========== 键盘选中状态 ==========
    int selX_ = -1;
    int selY_ = -1;
    bool selInBench_ = false;
    bool hasSelection_ = false;
    
    // ========== 动画数据 ==========
    std::vector<UnitMoveAnim> moveAnims_;
    std::vector<DamageAnim> damageAnims_;
    std::vector<DeathAnim> deathAnims_;
    float equipDropAlpha_ = 0.0f;  ///< 装备掉落闪烁透明度
    QString ultimateOverlayText_;
    float ultimateOverlayAlpha_ = 0.0f;
    float ultiSvgDelay_ = 0.0f;            ///< SVG 延迟显示计时器
    QString ultiSvgPath_;
    QPointF ultiSvgPos_;
    
};