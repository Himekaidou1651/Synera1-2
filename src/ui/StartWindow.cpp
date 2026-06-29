/**
 * @file    StartWindow.cpp
 * @brief   动态标题页窗口实现
 * @author  
 * @date    2026-06-24
 */

#include "StartWindow.h"
#include "../core/Commondata.h"
#include <QApplication>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QPainterPath>
#include <QFontDatabase>
#include <QtMath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QParallelAnimationGroup>

// 将SVG直接渲染到QPixmap（背景填充黑色，SVG保持宽高比居中）
static QPixmap renderSvgToPixmap(const QString &svgPath, const QSize &size) {
    QSvgRenderer renderer(svgPath);
    if (!renderer.isValid()) return QPixmap();

    QPixmap pixmap(size);
    pixmap.fill(Qt::black);  // 用黑色填充背景，SVG渲染在上面
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 保持宽高比缩放并居中
    QRectF viewBox = renderer.viewBoxF();
    if (viewBox.isEmpty()) viewBox = QRectF(0, 0, WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);

    QSizeF scaledSize = viewBox.size().scaled(size, Qt::KeepAspectRatioByExpanding);
    QRectF targetRect(QPointF(0, 0), scaledSize);
    targetRect.moveCenter(QRectF(QPointF(0, 0), size).center());

    renderer.render(&painter, targetRect);
    painter.end();
    return pixmap;
}

StartWindow::StartWindow(QWidget *parent)
    : QWidget(parent)
    , bgLoaded_(false)
    , glowIntensity_(0.0)
    , glowIncreasing_(true)
    , animationStarted_(false)
    , isTransitioning_(false)
{
    setWindowTitle("Synera: Synergy Auto-Arena");
    setFixedSize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
    setMouseTracking(true);

    // 设置窗口背景为黑色（过渡时使用）
    setStyleSheet("StartWindow { background-color: #000000; }");

    setupUI();
    loadBackground();

    // 初始化动画
    setupAnimations();

    // 初始化发光动画定时器
    glowTimer_ = new QTimer(this);
    glowTimer_->setInterval(TIMER_GLOW_MS); // ~30fps 的发光动画
    connect(glowTimer_, &QTimer::timeout, this, &StartWindow::onGlowUpdate);

    // 自动跳转定时器（5秒后如果没有操作自动跳转）
    autoAdvanceTimer_ = new QTimer(this);
    autoAdvanceTimer_->setSingleShot(true);
    autoAdvanceTimer_->setInterval(TIMER_AUTO_ADVANCE_MS);
    connect(autoAdvanceTimer_, &QTimer::timeout, this, &StartWindow::onAutoAdvance);
    
    // 粒子动画定时器
    particleTimer_ = new QTimer(this);
    particleTimer_->setInterval(TIMER_ANIM_MS); // ~30fps
    connect(particleTimer_, &QTimer::timeout, this, [this]() {
        updateParticles();
        update();
    });
}

StartWindow::~StartWindow() {
}

