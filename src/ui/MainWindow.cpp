/**
 * @file    MainWindow.cpp
 * @brief   主窗口实现
 * @author  
 * @date    2026-06-24
 */

#include "MainWindow.h"
#include "BoardWidget.h"
#include "../core/Commondata.h"
#include "ShopWidget.h"
#include "ShopWindow.h"
#include "MusicPlayerWidget.h"
#include "BattleLogWidget.h"
#include "ChartWidget.h"
#include "StatsWidget.h"
#include "../core/Game.h"
#include "../core/Player.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <exception>
#include <QApplication>
#include <QFrame>
#include <QDir>
#include <QCoreApplication>
#include <QStyle>
#include <QIcon>
#include <QFile>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QFileInfo>
#include <QDateTime>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QScrollBar>
#include <QHeaderView>
#include <QCheckBox>
#include <QTabWidget>
#include <map>

static QPixmap loadAssetPixmap(const QString &relativePath) {
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 4; ++i) {
        QString candidate = dir.filePath(relativePath);
        QPixmap pixmap(candidate);
        if (!pixmap.isNull()) {
            return pixmap;
        }
        dir.cdUp();
    }
    return QPixmap();
}

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

// 从可执行文件目录向上查找资源文件
static QString loadAssetFile(const QString &relativePath) {
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 4; ++i) {
        QString candidate = dir.filePath(relativePath);
        if (QFile::exists(candidate)) {
            return candidate;
        }
        dir.cdUp();
    }
    return QString();
}

// 获取或创建 saves 目录，返回其绝对路径
static QString ensureSavesDir() {
    QDir dir(QCoreApplication::applicationDirPath());
    QString savesPath;

    // 先向上查找已有的 saves 目录
    for (int i = 0; i < 4; ++i) {
        QString candidate = dir.filePath("saves");
        if (QDir(candidate).exists()) {
            return QDir(candidate).absolutePath();
        }
        dir.cdUp();
    }

    // 未找到则创建在可执行文件同级目录
    savesPath = QCoreApplication::applicationDirPath() + "/saves";
    QDir().mkpath(savesPath);
    return QDir(savesPath).absolutePath();
}

// 前向声明
static QString processInlineFormatting(const QString &text);

// 简单 Markdown → HTML 转换（支持常用语法）
static QString markdownToHtml(const QString &md) {
    QString html;
    QStringList lines = md.split('\n');

    // 在代码块中标记
    bool inCodeBlock = false;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];

        // 代码块 ``` ...
        if (line.trimmed().startsWith("```")) {
            if (inCodeBlock) {
                html += "</code></pre>\n";
                inCodeBlock = false;
            } else {
                html += "<pre><code>";
                inCodeBlock = true;
            }
            continue;
        }
        if (inCodeBlock) {
            html += line.toHtmlEscaped() + "\n";
            continue;
        }

        // 空行 → 段落分隔
        if (line.trimmed().isEmpty()) {
            // 如果上一段没关闭，这里也不需要额外操作
            continue;
        }

        // 水平分割线 ---
        if (line.trimmed().startsWith("---")) {
            html += "<hr>\n";
            continue;
        }

        // 表格行 (| ... |)
        if (line.trimmed().startsWith("|") && line.trimmed().endsWith("|")) {
            // 跳过表头分隔行 (| --- | --- |)
            if (line.contains("---")) {
                continue;
            }
            QStringList cells;
            QString trimmed = line.trimmed();
            // 去掉首尾的 |
            if (trimmed.startsWith("|")) trimmed = trimmed.mid(1);
            if (trimmed.endsWith("|")) trimmed = trimmed.left(trimmed.length() - 1);
            QStringList parts = trimmed.split('|');
            for (const QString &part : parts) {
                QString cell = part.trimmed();
                // 处理单元格内内联格式
                cell.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<b>\\1</b>");
                cell.replace(QRegularExpression("\\*(.+?)\\*"), "<i>\\1</i>");
                cells << "<td>" + cell + "</td>";
            }
            // 检查是否是表头（上一行是空行或者还没有表格开始）
            bool isHeader = false;
            if (i == 0 || lines[i-1].trimmed().isEmpty()) {
                isHeader = true;
                html += "<table>\n";
            }
            // 检查下面是否还有表格行
            bool hasNextRow = (i + 1 < lines.size()) &&
                              lines[i+1].trimmed().startsWith("|") &&
                              !lines[i+1].contains("---");
            if (!hasNextRow && i + 1 < lines.size() && lines[i+1].trimmed().startsWith("|")) {
                // 下一行是分隔行，跳过
            }

            if (isHeader) {
                html += "<tr>" + cells.join(QString()).replace("<td>", "<th>").replace("</td>", "</th>") + "</tr>\n";
            } else {
                html += "<tr>" + cells.join(QString()) + "</tr>\n";
            }

            // 检查是否是最后一行表格
            bool isLastRow = (i + 1 >= lines.size()) ||
                             !lines[i+1].trimmed().startsWith("|") ||
                             lines[i+1].contains("---");
            if (isLastRow) {
                html += "</table>\n";
            }
            continue;
        }

        // 标题 # ## ###
        QRegularExpression headingRe("^(#{1,3})\\s+(.+)$");
        auto headingMatch = headingRe.match(line);
        if (headingMatch.hasMatch()) {
            int level = headingMatch.captured(1).length();
            QString text = processInlineFormatting(headingMatch.captured(2));
            html += QString("<h%1>%2</h%1>\n").arg(level).arg(text);
            continue;
        }

        // 无序列表 - 或 *
        QRegularExpression ulRe("^[\\*-]\\s+(.+)$");
        auto ulMatch = ulRe.match(line.trimmed());
        if (ulMatch.hasMatch()) {
            // 检查是否需要开启 <ul>
            if (i == 0 || !lines[i-1].trimmed().startsWith("-") && !lines[i-1].trimmed().startsWith("*")) {
                html += "<ul>\n";
            }
            html += "<li>" + processInlineFormatting(ulMatch.captured(1)) + "</li>\n";
            // 检查是否需要关闭 </ul>
            if (i + 1 >= lines.size() ||
                (!lines[i+1].trimmed().startsWith("-") && !lines[i+1].trimmed().startsWith("*"))) {
                html += "</ul>\n";
            }
            continue;
        }

        // 有序列表 1. 2.
        QRegularExpression olRe("^\\d+\\.\\s+(.+)$");
        auto olMatch = olRe.match(line.trimmed());
        if (olMatch.hasMatch()) {
            if (i == 0 || !lines[i-1].trimmed().contains(QRegularExpression("^\\d+\\.\\s+"))) {
                html += "<ol>\n";
            }
            html += "<li>" + processInlineFormatting(olMatch.captured(1)) + "</li>\n";
            if (i + 1 >= lines.size() ||
                !lines[i+1].trimmed().contains(QRegularExpression("^\\d+\\.\\s+"))) {
                html += "</ol>\n";
            }
            continue;
        }

        // 普通段落
        html += "<p>" + processInlineFormatting(line) + "</p>\n";
    }

    return html;
}

