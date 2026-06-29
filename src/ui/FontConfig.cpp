/**
 * @file    FontConfig.cpp
 * @brief   全局字体配置实现
 * @author  
 * @date    2026-06-24
 */

#include "FontConfig.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>

// 查找资源文件的辅助函数（与 MainWindow.cpp 中逻辑一致）
static QString findAssetFile(const QString &relativePath) {
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

// 自定义字体族名称（由 setupAppFont() 设置）
static QString g_customFontFamily;

// 主字体族名称（优先使用自定义字体，否则回退到系统字体）
static QString mainFontFamily() {
    if (!g_customFontFamily.isEmpty()) {
        return g_customFontFamily;
    }
#ifdef Q_OS_WIN
    return "Microsoft YaHei UI, Microsoft YaHei, Segoe UI, Arial";
#else
    return "Noto Sans CJK SC, Noto Sans, Sans Serif";
#endif
}

// 等宽字体族名称
static QString monoFontFamily() {
    return "Consolas, Courier New, monospace";
}

QFont appFont(int pixelSize, bool bold) {
    QFont font(mainFontFamily());
    font.setPixelSize(pixelSize);
    font.setBold(bold);
    // 禁用 Qt 自动字体缩放，确保 setPixelSize 生效
    font.setStyleStrategy(QFont::PreferDefault);
    return font;
}

QFont monoFont(int pixelSize, bool bold) {
    QFont font(monoFontFamily());
    font.setPixelSize(pixelSize);
    font.setBold(bold);
    font.setStyleStrategy(QFont::PreferDefault);
    return font;
}

void setupAppFont() {
    // 尝试加载自定义字体文件（如果 assets/fonts/ 目录下有 .ttf/.otf 文件）
    QString fontDirs[] = {
        findAssetFile("assets/fonts"),
        findAssetFile("fonts")
    };

    bool customFontLoaded = false;
    for (const QString &fontDir : fontDirs) {
        if (fontDir.isEmpty()) continue;
        QDir dir(fontDir);
        QStringList filters;
        filters << "*.ttf" << "*.otf" << "*.ttc";
        QStringList files = dir.entryList(filters, QDir::Files);
        for (const QString &file : files) {
            QString fontPath = dir.filePath(file);
            int id = QFontDatabase::addApplicationFont(fontPath);
            if (id >= 0) {
                // 获取加载的字体族名称，供 appFont() 使用
                QStringList families = QFontDatabase::applicationFontFamilies(id);
                if (!families.isEmpty()) {
                    g_customFontFamily = families.first();
                }
                customFontLoaded = true;
                qDebug() << "[FontConfig] 加载字体:" << fontPath
                         << "族名:" << g_customFontFamily;
            }
        }
    }

    if (customFontLoaded) {
        qDebug() << "[FontConfig] 自定义字体加载成功，使用:" << g_customFontFamily;
    } else {
        qDebug() << "[FontConfig] 未找到自定义字体文件，使用系统字体";
    }

    // 设置应用全局默认字体
    QFont defaultFont = appFont(FONT_NORMAL);
    QApplication::setFont(defaultFont);
}
