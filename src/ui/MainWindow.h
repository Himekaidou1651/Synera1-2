/**
 * @file    MainWindow.h
 * @brief   主窗口，管理菜单导航、游戏区布局与所有子组件交互
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QWidget>
#include <QResizeEvent>
#include <QProgressBar>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPalette>
#include <QColor>
#include <QImage>
#include <QSlider>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSvgRenderer>
#include <QTimer>
#include <QListWidget>
#include <QScrollArea>
#include <QTextEdit>
#include <QDialog>
#include <QTableWidget>
#include <memory>

// 前向声明
class Game;
class BoardWidget;
class ShopWidget;
class ShopWindow;
class MusicPlayerWidget;
class BattleLogWidget;
class StatsWidget;

/**
 * @class   MainWindow
 * @brief   应用程序主窗口，协调游戏逻辑与 UI 交互
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initialize();
    void updateDisplay();

    // ========== 音量/亮度查询 ==========
    int getSfxVolume() const { return sfxVolume_; }
    int getBrightness() const { return brightness_; }

private slots:
    void onStartGame();
    void onStartRound();
    void onCombatStep();
    void onAnimationUpdate();
    void onGameTick();
    void onSaveGame();
    void onLoadGame();
    void onNewGame();
    void onSettings();
    void onHelp();
    void onAbout();
    void onExitApp();
    void onBackToMenu();
    void onBackToMenuFromGame();
    void onMusicVolumeChanged(int value);
    void onSfxVolumeChanged(int value);
    void onBrightnessChanged(int value);
    void onToggleMusicPlayer();
    void onToggleBattleLog();
    void onToggleStatsReport();
    void onPurchaseUnit(int index);  ///< 处理购买请求（含确认弹窗）
    void onOpenShop();               ///< 打开商店弹窗

    // ========== StartWindow 动画槽函数 ==========
    void onStartFadeInFinished();
    void onStartGlowUpdate();
    void onStartAutoAdvance();
    void onStartSkipClicked();

    // ========== 加载游戏页面 ==========
    void onLoadGameSlot();
    void onLoadGameListClicked();
    void onLoadGameBack();
    void onDeleteSave();

private:
    void setupUI();
    void setupConnections();
    void setupStartScreen();
    void setupStartAnimations();
    void loadStartBackground();
    void drawStartBackground(QPainter &painter);
    void startIntroAnimation();
    void updatePlayerInfo();
    void updateGameState();
    void showMainMenu();
    void showGameArea();
    void showSettings();
    void showHelp();
    void showAbout();
    void showLoadGame();
    void loadBackgrounds();
    QWidget* createBgWidget(const QPixmap& bg, QWidget* parent);
    void paintBgForWidget(QWidget* w, const QPixmap& bg);
    void applyBrightness(int value);
    void refreshSaveList();
    void updateCombatPhaseLabel();
    void updatePhaseDisplay();
    void showCombatStartTransition();
    void showCombatEndTransition();
    void updateMusicPlayerPosition();
    void repositionMusicControls();
    void updateTraitsPanel();  ///< 更新羁绊面板
    void updateCombatLog();    ///< 更新战斗日志
    void updateStatsReport();  ///< 更新统计报表
    void animateGoldChange(int oldValue, int newValue);  ///< 金币计数动画
    void startPurchaseFlyAnimation(QPoint fromPos);       ///< 购买飞行动画
    void updateStreakDisplay(); ///< 更新连胜/连败显示
    void showRoundSettlement(); ///< 显示回合结算弹窗
    void showGameOverDialog();  ///< 显示游戏结束结算弹窗

    // ========== 全屏切换 ==========
    void toggleFullScreen();

    // ========== 核心游戏逻辑 ==========
    std::shared_ptr<Game> game_;

    // ========== 主要 UI 组件 ==========
    QWidget *centralWidget_ = nullptr;
    QStackedWidget *centralStack_ = nullptr;
    QWidget *gameAreaWidget_ = nullptr;
    QWidget *mainMenuWidget_ = nullptr;
    QWidget *settingsWidget_ = nullptr;
    QWidget *helpWidget_ = nullptr;
    QWidget *aboutWidget_ = nullptr;
    QWidget *loadGameWidget_ = nullptr;  ///< 加载游戏页面

    // ===== 开场动画相关 =====
    QWidget *startScreenWidget_ = nullptr;   ///< 开场画面 widget
    QLabel *startTitleLabel_ = nullptr;      ///< 标题文字
    QLabel *startSubtitleLabel_ = nullptr;   ///< 副标题/装饰文字
    QLabel *startHintLabel_ = nullptr;       ///< 提示文字 "点击任意处继续..."
    QPushButton *startSkipBtn_ = nullptr;    ///< 跳过按钮
    QPixmap startBgCache_;                   ///< SVG 渲染后的背景缓存
    bool startBgLoaded_ = false;

    // ========== 开场动画效果 ==========
    QGraphicsOpacityEffect *startTitleOpacity_ = nullptr;
    QPropertyAnimation *startTitleFadeIn_ = nullptr;
    QGraphicsOpacityEffect *startSubtitleOpacity_ = nullptr;
    QPropertyAnimation *startSubtitleFadeIn_ = nullptr;
    QGraphicsOpacityEffect *startHintOpacity_ = nullptr;
    QPropertyAnimation *startHintFadeIn_ = nullptr;

    // ========== 发光动画（定时器驱动） ==========
    QTimer *startGlowTimer_ = nullptr;
    qreal startGlowIntensity_ = 0.0;         ///< 当前发光强度 0.0~1.0
    bool startGlowIncreasing_ = true;        ///< 发光强度是否在增加

    // ========== 自动跳转计时器 ==========
    QTimer *startAutoAdvanceTimer_ = nullptr;

    // ========== 动画状态 ==========
    bool startAnimationStarted_ = false;
    bool startIsTransitioning_ = false;

    // ========== 主菜单按钮 ==========
    QPushButton *newGameBtn_;
    QPushButton *loadGameBtn_;
    QPushButton *settingsBtn_;
    QPushButton *helpBtn_;
    QPushButton *aboutBtn_;
    QPushButton *exitBtn_;

    // ========== 设置/帮助/关于界面的返回按钮 ==========
    QPushButton *settingsBackBtn_;
    QPushButton *helpBackBtn_;
    QPushButton *aboutBackBtn_;

    // ========== 加载游戏页面组件 ==========
    QListWidget *saveListWidget_ = nullptr;
    QPushButton *loadGameBackBtn_ = nullptr;
    QPushButton *loadGameConfirmBtn_ = nullptr;
    QPushButton *loadGameDeleteBtn_ = nullptr;
    QLabel *loadGameTitleLabel_ = nullptr;

    // ========== 设置界面滑动条 ==========
    QSlider *musicVolumeSlider_ = nullptr;
    QSlider *sfxVolumeSlider_ = nullptr;
    QSlider *brightnessSlider_ = nullptr;
    QLabel *musicVolumeValueLabel_ = nullptr;
    QLabel *sfxVolumeValueLabel_ = nullptr;
    QLabel *brightnessValueLabel_ = nullptr;

    // ========== 背景图 ==========
    QPixmap bgMainMenu_;
    QPixmap bgSettings_;
    QPixmap bgHelp_;
    QPixmap bgAbout_;
    QPixmap bgLoadGame_;

    // ========== 左侧：棋盘显示 ==========
    BoardWidget *boardWidget_ = nullptr;

    // ========== 右侧：信息面板 ==========
    QWidget *infoPanel_ = nullptr;

    // ========== 商店弹窗 ==========
    ShopWindow *shopWindow_ = nullptr;

    // ========== 商店按钮（替换原商店区域） ==========
    QPushButton *shopBtn_ = nullptr;

    // ========== 玩家信息显示 ==========
    QLabel *healthLabel_ = nullptr;
    QLabel *goldLabel_ = nullptr;
    QLabel *levelLabel_ = nullptr;
    QProgressBar *xpBar_ = nullptr;
    QLabel *xpLevelLabel_ = nullptr;
    QLabel *populationLabel_ = nullptr;
    QLabel *enemyHealthLabel_ = nullptr;
    QLabel *roundLabel_ = nullptr;
    QLabel *phaseLabel_ = nullptr;
    // ========== 资源图标（SVG 替代 Unicode） ==========
    QLabel *healthTitleImg_ = nullptr;
    QLabel *goldTitleImg_ = nullptr;
    QLabel *levelTitleImg_ = nullptr;
    QLabel *popTitleImg_ = nullptr;

    // ========== 控制按钮 ==========
    QPushButton *startRoundBtn_ = nullptr;
    QPushButton *saveBtn_ = nullptr;
    QPushButton *inExitBtn_ = nullptr;

    // ========== 战斗日志（浮动面板） ==========
    BattleLogWidget *battleLogWidget_ = nullptr;
    QPushButton *logToggleBtn_ = nullptr;
    
    // ========== 统计报表（浮动面板） ==========
    StatsWidget *statsWidget_ = nullptr;
    QPushButton *statsToggleBtn_ = nullptr;
    
    // ========== 连胜/连败显示 ==========
    QLabel *streakLabel_ = nullptr;

    // ========== 布局 ==========
    QHBoxLayout *mainLayout_ = nullptr;
    QVBoxLayout *rightLayout_ = nullptr;
    QVBoxLayout *infoPanelLayout_ = nullptr;

    // ========== 游戏计时器 ==========
    int gameTickCounter_;

    // ========== 分步战斗定时器 ==========
    QTimer *combatTimer_ = nullptr;
    int combatAnimSkipCounter_ = 0;
    bool combatEndPending_ = false;       ///< 战斗已结束但等大招特效播完
    bool gameOverDialogShown_ = false;   ///< 游戏结束弹窗已弹出，防止重复
    
    // ========== 动画更新定时器 ==========
    QTimer *animTimer_ = nullptr;
    
    // ========== 战斗动画状态 ==========
    enum class CombatAnimPhase {
        None,           // 无动画
        UnitMove,       // 单位移动动画阶段
        DamageDisplay,  // 伤害数字显示阶段
        DeathDisplay,   // 死亡效果显示阶段
        WaitForNext     // 等待下一步
    };
    CombatAnimPhase currentAnimPhase_ = CombatAnimPhase::None;
    float animPhaseTimer_ = 0.0f;

    // ========== 音量/亮度状态 ==========
    int musicVolume_ = 65;
    int sfxVolume_ = 80;
    int brightness_ = 100;
    
    // ========== 快速购买（跳过确认弹窗） ==========
    bool quickBuyEnabled_ = true;
    int lastDisplayedGold_ = 0;  ///< 用于金币计数动画

    // ========== 亮度遮罩层 ==========
    QWidget *brightnessOverlay_ = nullptr;
    QGraphicsOpacityEffect *brightnessEffect_ = nullptr;

    // ========== 音乐播放器悬浮窗 ==========
    MusicPlayerWidget *musicPlayerWidget_ = nullptr;
    QPushButton *musicToggleBtn_ = nullptr;

    // ========== 羁绊面板（独立于棋盘，显示在右侧信息区） ==========
    QScrollArea *traitsScrollArea_ = nullptr;
    QLabel *traitsContentLabel_ = nullptr;

    // ========== 音乐按钮拖拽状态 ==========
    bool musicToggleDragging_ = false;
    QPoint musicToggleDragStart_;
    bool musicControlsPinned_ = false;  ///< 是否固定到右下角（全屏时）

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};