// 处理内联 Markdown 格式（加粗、斜体、链接）
static QString processInlineFormatting(const QString &text) {
    QString result = text.toHtmlEscaped();
    // 注意：先处理加粗再处理斜体，避免冲突
    // **bold**
    result.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<b>\\1</b>");
    // *italic*
    result.replace(QRegularExpression("\\*(.+?)\\*"), "<i>\\1</i>");
    // [text](url)
    result.replace(QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)"), "<a href=\"\\2\" style=\"color: #63b3ed;\">\\1</a>");
    // ![alt](url)
    QRegularExpression imgRe("!\\[([^\\]]*)\\]\\(([^)]+)\\)");
    QRegularExpressionMatchIterator it = imgRe.globalMatch(result);
    QStringList parts;
    int lastEnd = 0;
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        parts << result.mid(lastEnd, m.capturedStart() - lastEnd);
        QString alt = m.captured(1);
        QString url = m.captured(2);
        QString resolved = loadAssetFile(url);
        if (!resolved.isEmpty())
            parts << QString("<img src=\"%1\" alt=\"%2\" style=\"max-width:200px;\">").arg(resolved, alt);
        else
            parts << QString("<i>%1</i>").arg(alt.isEmpty() ? "[图片]" : alt);
        lastEnd = m.capturedEnd();
    }
    parts << result.mid(lastEnd);
    result = parts.join(QString());
    return result;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , game_(std::make_shared<Game>())
    , gameTickCounter_(0)
    , startGlowIntensity_(0.0)
    , startGlowIncreasing_(true)
    , startAnimationStarted_(false)
    , startIsTransitioning_(false)
    , startBgLoaded_(false)
{
    setWindowTitle("Synera: Synergy Auto-Arena");
    // 移除固定尺寸，改为最小尺寸并允许调整大小
    setMinimumSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    resize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);  // 默认启动尺寸

    // 设置窗口图标（标签页/任务栏图标）
    QPixmap iconPixmap = loadAssetPixmap("assets/icons/leftup.svg");
    if (!iconPixmap.isNull()) {
        setWindowIcon(QIcon(iconPixmap));
    }

    setupUI();
    setupConnections();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    // 创建中央widget
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);

    // 创建堆叠布局用于主菜单和游戏界面切换
    centralStack_ = new QStackedWidget(centralWidget_);
    centralStack_->setContentsMargins(0, 0, 0, 0);

    // 加载背景图
    loadBackgrounds();

    // ============ 开场动画界面（StartScreen） ============
    setupStartScreen();

    // ============ 主菜单界面 ============
    mainMenuWidget_ = new QWidget(centralWidget_);
    QVBoxLayout *menuLayout = new QVBoxLayout(mainMenuWidget_);
    menuLayout->setSpacing(12);
    menuLayout->setContentsMargins(60, 40, 60, 60);

    // 标题
    QLabel *menuTitleLabel = new QLabel("Synera: Synergy Auto-Arena");
    menuTitleLabel->setAlignment(Qt::AlignCenter);
    menuTitleLabel->setStyleSheet("QLabel { font-size: 32px; font-weight: bold; color: #ffffff; padding-bottom: 4px; }");
    menuLayout->addWidget(menuTitleLabel);

    // 副标题
    QLabel *menuSubtitleLabel = new QLabel("20260522");
    menuSubtitleLabel->setAlignment(Qt::AlignCenter);
    menuSubtitleLabel->setStyleSheet("QLabel { font-size: 14px; color: #bdc3c7; margin-bottom: 20px; }");
    menuLayout->addWidget(menuSubtitleLabel);

    // 按钮通用样式
    QString btnStyle = 
        "QPushButton { "
        "    background-color: #34495e; "
        "    color: white; "
        "    border: 2px solid #2c3e50; "
        "    border-radius: 8px; "
        "    font-size: 16px; "
        "    font-weight: bold; "
        "    padding: 10px 30px; "
        "    min-width: 240px; "
        "} "
        "QPushButton:hover { background-color: #3d566e; border: 2px solid #5dade2; } "
        "QPushButton:pressed { background-color: #2c3e50; }";

        // 新游戏按钮（绿色高亮）
    newGameBtn_ = new QPushButton("新游戏");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_newgame.svg");
        if (!px.isNull()) newGameBtn_->setIcon(QIcon(px));
        newGameBtn_->setIconSize(QSize(22, 22));
    }
    newGameBtn_->setFocusPolicy(Qt::TabFocus);
    newGameBtn_->setMinimumSize(260, 48);
    newGameBtn_->setStyleSheet(
        "QPushButton { "
        "    background-color: #27ae60; "
        "    color: white; "
        "    border: 2px solid #1e8449; "
        "    border-radius: 8px; "
        "    font-size: 18px; "
        "    font-weight: bold; "
        "    padding: 12px 30px; "
        "    min-width: 260px; "
        "} "
        "QPushButton:hover { background-color: #2ecc71; border: 2px solid #27ae60; } "
        "QPushButton:pressed { background-color: #1e8449; }"
    );
    menuLayout->addWidget(newGameBtn_, 0, Qt::AlignCenter);

        // 加载游戏按钮
    loadGameBtn_ = new QPushButton("加载游戏");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_load.svg");
        if (!px.isNull()) {
            loadGameBtn_->setIcon(QIcon(px));
            loadGameBtn_->setIconSize(QSize(22, 22));
        }
    }
    loadGameBtn_->setFocusPolicy(Qt::TabFocus);
    loadGameBtn_->setMinimumSize(260, 48);
    loadGameBtn_->setStyleSheet(btnStyle);
    menuLayout->addWidget(loadGameBtn_, 0, Qt::AlignCenter);

    // 设置按钮
    settingsBtn_ = new QPushButton("设置");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_settings.svg");
        if (!px.isNull()) {
            settingsBtn_->setIcon(QIcon(px));
            settingsBtn_->setIconSize(QSize(22, 22));
        }
    }
    settingsBtn_->setFocusPolicy(Qt::TabFocus);
    settingsBtn_->setMinimumSize(260, 48);
    settingsBtn_->setStyleSheet(btnStyle);
    menuLayout->addWidget(settingsBtn_, 0, Qt::AlignCenter);

    // 帮助按钮
    helpBtn_ = new QPushButton("帮助");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_help.svg");
        if (!px.isNull()) {
            helpBtn_->setIcon(QIcon(px));
            helpBtn_->setIconSize(QSize(22, 22));
        }
    }
    helpBtn_->setFocusPolicy(Qt::TabFocus);
    helpBtn_->setMinimumSize(260, 48);
    helpBtn_->setStyleSheet(btnStyle);
    menuLayout->addWidget(helpBtn_, 0, Qt::AlignCenter);

        // 关于按钮
    aboutBtn_ = new QPushButton("关于");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_about.svg");
        if (!px.isNull()) aboutBtn_->setIcon(QIcon(px));
        aboutBtn_->setIconSize(QSize(22, 22));
    }
    aboutBtn_->setFocusPolicy(Qt::TabFocus);
    aboutBtn_->setMinimumSize(260, 48);
    aboutBtn_->setStyleSheet(btnStyle);
    menuLayout->addWidget(aboutBtn_, 0, Qt::AlignCenter);

        // 退出按钮（红色警示）
    exitBtn_ = new QPushButton("退出");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_exit.svg");
        if (!px.isNull()) {
            exitBtn_->setIcon(QIcon(px));
            exitBtn_->setIconSize(QSize(22, 22));
        }
    }
    exitBtn_->setFocusPolicy(Qt::TabFocus);
    exitBtn_->setMinimumSize(260, 48);
    exitBtn_->setStyleSheet(
        "QPushButton { "
        "    background-color: #c0392b; "
        "    color: white; "
        "    border: 2px solid #a93226; "
        "    border-radius: 8px; "
        "    font-size: 16px; "
        "    font-weight: bold; "
        "    padding: 10px 30px; "
        "    min-width: 260px; "
        "} "
        "QPushButton:hover { background-color: #e74c3c; border: 2px solid #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; }"
    );
    menuLayout->addWidget(exitBtn_, 0, Qt::AlignCenter);

    menuLayout->addStretch();

    mainMenuWidget_->setStyleSheet("QWidget { background: transparent; }");
        centralStack_->addWidget(mainMenuWidget_);

    // 创建背景标签（使用 centralWidget_ 的尺寸作为参考，避免布局未完成时 rect() 返回极小值）
    {
        QLabel* menuBgLabel = new QLabel(mainMenuWidget_);
        menuBgLabel->setObjectName("__bg_label__");
        QSize refSize = centralWidget_ ? centralWidget_->size() : QSize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
        menuBgLabel->setGeometry(QRect(QPoint(0, 0), refSize));
        menuBgLabel->setScaledContents(true);
        menuBgLabel->lower();
        if (!bgMainMenu_.isNull()) {
            menuBgLabel->setPixmap(QPixmap::fromImage(bgMainMenu_.toImage().scaled(refSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
    }

    // 添加开场画面到堆叠（索引0）
    if (startScreenWidget_) {
        centralStack_->insertWidget(0, startScreenWidget_);
    }

    // 游戏界面
    gameAreaWidget_ = new QWidget(centralWidget_);
    mainLayout_ = new QHBoxLayout(gameAreaWidget_);
    mainLayout_->setSpacing(LAYOUT_SPACING_DEFAULT);
    mainLayout_->setContentsMargins(GAMEAREA_MARGIN, GAMEAREA_MARGIN, GAMEAREA_MARGIN, GAMEAREA_MARGIN);

    // ============ 左侧：棋盘区域（使用相对尺寸，由 resizeEvent 动态调整） ============
    boardWidget_ = new BoardWidget(game_);
    boardWidget_->setMinimumSize(BOARD_AREA_MIN_WIDTH, BOARD_AREA_MIN_HEIGHT);
    mainLayout_->addWidget(boardWidget_, 4);

    // ============ 右侧：信息和商店面板 ============
    rightLayout_ = new QVBoxLayout();
    rightLayout_->setSpacing(LAYOUT_SPACING_PANEL);

    // --- 玩家信息面板 ---
    infoPanel_ = new QWidget();
    infoPanelLayout_ = new QVBoxLayout(infoPanel_);
    infoPanelLayout_->setSpacing(LAYOUT_SPACING_TIGHT);
    infoPanelLayout_->setContentsMargins(PANEL_MARGIN_LR, PANEL_MARGIN_TB, PANEL_MARGIN_LR, PANEL_MARGIN_TB);

    // 标题
    QLabel *infoPanelTitle = new QLabel("游戏状态");
    infoPanelTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; color: #2c3e50; margin-bottom: 5px; }");
    infoPanelLayout_->addWidget(infoPanelTitle);

    // --- 资源信息分组 ---
    QWidget *resourceGroup = new QWidget();
    QHBoxLayout *resourceLayout = new QHBoxLayout(resourceGroup);
resourceLayout->setSpacing(LAYOUT_SPACING_PANEL);
    resourceLayout->setContentsMargins(0, 0, 0, 0);

        // 卡片通用样式 - 带背景色的卡片包裹
    QString cardBaseStyle = "QWidget { background-color: %1; border-radius: 6px; padding: 4px 6px; }";

    // 生命值卡片
    QWidget *healthWidget = new QWidget();
    healthWidget->setStyleSheet(cardBaseStyle.arg("#fef0f0"));  // 浅红色背景
    QVBoxLayout *healthLayout = new QVBoxLayout(healthWidget);
    healthLayout->setSpacing(CARD_SPACING);
    healthLayout->setContentsMargins(CARD_MARGIN_LR, CARD_MARGIN_TB, CARD_MARGIN_LR, CARD_MARGIN_TB);
    QLabel *healthTitle = new QLabel();
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_heart.svg");
        if (!px.isNull()) {
            healthTitle->setPixmap(px.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            healthTitle->setText(QString::fromUtf8("\xE2\x9D\xA4"));
        }
    }
    healthTitle->setAlignment(Qt::AlignCenter);
    healthTitle->setStyleSheet("QLabel { font-size: 16px; background: transparent; }");
    healthLayout->addWidget(healthTitle);
    healthLabel_ = new QLabel("100");
    healthLabel_->setAlignment(Qt::AlignCenter);
    healthLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #e74c3c; background: transparent; }");
    healthLayout->addWidget(healthLabel_);
    resourceLayout->addWidget(healthWidget);

    // 金币卡片
    QWidget *goldWidget = new QWidget();
    goldWidget->setStyleSheet(cardBaseStyle.arg("#fef9e7"));  // 浅黄色背景
    QVBoxLayout *goldLayout = new QVBoxLayout(goldWidget);
    goldLayout->setSpacing(CARD_SPACING);
    goldLayout->setContentsMargins(CARD_MARGIN_LR, CARD_MARGIN_TB, CARD_MARGIN_LR, CARD_MARGIN_TB);
    QLabel *goldTitle = new QLabel();
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_gold.svg");
        if (!px.isNull()) {
            goldTitle->setPixmap(px.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            goldTitle->setText(QString::fromUtf8("\xF0\x9F\x92\xB0"));
        }
    }
    goldTitle->setAlignment(Qt::AlignCenter);
    goldTitle->setStyleSheet("QLabel { font-size: 16px; background: transparent; }");
    goldLayout->addWidget(goldTitle);
    goldLabel_ = new QLabel("0");
    goldLabel_->setAlignment(Qt::AlignCenter);
    goldLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #f39c12; background: transparent; }");
    goldLayout->addWidget(goldLabel_);
    resourceLayout->addWidget(goldWidget);

    // 等级卡片
    QWidget *levelWidget = new QWidget();
    levelWidget->setStyleSheet(cardBaseStyle.arg("#f4ecf7"));  // 浅紫色背景
    QVBoxLayout *levelLayout = new QVBoxLayout(levelWidget);
    levelLayout->setSpacing(CARD_SPACING);
    levelLayout->setContentsMargins(CARD_MARGIN_LR, CARD_MARGIN_TB, CARD_MARGIN_LR, CARD_MARGIN_TB);
    QLabel *levelTitle = new QLabel();
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_star.svg");
        if (!px.isNull()) {
            levelTitle->setPixmap(px.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            levelTitle->setText(QString::fromUtf8("\xE2\xAD\x90"));
        }
    }
    levelTitle->setAlignment(Qt::AlignCenter);
    levelTitle->setStyleSheet("QLabel { font-size: 16px; background: transparent; }");
    levelLayout->addWidget(levelTitle);
    levelLabel_ = new QLabel("1");
    levelLabel_->setAlignment(Qt::AlignCenter);
    levelLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #9b59b6; background: transparent; }");
    levelLayout->addWidget(levelLabel_);
    resourceLayout->addWidget(levelWidget);

    // 人口卡片
    QWidget *popWidget = new QWidget();
    popWidget->setStyleSheet(cardBaseStyle.arg("#e8f8f5"));  // 浅绿色背景
    QVBoxLayout *popLayout = new QVBoxLayout(popWidget);
    popLayout->setSpacing(CARD_SPACING);
    popLayout->setContentsMargins(CARD_MARGIN_LR, CARD_MARGIN_TB, CARD_MARGIN_LR, CARD_MARGIN_TB);
    QLabel *popTitle = new QLabel();
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_people.svg");
        if (!px.isNull()) {
            popTitle->setPixmap(px.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            popTitle->setText(QString::fromUtf8("\xF0\x9F\x91\xA5"));
        }
    }
    popTitle->setAlignment(Qt::AlignCenter);
    popTitle->setStyleSheet("QLabel { font-size: 16px; background: transparent; }");
    popLayout->addWidget(popTitle);
    populationLabel_ = new QLabel("0/3");
    populationLabel_->setAlignment(Qt::AlignCenter);
    populationLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #16a085; background: transparent; }");
    popLayout->addWidget(populationLabel_);
    resourceLayout->addWidget(popWidget);

    // 敌方生命值卡片
    QWidget *enemyHpWidget = new QWidget();
    enemyHpWidget->setStyleSheet(cardBaseStyle.arg("#3d000011"));  // 黑红色背景
    QVBoxLayout *enemyHpLayout = new QVBoxLayout(enemyHpWidget);
    enemyHpLayout->setSpacing(CARD_SPACING);
    enemyHpLayout->setContentsMargins(CARD_MARGIN_LR, CARD_MARGIN_TB, CARD_MARGIN_LR, CARD_MARGIN_TB);
    QLabel *enemyHpTitle = new QLabel();
    enemyHpTitle->setText(QString::fromUtf8("\xF0\x9F\x96\xA4"));
    enemyHpTitle->setAlignment(Qt::AlignCenter);
    enemyHpTitle->setStyleSheet("QLabel { font-size: 16px; background: transparent; }");
    enemyHpLayout->addWidget(enemyHpTitle);
    enemyHealthLabel_ = new QLabel("100");
    enemyHealthLabel_->setAlignment(Qt::AlignCenter);
    enemyHealthLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #e74c3c; background: transparent; }");
    enemyHpLayout->addWidget(enemyHealthLabel_);
    resourceLayout->addWidget(enemyHpWidget);

    infoPanelLayout_->addWidget(resourceGroup);

    // --- 经验条（独立满宽） ---
    QWidget *xpRow = new QWidget();
    QHBoxLayout *xpRowLayout = new QHBoxLayout(xpRow);
    xpRowLayout->setContentsMargins(0, 2, 0, 2);
    xpRowLayout->setSpacing(6);
    xpLevelLabel_ = new QLabel("经验");
    xpLevelLabel_->setStyleSheet("QLabel { font-size: 10px; font-weight: bold; color: #2980b9; }");
    xpRowLayout->addWidget(xpLevelLabel_);
    xpBar_ = new QProgressBar();
    xpBar_->setRange(0, 100);
    xpBar_->setValue(0);
    xpBar_->setTextVisible(true);
    xpBar_->setFormat("%v/%m");
    xpBar_->setMinimumHeight(18);
    xpBar_->setMaximumHeight(18);
    xpBar_->setStyleSheet(
        "QProgressBar { border: 1px solid #bdc3c7; border-radius: 4px; background: #ecf0f1; text-align: center; font-size: 9px; }"
        "QProgressBar::chunk { background-color: #3498db; border-radius: 3px; }");
    xpRowLayout->addWidget(xpBar_, 1);
    infoPanelLayout_->addWidget(xpRow);

    // --- 分隔线 ---
    QFrame *separator1 = new QFrame();
    separator1->setFrameShape(QFrame::HLine);
    separator1->setStyleSheet("QFrame { border: 1px solid #ecf0f1; margin: 4px 0px; }");
    infoPanelLayout_->addWidget(separator1);

    // --- 轮次和阶段 ---
    roundLabel_ = new QLabel("轮次: 1");
    roundLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #3498db; }");
    infoPanelLayout_->addWidget(roundLabel_);

    phaseLabel_ = new QLabel("准备阶段");
    phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #27ae60; background-color: #d5f4e6; padding: 4px 6px; border-radius: 3px; }");
    infoPanelLayout_->addWidget(phaseLabel_);

    // --- 分隔线 ---
    QFrame *separator2 = new QFrame();
    separator2->setFrameShape(QFrame::HLine);
    separator2->setStyleSheet("QFrame { border: 1px solid #ecf0f1; margin: 4px 0px; }");
    infoPanelLayout_->addWidget(separator2);

    // --- 连胜/连败显示 ---
    QWidget *streakWidget = new QWidget();
    streakWidget->setStyleSheet("QWidget { background-color: #fef9e7; border-radius: 6px; padding: 4px 8px; }");
    QHBoxLayout *streakLayout = new QHBoxLayout(streakWidget);
    streakLayout->setContentsMargins(8, 4, 8, 4);
    streakLayout->setSpacing(6);

    streakLabel_ = new QLabel("⚔️ 连胜: 0");
    streakLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #e67e22; background: transparent; }");
    streakLayout->addWidget(streakLabel_);
    streakLayout->addStretch();
    infoPanelLayout_->addWidget(streakWidget);

    // 控制按钮
    startRoundBtn_ = new QPushButton("结束回合");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_endround.svg");
        if (!px.isNull()) startRoundBtn_->setIcon(QIcon(px));
        startRoundBtn_->setIconSize(QSize(20, 20));
    }
    startRoundBtn_->setFocusPolicy(Qt::TabFocus);
    startRoundBtn_->setMinimumHeight(42);
    startRoundBtn_->setToolTip("结束准备阶段并进入战斗");
    startRoundBtn_->setStyleSheet(
        "QPushButton { "
        "    background-color: #3498db; "
        "    color: white; "
        "    border: 2px solid #2980b9; "
        "    border-radius: 8px; "
        "    padding: 10px; "
        "    font-size: 14px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; } "
        "QPushButton:disabled { background-color: #bdc3c7; border: 2px solid #95a5a6; color: #7f8c8d; }"
    );
    infoPanelLayout_->addWidget(startRoundBtn_);

    saveBtn_ = new QPushButton("存档");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_save.svg");
        if (!px.isNull()) {
            saveBtn_->setIcon(QIcon(px));
            saveBtn_->setIconSize(QSize(18, 18));
        }
    }
    saveBtn_->setFocusPolicy(Qt::TabFocus);
    saveBtn_->setMinimumHeight(42);
    saveBtn_->setToolTip("保存游戏进度");
    saveBtn_->setStyleSheet(
        "QPushButton { "
        "    background-color: #16a085; "
        "    color: white; "
        "    border: 2px solid #117a65; "
        "    border-radius: 8px; "
        "    padding: 10px; "
        "    font-size: 14px; "
        "} "
        "QPushButton:hover { background-color: #117a65; } "
        "QPushButton:pressed { background-color: #0d5f56; } "
        "QPushButton:disabled { background-color: #bdc3c7; border: 2px solid #95a5a6; color: #7f8c8d; }"
    );
    infoPanelLayout_->addWidget(saveBtn_);

    inExitBtn_ = new QPushButton("返回主菜单");
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_back.svg");
        if (!px.isNull()) {
            inExitBtn_->setIcon(QIcon(px));
            inExitBtn_->setIconSize(QSize(18, 18));
        }
    }
    inExitBtn_->setFocusPolicy(Qt::TabFocus);
    inExitBtn_->setMinimumHeight(42);
    inExitBtn_->setToolTip("返回主菜单");
    inExitBtn_->setStyleSheet(
        "QPushButton { "
        "    background-color: #7f8c8d; "
        "    color: white; "
        "    border: 2px solid #6c7a7d; "
        "    border-radius: 8px; "
        "    padding: 10px; "
        "    font-size: 14px; "
        "} "
        "QPushButton:hover { background-color: #95a5a6; } "
        "QPushButton:pressed { background-color: #6c7a7d; } "
        "QPushButton:disabled { background-color: #bdc3c7; border: 2px solid #95a5a6; color: #7f8c8d; }"
    );
    infoPanelLayout_->addWidget(inExitBtn_);

    infoPanelLayout_->addStretch();
    infoPanel_->setMinimumWidth(200);
    infoPanel_->setMaximumWidth(320);
    infoPanel_->setStyleSheet("QWidget { border: 2px solid #34495e; border-radius: 8px; padding: 10px; background-color: #ecf0f1; }");

    rightLayout_->addWidget(infoPanel_, 1);

    // --- 羁绊面板（独立板块，位于游戏状态与招募之间） ---
    // 用 QWidget 包裹羁绊面板，使其有和 infoPanel 一致的背景
    QWidget *traitsContainer = new QWidget();
    traitsContainer->setStyleSheet("QWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 8px; padding: 6px; }");
    QVBoxLayout *traitsLayout = new QVBoxLayout(traitsContainer);
    traitsLayout->setContentsMargins(TRAIT_MARGIN_LR, TRAIT_MARGIN_TB, TRAIT_MARGIN_LR, TRAIT_MARGIN_TB);
    traitsLayout->setSpacing(4);

    QLabel *traitsTitle = new QLabel("✨ 羁绊");
    traitsTitle->setStyleSheet("QLabel { font-size: 13px; font-weight: bold; color: #8e44ad; background: transparent; }");
    traitsLayout->addWidget(traitsTitle);

    traitsScrollArea_ = new QScrollArea();
    traitsScrollArea_->setWidgetResizable(true);
    traitsScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    traitsScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    traitsScrollArea_->setMinimumHeight(60);
    traitsScrollArea_->setMaximumHeight(220);
    traitsScrollArea_->setStyleSheet(
        "QScrollArea { background-color: transparent; border: none; }"
        "QScrollBar:vertical { background: rgba(0,0,0,0.05); width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: rgba(0,0,0,0.15); border-radius: 4px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );
    traitsContentLabel_ = new QLabel();
    traitsContentLabel_->setWordWrap(true);
    traitsContentLabel_->setTextFormat(Qt::RichText);
    traitsContentLabel_->setStyleSheet("QLabel { color: #2c3e50; padding: 4px; font-size: 11px; background: transparent; }");
    traitsScrollArea_->setWidget(traitsContentLabel_);
    traitsLayout->addWidget(traitsScrollArea_, 1);

    rightLayout_->addWidget(traitsContainer, 1);

    // --- 商店按钮（替代原商店区域） ---
    shopBtn_ = new QPushButton(QString::fromUtf8("商店"));
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_shop.svg");
        if (!px.isNull()) {
            shopBtn_->setIcon(QIcon(px));
            shopBtn_->setIconSize(QSize(24, 24));
        }
    }
    shopBtn_->setMinimumHeight(48);
    shopBtn_->setToolTip(QString::fromUtf8("打开商店（招募单位 / 装备管理）"));
    shopBtn_->setStyleSheet(
        "QPushButton { "
        "    background-color: #f39c12; "
        "    color: white; "
        "    border: 2px solid #e67e22; "
        "    border-radius: 10px; "
        "    padding: 10px; "
        "    font-size: 18px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #e67e22; } "
        "QPushButton:pressed { background-color: #d35400; }"
    );
    rightLayout_->addWidget(shopBtn_, 2);

    mainLayout_->addLayout(rightLayout_, 1);
    centralStack_->addWidget(gameAreaWidget_);

    // ============ 战斗日志浮动面板 ============
    battleLogWidget_ = new BattleLogWidget(this);
    battleLogWidget_->move(width() - battleLogWidget_->width() - 24, 80);
    battleLogWidget_->hide();

    // 连接清空日志信号
    connect(battleLogWidget_, &BattleLogWidget::clearLogRequested, this, [this]() {
        if (game_) {
            game_->clearBattleLog();
        }
    });

    // 战斗日志图标触发按钮（固定在音乐按钮上方）
    logToggleBtn_ = new QPushButton(this);
    logToggleBtn_->setText("📜");
    logToggleBtn_->setFixedSize(BUTTON_SIZE_LARGE, BUTTON_SIZE_LARGE);
    logToggleBtn_->setToolTip("打开战斗日志");
    logToggleBtn_->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(12, 14, 18, 200);"
        "    color: white;"
        "    border: 1px solid rgba(255, 255, 255, 0.15);"
        "    border-radius: 24px;"
        "    font-size: 22px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(241, 196, 15, 220);"
        "    border: 1px solid rgba(255, 255, 255, 0.3);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(200, 160, 10, 220);"
        "}"
    );
    logToggleBtn_->raise();

    // ============ 统计报表浮动面板 ============
    statsWidget_ = new StatsWidget(this);
    // 放在战斗日志左边，由 onToggleStatsReport 定位
    statsWidget_->hide();

    // 统计报表图标触发按钮（固定在日志按钮上方）
    statsToggleBtn_ = new QPushButton(this);
    statsToggleBtn_->setText("📊");
    statsToggleBtn_->setFixedSize(BUTTON_SIZE_LARGE, BUTTON_SIZE_LARGE);
    statsToggleBtn_->setToolTip("打开统计报表");
    statsToggleBtn_->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(12, 14, 18, 200);"
        "    color: white;"
        "    border: 1px solid rgba(255, 255, 255, 0.15);"
        "    border-radius: 24px;"
        "    font-size: 22px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(93, 173, 226, 220);"
        "    border: 1px solid rgba(255, 255, 255, 0.3);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(50, 130, 190, 220);"
        "}"
    );
    statsToggleBtn_->raise();
    statsToggleBtn_->hide();  // 初始隐藏，仅在游戏界面可见

    // 音乐播放器 - 初始隐藏，通过音乐图标按钮触发
    // 注意：音乐控件作为 MainWindow 的子部件，浮动在 centralWidget_ 之上
    musicPlayerWidget_ = new MusicPlayerWidget(this);
    musicPlayerWidget_->setFixedWidth(MUSICPLAYER_WIDTH);
    musicPlayerWidget_->move(width() - musicPlayerWidget_->width() - 24, height() - musicPlayerWidget_->height() - 24);
    musicPlayerWidget_->hide();

    // 音乐图标触发按钮（固定在右下角）
    musicToggleBtn_ = new QPushButton(this);
    {
        QPixmap px = loadAssetPixmap("assets/button/btn_music.svg");
        if (!px.isNull()) {
            musicToggleBtn_->setIcon(QIcon(px));
            musicToggleBtn_->setIconSize(QSize(28, 28));
        } else {
            musicToggleBtn_->setText(QString::fromUtf8("\xF0\x9F\x8E\xB5"));
        }
    }
    musicToggleBtn_->setFixedSize(BUTTON_SIZE_LARGE, BUTTON_SIZE_LARGE);
    musicToggleBtn_->setToolTip("打开音乐播放器");
    musicToggleBtn_->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(12, 14, 18, 200);"
        "    color: white;"
        "    border: 1px solid rgba(255, 255, 255, 0.15);"
        "    border-radius: 24px;"
        "    font-size: 22px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(241, 196, 15, 220);"
        "    border: 1px solid rgba(255, 255, 255, 0.3);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(200, 160, 10, 220);"
        "}"
    );
    // 定位日志按钮在音乐按钮上方
    logToggleBtn_->move(width() - logToggleBtn_->width() - 24, height() - musicToggleBtn_->height() - logToggleBtn_->height() - 36);
    logToggleBtn_->installEventFilter(this);
    logToggleBtn_->setMouseTracking(true);
    logToggleBtn_->hide();  // 初始隐藏，仅在游戏界面可见

    // 定位统计按钮在日志按钮上方
    statsToggleBtn_->move(width() - statsToggleBtn_->width() - 24, logToggleBtn_->y() - statsToggleBtn_->height() - 8);
    statsToggleBtn_->installEventFilter(this);
    statsToggleBtn_->setMouseTracking(true);

    musicToggleBtn_->raise();
    musicToggleBtn_->move(width() - musicToggleBtn_->width() - 24, height() - musicToggleBtn_->height() - 24);
    // 安装事件过滤器让音乐按钮自己处理拖拽
    musicToggleBtn_->installEventFilter(this);
    musicToggleBtn_->setMouseTracking(true);

    QVBoxLayout *containerLayout = new QVBoxLayout(centralWidget_);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(centralStack_);

    // 亮度遮罩层（覆盖在centralStack之上，但低于音乐播放器和按钮）
    brightnessOverlay_ = new QWidget(centralWidget_);
    brightnessOverlay_->setGeometry(centralWidget_->rect());
    brightnessOverlay_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    brightnessOverlay_->setStyleSheet("background-color: #000000;");
    brightnessEffect_ = new QGraphicsOpacityEffect(brightnessOverlay_);
    brightnessEffect_->setOpacity(0.0);
    brightnessOverlay_->setGraphicsEffect(brightnessEffect_);
    brightnessOverlay_->raise();
    // 确保浮动窗口和按钮在遮罩层之上
    if (battleLogWidget_) battleLogWidget_->raise();
    if (logToggleBtn_) logToggleBtn_->raise();
    if (statsWidget_) statsWidget_->raise();
    if (statsToggleBtn_) statsToggleBtn_->raise();
    if (musicPlayerWidget_) musicPlayerWidget_->raise();
    if (musicToggleBtn_) musicToggleBtn_->raise();

    mainLayout_->setStretch(0, 4);
    mainLayout_->setStretch(1, 1);

    // 应用样式
    setStyleSheet(
        "MainWindow { background-color: #ecf0f1; }"
        "QLabel { color: #2c3e50; }"
    );
}