void StartWindow::setupUI() {
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(START_MARGIN_LR, START_MARGIN_TB, START_MARGIN_LR, START_MARGIN_B);

    // 顶部弹性空间
    mainLayout->addStretch(2);

    // ===== 标题 "xxx" =====
    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(8);
    titleLayout->setAlignment(Qt::AlignCenter);

    titleLabel_ = new QLabel("Synera", this);
    titleLabel_->setAlignment(Qt::AlignCenter);
    titleLabel_->setStyleSheet(
        "QLabel {"
        "   font-size: 72px;"
        "   font-weight: bold;"
        "   color: #ffffff;"
        "   letter-spacing: 12px;"
        "   background: transparent;"
        "}"
    );
    titleOpacity_ = new QGraphicsOpacityEffect(titleLabel_);
    titleOpacity_->setOpacity(0.0);
    titleLabel_->setGraphicsEffect(titleOpacity_);
    titleLayout->addWidget(titleLabel_);

    // ===== 副标题/装饰文字 =====
    subtitleLabel_ = new QLabel("✦ Synergy Auto-Arena ✦", this);
    subtitleLabel_->setAlignment(Qt::AlignCenter);
    subtitleLabel_->setStyleSheet(
        "QLabel {"
        "   font-size: 20px;"
        "   color: rgba(255, 255, 255, 0.7);"
        "   letter-spacing: 6px;"
        "   background: transparent;"
        "   margin-top: 4px;"
        "}"
    );
    subtitleOpacity_ = new QGraphicsOpacityEffect(subtitleLabel_);
    subtitleOpacity_->setOpacity(0.0);
    subtitleLabel_->setGraphicsEffect(subtitleOpacity_);
    titleLayout->addWidget(subtitleLabel_);

    mainLayout->addLayout(titleLayout);

    // 中间弹性空间
    mainLayout->addStretch(3);

    // ===== 底部提示文字 =====
    hintLabel_ = new QLabel("点击任意处进入游戏...", this);
    hintLabel_->setAlignment(Qt::AlignCenter);
    hintLabel_->setStyleSheet(
        "QLabel {"
        "   font-size: 14px;"
        "   color: rgba(255, 255, 255, 0.5);"
        "   background: transparent;"
        "   margin-bottom: 20px;"
        "}"
    );
    hintOpacity_ = new QGraphicsOpacityEffect(hintLabel_);
    hintOpacity_->setOpacity(0.0);
    hintLabel_->setGraphicsEffect(hintOpacity_);
    mainLayout->addWidget(hintLabel_);

    // ===== 跳过按钮 =====
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, BOTTOM_MARGIN_R, BOTTOM_MARGIN_B);
    bottomLayout->addStretch();

    skipBtn_ = new QPushButton("跳过 ▶", this);
    skipBtn_->setFixedSize(SKIPBTN_WIDTH, SKIPBTN_HEIGHT);
    skipBtn_->setCursor(Qt::PointingHandCursor);
    skipBtn_->setFocusPolicy(Qt::TabFocus);
    skipBtn_->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(255, 255, 255, 0.1);"
        "   color: rgba(255, 255, 255, 0.6);"
        "   border: 1px solid rgba(255, 255, 255, 0.2);"
        "   border-radius: 16px;"
        "   font-size: 12px;"
        "   padding: 4px 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(255, 255, 255, 0.2);"
        "   color: rgba(255, 255, 255, 0.9);"
        "   border: 1px solid rgba(255, 255, 255, 0.4);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(255, 255, 255, 0.3);"
        "}"
    );
    connect(skipBtn_, &QPushButton::clicked, this, &StartWindow::onAutoAdvance);
    bottomLayout->addWidget(skipBtn_);

    mainLayout->addLayout(bottomLayout);

    // 设置整个窗口鼠标点击事件
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    
    // 初始化粒子
    initParticles();
}

void StartWindow::setupAnimations() {
    // ===== 背景淡入 =====
    bgOpacity_ = new QGraphicsOpacityEffect(this);
    bgOpacity_->setOpacity(0.0);
    // 注意: 背景通过paintEvent绘制，不使用此效果

    // ===== 标题淡入动画 (0.5s延迟, 1.5s持续) =====
    titleFadeIn_ = new QPropertyAnimation(titleOpacity_, "opacity", this);
    titleFadeIn_->setDuration(TITLE_FADEIN_DURATION_MS);
    titleFadeIn_->setStartValue(0.0);
    titleFadeIn_->setEndValue(1.0);
    titleFadeIn_->setEasingCurve(QEasingCurve::OutCubic);

    // ===== 副标题淡入动画 (1.2s延迟, 1.0s持续) =====
    subtitleFadeIn_ = new QPropertyAnimation(subtitleOpacity_, "opacity", this);
    subtitleFadeIn_->setDuration(SUBTITLE_FADEIN_DURATION_MS);
    subtitleFadeIn_->setStartValue(0.0);
    subtitleFadeIn_->setEndValue(1.0);
    subtitleFadeIn_->setEasingCurve(QEasingCurve::OutCubic);

    // ===== 提示文字淡入动画 (2.5s延迟, 1.0s持续) =====
    hintFadeIn_ = new QPropertyAnimation(hintOpacity_, "opacity", this);
    hintFadeIn_->setDuration(HINT_FADEIN_DURATION_MS);
    hintFadeIn_->setStartValue(0.0);
    hintFadeIn_->setEndValue(1.0);
    hintFadeIn_->setEasingCurve(QEasingCurve::OutCubic);

    // 连接动画完成信号
    connect(titleFadeIn_, &QPropertyAnimation::finished, this, &StartWindow::onFadeInFinished);
}

