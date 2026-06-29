/**
 * @file    StartWindow.h
 * @brief   动态标题页窗口，显示开场动画与粒子特效
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QSvgRenderer>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QPointF>
#include <QVector>

/**
 * @brief 动态标题页窗口
 * 
 * 在进入主菜单之前显示，配有 bg_door.svg 背景动画、粒子效果和标题 "Synera"
 */
class StartWindow : public QWidget {
    Q_OBJECT

public:
    explicit StartWindow(QWidget *parent = nullptr);
    ~StartWindow();

    /**
     * @brief 开始播放入场动画
     */
    void startIntroAnimation();

    /**
     * @brief 设置动画播放完毕后的回调
     */
    void setOnFinished(std::function<void()> callback);

signals:
    /**
     * @brief 标题页动画完成，可以切换到主窗口
     */
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onFadeInFinished();
    void onGlowUpdate();
    void onAutoAdvance();

private:
    void setupUI();
    void setupAnimations();
    void loadBackground();
    void drawBackground(QPainter &painter);

    // ========== UI 组件 ==========
    QLabel *titleLabel_;        ///< 标题文字 "Synera"
    QLabel *subtitleLabel_;     ///< 副标题/装饰文字
    QLabel *hintLabel_;         ///< 提示文字 "点击任意处继续..."
    QPushButton *skipBtn_;      ///< 跳过按钮

    // 背景渲染
    QPixmap bgCache_;            ///< SVG 渲染后的背景缓存
    bool bgLoaded_;

    // 动画效果
    QGraphicsOpacityEffect *titleOpacity_;
    QPropertyAnimation *titleFadeIn_;
    QGraphicsOpacityEffect *subtitleOpacity_;
    QPropertyAnimation *subtitleFadeIn_;
    QGraphicsOpacityEffect *hintOpacity_;
    QPropertyAnimation *hintFadeIn_;
    QGraphicsOpacityEffect *bgOpacity_;

    // 发光动画（定时器驱动）
    QTimer *glowTimer_;
    qreal glowIntensity_;
    bool glowIncreasing_;

    // 粒子系统
    struct Particle {
        QPointF pos;
        QPointF velocity;
        qreal size;
        qreal opacity;
        QColor color;
    };
    QVector<Particle> particles_;
    QTimer *particleTimer_;
    void initParticles();
    void updateParticles();
    void drawParticles(QPainter &painter);

    // 自动跳转计时器
    QTimer *autoAdvanceTimer_;

    // 回调
    std::function<void()> onFinishedCallback_;

    // 动画状态
    bool animationStarted_;
    bool isTransitioning_;
};