void MainWindow::setupConnections() {
    connect(newGameBtn_, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(loadGameBtn_, &QPushButton::clicked, this, &MainWindow::onLoadGame);
    connect(settingsBtn_, &QPushButton::clicked, this, &MainWindow::onSettings);
    connect(helpBtn_, &QPushButton::clicked, this, &MainWindow::onHelp);
    connect(aboutBtn_, &QPushButton::clicked, this, &MainWindow::onAbout);
    connect(exitBtn_, &QPushButton::clicked, this, &MainWindow::onExitApp);
    connect(startRoundBtn_, &QPushButton::clicked, this, &MainWindow::onStartRound);
    connect(saveBtn_, &QPushButton::clicked, this, &MainWindow::onSaveGame);
    connect(inExitBtn_, &QPushButton::clicked, this, &MainWindow::onBackToMenuFromGame);
    connect(musicToggleBtn_, &QPushButton::clicked, this, &MainWindow::onToggleMusicPlayer);
    connect(logToggleBtn_, &QPushButton::clicked, this, &MainWindow::onToggleBattleLog);
    connect(statsToggleBtn_, &QPushButton::clicked, this, &MainWindow::onToggleStatsReport);
    connect(shopBtn_, &QPushButton::clicked, this, &MainWindow::onOpenShop);
    
    // 定时更新显示
    QTimer *updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::onGameTick);
    updateTimer->start(TIMER_UPDATE_MS);
    
    // 动画更新定时器（每33ms约30fps）
    animTimer_ = new QTimer(this);
    animTimer_->setSingleShot(false);
    connect(animTimer_, &QTimer::timeout, this, &MainWindow::onAnimationUpdate);
    animTimer_->start(TIMER_ANIM_MS);
}