void StartWindow::loadBackground() {
    // 尝试加载 bg_door.svg
    QDir dir(QCoreApplication::applicationDirPath());
    QString svgPath;
    for (int i = 0; i < 4; ++i) {
        QString candidate = dir.filePath("assets/ui/bg_door.svg");
        if (QFile::exists(candidate)) {
            svgPath = candidate;
            break;
        }
        dir.cdUp();
    }

    if (!svgPath.isEmpty()) {
        // 直接渲染SVG到缓存图像
        bgCache_ = renderSvgToPixmap(svgPath, size());
        if (!bgCache_.isNull()) {
            bgLoaded_ = true;
        }
    }

    // 如果加载失败，创建纯色渐变背景
    if (!bgLoaded_) {
        bgCache_ = QPixmap(size());
        bgCache_.fill(QColor("#0a0a1a"));
        QPainter painter(&bgCache_);
        QLinearGradient gradient(0, 0, width(), height());
        gradient.setColorAt(0.0, QColor("#0f0c29"));
        gradient.setColorAt(0.5, QColor("#302b63"));
        gradient.setColorAt(1.0, QColor("#24243e"));
        painter.fillRect(rect(), gradient);
        painter.end();
    }
}

void StartWindow::drawBackground(QPainter &painter) {
    // 直接绘制缓存的背景图像
    if (!bgCache_.isNull()) {
        painter.drawPixmap(0, 0, bgCache_);
    }
}

void StartWindow::startIntroAnimation() {
    if (animationStarted_) return;
    animationStarted_ = true;

    // 1. 背景淡入（立即开始，持续1.5秒）
    QPropertyAnimation *bgFadeIn = new QPropertyAnimation(bgOpacity_, "opacity", this);
    bgFadeIn->setDuration(TITLE_FADEIN_DURATION_MS);
    bgFadeIn->setStartValue(0.0);
    bgFadeIn->setEndValue(1.0);
    bgFadeIn->setEasingCurve(QEasingCurve::OutCubic);
    bgFadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    // 2. 标题淡入（延迟0.5秒）
    QTimer::singleShot(INTRO_TITLE_DELAY_MS, this, [this]() {
        titleFadeIn_->start();
    });

    // 3. 副标题淡入（延迟1.2秒）
    QTimer::singleShot(INTRO_SUBTITLE_DELAY_MS, this, [this]() {
        subtitleFadeIn_->start();
    });

    // 4. 开始发光动画（延迟2.0秒）
    QTimer::singleShot(INTRO_GLOW_DELAY_MS, this, [this]() {
        glowTimer_->start();
    });

    // 5. 提示文字淡入（延迟2.5秒）
    QTimer::singleShot(INTRO_HINT_DELAY_MS, this, [this]() {
        hintFadeIn_->start();
    });

    // 6. 启动粒子动画（延迟1.0秒）
    QTimer::singleShot(1000, this, [this]() {
        particleTimer_->start();
    });

    // 7. 启动自动跳转计时器
    autoAdvanceTimer_->start();
}

void StartWindow::setOnFinished(std::function<void()> callback) {
    onFinishedCallback_ = callback;
}

void StartWindow::onFadeInFinished() {
    // 标题淡入完成后，可以做一些额外效果
}