void MainWindow::showMainMenu() {
    paintBgForWidget(mainMenuWidget_, bgMainMenu_);
    centralStack_->setCurrentWidget(mainMenuWidget_);
    // 离开游戏界面时隐藏战斗日志和统计报表相关控件
    if (battleLogWidget_) battleLogWidget_->hide();
    if (logToggleBtn_) logToggleBtn_->hide();
    if (statsWidget_) statsWidget_->hide();
    if (statsToggleBtn_) statsToggleBtn_->hide();
    // 强制用 centralWidget_ 的尺寸更新背景标签（确保尺寸正确）
    QLabel* bgLabel = mainMenuWidget_->findChild<QLabel*>("__bg_label__");
    if (bgLabel && centralWidget_) {
        bgLabel->setGeometry(centralWidget_->rect());
        if (!bgMainMenu_.isNull()) {
            bgLabel->setPixmap(QPixmap::fromImage(bgMainMenu_.toImage().scaled(centralWidget_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
    }
}

void MainWindow::showGameArea() {
    centralStack_->setCurrentWidget(gameAreaWidget_);
    // 进入游戏界面时确保音乐控件定位到右下角
    repositionMusicControls();
    // 显示浮动功能按钮
    if (logToggleBtn_) {
        logToggleBtn_->show();
    }
    if (statsToggleBtn_) {
        statsToggleBtn_->show();
    }
    // 将焦点设置到棋盘，支持键盘导航
    if (boardWidget_) {
        boardWidget_->setFocus();
    }
}

void MainWindow::initialize() {
    game_->initialize();
    updateDisplay();

    // 显示开场动画（如果startScreenWidget_在堆叠中，切换到它并开始动画）
    if (startScreenWidget_ && centralStack_->indexOf(startScreenWidget_) >= 0) {
        centralStack_->setCurrentWidget(startScreenWidget_);
        startIntroAnimation();
    } else {
        showMainMenu();
    }
}

void MainWindow::updateDisplay() {
    updatePlayerInfo();
    updateGameState();
    
    // P3-4: 检查装备掉落动画
    if (game_ && game_->consumeEquipDropAnim() && boardWidget_) {
        boardWidget_->startEquipDropAnim();
    }
    
    boardWidget_->updateDisplay();
    // 商店更新由 ShopWindow 管理，此处不再直接调用
    updateTraitsPanel();
    updateCombatLog();
    updateStatsReport();
    updateStreakDisplay();
}

void MainWindow::updatePlayerInfo() {
    const Player &player = game_->getPlayer();
    healthLabel_->setText(QString::number(player.getHealth()));
    // 记录当前金币值供动画使用（非动画更新时直接设置）
    lastDisplayedGold_ = player.getGold();
    goldLabel_->setText(QString::number(player.getGold()));
    levelLabel_->setText(QString::number(player.getLevel()));
    if (xpBar_) {
        xpBar_->setRange(0, player.getExperienceNextLevel());
        xpBar_->setValue(player.getExperience());
    }
    if (xpLevelLabel_) {
        xpLevelLabel_->setText(QString("Lv.%1 经验").arg(player.getLevel()));
    }
    populationLabel_->setText(QString("%1/%2")
                              .arg(player.getCurrentPopulation())
                              .arg(player.getPopulationLimit()));
    // 敌方生命值
    const Player &enemy = game_->getEnemyPlayer();
    enemyHealthLabel_->setText(QString::number(std::max(0, enemy.getHealth())));
    roundLabel_->setText(QString("轮次: %1").arg(game_->getCurrentRound()));
}

void MainWindow::updateGameState() {
    if (!game_) {
        phaseLabel_->setText("未知");
        return;
    }
    if (game_->isGameOver()) {
        if (game_->isPlayerWonGame()) {
            phaseLabel_->setText("🏆 你赢了！");
            phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #f39c12; background-color: #fef9e7; padding: 4px 6px; border-radius: 3px; }");
        } else {
            phaseLabel_->setText("💀 游戏结束");
            phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #c0392b; background-color: #fadbd8; padding: 4px 6px; border-radius: 3px; }");
        }
        startRoundBtn_->setEnabled(false);
        // 兜底：如果游戏结束弹窗因任何原因未弹出，在此触发
        if (!gameOverDialogShown_ && !game_->isInCombatPhase()) {
            gameOverDialogShown_ = true;
            // 确保战斗定时器已清理
            if (combatTimer_) {
                combatTimer_->stop();
                delete combatTimer_;
                combatTimer_ = nullptr;
            }
            combatEndPending_ = false;
            QTimer::singleShot(ROUND_SETTLEMENT_DELAY_MS, this, [this]() {
                if (game_ && game_->isGameOver()) showGameOverDialog();
            });
        }
    } else if (game_->isInCombatPhase()) {
        phaseLabel_->setText("战斗阶段");
        phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #c0392b; background-color: #fadbd8; padding: 4px 6px; border-radius: 3px; }");
        startRoundBtn_->setEnabled(false);
    } else {
        phaseLabel_->setText("准备阶段");
        phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #27ae60; background-color: #d5f4e6; padding: 4px 6px; border-radius: 3px; }");
        startRoundBtn_->setEnabled(true);
    }
}

void MainWindow::onNewGame() {
    gameOverDialogShown_ = false;
    if (game_) {
        game_->initialize();
        updateDisplay();
        showGameArea();
    }
}

void MainWindow::onStartGame() {
    onNewGame();
}

void MainWindow::onStartRound() {
    if (game_ && !game_->isGameOver() && !game_->isInCombatPhase()) {
        // 清理上一轮残留动画，清零超时计数器
        if (boardWidget_) boardWidget_->clearAnimations();
        combatAnimSkipCounter_ = 0;
        
        // 第一步：部署敌方单位到棋盘
        game_->deployEnemyUnits();
        updateDisplay();
        
        // 禁用按钮，防止重复点击
        startRoundBtn_->setEnabled(false);
        phaseLabel_->setText("敌方部署完成，战斗开始！");
        phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #e67e22; background-color: #fdebd0; padding: 4px 6px; border-radius: 3px; }");
        
        // 显示战斗开始过渡提示
        showCombatStartTransition();
        
        // 启动定时器，逐步执行战斗行动
        combatTimer_ = new QTimer(this);
        combatTimer_->setSingleShot(false);
        connect(combatTimer_, &QTimer::timeout, this, &MainWindow::onCombatStep);
        combatTimer_->start(TIMER_COMBAT_MS / std::max(1, game_->getCombatSpeed()));
    }
}

void MainWindow::onCombatStep() {
    if (!game_ || game_->isGameOver()) {
        if (combatTimer_) {
            combatTimer_->stop();
            delete combatTimer_;
            combatTimer_ = nullptr;
        }
        if (boardWidget_) boardWidget_->clearAnimations();
        combatAnimSkipCounter_ = 0;
        combatEndPending_ = false;
        updateDisplay();
        // 游戏结束弹窗（确保只弹一次）
        if (game_ && game_->isGameOver() && !gameOverDialogShown_) {
            gameOverDialogShown_ = true;
            showCombatEndTransition();
            updatePhaseDisplay();
            QTimer::singleShot(ROUND_SETTLEMENT_DELAY_MS, this, [this]() {
                if (game_ && game_->isGameOver()) showGameOverDialog();
            });
        }
        return;
    }
    
    if (!game_->isInCombatPhase()) {
        // 大招动画还在播 → 等播完再结算
        if (combatEndPending_ && boardWidget_ && boardWidget_->hasActiveAnimations()) {
            return;
        }
        combatEndPending_ = false;
        if (combatTimer_) {
            combatTimer_->stop();
            delete combatTimer_;
            combatTimer_ = nullptr;
        }
        showCombatEndTransition();
        updatePhaseDisplay();
        startRoundBtn_->setEnabled(true);
        if (boardWidget_) boardWidget_->clearAnimations();
        updateDisplay();
        QTimer::singleShot(ROUND_SETTLEMENT_DELAY_MS, this, [this]() {
            if (game_ && !game_->isGameOver()) showRoundSettlement();
        });
        return;
    }
    
    // 如果 BoardWidget 有正在播放的动画，等待动画完成（最多等 30 个 tick ≈ 3 秒）
    if (boardWidget_ && boardWidget_->hasActiveAnimations()) {
        combatAnimSkipCounter_++;
        if (combatAnimSkipCounter_ >= 30) {
            // 超时：强制清除卡死的动画，继续战斗
            boardWidget_->clearAnimations();
            combatAnimSkipCounter_ = 0;
        } else {
            return;
        }
    } else {
        combatAnimSkipCounter_ = 0;
    }

    // 记录旧位置和旧HP用于动画
    updateCombatPhaseLabel();

    auto oldPlayerUnits = game_->getBoard().getUnitsForOwner(Owner::PlayerCtrl);
    std::map<std::shared_ptr<Unit>, std::pair<int,int>> oldPositions;
    std::map<std::shared_ptr<Unit>, int> oldPlayerHP;
    for (auto& u : oldPlayerUnits) {
        if (u) {
            auto [x, y] = u->getPosition();
            oldPositions[u] = {x, y};
            oldPlayerHP[u] = u->getHP();
        }
    }
    auto oldEnemyUnits = game_->getEnemyUnits();
    std::map<std::shared_ptr<Unit>, std::pair<int,int>> oldEnemyPositions;
    std::map<std::shared_ptr<Unit>, int> oldEnemyHP;
    for (auto& u : oldEnemyUnits) {
        if (u) {
            auto [x, y] = u->getPosition();
            oldEnemyPositions[u] = {x, y};
            oldEnemyHP[u] = u->getHP();
        }
    }

    // 执行一步行动
    bool hasMore = game_->doEnemyMoveStep();

    // 大招字幕（最先显示）
    if (boardWidget_ && game_->hasUltimateUsed()) {
        boardWidget_->startUltimateOverlay(QString::fromStdString(game_->consumeUltimateName()),
                                             QString::fromStdString(game_->consumeUltimateChar()),
                                             game_->getUltimateUnitX(), game_->getUltimateUnitY());
    }

    // 检测移动、伤害、死亡并启动动画
    if (boardWidget_) {
        auto newPlayerUnits = game_->getBoard().getUnitsForOwner(Owner::PlayerCtrl);
        for (auto& u : newPlayerUnits) {
            if (!u) continue;
            auto [nx, ny] = u->getPosition();
            auto it = oldPositions.find(u);
            if (it != oldPositions.end()) {
                auto [ox, oy] = it->second;
                if (ox != nx || oy != ny) {
                    boardWidget_->startUnitMoveAnim(ox, oy, nx, ny);
                }
            }
            auto hpIt = oldPlayerHP.find(u);
            if (hpIt != oldPlayerHP.end()) {
                int dmg = hpIt->second - u->getHP();
                if (dmg > 0) boardWidget_->startDamageAnim(nx, ny, dmg, false);
            }
        }
        auto newEnemyUnits = game_->getEnemyUnits();
        for (auto& u : newEnemyUnits) {
            if (!u) continue;
            auto [nx, ny] = u->getPosition();
            auto it = oldEnemyPositions.find(u);
            if (it != oldEnemyPositions.end()) {
                auto [ox, oy] = it->second;
                if (ox != nx || oy != ny) {
                    boardWidget_->startUnitMoveAnim(ox, oy, nx, ny);
                }
            }
            auto hpIt = oldEnemyHP.find(u);
            if (hpIt != oldEnemyHP.end()) {
                int dmg = hpIt->second - u->getHP();
                if (dmg > 0) boardWidget_->startDamageAnim(nx, ny, dmg, false);
            }
        }
        for (auto& u : oldPlayerUnits) {
            if (u && !u->isAlive()) {
                auto it = oldPositions.find(u);
                if (it != oldPositions.end()) {
                    auto [x, y] = it->second;
                    boardWidget_->startDeathAnim(x, y);
                }
            }
        }
        for (auto& u : oldEnemyUnits) {
            if (u && !u->isAlive()) {
                auto it = oldEnemyPositions.find(u);
                if (it != oldEnemyPositions.end()) {
                    auto [x, y] = it->second;
                    boardWidget_->startDeathAnim(x, y);
                }
            }
        }
    }

    updateDisplay();

    // 立即检查游戏是否结束（不再等下一 tick，防止定时器异常导致弹窗丢失）
    if (game_->isGameOver()) {
        if (combatTimer_) {
            combatTimer_->stop();
            delete combatTimer_;
            combatTimer_ = nullptr;
        }
        if (boardWidget_) boardWidget_->clearAnimations();
        combatAnimSkipCounter_ = 0;
        combatEndPending_ = false;
        if (!gameOverDialogShown_) {
            gameOverDialogShown_ = true;
            showCombatEndTransition();
            updatePhaseDisplay();
            QTimer::singleShot(ROUND_SETTLEMENT_DELAY_MS, this, [this]() {
                if (game_ && game_->isGameOver()) showGameOverDialog();
            });
        }
        return;
    }

    if (!hasMore || !game_->isInCombatPhase()) {
        combatEndPending_ = true;
    }
}

void MainWindow::onAnimationUpdate() {
    if (boardWidget_) {
        boardWidget_->updateAnimations(0.033f);
        if (boardWidget_->hasActiveAnimations()) {
            boardWidget_->update();
        }
    }
}

void MainWindow::updateCombatPhaseLabel() {
    if (!game_) return;
    
    if (game_->isEnemyTurnNow()) {
        int idx = game_->getCombatEnemyIdx();
        int total = std::max(1, static_cast<int>(game_->getEnemyUnits().size()));
        if (idx >= total) idx = total;  // 防止越界显示
        phaseLabel_->setText(QString("敌方行动中 (%1/%2)")
            .arg(idx)
            .arg(total));
        phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #e74c3c; background-color: #fadbd8; padding: 4px 6px; border-radius: 3px; }");
    } else {
        auto playerUnits = game_->getBoard().getUnitsForOwner(Owner::PlayerCtrl);
        int aliveCount = 0;
        for (auto& u : playerUnits) if (u && u->isAlive()) aliveCount++;
        int idx = game_->getCombatPlayerIdx();
        if (idx > aliveCount) idx = aliveCount;
        phaseLabel_->setText(QString("我方行动中 (%1/%2)")
            .arg(idx)
            .arg(std::max(1, aliveCount)));
        phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #3498db; background-color: #d6eaf8; padding: 4px 6px; border-radius: 3px; }");
    }
}

void MainWindow::updatePhaseDisplay() {
    if (!game_) return;
    
    if (game_->isGameOver()) {
        if (game_->isPlayerWonGame()) {
            phaseLabel_->setText("🏆 你赢了！");
            phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #f39c12; background-color: #fef9e7; padding: 4px 6px; border-radius: 3px; }");
        } else {
            phaseLabel_->setText("💀 游戏结束");
            phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #c0392b; background-color: #fadbd8; padding: 4px 6px; border-radius: 3px; }");
        }
    } else if (game_->isInCombatPhase()) {
        phaseLabel_->setText("战斗阶段");
        phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #e74c3c; background-color: #fadbd8; padding: 4px 6px; border-radius: 3px; }");
    } else {
        phaseLabel_->setText("准备阶段");
        phaseLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #27ae60; background-color: #d5f4e6; padding: 4px 6px; border-radius: 3px; }");
    }
}

// ============ 战斗阶段切换过渡提示 ============

void MainWindow::showCombatStartTransition() {
    if (!boardWidget_) return;
    
    // 创建半透明遮罩标签
    QLabel *overlay = new QLabel(boardWidget_);
    overlay->setGeometry(boardWidget_->rect());
    overlay->setStyleSheet("background-color: rgba(0, 0, 0, 0.5);");
    overlay->setAlignment(Qt::AlignCenter);
    overlay->setText("⚔️ 战斗开始 ⚔️");
    overlay->setStyleSheet(
        "background-color: rgba(0, 0, 0, 0.6);"
        "color: #e74c3c;"
        "font-size: 36px;"
        "font-weight: bold;"
        "border: 3px solid #e74c3c;"
        "border-radius: 12px;"
    );
    overlay->show();
    overlay->raise();
    
    // 淡入动画
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(overlay);
    overlay->setGraphicsEffect(effect);
    effect->setOpacity(0.0);
    
    QPropertyAnimation *fadeIn = new QPropertyAnimation(effect, "opacity");
    fadeIn->setDuration(EFFECT_FADEIN_DURATION_MS);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start();
    
    // 1.2秒后淡出并删除
    QTimer::singleShot(EFFECT_FADE_DELAY_MS, this, [overlay, effect]() {
        QPropertyAnimation *fadeOut = new QPropertyAnimation(effect, "opacity");
        fadeOut->setDuration(EFFECT_FADEOUT_DURATION_MS);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, overlay, &QLabel::deleteLater);
        fadeOut->start();
    });
}

void MainWindow::showCombatEndTransition() {
    if (!boardWidget_ || !game_) return;
    
    QString message;
    QString color;
    
    if (game_->isGameOver()) {
        if (game_->isPlayerWonGame()) {
            message = "🏆 敌方全灭，你赢得了游戏！🏆";
            color = "#f1c40f";
        } else {
            message = "💀 生命耗尽，游戏结束 💀";
            color = "#c0392b";
        }
    } else {
        // 检查玩家是否赢了这一轮
        auto playerUnits = game_->getBoard().getUnitsForOwner(Owner::PlayerCtrl);
        bool playerAlive = false;
        for (auto& u : playerUnits) {
            if (u && u->isAlive()) { playerAlive = true; break; }
        }
        auto enemyUnits = game_->getEnemyUnits();
        bool enemyAlive = false;
        for (auto& u : enemyUnits) {
            if (u && u->isAlive()) { enemyAlive = true; break; }
        }
        
        if (playerAlive && !enemyAlive) {
            message = "🎉 胜利 🎉";
            color = "#27ae60";
        } else {
            message = "💔 失败 💔";
            color = "#e74c3c";
        }
    }
    
    // 创建半透明遮罩标签
    QLabel *overlay = new QLabel(boardWidget_);
    overlay->setGeometry(boardWidget_->rect());
    overlay->setAlignment(Qt::AlignCenter);
    overlay->setText(message);
    overlay->setStyleSheet(
        QString("background-color: rgba(0, 0, 0, 0.6);"
                "color: %1;"
                "font-size: 36px;"
                "font-weight: bold;"
                "border: 3px solid %1;"
                "border-radius: 12px;").arg(color)
    );
    overlay->show();
    overlay->raise();
    
    // 淡入动画
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(overlay);
    overlay->setGraphicsEffect(effect);
    effect->setOpacity(0.0);
    
    QPropertyAnimation *fadeIn = new QPropertyAnimation(effect, "opacity");
    fadeIn->setDuration(EFFECT_FADEIN_DURATION_MS);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start();
    
    // 1.5秒后淡出并删除
    QTimer::singleShot(BG_TRANSITION_DELAY_MS, this, [overlay, effect]() {
        QPropertyAnimation *fadeOut = new QPropertyAnimation(effect, "opacity");
        fadeOut->setDuration(EFFECT_FADEOUT_DURATION2_MS);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, overlay, &QLabel::deleteLater);
        fadeOut->start();
    });
}

void MainWindow::updateTraitsPanel() {
    if (!traitsScrollArea_ || !traitsContentLabel_ || !boardWidget_) return;
    
    const auto& activeTraits = boardWidget_->getActiveTraits();
    
    // 构建羁绊HTML内容
    QString html;
    html += "<div style='color: #8e44ad; font-size: 13px; font-weight: bold; margin-bottom: 6px;'>✨ 已激活羁绊</div>";
    
    if (activeTraits.empty()) {
        html += "<div style='color: #95a5a6; font-size: 11px;'>（无激活羁绊）</div>";
    } else {
        html += "<hr style='border: 0.5px solid #bdc3c7; margin: 4px 0;'>";
        for (const auto &qt : activeTraits) {
            QStringList lines = qt.split('\n');
            if (!lines.isEmpty()) {
                QString nameLine = lines[0].trimmed();
                
                // 装备套装条目（含 】— 角色名）
                if (nameLine.contains(QString::fromUtf8("】—"))) {
                    html += QString("<div style='color: #e67e22; font-size: 12px; font-weight: bold; margin-top: 5px;'>"
                                    "🔶 %1</div>").arg(nameLine.toHtmlEscaped());
                    continue;
                }
                
                // 解析羁绊名称中的数量信息
                int currentCount = 0;
                int firstTierThreshold = 2;
                int secondTierThreshold = 4;
                
                // 从括号中提取当前数量
                QRegularExpression countRegex("\\((\\d+)\\)");
                QRegularExpressionMatch countMatch = countRegex.match(nameLine);
                if (countMatch.hasMatch()) {
                    currentCount = countMatch.captured(1).toInt();
                }
                
                // 羁绊名称 - 蓝色标题
                html += QString("<div style='color: #2980b9; font-size: 12px; font-weight: bold; margin-top: 5px;'>%1</div>")
                    .arg(nameLine.toHtmlEscaped());
                
                // 激活进度条
                int firstProgress = qMin(currentCount, secondTierThreshold);
                float firstPercent = static_cast<float>(firstProgress) / secondTierThreshold;
                QString firstBarColor;
                if (firstProgress >= firstTierThreshold) {
                    firstBarColor = "#27ae60";
                } else {
                    firstBarColor = "#e74c3c";
                }
                int firstBarWidth = static_cast<int>(firstPercent * 100);
                
                html += "<div style='margin: 2px 0 1px 10px; font-size: 10px; color: #7f8c8d;'>";
                html += QString("第1阶 (%1/%2)").arg(firstProgress).arg(secondTierThreshold);
                html += "<div style='background-color: #ecf0f1; border-radius: 3px; height: 6px; width: 100%; margin-top: 1px;'>";
                html += QString("<div style='background-color: %1; border-radius: 3px; height: 6px; width: %2%; transition: width 0.3s;'></div>")
                    .arg(firstBarColor).arg(firstBarWidth);
                html += "</div></div>";
                
                // 第二阶进度条
                if (currentCount > firstTierThreshold) {
                    int secondProgress = currentCount - firstTierThreshold;
                    float secondPercent = static_cast<float>(secondProgress) / secondTierThreshold;
                    QString secondBarColor = (currentCount >= secondTierThreshold + firstTierThreshold) ? "#27ae60" : "#e67e22";
                    int secondBarWidth = static_cast<int>(secondPercent * 100);
                    
                    html += "<div style='margin: 2px 0 1px 10px; font-size: 10px; color: #7f8c8d;'>";
                    html += QString("第2阶 (%1/%2)").arg(secondProgress).arg(secondTierThreshold);
                    html += "<div style='background-color: #ecf0f1; border-radius: 3px; height: 6px; width: 100%; margin-top: 1px;'>";
                    html += QString("<div style='background-color: %1; border-radius: 3px; height: 6px; width: %2%; transition: width 0.3s;'></div>")
                        .arg(secondBarColor).arg(secondBarWidth);
                    html += "</div></div>";
                }
            }
            for (int i = 1; i < lines.size(); ++i) {
                QString line = lines[i].trimmed();
                if (line.isEmpty()) continue;
                QString color;
                if (line.startsWith("效果:")) color = "#27ae60";
                else if (line.startsWith("受影响")) color = "#2c3e50cc";
                else color = "#7f8c8d";
                html += QString("<div style='color: %1; font-size: 10px; padding-left: 10px;'>%2</div>")
                    .arg(color, line.toHtmlEscaped());
            }
        }
    }
    traitsContentLabel_->setText(html);
}

void MainWindow::updateCombatLog() {
    if (!battleLogWidget_ || !game_) return;
    battleLogWidget_->updateLog(game_.get());
    
    // 如果有新日志且面板隐藏时，在按钮上显示通知小红点
    const auto& log = game_->getBattleLog();
    if (!log.empty() && battleLogWidget_->isHidden()) {
        logToggleBtn_->setText("📜🔴");
    } else {
        logToggleBtn_->setText("📜");
    }
}

void MainWindow::updateStatsReport() {
    if (!statsWidget_ || !game_) return;
    statsWidget_->updateStats(game_.get());
    
    // 有数据且面板隐藏时，在按钮上显示通知小红点（用文字标记）
    const auto& log = game_->getBattleLog();
    if (!log.empty() && statsWidget_->isHidden()) {
        statsToggleBtn_->setText("📊🔴");
    } else {
        statsToggleBtn_->setText("📊");
    }
}

void MainWindow::updateStreakDisplay() {
    if (!streakLabel_ || !game_) return;
    
    int winStreak = game_->getWinStreak();
    int lossStreak = game_->getLossStreak();
    
    if (winStreak > 1) {
        // 连胜——用火焰图标
        QString fireIcons;
        int iconCount = std::min(winStreak, 5);
        for (int i = 0; i < iconCount; ++i) fireIcons += "🔥";
        streakLabel_->setText(QString("%1 连胜: %2").arg(fireIcons).arg(winStreak));
        streakLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #e74c3c; background: transparent; }");
    } else if (lossStreak > 1) {
        // 连败——用破碎图标
        QString brokenIcons;
        int iconCount = std::min(lossStreak, 5);
        for (int i = 0; i < iconCount; ++i) brokenIcons += "💔";
        streakLabel_->setText(QString("%1 连败: %2").arg(brokenIcons).arg(lossStreak));
        streakLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #8e44ad; background: transparent; }");
    } else if (winStreak == 1) {
        streakLabel_->setText("⚔️ 胜场: 1");
        streakLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #27ae60; background: transparent; }");
    } else {
        streakLabel_->setText("⚔️ 连胜: 0");
        streakLabel_->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #7f8c8d; background: transparent; }");
    }
}

void MainWindow::showRoundSettlement() {
    if (!game_) return;
    
    const auto& log = game_->getBattleLog();
    if (log.empty()) return;
    
    const auto& entry = log.back();
    
    // 创建结算弹窗
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("回合 %1 结算").arg(entry.round));
    dialog->setFixedSize(380, 320);
    dialog->setModal(true);
    dialog->setStyleSheet(
        "QDialog {"
        "    background-color: #f5f6fa;"
        "    border: 2px solid #34495e;"
        "    border-radius: 12px;"
        "}"
    );
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setSpacing(10);
layout->setContentsMargins(DIALOG_MARGIN_LR, DIALOG_MARGIN_TB, DIALOG_MARGIN_LR, DIALOG_MARGIN_TB);
    
    // 标题
    QString titleText = entry.playerWon ? "🏆 战斗胜利！" : "💀 战斗失败...";
    QString titleColor = entry.playerWon ? "#27ae60" : "#e74c3c";
    QLabel *titleLabel = new QLabel(titleText);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString("QLabel { font-size: 22px; font-weight: bold; color: %1; }").arg(titleColor));
    layout->addWidget(titleLabel);
    
    // 详细信息表格
    QTableWidget *table = new QTableWidget(6, 2);
    table->setHorizontalHeaderLabels({"项目", "数值"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(false);
    table->setStyleSheet(
        "QTableWidget {"
        "    background-color: transparent;"
        "    border: none;"
        "    font-size: 13px;"
        "}"
        "QHeaderView::section {"
        "    background-color: #ecf0f1;"
        "    color: #2c3e50;"
        "    font-weight: bold;"
        "    border: none;"
        "    padding: 6px 10px;"
        "    font-size: 12px;"
        "}"
        "QTableWidget::item {"
        "    padding: 4px 10px;"
        "    border-bottom: 1px solid #ecf0f1;"
        "}"
    );
    
    auto addItem = [&](int row, const QString& label, const QString& value, const QString& valueColor = "#2c3e50") {
        QTableWidgetItem *labelItem = new QTableWidgetItem(label);
        labelItem->setForeground(QColor("#7f8c8d"));
        labelItem->setFont(QFont("", 12, QFont::Bold));
        table->setItem(row, 0, labelItem);
        
        QTableWidgetItem *valueItem = new QTableWidgetItem(value);
        valueItem->setForeground(QColor(valueColor));
        valueItem->setFont(QFont("", 12, QFont::Bold));
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, 1, valueItem);
    };
    
    table->setRowCount(8);
    addItem(0, "剩余单位", QString::number(entry.remainingUnits));
    addItem(1, "基础金币", QString("+%1").arg(entry.baseGold), "#27ae60");
    if (entry.victoryGold > 0) {
        addItem(2, "胜利奖励", QString("+%1").arg(entry.victoryGold), "#27ae60");
    } else {
        addItem(2, "胜利奖励", "0", "#95a5a6");
    }
    addItem(3, "连胜/补偿", QString("%1%2").arg(entry.streakGold >= 0 ? "+" : "").arg(entry.streakGold), "#e67e22");
    addItem(4, "击杀", QString("%1%2").arg(entry.killGold >= 0 ? "+" : "").arg(entry.killGold), "#c0392b");
    addItem(5, "利息", QString("%1%2").arg(entry.interestGold >= 0 ? "+" : "").arg(entry.interestGold), "#3498db");
    addItem(6, "本回合总收入", QString("%1%2").arg(entry.totalGoldEarned >= 0 ? "+" : "").arg(entry.totalGoldEarned), "#2c3e50");
    addItem(7, "当前金币", QString::number(entry.currentGold), "#f39c12");
    
    if (entry.healthLost > 0) {
        table->setRowCount(9);
        addItem(8, "生命损失", QString("-%1").arg(entry.healthLost), "#e74c3c");
    }
    
    table->resizeColumnsToContents();
    table->setFixedHeight(table->rowCount() * 32 + 30);
    layout->addWidget(table);
    
    // 确定按钮
    QPushButton *okBtn = new QPushButton("确定");
    okBtn->setMinimumHeight(38);
    okBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    padding: 8px 24px;"
        "}"
        "QPushButton:hover { background-color: #2980b9; }"
        "QPushButton:pressed { background-color: #1f618d; }"
    );
    layout->addWidget(okBtn, 0, Qt::AlignCenter);
    
    connect(okBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::showGameOverDialog() {
    if (!game_ || !game_->isGameOver()) return;
    
    const Player &player = game_->getPlayer();
    const Player &enemy = game_->getEnemyPlayer();
    bool playerWon = game_->isPlayerWonGame();
    int totalRounds = game_->getCurrentRound();
    
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("游戏结束");
    dialog->setFixedSize(620, 560);
    dialog->setModal(true);
    
    QString borderColor = playerWon ? "#f39c12" : "#c0392b";
    QString bgColor = playerWon ? "#fef9e7" : "#fdf2f2";
    dialog->setStyleSheet(QString(
        "QDialog { background-color: %1; border: 3px solid %2; border-radius: 14px; }"
    ).arg(bgColor, borderColor));
    
    QVBoxLayout *rootLayout = new QVBoxLayout(dialog);
    rootLayout->setSpacing(8);
    rootLayout->setContentsMargins(20, 16, 20, 16);
    
    // === 结局标题 ===
    QString titleText = playerWon
        ? QString("🏆  敌方全灭 —— 第 %1 回合获胜！").arg(totalRounds)
        : QString("💀  生命耗尽 —— 第 %1 回合战败").arg(totalRounds);
    QString titleColor = playerWon ? "#e67e22" : "#c0392b";
    QLabel *titleLabel = new QLabel(titleText);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString(
        "QLabel { font-size: 20px; font-weight: bold; color: %1; padding: 6px; }"
    ).arg(titleColor));
    rootLayout->addWidget(titleLabel);
    
    // === 主标签页 ===
    QString tabStyle =
        "QTabWidget::pane { border: 1px solid rgba(0,0,0,0.1); border-radius: 8px; background: rgba(255,255,255,0.7); }"
        "QTabBar::tab {"
        "  background: rgba(0,0,0,0.06); color: #555; padding: 8px 18px; font-weight: bold; font-size: 13px;"
        "  border-top-left-radius: 6px; border-top-right-radius: 6px;"
        "  border: 1px solid rgba(0,0,0,0.08); border-bottom: none;"
        "}"
        "QTabBar::tab:selected { background: rgba(255,255,255,0.9); color: #2c3e50; border-bottom: 2px solid #3498db; }"
        "QTabBar::tab:hover:!selected { background: rgba(255,255,255,0.5); }";
    
    QTabWidget *mainTabs = new QTabWidget();
    mainTabs->setStyleSheet(tabStyle);
    
    // =================== 标签页1: 结算摘要 ===================
    QWidget *summaryTab = new QWidget();
    QVBoxLayout *summaryLayout = new QVBoxLayout(summaryTab);
    summaryLayout->setContentsMargins(16, 12, 16, 12);
    summaryLayout->setSpacing(10);
    
    QString verdictText = playerWon
        ? "🎉  你赢得了这场游戏！  🎉"
        : "游戏结束，请重新开始";
    QLabel *verdictLabel = new QLabel(verdictText);
    verdictLabel->setAlignment(Qt::AlignCenter);
    verdictLabel->setStyleSheet(QString(
        "QLabel { font-size: 18px; font-weight: bold; color: %1; padding: 6px; }"
    ).arg(playerWon ? "#e67e22" : "#c0392b"));
    summaryLayout->addWidget(verdictLabel);
    
    QTableWidget *table = new QTableWidget(5, 2);
    table->setHorizontalHeaderLabels({"项目", "数值"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(false);
    table->setStyleSheet(
        "QTableWidget { background-color: transparent; border: none; font-size: 13px; }"
        "QHeaderView::section { background: transparent; color: #2c3e50; font-weight: bold; border: none; padding: 6px 10px; font-size: 12px; }"
        "QTableWidget::item { padding: 5px 10px; border-bottom: 1px solid rgba(0,0,0,0.06); }"
    );
    
    auto addRow = [&](int row, const QString& label, const QString& value, const QString& valueColor = "#2c3e50") {
        QTableWidgetItem *labelItem = new QTableWidgetItem(label);
        labelItem->setForeground(QColor("#7f8c8d"));
        labelItem->setFont(QFont("", 12, QFont::Bold));
        table->setItem(row, 0, labelItem);
        QTableWidgetItem *valueItem = new QTableWidgetItem(value);
        valueItem->setForeground(QColor(valueColor));
        valueItem->setFont(QFont("", 12, QFont::Bold));
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, 1, valueItem);
    };
    
    addRow(0, "总回合数", QString::number(totalRounds));
    addRow(1, "我方剩余生命", QString::number(std::max(0, player.getHealth())),
        player.getHealth() > 0 ? "#27ae60" : "#e74c3c");
    addRow(2, "敌方剩余生命", QString::number(std::max(0, enemy.getHealth())),
        enemy.getHealth() > 0 ? "#e74c3c" : "#27ae60");
    addRow(3, "最终金币", QString::number(player.getGold()), "#f39c12");
    addRow(4, "最终等级", QString::number(player.getLevel()), "#3498db");
    
    table->resizeColumnsToContents();
    table->setFixedHeight(table->rowCount() * 34 + 30);
    summaryLayout->addWidget(table);
    summaryLayout->addStretch();
    mainTabs->addTab(summaryTab, "📋 结算");
    
    // =================== 标签页2: 战斗日志 ===================
    QWidget *logTab = new QWidget();
    QVBoxLayout *logLayout = new QVBoxLayout(logTab);
    logLayout->setContentsMargins(12, 8, 12, 8);
    
    QTextEdit *logEdit = new QTextEdit();
    logEdit->setReadOnly(true);
    logEdit->setStyleSheet(
        "QTextEdit {"
        "  background-color: rgba(0,0,0,0.05);"
        "  border: 1px solid rgba(0,0,0,0.1);"
        "  border-radius: 6px; padding: 8px 10px;"
        "  font-size: 12px; color: #2c3e50;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "}"
        "QScrollBar:vertical { background: rgba(0,0,0,0.05); width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: rgba(0,0,0,0.15); border-radius: 4px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );
    
    const auto& battleLog = game_->getBattleLog();
    QString logText;
    for (const auto& entry : battleLog) {
        logText += QString::fromStdString(entry.summary);
        if (!entry.equipmentDropped.empty()) {
            logText += QString("\n  [装备掉落] %1").arg(QString::fromStdString(entry.equipmentDropped));
        }
        logText += "\n\n";
    }
    if (logText.isEmpty()) logText = "（无战斗记录）";
    logEdit->setPlainText(logText);
    logLayout->addWidget(logEdit);
    mainTabs->addTab(logTab, "📜 战斗日志");
    
    // =================== 标签页3: 统计报表 ===================
    QWidget *statsTab = new QWidget();
    QVBoxLayout *statsLayout = new QVBoxLayout(statsTab);
    statsLayout->setContentsMargins(8, 8, 8, 8);
    
    QTabWidget *statsSubTabs = new QTabWidget();
    statsSubTabs->setStyleSheet(tabStyle);
    
    // 子标签: 支出/收入/净收入折线图
    auto makeChartTab = [&](const QString& title, const QColor& color, 
                            std::function<double(const BattleLogEntry&)> getter) -> QWidget* {
        QWidget *w = new QWidget();
        QVBoxLayout *l = new QVBoxLayout(w);
        l->setContentsMargins(4, 4, 4, 4);
        ChartWidget *chart = new ChartWidget(w);
        chart->setTitle(title);
        chart->setLineColor(color);
        chart->setValueSuffix(" 金");
        std::vector<ChartWidget::DataPoint> data;
        for (const auto& e : battleLog) {
            data.push_back({e.round, getter(e)});
        }
        chart->setData(data);
        l->addWidget(chart);
        return w;
    };
    
    statsSubTabs->addTab(
        makeChartTab("每局支出", QColor(231, 76, 60),
            [](const BattleLogEntry& e) { return (double)std::max(0, e.getGoldSpent()); }),
        "💰 支出");
    statsSubTabs->addTab(
        makeChartTab("每局收入", QColor(46, 204, 113),
            [](const BattleLogEntry& e) { return (double)e.totalGoldEarned; }),
        "📈 收入");
    statsSubTabs->addTab(
        makeChartTab("每局净收入", QColor(241, 196, 15),
            [](const BattleLogEntry& e) { return (double)e.getNetIncome(); }),
        "⚖ 净收入");
    
    // 子标签: 单位列表
    {
        QWidget *unitsTab = new QWidget();
        QVBoxLayout *ul = new QVBoxLayout(unitsTab);
        ul->setContentsMargins(4, 4, 4, 4);
        QTextEdit *unitsEdit = new QTextEdit();
        unitsEdit->setReadOnly(true);
        unitsEdit->setStyleSheet(logEdit->styleSheet());
        
        QString html;
        html += "<style>"
            " .us { margin-bottom:8px; } .ut { font-size:13px; font-weight:bold; color:#5dade2; margin-bottom:4px; }"
            " .uc { background:rgba(0,0,0,0.04); border-radius:6px; padding:4px 8px; margin-bottom:3px; }"
            " .un { color:#2c3e50; font-weight:bold; font-size:12px; }"
            " .us { color:#f39c12; font-size:11px; }"
            " .ui { color:rgba(0,0,0,0.5); font-size:11px; }"
            " .uemp { color:rgba(0,0,0,0.3); text-align:center; padding:16px; font-size:12px; }"
            " .usp { border:none; border-top:1px solid rgba(0,0,0,0.08); margin:6px 0; }"
            "</style>";
        
        auto appendUnitHtml = [&html](const std::shared_ptr<Unit>& u) {
            if (!u) return;
            QString stars; for (int s=0; s<u->getStarLevel(); ++s) stars += "⭐";
            html += "<div class='uc'>";
            html += QString("<div><span class='un'>%1</span> <span class='us'>%2</span></div>")
                .arg(QString::fromStdString(u->debugName())).arg(stars);
            html += QString("<div class='ui'>❤ %1  |  ⚔ %2  |  ⭐ %3</div>")
                .arg(u->getHP()).arg(u->getATK()).arg(u->getStarLevel());
            html += "</div>";
        };
        
        html += "<div class='us'><div class='ut'>⚔ 棋盘单位</div>";
        auto boardUnits = game_->getBoard().getUnitsForOwner(Owner::PlayerCtrl);
        if (boardUnits.empty()) { html += "<div class='uemp'>棋盘上没有单位</div>"; }
        else { for (auto& u : boardUnits) appendUnitHtml(u); }
        html += "</div><hr class='usp'>";
        
        html += "<div class='us'><div class='ut'>🪑 备战区单位</div>";
        const auto& bench = player.getBench();
        bool hasBench = false;
        for (int i = 0; i < BENCH_SIZE; ++i) {
            auto u = bench.getUnit(i);
            if (u) { hasBench = true; appendUnitHtml(u); }
        }
        if (!hasBench) html += "<div class='uemp'>备战区没有单位</div>";
        html += "</div>";
        
        unitsEdit->setHtml(html);
        ul->addWidget(unitsEdit);
        statsSubTabs->addTab(unitsTab, "👥 单位");
    }
    
    statsLayout->addWidget(statsSubTabs);
    mainTabs->addTab(statsTab, "📊 统计");
    
    rootLayout->addWidget(mainTabs, 1);
    
    // === 底部按钮 ===
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(16);
    
    QPushButton *newGameBtn = new QPushButton("🔄  新游戏");
    newGameBtn->setMinimumHeight(42);
    newGameBtn->setMinimumWidth(140);
    newGameBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 8px;"
        "  font-size: 15px; font-weight: bold; padding: 8px 20px; }"
        "QPushButton:hover { background-color: #219a52; }"
    );
    
    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setMinimumHeight(42);
    closeBtn->setMinimumWidth(100);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #95a5a6; color: white; border: none; border-radius: 8px;"
        "  font-size: 14px; padding: 8px 16px; }"
        "QPushButton:hover { background-color: #7f8c8d; }"
    );
    
    btnLayout->addStretch();
    btnLayout->addWidget(newGameBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    rootLayout->addLayout(btnLayout);
    
    connect(newGameBtn, &QPushButton::clicked, this, [this, dialog]() {
        dialog->accept();
        onNewGame();
    });
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::updateMusicPlayerPosition() {
    if (!musicPlayerWidget_ || !musicToggleBtn_) return;
    // 播放器定位在音乐按钮上方，右对齐
    int px = musicToggleBtn_->x() - musicPlayerWidget_->width() + musicToggleBtn_->width();
    int py = musicToggleBtn_->y() - musicPlayerWidget_->height() - 12;
    // 确保不超出左边界
    if (px < 10) px = 10;
    // 确保不超出上边界
    if (py < 10) py = musicToggleBtn_->y() + musicToggleBtn_->height() + 12;
    musicPlayerWidget_->move(px, py);
}

void MainWindow::onGameTick() {
    if (game_) {
        gameTickCounter_++;
        if (gameTickCounter_ % 2 == 0) {
            updateDisplay();
        }
    }
}

void MainWindow::onSaveGame() {
    try {
        QString savesPath = ensureSavesDir();

        QString defaultName = QString("save_round%1_%2.sav")
            .arg(game_->getCurrentRound())
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

        QString filename = QFileDialog::getSaveFileName(
            this,
            "存档",
            savesPath + "/" + defaultName,
            "存档文件 (*.sav)"
        );

        if (!filename.isEmpty() && game_) {
            if (game_->saveGame(filename.toStdString())) {
                QMessageBox::information(this, "保存成功", "存档已成功保存到 saves 目录。");
            } else {
                QMessageBox::warning(this, "保存失败", "无法保存存档，请检查文件路径。");
            }
        }
    } catch (const std::exception &) {
        QMessageBox::warning(this, "保存失败", "存档过程中发生错误，请重试。");
    } catch (...) {
        QMessageBox::warning(this, "保存失败", "存档过程中发生未知错误，请重试。");
    }
}

void MainWindow::onLoadGame() {
    // 如同点击设置按钮一样，转入显示存档的页面
    showLoadGame();
}

void MainWindow::onSettings() {
    showSettings();
}

void MainWindow::onHelp() {
    showHelp();
}

void MainWindow::onAbout() {
    showAbout();
}

void MainWindow::onExitApp() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认退出",
        "确定要退出游戏吗？",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QApplication::quit();
    }
}