void StartWindow::onGlowUpdate() {
    // 更新发光强度（呼吸灯效果）
    if (glowIncreasing_) {
        glowIntensity_ += 0.03;
        if (glowIntensity_ >= 1.0) {
            glowIntensity_ = 1.0;
            glowIncreasing_ = false;
        }
    } else {
        glowIntensity_ -= 0.03;
        if (glowIntensity_ <= 0.3) {
            glowIntensity_ = 0.3;
            glowIncreasing_ = true;
        }
    }

    // 触发重绘，在paintEvent中绘制发光效果
    update();
}

void StartWindow::onAutoAdvance() {
    if (isTransitioning_) return;
    isTransitioning_ = true;

    // 停止所有定时器
    glowTimer_->stop();
    autoAdvanceTimer_->stop();
    particleTimer_->stop();

    // 对每个子组件分别淡出，避免对整个widget应用graphics effect导致布局错乱
    // 先清除可能存在的全局graphics effect
    setGraphicsEffect(nullptr);

    auto fadeOutWidget = [this](QWidget *widget) {
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(widget);
        effect->setOpacity(1.0);
        widget->setGraphicsEffect(effect);

        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", this);
        anim->setDuration(ANIM_DURATION_600_MS);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InCubic);
        return anim;
    };

    // 淡出标题、副标题、提示、跳过按钮
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(fadeOutWidget(titleLabel_));
    group->addAnimation(fadeOutWidget(subtitleLabel_));
    group->addAnimation(fadeOutWidget(hintLabel_));
    group->addAnimation(fadeOutWidget(skipBtn_));

    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        // 动画完成后发射信号并调用回调
        emit finished();
        if (onFinishedCallback_) {
            onFinishedCallback_();
        }
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void StartWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 绘制背景（SVG或渐变背景），使用bgOpacity_控制淡入
    if (!bgCache_.isNull()) {
        qreal opacity = bgOpacity_ ? bgOpacity_->opacity() : 1.0;
        painter.setOpacity(opacity);
        painter.drawPixmap(0, 0, bgCache_);
        painter.setOpacity(1.0);
    }

    // 绘制额外的装饰效果
    if (animationStarted_) {
        // 绘制顶部和底部的渐变遮罩（让文字更清晰）
        QLinearGradient topGradient(0, 0, 0, 250);
        topGradient.setColorAt(0.0, QColor(0, 0, 0, 100));
        topGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(0, 0, width(), 250, topGradient);

        QLinearGradient bottomGradient(0, height() - 200, 0, height());
        bottomGradient.setColorAt(0.0, QColor(0, 0, 0, 0));
        bottomGradient.setColorAt(1.0, QColor(0, 0, 0, 100));
        painter.fillRect(0, height() - 200, width(), 200, bottomGradient);

        // 绘制装饰性光线（围绕标题的微光）
        if (glowIntensity_ > 0.1) {
            painter.setPen(Qt::NoPen);
            QRadialGradient radialGradient(width() / 2, height() / 2 - 40, 350);
            radialGradient.setColorAt(0.0, QColor(100, 180, 255, static_cast<int>(20 * glowIntensity_)));
            radialGradient.setColorAt(0.5, QColor(100, 180, 255, static_cast<int>(10 * glowIntensity_)));
            radialGradient.setColorAt(1.0, QColor(100, 180, 255, 0));
            painter.setBrush(radialGradient);
            painter.drawEllipse(QPointF(width() / 2, height() / 2 - 40), 350, 140);
        }

        // 绘制标题发光光晕（在标题文字后面）
        if (glowIntensity_ > 0.1) {
            painter.setPen(Qt::NoPen);
            QRadialGradient titleGlow(width() / 2, height() / 2 - 60, 300);
            titleGlow.setColorAt(0.0, QColor(100, 180, 255, static_cast<int>(30 * glowIntensity_)));
            titleGlow.setColorAt(0.4, QColor(100, 180, 255, static_cast<int>(15 * glowIntensity_)));
            titleGlow.setColorAt(1.0, QColor(100, 180, 255, 0));
            painter.setBrush(titleGlow);
            painter.drawEllipse(QPointF(width() / 2, height() / 2 - 60), 250, 80);
        }

        // 绘制粒子
        drawParticles(painter);
    }
}

void StartWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // 窗口缩放时重新加载背景（但StartWindow是fixed size，实际不会触发）
    if (!bgCache_.isNull() && bgLoaded_) {
        // 重新查找并渲染SVG
        QDir dir(QCoreApplication::applicationDirPath());
        QString svgPath;
        for (int i = 0; i < 4; ++i) {
            QString candidate = dir.filePath("assets/ui/bg_door.svg");
            if (QFile::exists(candidate)) {
                svgPath = candidate;
                break;
            }
            dir.cdUp();
        }
        if (!svgPath.isEmpty()) {
            bgCache_ = renderSvgToPixmap(svgPath, size());
        }
    }
}

void StartWindow::initParticles() {
    particles_.clear();
    const int particleCount = 60;
    for (int i = 0; i < particleCount; ++i) {
        Particle p;
        p.pos = QPointF(
            QRandomGenerator::global()->bounded(width()),
            QRandomGenerator::global()->bounded(height())
        );
        p.velocity = QPointF(
            (QRandomGenerator::global()->bounded(200) - 100) / 100.0 * 0.5,
            -(QRandomGenerator::global()->bounded(100) + 20) / 100.0 * 0.8
        );
        p.size = QRandomGenerator::global()->bounded(6) + 2;
        p.opacity = QRandomGenerator::global()->bounded(100) / 100.0 * 0.6 + 0.1;
        
        // 随机颜色：金色、蓝色、白色
        int colorType = QRandomGenerator::global()->bounded(3);
        switch (colorType) {
            case 0: p.color = QColor(255, 215, 0); break;   // 金色
            case 1: p.color = QColor(100, 180, 255); break; // 蓝色
            case 2: p.color = QColor(255, 255, 255); break; // 白色
        }
        particles_.append(p);
    }
}

void StartWindow::updateParticles() {
    for (auto &p : particles_) {
        // 移动
        p.pos += p.velocity;
        
        // 透明度闪烁
        p.opacity += (QRandomGenerator::global()->bounded(100) - 50) / 1000.0;
        p.opacity = qBound(0.05, p.opacity, 0.8);
        
        // 超出屏幕则重置到底部
        if (p.pos.y() < -10 || p.pos.x() < -10 || p.pos.x() > width() + 10) {
            p.pos = QPointF(
                QRandomGenerator::global()->bounded(width()),
                height() + QRandomGenerator::global()->bounded(50)
            );
            p.velocity = QPointF(
                (QRandomGenerator::global()->bounded(200) - 100) / 100.0 * 0.5,
                -(QRandomGenerator::global()->bounded(100) + 20) / 100.0 * 0.8
            );
        }
    }
}

void StartWindow::drawParticles(QPainter &painter) {
    painter.setPen(Qt::NoPen);
    for (const auto &p : particles_) {
        QColor c = p.color;
        c.setAlphaF(p.opacity);
        painter.setBrush(c);
        painter.drawEllipse(p.pos, p.size, p.size);
    }
}

void StartWindow::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    
    // 如果动画已经开始且未在过渡中，点击可提前跳转
    if (animationStarted_ && !isTransitioning_) {
        // 检查是否点击了跳过按钮（由按钮自己处理）
        // 对于其他区域，也触发跳转
        if (!skipBtn_->underMouse()) {
            onAutoAdvance();
        }
    }
}

void StartWindow::keyPressEvent(QKeyEvent *event) {
    // Enter/Space: 跳转进入主菜单
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {
        if (animationStarted_ && !isTransitioning_) {
            onAutoAdvance();
            event->accept();
            return;
        }
    }
    // Esc: 也可以触发跳转
    if (event->key() == Qt::Key_Escape) {
        if (animationStarted_ && !isTransitioning_) {
            onAutoAdvance();
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}