void MainWindow::loadBackgrounds() {
    // 尝试加载SVG背景图，从可执行文件目录向上查找 assets/ui
    bgMainMenu_ = loadAssetPixmap("assets/ui/bg_mainmenu.svg");
    bgSettings_ = loadAssetPixmap("assets/ui/bg_settings.svg");
    bgHelp_ = loadAssetPixmap("assets/ui/bg_help.svg");
    bgAbout_ = loadAssetPixmap("assets/ui/bg_about.svg");

    // 使用默认启动尺寸创建背景图，实际显示时由 paintBgForWidget 缩放
    QSize defaultSize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
    if (bgMainMenu_.isNull()) {
        bgMainMenu_ = QPixmap(defaultSize);
        bgMainMenu_.fill(QColor("#0f1923"));
    }
    if (bgSettings_.isNull()) {
        bgSettings_ = QPixmap(defaultSize);
        bgSettings_.fill(QColor("#1a202c"));
    }
    if (bgHelp_.isNull()) {
        bgHelp_ = QPixmap(defaultSize);
        bgHelp_.fill(QColor("#1a365d"));
    }
    if (bgAbout_.isNull()) {
            bgAbout_ = QPixmap(defaultSize);
            bgAbout_.fill(QColor("#1a1a2e"));
        }
        if (bgLoadGame_.isNull()) {
            bgLoadGame_ = QPixmap(defaultSize);
            bgLoadGame_.fill(QColor("#1a202c"));
        }
}

// ============ 开场动画（StartScreen）功能 ============

void MainWindow::setupStartScreen() {
    startScreenWidget_ = new QWidget(centralWidget_);
    startScreenWidget_->setStyleSheet("background-color: #000000;");
    startScreenWidget_->setMouseTracking(true);
    startScreenWidget_->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    QVBoxLayout *mainLayout = new QVBoxLayout(startScreenWidget_);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(60, 80, 60, 60);

    // 顶部弹性空间
    mainLayout->addStretch(2);

    // ===== 标题 "xxx" =====
    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(8);
    titleLayout->setAlignment(Qt::AlignCenter);

    startTitleLabel_ = new QLabel("xxx", startScreenWidget_);
    startTitleLabel_->setAlignment(Qt::AlignCenter);
    startTitleLabel_->setStyleSheet(
        "QLabel {"
        "   font-size: 72px;"
        "   font-weight: bold;"
        "   color: #ffffff;"
        "   letter-spacing: 12px;"
        "   background: transparent;"
        "}"
    );
    startTitleOpacity_ = new QGraphicsOpacityEffect(startTitleLabel_);
    startTitleOpacity_->setOpacity(0.0);
    startTitleLabel_->setGraphicsEffect(startTitleOpacity_);
    titleLayout->addWidget(startTitleLabel_);

    // ===== 副标题/装饰文字 =====
    startSubtitleLabel_ = new QLabel("✦ 敬请期待 ✦", startScreenWidget_);
    startSubtitleLabel_->setAlignment(Qt::AlignCenter);
    startSubtitleLabel_->setStyleSheet(
        "QLabel {"
        "   font-size: 20px;"
        "   color: rgba(255, 255, 255, 0.7);"
        "   letter-spacing: 6px;"
        "   background: transparent;"
        "   margin-top: 4px;"
        "}"
    );
    startSubtitleOpacity_ = new QGraphicsOpacityEffect(startSubtitleLabel_);
    startSubtitleOpacity_->setOpacity(0.0);
    startSubtitleLabel_->setGraphicsEffect(startSubtitleOpacity_);
    titleLayout->addWidget(startSubtitleLabel_);

    mainLayout->addLayout(titleLayout);

    // 中间弹性空间
    mainLayout->addStretch(3);

    // ===== 底部提示文字 =====
    startHintLabel_ = new QLabel("点击任意处进入游戏...", startScreenWidget_);
    startHintLabel_->setAlignment(Qt::AlignCenter);
    startHintLabel_->setStyleSheet(
        "QLabel {"
        "   font-size: 14px;"
        "   color: rgba(255, 255, 255, 0.5);"
        "   background: transparent;"
        "   margin-bottom: 20px;"
        "}"
    );
    startHintOpacity_ = new QGraphicsOpacityEffect(startHintLabel_);
    startHintOpacity_->setOpacity(0.0);
    startHintLabel_->setGraphicsEffect(startHintOpacity_);
    mainLayout->addWidget(startHintLabel_);

    // ===== 跳过按钮 =====
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, BOTTOM_MARGIN_R, BOTTOM_MARGIN_B);
    bottomLayout->addStretch();

    startSkipBtn_ = new QPushButton("跳过 ▶", startScreenWidget_);
    startSkipBtn_->setFixedSize(100, 32);
    startSkipBtn_->setCursor(Qt::PointingHandCursor);
    startSkipBtn_->setStyleSheet(
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
    connect(startSkipBtn_, &QPushButton::clicked, this, &MainWindow::onStartSkipClicked);
    bottomLayout->addWidget(startSkipBtn_);

    mainLayout->addLayout(bottomLayout);

    // 加载开场背景
    loadStartBackground();

    // 初始化开场动画
    setupStartAnimations();

    // 初始化发光动画定时器
    startGlowTimer_ = new QTimer(this);
    startGlowTimer_->setInterval(33); // ~30fps 的发光动画
    connect(startGlowTimer_, &QTimer::timeout, this, &MainWindow::onStartGlowUpdate);

    // 自动跳转定时器（5秒后如果没有操作自动跳转）
    startAutoAdvanceTimer_ = new QTimer(this);
    startAutoAdvanceTimer_->setSingleShot(true);
    startAutoAdvanceTimer_->setInterval(TIMER_AUTO_ADVANCE_MS);
    connect(startAutoAdvanceTimer_, &QTimer::timeout, this, &MainWindow::onStartAutoAdvance);
}

void MainWindow::setupStartAnimations() {
    // ===== 标题淡入动画 (0.5s延迟, 1.5s持续) =====
    startTitleFadeIn_ = new QPropertyAnimation(startTitleOpacity_, "opacity", this);
    startTitleFadeIn_->setDuration(TITLE_FADEIN_DURATION_MS);
    startTitleFadeIn_->setStartValue(0.0);
    startTitleFadeIn_->setEndValue(1.0);
    startTitleFadeIn_->setEasingCurve(QEasingCurve::OutCubic);

    // ===== 副标题淡入动画 (1.2s延迟, 1.0s持续) =====
    startSubtitleFadeIn_ = new QPropertyAnimation(startSubtitleOpacity_, "opacity", this);
    startSubtitleFadeIn_->setDuration(SUBTITLE_FADEIN_DURATION_MS);
    startSubtitleFadeIn_->setStartValue(0.0);
    startSubtitleFadeIn_->setEndValue(1.0);
    startSubtitleFadeIn_->setEasingCurve(QEasingCurve::OutCubic);

    // ===== 提示文字淡入动画 (2.5s延迟, 1.0s持续) =====
    startHintFadeIn_ = new QPropertyAnimation(startHintOpacity_, "opacity", this);
    startHintFadeIn_->setDuration(HINT_FADEIN_DURATION_MS);
    startHintFadeIn_->setStartValue(0.0);
    startHintFadeIn_->setEndValue(1.0);
    startHintFadeIn_->setEasingCurve(QEasingCurve::OutCubic);

    // 连接动画完成信号
    connect(startTitleFadeIn_, &QPropertyAnimation::finished, this, &MainWindow::onStartFadeInFinished);
}

void MainWindow::loadStartBackground() {
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
        startBgCache_ = renderSvgToPixmap(svgPath, size());
        if (!startBgCache_.isNull()) {
            startBgLoaded_ = true;
        }
    }

    // 如果加载失败，创建纯色渐变背景
    if (!startBgLoaded_) {
        startBgCache_ = QPixmap(size());
        startBgCache_.fill(QColor("#0a0a1a"));
        QPainter painter(&startBgCache_);
        QLinearGradient gradient(0, 0, width(), height());
        gradient.setColorAt(0.0, QColor("#0f0c29"));
        gradient.setColorAt(0.5, QColor("#302b63"));
        gradient.setColorAt(1.0, QColor("#24243e"));
        painter.fillRect(rect(), gradient);
        painter.end();
    }
}

void MainWindow::drawStartBackground(QPainter &painter) {
    // 直接绘制缓存的背景图像
    if (!startBgCache_.isNull()) {
        painter.drawPixmap(0, 0, startBgCache_);
    }
}

void MainWindow::startIntroAnimation() {
    if (startAnimationStarted_) return;
    startAnimationStarted_ = true;

    // 1. 标题淡入（延迟0.5秒）
    QTimer::singleShot(INTRO_TITLE_DELAY_MS, this, [this]() {
        startTitleFadeIn_->start();
    });

    // 2. 副标题淡入（延迟1.2秒）
    QTimer::singleShot(INTRO_SUBTITLE_DELAY_MS, this, [this]() {
        startSubtitleFadeIn_->start();
    });

    // 3. 开始发光动画（延迟2.0秒）
    QTimer::singleShot(INTRO_GLOW_DELAY_MS, this, [this]() {
        startGlowTimer_->start();
    });

    // 4. 提示文字淡入（延迟2.5秒）
    QTimer::singleShot(INTRO_HINT_DELAY_MS, this, [this]() {
        startHintFadeIn_->start();
    });

    // 5. 启动自动跳转计时器
    startAutoAdvanceTimer_->start();

    // 触发首次重绘
    update();
}

void MainWindow::onStartFadeInFinished() {
    // 标题淡入完成后，可以做一些额外效果
}

void MainWindow::onStartGlowUpdate() {
    // 更新发光强度（呼吸灯效果）
    if (startGlowIncreasing_) {
        startGlowIntensity_ += 0.03;
        if (startGlowIntensity_ >= 1.0) {
            startGlowIntensity_ = 1.0;
            startGlowIncreasing_ = false;
        }
    } else {
        startGlowIntensity_ -= 0.03;
        if (startGlowIntensity_ <= 0.3) {
            startGlowIntensity_ = 0.3;
            startGlowIncreasing_ = true;
        }
    }

    // 触发重绘，在paintEvent中绘制发光效果
    update();
}

void MainWindow::onStartAutoAdvance() {
    if (startIsTransitioning_) return;
    startIsTransitioning_ = true;

    // 停止所有定时器
    startGlowTimer_->stop();
    startAutoAdvanceTimer_->stop();

    // 先清除可能存在的全局graphics effect
    startScreenWidget_->setGraphicsEffect(nullptr);

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
    group->addAnimation(fadeOutWidget(startTitleLabel_));
    group->addAnimation(fadeOutWidget(startSubtitleLabel_));
    group->addAnimation(fadeOutWidget(startHintLabel_));
    group->addAnimation(fadeOutWidget(startSkipBtn_));

    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        // 动画完成后切换到主菜单
        showMainMenu();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onStartSkipClicked() {
    onStartAutoAdvance();
}

QWidget* MainWindow::createBgWidget(const QPixmap& bg, QWidget* parent) {
    QWidget* w = new QWidget(parent);
    w->setStyleSheet(QString("QWidget { background: transparent; }"));
    return w;
}

void MainWindow::paintBgForWidget(QWidget* w, const QPixmap& bg) {
    // 用 QLabel 作为背景层
    QLabel* bgLabel = w->findChild<QLabel*>("__bg_label__");
    if (!bgLabel) {
        bgLabel = new QLabel(w);
        bgLabel->setObjectName("__bg_label__");
        bgLabel->setGeometry(w->rect());
        bgLabel->setScaledContents(true);
        bgLabel->lower();
    }
    if (!bg.isNull()) {
        // 如果尺寸无效（布局未完成），使用父窗口或默认尺寸
        QSize targetSize = w->size();
        if (!targetSize.isValid() || targetSize.width() < 100 || targetSize.height() < 100) {
            targetSize = centralWidget_ ? centralWidget_->size() : QSize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
        }
        bgLabel->setPixmap(QPixmap::fromImage(bg.toImage().scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    } else {
        bgLabel->setStyleSheet("background-color: transparent;");
    }
    bgLabel->setGeometry(QRect(QPoint(0, 0), 
        w->size().isValid() && w->size().width() >= 100 ? w->size() : 
        (centralWidget_ ? centralWidget_->size() : QSize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT))));
    bgLabel->lower();
}

void MainWindow::showSettings() {
    if (!settingsWidget_) {
        settingsWidget_ = new QWidget(centralWidget_);
        QVBoxLayout *layout = new QVBoxLayout(settingsWidget_);
        layout->setSpacing(LAYOUT_SPACING_WIDE);
        layout->setContentsMargins(SETTINGS_MARGIN_LR, SETTINGS_MARGIN_TB, SETTINGS_MARGIN_LR, SETTINGS_MARGIN_B);

        QLabel *title = new QLabel("设置");
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("QLabel { font-size: 28px; font-weight: bold; color: #ffffff; margin-bottom: 10px; }");
        layout->addWidget(title);

        // 通用滑动条行样式
        QString rowStyle = "QWidget { background-color: rgba(255,255,255,0.06); border-radius: 10px; padding: 12px 18px; }";
        QString labelStyle = "QLabel { font-size: 15px; color: #e2e8f0; font-weight: bold; }";
        QString valueStyle = "QLabel { font-size: 14px; color: #f1c40f; font-weight: bold; min-width: 40px; }";
        QString sliderStyle =
            "QSlider::groove:horizontal { background: rgba(255,255,255,0.14); height: 8px; border-radius: 4px; }"
            "QSlider::handle:horizontal { background: #ffffff; width: 16px; border-radius: 8px; margin: -4px 0; }"
            "QSlider::handle:horizontal:hover { background: #f1c40f; }";
        QString descStyle = "QLabel { font-size: 12px; color: #94a3b8; padding-left: 2px; }";

        // ======== 音乐音量 ========
        QWidget *musicRow = new QWidget();
        musicRow->setStyleSheet(rowStyle);
        QVBoxLayout *musicRowLayout = new QVBoxLayout(musicRow);
        musicRowLayout->setSpacing(6);
        musicRowLayout->setContentsMargins(0, 0, 0, 0);

        QWidget *musicHeader = new QWidget();
        QHBoxLayout *musicHeaderLayout = new QHBoxLayout(musicHeader);
        musicHeaderLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *musicLabel = new QLabel("音乐音量");
        {
            QPixmap px = loadAssetPixmap("assets/button/btn_music.svg");
            if (!px.isNull()) musicLabel->setPixmap(px.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        musicLabel->setStyleSheet(labelStyle);
        musicVolumeValueLabel_ = new QLabel("65%");
        musicVolumeValueLabel_->setStyleSheet(valueStyle);
        musicHeaderLayout->addWidget(musicLabel);
        musicHeaderLayout->addStretch();
        musicHeaderLayout->addWidget(musicVolumeValueLabel_);
        musicRowLayout->addWidget(musicHeader);

        musicVolumeSlider_ = new QSlider(Qt::Horizontal);
        musicVolumeSlider_->setRange(0, 100);
        musicVolumeSlider_->setValue(65);
        musicVolumeSlider_->setStyleSheet(sliderStyle);
        musicRowLayout->addWidget(musicVolumeSlider_);

        QLabel *musicDesc = new QLabel("控制背景音乐的音量大小");
        musicDesc->setStyleSheet(descStyle);
        musicRowLayout->addWidget(musicDesc);

        layout->addWidget(musicRow);

        // ======== 音效音量 ========
        QWidget *sfxRow = new QWidget();
        sfxRow->setStyleSheet(rowStyle);
        QVBoxLayout *sfxRowLayout = new QVBoxLayout(sfxRow);
        sfxRowLayout->setSpacing(6);
        sfxRowLayout->setContentsMargins(0, 0, 0, 0);

        QWidget *sfxHeader = new QWidget();
        QHBoxLayout *sfxHeaderLayout = new QHBoxLayout(sfxHeader);
        sfxHeaderLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *sfxLabel = new QLabel("音效音量");
        {
            QPixmap px = loadAssetPixmap("assets/button/btn_combat.svg");
            if (!px.isNull()) sfxLabel->setPixmap(px.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        sfxLabel->setStyleSheet(labelStyle);
        sfxVolumeValueLabel_ = new QLabel("80%");
        sfxVolumeValueLabel_->setStyleSheet(valueStyle);
        sfxHeaderLayout->addWidget(sfxLabel);
        sfxHeaderLayout->addStretch();
        sfxHeaderLayout->addWidget(sfxVolumeValueLabel_);
        sfxRowLayout->addWidget(sfxHeader);

        sfxVolumeSlider_ = new QSlider(Qt::Horizontal);
        sfxVolumeSlider_->setRange(0, 100);
        sfxVolumeSlider_->setValue(80);
        sfxVolumeSlider_->setStyleSheet(sliderStyle);
        sfxRowLayout->addWidget(sfxVolumeSlider_);

        QLabel *sfxDesc = new QLabel("控制战斗音效、点击反馈等效果音量");
        sfxDesc->setStyleSheet(descStyle);
        sfxRowLayout->addWidget(sfxDesc);

        layout->addWidget(sfxRow);

        // ======== 屏幕亮度 ========
        QWidget *brightnessRow = new QWidget();
        brightnessRow->setStyleSheet(rowStyle);
        QVBoxLayout *brightnessRowLayout = new QVBoxLayout(brightnessRow);
        brightnessRowLayout->setSpacing(6);
        brightnessRowLayout->setContentsMargins(0, 0, 0, 0);

        QWidget *brightnessHeader = new QWidget();
        QHBoxLayout *brightnessHeaderLayout = new QHBoxLayout(brightnessHeader);
        brightnessHeaderLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *brightnessLabel = new QLabel("屏幕亮度");
        brightnessLabel->setStyleSheet(labelStyle);
        {
            QPixmap px = loadAssetPixmap("assets/button/ico_brightness.svg");
            if (!px.isNull()) brightnessLabel->setPixmap(px.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        brightnessValueLabel_ = new QLabel("100%");
        brightnessValueLabel_->setStyleSheet(valueStyle);
        brightnessHeaderLayout->addWidget(brightnessLabel);
        brightnessHeaderLayout->addStretch();
        brightnessHeaderLayout->addWidget(brightnessValueLabel_);
        brightnessRowLayout->addWidget(brightnessHeader);

        brightnessSlider_ = new QSlider(Qt::Horizontal);
        brightnessSlider_->setRange(30, 100);
        brightnessSlider_->setValue(100);
        brightnessSlider_->setStyleSheet(sliderStyle);
        brightnessRowLayout->addWidget(brightnessSlider_);

        QLabel *brightnessDesc = new QLabel("调整游戏画面的整体亮度（仅视觉效果）");
        brightnessDesc->setStyleSheet(descStyle);
        brightnessRowLayout->addWidget(brightnessDesc);

        layout->addWidget(brightnessRow);

        // ======== 显示 AI 信息开关 ========
        QWidget *aiInfoRow = new QWidget();
        aiInfoRow->setStyleSheet(rowStyle);
        QVBoxLayout *aiInfoRowLayout = new QVBoxLayout(aiInfoRow);
        aiInfoRowLayout->setSpacing(6);
        aiInfoRowLayout->setContentsMargins(0, 0, 0, 0);

        QWidget *aiInfoHeader = new QWidget();
        QHBoxLayout *aiInfoHeaderLayout = new QHBoxLayout(aiInfoHeader);
        aiInfoHeaderLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *aiInfoLabel = new QLabel("🤖 显示 AI 信息");
        aiInfoLabel->setStyleSheet(labelStyle);
        aiInfoHeaderLayout->addWidget(aiInfoLabel);
        aiInfoHeaderLayout->addStretch();

        // 复选框
        QCheckBox *aiInfoCheckbox = new QCheckBox();
        aiInfoCheckbox->setChecked(game_ ? game_->getShowAiInfo() : false);
        aiInfoCheckbox->setStyleSheet(
            "QCheckBox { spacing: 8px; }"
            "QCheckBox::indicator { width: 20px; height: 20px; border-radius: 4px; border: 2px solid rgba(255,255,255,0.3); }"
            "QCheckBox::indicator:checked { background-color: #e74c3c; border: 2px solid #e74c3c; }"
            "QCheckBox::indicator:unchecked { background-color: transparent; }"
            "QCheckBox::indicator:hover { border: 2px solid rgba(255,255,255,0.6); }"
        );
        aiInfoHeaderLayout->addWidget(aiInfoCheckbox);
        aiInfoRowLayout->addWidget(aiInfoHeader);

        QLabel *aiInfoDesc = new QLabel("开启后将在战斗日志中显示敌方 AI 的决策过程，并在棋盘上显示敌方单位的行动意图");
        aiInfoDesc->setStyleSheet(descStyle);
        aiInfoDesc->setWordWrap(true);
        aiInfoRowLayout->addWidget(aiInfoDesc);

        layout->addWidget(aiInfoRow);

        // ======== 快速购买开关 ========
        QWidget *quickBuyRow = new QWidget();
        quickBuyRow->setStyleSheet(rowStyle);
        QVBoxLayout *quickBuyRowLayout = new QVBoxLayout(quickBuyRow);
        quickBuyRowLayout->setSpacing(6);
        quickBuyRowLayout->setContentsMargins(0, 0, 0, 0);

        QWidget *quickBuyHeader = new QWidget();
        QHBoxLayout *quickBuyHeaderLayout = new QHBoxLayout(quickBuyHeader);
        quickBuyHeaderLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *quickBuyLabel = new QLabel("⚡ 快速购买");
        quickBuyLabel->setStyleSheet(labelStyle);
        quickBuyHeaderLayout->addWidget(quickBuyLabel);
        quickBuyHeaderLayout->addStretch();

        QCheckBox *quickBuyCheckbox = new QCheckBox();
        quickBuyCheckbox->setChecked(quickBuyEnabled_);
        quickBuyCheckbox->setStyleSheet(
            "QCheckBox { spacing: 8px; }"
            "QCheckBox::indicator { width: 20px; height: 20px; border-radius: 4px; border: 2px solid rgba(255,255,255,0.3); }"
            "QCheckBox::indicator:checked { background-color: #27ae60; border: 2px solid #27ae60; }"
            "QCheckBox::indicator:unchecked { background-color: transparent; }"
            "QCheckBox::indicator:hover { border: 2px solid rgba(255,255,255,0.6); }"
        );
        quickBuyHeaderLayout->addWidget(quickBuyCheckbox);
        quickBuyRowLayout->addWidget(quickBuyHeader);

        QLabel *quickBuyDesc = new QLabel("开启后购买单位时将跳过确认弹窗，直接购买");
        quickBuyDesc->setStyleSheet(descStyle);
        quickBuyDesc->setWordWrap(true);
        quickBuyRowLayout->addWidget(quickBuyDesc);

        layout->addWidget(quickBuyRow);

        connect(quickBuyCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            quickBuyEnabled_ = checked;
        });

        layout->addStretch();

        settingsBackBtn_ = new QPushButton("\u2190 返回主菜单");
        settingsBackBtn_->setFocusPolicy(Qt::TabFocus);
        settingsBackBtn_->setMinimumSize(220, 48);
        settingsBackBtn_->setStyleSheet(
            "QPushButton { "
            "    background-color: #7f8c8d; "
            "    color: white; "
            "    border: 2px solid #6c7a7d; "
            "    border-radius: 8px; "
            "    font-size: 16px; "
            "    font-weight: bold; "
            "} "
            "QPushButton:hover { background-color: #95a5a6; } "
            "QPushButton:pressed { background-color: #6c7a7d; }"
        );
        layout->addWidget(settingsBackBtn_, 0, Qt::AlignCenter);

        settingsWidget_->setStyleSheet("QWidget { background: transparent; }");
        centralStack_->addWidget(settingsWidget_);

        connect(settingsBackBtn_, &QPushButton::clicked, this, &MainWindow::onBackToMenu);
        connect(musicVolumeSlider_, &QSlider::valueChanged, this, &MainWindow::onMusicVolumeChanged);
        connect(sfxVolumeSlider_, &QSlider::valueChanged, this, &MainWindow::onSfxVolumeChanged);
        connect(brightnessSlider_, &QSlider::valueChanged, this, &MainWindow::onBrightnessChanged);
        
        // AI 信息开关
        connect(aiInfoCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            if (game_) {
                game_->setShowAiInfo(checked);
                updateDisplay();
            }
        });
    }
    paintBgForWidget(settingsWidget_, bgSettings_);
        centralStack_->setCurrentWidget(settingsWidget_);
        // 强制用 centralWidget_ 的尺寸更新背景标签（确保尺寸正确）
        QLabel* bgLabel = settingsWidget_->findChild<QLabel*>("__bg_label__");
        if (bgLabel && centralWidget_) {
            bgLabel->setGeometry(centralWidget_->rect());
            if (!bgSettings_.isNull()) {
                bgLabel->setPixmap(QPixmap::fromImage(bgSettings_.toImage().scaled(centralWidget_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            }
        }
    }

// 从 .md 文件加载并创建带滚动区域的内容标签页
static QWidget* createHelpTabFromMd(const QString &mdRelativePath, QWidget *parent = nullptr) {
    QScrollArea *scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical {"
        "    background: rgba(255,255,255,0.05); width: 8px; border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(255,255,255,0.2); border-radius: 4px; min-height: 30px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );

    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setSpacing(8);
    layout->setContentsMargins(24, 28, 24, 28);
    container->setStyleSheet("background-color: rgba(0,0,0,0.35); border-radius: 12px;");

    // 从 .md 文件读取
    QString mdPath = loadAssetFile(mdRelativePath);
    if (mdPath.isEmpty()) {
        QLabel *err = new QLabel("未找到文件: " + mdRelativePath);
        err->setStyleSheet("color: #fc8181;");
        layout->addWidget(err);
        scroll->setWidget(container);
        return scroll;
    }
    QFile file(mdPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QLabel *err = new QLabel("无法读取文件: " + mdRelativePath);
        err->setStyleSheet("color: #fc8181;");
        layout->addWidget(err);
        scroll->setWidget(container);
        return scroll;
    }

    QString mdContent = QString::fromUtf8(file.readAll());
    file.close();

    // 处理 markdown → HTML + 提取图片
    QString html;
    QStringList lines = mdContent.split('\n');
    bool inCodeBlock = false;

    for (const QString &line : lines) {
        if (line.trimmed().startsWith("```")) {
            inCodeBlock = !inCodeBlock;
            continue;
        }
        if (inCodeBlock) continue;

        // 检测图片行 ![...](...)
        QRegularExpression imgRe("!\\[([^\\]]*)\\]\\(([^)]+)\\)");
        auto imgMatch = imgRe.match(line);
        if (imgMatch.hasMatch()) {
            QString imgPath = loadAssetFile(imgMatch.captured(2));
            if (!imgPath.isEmpty()) {
                QLabel *imgLabel = new QLabel();
                QPixmap px(imgPath);
                if (!px.isNull()) {
                    imgLabel->setPixmap(px.scaledToWidth(400, Qt::SmoothTransformation));
                    imgLabel->setAlignment(Qt::AlignCenter);
                    layout->addWidget(imgLabel);
                }
            }
            continue;
        }

        // 其余内容走 HTML
        html += line + "\n";
    }

    html = markdownToHtml(html);
    QLabel *textLabel = new QLabel(html);
    textLabel->setWordWrap(true);
    textLabel->setTextFormat(Qt::RichText);
    textLabel->setStyleSheet("QLabel { font-size: 15px; color: #e2e8f0; background: transparent; }");
    layout->addWidget(textLabel);

    layout->addStretch();
    scroll->setWidget(container);
    return scroll;
}

void MainWindow::showHelp() {
    if (!helpWidget_) {
        helpWidget_ = new QWidget(centralWidget_);
        QVBoxLayout *layout = new QVBoxLayout(helpWidget_);
        layout->setSpacing(16);
        layout->setContentsMargins(60, 40, 60, 60);

        QLabel *title = new QLabel("帮助");
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("QLabel { font-size: 28px; font-weight: bold; color: #ffffff; }");
        layout->addWidget(title);

        // ============ 主标签页 ============
        QTabWidget *mainTabs = new QTabWidget();
        mainTabs->setStyleSheet(
            "QTabWidget::pane {"
            "    border: none;"
            "    background: transparent;"
            "}"
            "QTabBar::tab {"
            "    background: rgba(0,0,0,0.35);"
            "    color: #94a3b8;"
            "    padding: 10px 24px;"
            "    margin-right: 4px;"
            "    border-top-left-radius: 8px;"
            "    border-top-right-radius: 8px;"
            "    font-size: 15px;"
            "    font-weight: bold;"
            "}"
            "QTabBar::tab:selected {"
            "    background: rgba(0,0,0,0.55);"
            "    color: #fbbf24;"
            "}"
            "QTabBar::tab:hover {"
            "    background: rgba(0,0,0,0.45);"
            "    color: #e2e8f0;"
            "}"
        );

        // ---------- 标签页1: 游戏总体介绍 (来自 markdowns/help_intro.md) ----------
        mainTabs->addTab(createHelpTabFromMd("markdowns/help_intro.md", mainTabs), "📖 游戏总体介绍");

        // ---------- 标签页2: 角色介绍 (9个子标签页, 每个来自 markdowns/characters/*.md) ----------
        {
            QTabWidget *charTabs = new QTabWidget();
            charTabs->setStyleSheet(
                "QTabWidget::pane {"
                "    border: none;"
                "    background: transparent;"
                "}"
                "QTabBar::tab {"
                "    background: rgba(0,0,0,0.30);"
                "    color: #94a3b8;"
                "    padding: 8px 16px;"
                "    margin-right: 2px;"
                "    border-top-left-radius: 6px;"
                "    border-top-right-radius: 6px;"
                "    font-size: 13px;"
                "}"
                "QTabBar::tab:selected {"
                "    background: rgba(0,0,0,0.50);"
                "    color: #fbbf24;"
                "}"
                "QTabBar::tab:hover {"
                "    background: rgba(0,0,0,0.40);"
                "    color: #e2e8f0;"
                "}"
            );

            // 9个角色: {md文件名, 标签显示名}
            struct { QString file; QString label; } charDefs[] = {
                {"markdowns/characters/reimu.md",  "霊夢"},
                {"markdowns/characters/marisa.md", "魔理沙"},
                {"markdowns/characters/sanae.md",  "早苗"},
                {"markdowns/characters/sakuya.md", "咲夜"},
                {"markdowns/characters/youmu.md",  "妖夢"},
                {"markdowns/characters/alice.md",  "アリス"},
                {"markdowns/characters/koishi.md", "こいし"},
                {"markdowns/characters/aya.md",    "文"},
                {"markdowns/characters/hatate.md", "はたて"},
            };

            for (const auto &cd : charDefs) {
                charTabs->addTab(createHelpTabFromMd(cd.file, charTabs), cd.label);
            }

            mainTabs->addTab(charTabs, "👤 角色介绍");
        }

        // ---------- 标签页3: 游戏操作步骤 (来自 markdowns/help_operation.md) ----------
        mainTabs->addTab(createHelpTabFromMd("markdowns/help_operation.md", mainTabs), "🎮 游戏操作步骤");

        layout->addWidget(mainTabs, 1);

        helpBackBtn_ = new QPushButton("\u2190 返回主菜单");
        helpBackBtn_->setFocusPolicy(Qt::TabFocus);
        helpBackBtn_->setMinimumSize(220, 48);
        helpBackBtn_->setStyleSheet(
            "QPushButton { "
            "    background-color: #7f8c8d; "
            "    color: white; "
            "    border: 2px solid #6c7a7d; "
            "    border-radius: 8px; "
            "    font-size: 16px; "
            "    font-weight: bold; "
            "} "
            "QPushButton:hover { background-color: #95a5a6; } "
            "QPushButton:pressed { background-color: #6c7a7d; }"
        );
        layout->addWidget(helpBackBtn_, 0, Qt::AlignCenter);

        helpWidget_->setStyleSheet("QWidget { background: transparent; }");
        centralStack_->addWidget(helpWidget_);

        connect(helpBackBtn_, &QPushButton::clicked, this, &MainWindow::onBackToMenu);
    }
    paintBgForWidget(helpWidget_, bgHelp_);
    centralStack_->setCurrentWidget(helpWidget_);
    // 强制用 centralWidget_ 的尺寸更新背景标签（确保尺寸正确）
    QLabel* bgLabel = helpWidget_->findChild<QLabel*>("__bg_label__");
    if (bgLabel && centralWidget_) {
        bgLabel->setGeometry(centralWidget_->rect());
        if (!bgHelp_.isNull()) {
            bgLabel->setPixmap(QPixmap::fromImage(bgHelp_.toImage().scaled(centralWidget_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
    }
}

void MainWindow::showAbout() {
    if (!aboutWidget_) {
        aboutWidget_ = new QWidget(centralWidget_);
        QVBoxLayout *layout = new QVBoxLayout(aboutWidget_);
        layout->setSpacing(16);
        layout->setContentsMargins(60, 40, 60, 60);

        QLabel *title = new QLabel("关于");
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("QLabel { font-size: 28px; font-weight: bold; color: #ffffff; }");
        layout->addWidget(title);

        // 使用 QScrollArea 包裹内容，支持滚动
        QScrollArea *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet(
            "QScrollArea { border: none; background: transparent; }"
            "QScrollBar:vertical {"
            "    background: rgba(255,255,255,0.05);"
            "    width: 8px;"
            "    border-radius: 4px;"
            "}"
            "QScrollBar::handle:vertical {"
            "    background: rgba(255,255,255,0.2);"
            "    border-radius: 4px;"
            "    min-height: 30px;"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "    height: 0px;"
            "}"
        );

        QLabel *content = new QLabel();
        content->setWordWrap(true);
        content->setTextFormat(Qt::RichText);
        content->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        content->setStyleSheet(
            "QLabel {"
            "    font-size: 15px;"
            "    color: #e2e8f0;"
            "    padding: 24px 28px;"
            "    background-color: rgba(0,0,0,0.35);"
            "    border-radius: 12px;"
            "    line-height: 1.6;"
            "}"
            "QLabel a { color: #63b3ed; }"
        );

        // 从 markdowns/about.md 读取内容
        QString mdPath = loadAssetFile("markdowns/about.md");
        if (!mdPath.isEmpty()) {
            QFile file(mdPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString mdContent = QString::fromUtf8(file.readAll());
                file.close();
                QString html = markdownToHtml(mdContent);
                // 包装成完整 HTML 文档，使样式更可控
                html = "<html><body style='font-size:15px; color:#e2e8f0; line-height:1.7;'>"
                       + html + "</body></html>";
                content->setText(html);
            } else {
                content->setText("<p style='color:#fc8181;'>无法读取关于文件。</p>");
            }
        } else {
            content->setText("<p style='color:#fc8181;'>未找到 markdowns/about.md</p>");
        }

        scrollArea->setWidget(content);
        layout->addWidget(scrollArea, 1);

        aboutBackBtn_ = new QPushButton("\u2190 返回主菜单");
        aboutBackBtn_->setFocusPolicy(Qt::TabFocus);
        aboutBackBtn_->setMinimumSize(220, 48);
        aboutBackBtn_->setStyleSheet(
            "QPushButton { "
            "    background-color: #7f8c8d; "
            "    color: white; "
            "    border: 2px solid #6c7a7d; "
            "    border-radius: 8px; "
            "    font-size: 16px; "
            "    font-weight: bold; "
            "} "
            "QPushButton:hover { background-color: #95a5a6; } "
            "QPushButton:pressed { background-color: #6c7a7d; }"
        );
        layout->addWidget(aboutBackBtn_, 0, Qt::AlignCenter);

        aboutWidget_->setStyleSheet("QWidget { background: transparent; }");
        centralStack_->addWidget(aboutWidget_);

        connect(aboutBackBtn_, &QPushButton::clicked, this, &MainWindow::onBackToMenu);
    }
    paintBgForWidget(aboutWidget_, bgAbout_);
    centralStack_->setCurrentWidget(aboutWidget_);
    // 强制用 centralWidget_ 的尺寸更新背景标签（确保尺寸正确）
    QLabel* bgLabel = aboutWidget_->findChild<QLabel*>("__bg_label__");
    if (bgLabel && centralWidget_) {
        bgLabel->setGeometry(centralWidget_->rect());
        if (!bgAbout_.isNull()) {
            bgLabel->setPixmap(QPixmap::fromImage(bgAbout_.toImage().scaled(centralWidget_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
    }
}

void MainWindow::onLoadGameSlot() {
    try {
        // 从列表中选择存档并加载
        QListWidgetItem *item = saveListWidget_->currentItem();
        if (!item) {
            QMessageBox::information(this, "提示", "请先选择一个存档。");
            return;
        }

        QString filepath = item->data(Qt::UserRole).toString();
        if (filepath.isEmpty()) {
            QMessageBox::warning(this, "错误", "无法获取存档文件路径。");
            return;
        }

        if (!game_) {
            QMessageBox::warning(this, "错误", "游戏系统未初始化。");
            return;
        }

        if (game_->loadGame(filepath.toStdString())) {
            gameOverDialogShown_ = false;  // 加载存档时重置游戏结束弹窗状态
            updateDisplay();
            showGameArea();
            QMessageBox::information(this, "读取成功", "存档已成功加载。");
        } else {
            QMessageBox::warning(this, "读取失败", "无法加载存档，请检查文件格式。");
        }
    } catch (const std::exception &) {
        QMessageBox::warning(this, "读取失败", "加载存档时发生错误，存档文件可能已损坏。");
    } catch (...) {
        QMessageBox::warning(this, "读取失败", "加载存档时发生未知错误，存档文件可能已损坏。");
    }
}

void MainWindow::onLoadGameListClicked() {
    // 当列表项被点击时，启用确认按钮和删除按钮
    bool hasSelection = saveListWidget_->currentItem() != nullptr;
    if (loadGameConfirmBtn_) loadGameConfirmBtn_->setEnabled(hasSelection);
    if (loadGameDeleteBtn_) loadGameDeleteBtn_->setEnabled(hasSelection);
}

void MainWindow::onLoadGameBack() {
    // 返回主菜单
    onBackToMenu();
}

void MainWindow::onDeleteSave() {
    QListWidgetItem *item = saveListWidget_->currentItem();
    if (!item) return;
    
    QString filepath = item->data(Qt::UserRole).toString();
    QString filename = item->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除",
        QString("确定要删除存档「%1」吗？\n此操作不可撤销。").arg(filename),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QFile file(filepath);
        if (file.remove()) {
            refreshSaveList();
        } else {
            QMessageBox::warning(this, "删除失败", "无法删除存档文件。");
        }
    }
}

void MainWindow::refreshSaveList() {
    if (!saveListWidget_) return;

    saveListWidget_->clear();

    // 确保 saves 目录存在
    QString savesPath = ensureSavesDir();

    // 扫描 saves 目录中的所有 .sav 文件
    QDir savesDir(savesPath);
    QStringList filters;
    filters << "*.sav";
    QFileInfoList fileList = savesDir.entryInfoList(filters, QDir::Files, QDir::Time);

    if (fileList.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无存档文件\n\n点击主菜单的「新游戏」开始游戏后，\n在游戏内点击「存档」按钮保存进度。");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("QLabel { font-size: 14px; color: #94a3b8; padding: 40px; }");
        QListWidgetItem *emptyItem = new QListWidgetItem();
        emptyItem->setSizeHint(QSize(0, 160));
        emptyItem->setFlags(Qt::NoItemFlags);
        saveListWidget_->addItem(emptyItem);
        saveListWidget_->setItemWidget(emptyItem, emptyLabel);
        if (loadGameConfirmBtn_) loadGameConfirmBtn_->setEnabled(false);
        return;
    }

    // 显示所有存档文件（按修改时间倒序，最新的在前）
    for (const QFileInfo &fileInfo : fileList) {
        QString displayName = fileInfo.completeBaseName();
        QString lastModified = fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss");
        QString fileSize;
        if (fileInfo.size() < 1024) {
            fileSize = QString::number(fileInfo.size()) + " B";
        } else {
            fileSize = QString::number(fileInfo.size() / 1024.0, 'f', 1) + " KB";
        }

        // 创建自定义列表项
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(QString("%1\n   修改时间: %2  |  大小: %3")
            .arg(displayName, lastModified, fileSize));
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        item->setSizeHint(QSize(0, 65));
        saveListWidget_->addItem(item);
    }

    if (loadGameConfirmBtn_) loadGameConfirmBtn_->setEnabled(false);
}

void MainWindow::showLoadGame() {
    if (!loadGameWidget_) {
        loadGameWidget_ = new QWidget(centralWidget_);
        QVBoxLayout *layout = new QVBoxLayout(loadGameWidget_);
        layout->setSpacing(LAYOUT_SPACING_WIDE);
        layout->setContentsMargins(SETTINGS_MARGIN_LR, SETTINGS_MARGIN_TB, SETTINGS_MARGIN_LR, SETTINGS_MARGIN_B);

        // 标题
        loadGameTitleLabel_ = new QLabel("加载游戏");
        loadGameTitleLabel_->setAlignment(Qt::AlignCenter);
        loadGameTitleLabel_->setStyleSheet("QLabel { font-size: 28px; font-weight: bold; color: #ffffff; margin-bottom: 10px; }");
        layout->addWidget(loadGameTitleLabel_);

        // 说明文字
        QLabel *descLabel = new QLabel("请从下方列表中选择一个存档进行加载：");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setStyleSheet("QLabel { font-size: 14px; color: #94a3b8; margin-bottom: 10px; }");
        layout->addWidget(descLabel);

        // 存档列表
        saveListWidget_ = new QListWidget();
        saveListWidget_->setStyleSheet(
            "QListWidget {"
            "    background-color: rgba(255, 255, 255, 0.06);"
            "    border: 1px solid rgba(255, 255, 255, 0.1);"
            "    border-radius: 10px;"
            "    padding: 8px;"
            "    font-size: 14px;"
            "    color: #e2e8f0;"
            "}"
            "QListWidget::item {"
            "    background-color: rgba(255, 255, 255, 0.04);"
            "    border: 1px solid rgba(255, 255, 255, 0.06);"
            "    border-radius: 8px;"
            "    padding: 10px 14px;"
            "    margin: 4px 0px;"
            "}"
            "QListWidget::item:hover {"
            "    background-color: rgba(255, 255, 255, 0.1);"
            "    border: 1px solid rgba(255, 255, 255, 0.2);"
            "}"
            "QListWidget::item:selected {"
            "    background-color: rgba(52, 152, 219, 0.3);"
            "    border: 1px solid #3498db;"
            "}"
            "QScrollBar:vertical {"
            "    background: rgba(255, 255, 255, 0.05);"
            "    width: 10px;"
            "    border-radius: 5px;"
            "}"
            "QScrollBar::handle:vertical {"
            "    background: rgba(255, 255, 255, 0.2);"
            "    border-radius: 5px;"
            "    min-height: 30px;"
            "}"
            "QScrollBar::handle:vertical:hover {"
            "    background: rgba(255, 255, 255, 0.3);"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "    height: 0px;"
            "}"
        );
        saveListWidget_->setMinimumHeight(SAVE_LIST_MIN_HEIGHT);
        saveListWidget_->setMaximumHeight(520);
        saveListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
        layout->addWidget(saveListWidget_);

        // 按钮区域
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(LAYOUT_SPACING_BTN);
        btnLayout->addStretch();

        // 返回按钮
        loadGameBackBtn_ = new QPushButton("← 返回主菜单");
        loadGameBackBtn_->setFocusPolicy(Qt::TabFocus);
        loadGameBackBtn_->setMinimumSize(LOADBTN_W, LOADBTN_H);
        loadGameBackBtn_->setStyleSheet(
            "QPushButton { "
            "    background-color: #7f8c8d; "
            "    color: white; "
            "    border: 2px solid #6c7a7d; "
            "    border-radius: 8px; "
            "    font-size: 16px; "
            "    font-weight: bold; "
            "} "
            "QPushButton:hover { background-color: #95a5a6; } "
            "QPushButton:pressed { background-color: #6c7a7d; }"
        );
        btnLayout->addWidget(loadGameBackBtn_);

        // 删除按钮
        loadGameDeleteBtn_ = new QPushButton("🗑 删除存档");
        loadGameDeleteBtn_->setFocusPolicy(Qt::TabFocus);
        loadGameDeleteBtn_->setMinimumSize(LOADBTN_W, LOADBTN_H);
        loadGameDeleteBtn_->setEnabled(false);
        loadGameDeleteBtn_->setStyleSheet(
            "QPushButton { "
            "    background-color: #c0392b; "
            "    color: white; "
            "    border: 2px solid #a93226; "
            "    border-radius: 8px; "
            "    font-size: 16px; "
            "    font-weight: bold; "
            "} "
            "QPushButton:hover { background-color: #e74c3c; } "
            "QPushButton:pressed { background-color: #a93226; } "
            "QPushButton:disabled { background-color: #4a5568; border: 2px solid #2d3748; color: #a0aec0; }"
        );
        btnLayout->addWidget(loadGameDeleteBtn_);

        // 确认加载按钮
        loadGameConfirmBtn_ = new QPushButton("加载存档");
        loadGameConfirmBtn_->setFocusPolicy(Qt::TabFocus);
        loadGameConfirmBtn_->setMinimumSize(LOADBTN_W, LOADBTN_H);
        loadGameConfirmBtn_->setEnabled(false);
        loadGameConfirmBtn_->setStyleSheet(
            "QPushButton { "
            "    background-color: #27ae60; "
            "    color: white; "
            "    border: 2px solid #1e8449; "
            "    border-radius: 8px; "
            "    font-size: 16px; "
            "    font-weight: bold; "
            "} "
            "QPushButton:hover { background-color: #2ecc71; } "
            "QPushButton:pressed { background-color: #1e8449; } "
            "QPushButton:disabled { background-color: #4a5568; border: 2px solid #2d3748; color: #a0aec0; }"
        );
        btnLayout->addWidget(loadGameConfirmBtn_);

        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        layout->addStretch();

        loadGameWidget_->setStyleSheet("QWidget { background: transparent; }");
        centralStack_->addWidget(loadGameWidget_);

        // 连接信号
        connect(loadGameBackBtn_, &QPushButton::clicked, this, &MainWindow::onLoadGameBack);
        connect(loadGameConfirmBtn_, &QPushButton::clicked, this, &MainWindow::onLoadGameSlot);
        connect(loadGameDeleteBtn_, &QPushButton::clicked, this, &MainWindow::onDeleteSave);
        connect(saveListWidget_, &QListWidget::itemClicked, this, &MainWindow::onLoadGameListClicked);
        connect(saveListWidget_, &QListWidget::itemDoubleClicked, this, [this]() {
            onLoadGameSlot();
        });
    }

    // 刷新存档列表
    refreshSaveList();

    // 切换到加载游戏页面
    paintBgForWidget(loadGameWidget_, bgLoadGame_);
        centralStack_->setCurrentWidget(loadGameWidget_);
        // 强制用 centralWidget_ 的尺寸更新背景标签（确保尺寸正确）
        QLabel* bgLabel = loadGameWidget_->findChild<QLabel*>("__bg_label__");
        if (bgLabel && centralWidget_) {
            bgLabel->setGeometry(centralWidget_->rect());
            if (!bgLoadGame_.isNull()) {
                bgLabel->setPixmap(QPixmap::fromImage(bgLoadGame_.toImage().scaled(centralWidget_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            }
        }
    }

void MainWindow::onBackToMenu() {
    paintBgForWidget(mainMenuWidget_, bgMainMenu_);
    centralStack_->setCurrentWidget(mainMenuWidget_);
}

void MainWindow::onBackToMenuFromGame() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "退出到主菜单", 
        "确定要退出到主菜单吗？\n未保存的进度将丢失。",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        showMainMenu();
    }
}

void MainWindow::onMusicVolumeChanged(int value) {
    musicVolume_ = value;
    if (musicVolumeValueLabel_) {
        musicVolumeValueLabel_->setText(QString("%1%").arg(value));
    }
    if (musicPlayerWidget_) {
        musicPlayerWidget_->setMusicVolume(value);
    }
}

void MainWindow::onSfxVolumeChanged(int value) {
    sfxVolume_ = value;
    if (sfxVolumeValueLabel_) {
        sfxVolumeValueLabel_->setText(QString("%1%").arg(value));
    }
    // 音效音量已保存至 sfxVolume_，外部通过 getSfxVolume() 查询
    // 例如战斗系统中的 QMediaPlayer 可读取此值：
    //   int vol = mainWindow->getSfxVolume();
    //   sfxPlayer->setVolume(vol);
}

void MainWindow::onBrightnessChanged(int value) {
    brightness_ = value;
    if (brightnessValueLabel_) {
        brightnessValueLabel_->setText(QString("%1%").arg(value));
    }
    applyBrightness(value);
}

void MainWindow::applyBrightness(int value) {
    if (!brightnessOverlay_ || !brightnessEffect_) return;
    double opacity = 1.0 - (value - 30) / 70.0;
    opacity = qBound(0.0, opacity, 0.7);
    brightnessEffect_->setOpacity(opacity);
}

void MainWindow::onToggleMusicPlayer() {
    if (!musicPlayerWidget_) return;
    if (musicPlayerWidget_->isVisible()) {
        musicPlayerWidget_->hide();
        {
            QPixmap px = loadAssetPixmap("assets/button/btn_music.svg");
            if (!px.isNull()) {
                musicToggleBtn_->setIcon(QIcon(px));
                musicToggleBtn_->setIconSize(QSize(28, 28));
            } else {
                musicToggleBtn_->setText(QString::fromUtf8("\xF0\x9F\x8E\xB5"));
            }
        }
        musicToggleBtn_->setToolTip("打开音乐播放器");
    } else {
        musicPlayerWidget_->show();
        musicPlayerWidget_->raise();
        {
            QPixmap px = loadAssetPixmap("assets/button/btn_music_close.svg");
            if (!px.isNull()) {
                musicToggleBtn_->setIcon(QIcon(px));
                musicToggleBtn_->setIconSize(QSize(28, 28));
            } else {
                musicToggleBtn_->setText(QString::fromUtf8("\xF0\x9F\x94\x80"));
            }
        }
        musicToggleBtn_->setToolTip("关闭音乐播放器");
        // 播放器定位在音乐按钮上方，跟随按钮位置
        updateMusicPlayerPosition();
    }
}

void MainWindow::onToggleBattleLog() {
    if (!battleLogWidget_) return;
    if (battleLogWidget_->isVisible()) {
        battleLogWidget_->hide();
        logToggleBtn_->setText("📜");
        logToggleBtn_->setToolTip("打开战斗日志");
    } else {
        // 更新日志内容后再显示
        battleLogWidget_->updateLog(game_.get());
        battleLogWidget_->show();
        battleLogWidget_->raise();
        logToggleBtn_->setText("📜");
        logToggleBtn_->setToolTip("关闭战斗日志");
        // 定位在日志按钮上方
        int bx = logToggleBtn_->x() - battleLogWidget_->width() + logToggleBtn_->width();
        int by = logToggleBtn_->y() - battleLogWidget_->height() - 12;
        if (bx < 10) bx = 10;
        if (by < 10) by = logToggleBtn_->y() + logToggleBtn_->height() + 12;
        battleLogWidget_->move(bx, by);
    }
}

void MainWindow::onToggleStatsReport() {
    if (!statsWidget_) return;
    if (statsWidget_->isVisible()) {
        statsWidget_->hide();
        statsToggleBtn_->setText("📊");
        statsToggleBtn_->setToolTip("打开统计报表");
    } else {
        // 更新统计内容后再显示
        statsWidget_->updateStats(game_.get());
        statsWidget_->show();
        statsWidget_->raise();
        statsToggleBtn_->setText("📊");
        statsToggleBtn_->setToolTip("关闭统计报表");
        // 定位在统计按钮右侧（使用 this 而非 parentWidget 避免 nullptr 崩溃）
        int bx = statsToggleBtn_->x() + statsToggleBtn_->width() + 12;
        int by = statsToggleBtn_->y() - statsWidget_->height() + statsToggleBtn_->height() + 12;
        if (bx + statsWidget_->width() > width() - 10) {
            bx = statsToggleBtn_->x() - statsWidget_->width() - 12;
        }
        if (by < 10) by = 10;
        if (by + statsWidget_->height() > height() - 10) {
            by = height() - statsWidget_->height() - 10;
        }
        statsWidget_->move(bx, by);
    }
}

void MainWindow::onOpenShop() {
    if (!game_) return;

    // 延迟创建 ShopWindow
    if (!shopWindow_) {
        shopWindow_ = new ShopWindow(game_, this);
        // 连接 ShopWidget 信号
        if (shopWindow_->shopWidget()) {
            connect(shopWindow_->shopWidget(), &ShopWidget::purchaseRequested,
                    this, &MainWindow::onPurchaseUnit);
            // 连接备战区合成按钮 -> 商店自动三合一
            if (boardWidget_) {
                connect(boardWidget_, &BoardWidget::combineRequested,
                        shopWindow_->shopWidget(), &ShopWidget::onAutoCombine);
            }
        }
    }

    // 更新商店内容
    shopWindow_->updateDisplay();

    // 模态显示
    shopWindow_->exec();

    // 对话框关闭后刷新主界面
    updateDisplay();
}

void MainWindow::onPurchaseUnit(int index) {
    if (!game_ || !shopWindow_) return;
    ShopWidget* sw = shopWindow_->shopWidget();
    if (!sw) return;

    // 快速购买模式：直接执行
    if (quickBuyEnabled_) {
        int oldGold = game_->getPlayer().getGold();
        sw->executePurchase(index);
        int newGold = game_->getPlayer().getGold();
        // 如果金币发生了变化（购买成功），执行动画
        if (oldGold != newGold) {
            QPoint fromPos = sw->getSlotScreenPos(index);
            startPurchaseFlyAnimation(fromPos);
            animateGoldChange(oldGold, newGold);
        }
        return;
    }

    // 普通模式：弹出确认弹窗
    const auto& shopUnits = game_->getShop().getUnits();
    if (index < 0 || index >= static_cast<int>(shopUnits.size()) || !shopUnits[index]) return;

    auto unit = shopUnits[index];
    QString msg = QString("确定要购买「%1」吗？\n\n"
                          "❤ HP: %2\n"
                          "⚔ ATK: %3\n"
                          "⭐ %4 星\n"
                          "💰 价格: 2 金币")
                      .arg(QString::fromStdString(unit->debugName()))
                      .arg(unit->getHP())
                      .arg(unit->getATK())
                      .arg(unit->getStarLevel());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认购买", msg,
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        int oldGold = game_->getPlayer().getGold();
        sw->executePurchase(index);
        int newGold = game_->getPlayer().getGold();
        if (oldGold != newGold) {
            QPoint fromPos = sw->getSlotScreenPos(index);
            startPurchaseFlyAnimation(fromPos);
            animateGoldChange(oldGold, newGold);
        }
    }
}

void MainWindow::animateGoldChange(int oldValue, int newValue) {
    if (!goldLabel_) return;

    int diff = newValue - oldValue;
    int steps = 8;
    int delay = 40; // 毫秒

    // 使用定时器逐步更新金币显示
    QTimer *animTimer = new QTimer(this);
    int *counter = new int(0);
    animTimer->setProperty("steps", steps);
    animTimer->setProperty("oldVal", oldValue);
    animTimer->setProperty("newVal", newValue);

    connect(animTimer, &QTimer::timeout, this, [this, animTimer, counter, oldValue, newValue, steps]() {
        (*counter)++;
        if (*counter >= steps) {
            goldLabel_->setText(QString::number(newValue));
            animTimer->stop();
            animTimer->deleteLater();
            delete counter;
            return;
        }
        // 使用缓动函数：先快后慢
        double t = static_cast<double>(*counter) / steps;
        double eased = t * (2 - t); // 缓出曲线 (ease-out)
        int current = oldValue + static_cast<int>((newValue - oldValue) * eased);
        goldLabel_->setText(QString::number(current));
    });
    animTimer->start(delay);
}

void MainWindow::startPurchaseFlyAnimation(QPoint fromPos) {
    // 创建一个小的飞行动画粒子
    QLabel *flyIcon = new QLabel("⭐", this);
    flyIcon->setFixedSize(32, 32);
    flyIcon->setAlignment(Qt::AlignCenter);
    flyIcon->setStyleSheet("font-size: 24px; background: transparent;");
    flyIcon->raise();

    // 将屏幕坐标转换为 MainWindow 坐标
    QPoint fromLocal = mapFromGlobal(fromPos);
    flyIcon->move(fromLocal.x() - 16, fromLocal.y() - 16);
    flyIcon->show();

    // 终点：备战区大致位置（右侧 infoPanel 区域）
    QPoint toPos;
    if (shopWindow_ && shopWindow_->shopWidget()) {
        // 飞到商店所在大致区域的上方（备战区位置）
        ShopWidget* sw = shopWindow_->shopWidget();
        QPoint shopGlobal = sw->mapToGlobal(QPoint(sw->width() / 2, 0));
        toPos = mapFromGlobal(shopGlobal);
        toPos.setY(toPos.y() - 60);
    } else {
        toPos = QPoint(width() - 100, height() / 2);
    }

    // 使用 QPropertyAnimation 实现飞行动画
    QPropertyAnimation *anim = new QPropertyAnimation(flyIcon, "pos");
    anim->setDuration(EFFECT_FADEOUT_DURATION_MS);
    anim->setStartValue(flyIcon->pos());
    anim->setEndValue(toPos);
    anim->setEasingCurve(QEasingCurve::InOutQuad);

    // 透明度动画
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(flyIcon);
    flyIcon->setGraphicsEffect(opacityEffect);
    QPropertyAnimation *fadeAnim = new QPropertyAnimation(opacityEffect, "opacity");
    fadeAnim->setDuration(EFFECT_FADEOUT_DURATION_MS);
    fadeAnim->setKeyValueAt(0.0, 1.0);
    fadeAnim->setKeyValueAt(0.7, 1.0);
    fadeAnim->setKeyValueAt(1.0, 0.0);

    // 动画结束后删除图标
    connect(anim, &QPropertyAnimation::finished, flyIcon, &QLabel::deleteLater);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    fadeAnim->start();
}

void MainWindow::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);

    // 只在开场画面处于活动状态时绘制开场背景
    if (startScreenWidget_ && centralStack_->currentWidget() == startScreenWidget_) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // 绘制背景（SVG或渐变背景）
        if (!startBgCache_.isNull()) {
            painter.drawPixmap(0, 0, startBgCache_);
        }

        // 绘制额外的装饰效果
        if (startAnimationStarted_) {
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
            if (startGlowIntensity_ > 0.1) {
                painter.setPen(Qt::NoPen);
                QRadialGradient radialGradient(width() / 2, height() / 2 - 40, 350);
                radialGradient.setColorAt(0.0, QColor(100, 180, 255, static_cast<int>(20 * startGlowIntensity_)));
                radialGradient.setColorAt(0.5, QColor(100, 180, 255, static_cast<int>(10 * startGlowIntensity_)));
                radialGradient.setColorAt(1.0, QColor(100, 180, 255, 0));
                painter.setBrush(radialGradient);
                painter.drawEllipse(QPointF(width() / 2, height() / 2 - 40), 350, 140);
            }

            // 绘制标题发光光晕（在标题文字后面）
            if (startGlowIntensity_ > 0.1) {
                painter.setPen(Qt::NoPen);
                QRadialGradient titleGlow(width() / 2, height() / 2 - 60, 300);
                titleGlow.setColorAt(0.0, QColor(100, 180, 255, static_cast<int>(30 * startGlowIntensity_)));
                titleGlow.setColorAt(0.4, QColor(100, 180, 255, static_cast<int>(15 * startGlowIntensity_)));
                titleGlow.setColorAt(1.0, QColor(100, 180, 255, 0));
                painter.setBrush(titleGlow);
                painter.drawEllipse(QPointF(width() / 2, height() / 2 - 60), 250, 80);
            }
        }
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    // 音乐按钮拖拽
    if (watched == musicToggleBtn_) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                // 记录全局偏移：鼠标全局坐标 - 按钮左上角全局坐标
                QPoint btnGlobal = musicToggleBtn_->mapToGlobal(QPoint(0, 0));
                musicToggleDragStart_ = me->globalPosition().toPoint() - btnGlobal;
                musicToggleDragging_ = false;
                musicToggleBtn_->setCursor(Qt::ClosedHandCursor);
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (musicToggleBtn_->cursor().shape() == Qt::ClosedHandCursor) {
                QMouseEvent *me = static_cast<QMouseEvent *>(event);
                // 计算新位置：鼠标全局坐标 - 偏移量
                QPoint newPos = me->globalPosition().toPoint() - musicToggleDragStart_;
                // 按钮是 MainWindow 的子部件，所以用 MainWindow 的尺寸做边界约束
                int margin = 10;
                newPos.setX(qBound(margin, newPos.x(), width() - musicToggleBtn_->width() - margin));
                newPos.setY(qBound(margin, newPos.y(), height() - musicToggleBtn_->height() - margin));
                // 检查是否有实际移动，判断是否开始拖拽
                QPoint oldPos = musicToggleBtn_->pos();
                QPoint delta = newPos - oldPos;
                if (delta.manhattanLength() > 5) {
                    musicToggleDragging_ = true;
                }
                if (musicToggleDragging_) {
                    musicToggleBtn_->move(newPos);
                    if (musicPlayerWidget_ && musicPlayerWidget_->isVisible()) {
                        updateMusicPlayerPosition();
                    }
                }
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                bool wasDragging = musicToggleDragging_;
                musicToggleDragging_ = false;
                musicToggleBtn_->setCursor(Qt::ArrowCursor);
                if (!wasDragging) {
                    onToggleMusicPlayer();
                }
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    QMainWindow::mousePressEvent(event);

    // 如果开场动画已经开始且未在过渡中，点击可提前跳转
    if (startScreenWidget_ && centralStack_->currentWidget() == startScreenWidget_) {
        if (startAnimationStarted_ && !startIsTransitioning_) {
            if (startSkipBtn_ && !startSkipBtn_->underMouse()) {
                onStartAutoAdvance();
            }
        }
    }
}

void MainWindow::repositionMusicControls() {
    if (!musicToggleBtn_) return;
    // 重新定位到右下角，留出24px边距
    int margin = 24;
    int btnX = width() - musicToggleBtn_->width() - margin;
    int btnY = height() - musicToggleBtn_->height() - margin;
    btnX = qMax(margin, btnX);
    btnY = qMax(margin, btnY);
    musicToggleBtn_->move(btnX, btnY);
    musicToggleBtn_->raise();
    
    // 日志按钮在音乐按钮上方
    if (logToggleBtn_) {
        logToggleBtn_->move(btnX, btnY - logToggleBtn_->height() - 12);
        logToggleBtn_->raise();
    }
    
    // 如果播放器展开，也重新定位
    if (musicPlayerWidget_ && musicPlayerWidget_->isVisible()) {
        updateMusicPlayerPosition();
        musicPlayerWidget_->raise();
    }
    
    // 如果日志面板展开，也重新定位
    if (battleLogWidget_ && battleLogWidget_->isVisible()) {
        int bx = logToggleBtn_->x() - battleLogWidget_->width() + logToggleBtn_->width();
        int by = logToggleBtn_->y() - battleLogWidget_->height() - 12;
        if (bx < 10) bx = 10;
        if (by < 10) by = logToggleBtn_->y() + logToggleBtn_->height() + 12;
        battleLogWidget_->move(bx, by);
        battleLogWidget_->raise();
    }
}

void MainWindow::toggleFullScreen() {
    if (isFullScreen()) {
        showNormal();
        musicControlsPinned_ = false;  // 退出全屏，允许用户拖拽
    } else {
        showFullScreen();
        musicControlsPinned_ = true;   // 全屏时固定到右下角
        repositionMusicControls();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // 开场动画快捷键：Enter/Space/Esc 跳过开场
    if (startScreenWidget_ && centralStack_->currentWidget() == startScreenWidget_) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
            event->key() == Qt::Key_Space || event->key() == Qt::Key_Escape) {
            if (startAnimationStarted_ && !startIsTransitioning_) {
                onStartAutoAdvance();
            }
            event->accept();
            return;
        }
    }

    // F11: 全屏切换（始终有效）
    if (event->key() == Qt::Key_F11) {
        toggleFullScreen();
        event->accept();
        return;
    }

    // Esc: 上下文相关返回
    if (event->key() == Qt::Key_Escape) {
        if (isFullScreen()) {
            showNormal();
            event->accept();
            return;
        }
        // 在游戏界面中返回主菜单
        if (centralStack_->currentWidget() == gameAreaWidget_) {
            onBackToMenuFromGame();
            event->accept();
            return;
        }
        // 在设置/帮助/关于/加载存档界面中返回主菜单
        QWidget *current = centralStack_->currentWidget();
        if (current == settingsWidget_ || current == helpWidget_ ||
            current == aboutWidget_ || current == loadGameWidget_) {
            showMainMenu();
            event->accept();
            return;
        }
        QMainWindow::keyPressEvent(event);
        return;
    }

    // 以下快捷键仅在游戏界面有效
    if (centralStack_->currentWidget() == gameAreaWidget_) {
        switch (event->key()) {
        case Qt::Key_Space:
            // Space: 结束回合（准备阶段有效）
            if (!game_->isGameOver() && !game_->isInCombatPhase()) {
                onStartRound();
                event->accept();
                return;
            }
            break;
        case Qt::Key_R:
            // R: 刷新商店
            if (shopWindow_ && shopWindow_->shopWidget()) {
                shopWindow_->shopWidget()->doRefreshShop();
                if (shopWindow_->isVisible()) {
                    shopWindow_->updateDisplay();
                }
                updateDisplay();
                event->accept();
                return;
            }
            break;
        case Qt::Key_B:
            // B: 购买选中单位
            if (shopWindow_ && shopWindow_->shopWidget()) {
                shopWindow_->shopWidget()->doBuySelected();
                if (shopWindow_->isVisible()) {
                    shopWindow_->updateDisplay();
                }
                updateDisplay();
                event->accept();
                return;
            }
            break;
        case Qt::Key_S:
            // S: 存档
            onSaveGame();
            event->accept();
            return;
        default:
            break;
        }
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    
    // 更新亮度遮罩层尺寸
    if (brightnessOverlay_) {
        brightnessOverlay_->setGeometry(centralWidget_->rect());
    }
    
    // 使用 lambda 统一更新/创建所有页面的背景图标签尺寸和 pixmap
    auto updateBgForPage = [this](QWidget* page, const QPixmap& bg) {
        if (!page) return;
        QSize pageSize = page->size();
        if (!pageSize.isValid() || pageSize.width() < 100 || pageSize.height() < 100) return;
        
        QLabel* bgLabel = page->findChild<QLabel*>("__bg_label__");
        if (!bgLabel) {
            bgLabel = new QLabel(page);
            bgLabel->setObjectName("__bg_label__");
            bgLabel->setScaledContents(true);
            bgLabel->lower();
        }
        bgLabel->setGeometry(page->rect());
        if (!bg.isNull()) {
            bgLabel->setPixmap(QPixmap::fromImage(bg.toImage().scaled(pageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        } else {
            bgLabel->setStyleSheet("background-color: transparent;");
        }
        bgLabel->lower();
    };
    
    updateBgForPage(mainMenuWidget_, bgMainMenu_);
    updateBgForPage(settingsWidget_, bgSettings_);
    updateBgForPage(helpWidget_, bgHelp_);
    updateBgForPage(aboutWidget_, bgAbout_);
    updateBgForPage(loadGameWidget_, bgLoadGame_);
    
    // 更新棋盘和右侧面板的尺寸比例
    if (gameAreaWidget_ && mainLayout_) {
        int totalWidth = gameAreaWidget_->width();
        int totalHeight = gameAreaWidget_->height();
        
        // 动态调整棋盘最小尺寸（基于窗口高度）
        if (boardWidget_) {
            int boardMinW = qMax(BOARD_MIN_WIDTH, totalWidth * 2 / 3);
            int boardMinH = qMax(BOARD_MIN_HEIGHT, totalHeight - 40);
            boardWidget_->setMinimumSize(boardMinW, boardMinH);
        }
    }
    
    // 全屏或固定模式下，窗口大小改变时重新定位音乐控件
    if (musicControlsPinned_ || isFullScreen()) {
        repositionMusicControls();
    }
    if (musicPlayerWidget_) musicPlayerWidget_->raise();
    if (musicToggleBtn_) musicToggleBtn_->raise();
}