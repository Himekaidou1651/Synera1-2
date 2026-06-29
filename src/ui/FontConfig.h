/**
 * @file    FontConfig.h
 * @brief   全局字体配置，定义字体层级常量和加载工具函数
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QFont>
#include <QString>

// ========== 字体层级常量（使用 setPixelSize 确保跨 DPI 一致） ==========
constexpr int FONT_TINY   = 9;    ///< 极小文字（角标、辅助信息）
constexpr int FONT_SMALL  = 11;   ///< 小文字（详情、描述）
constexpr int FONT_NORMAL = 13;   ///< 常规文字（正文、按钮）
constexpr int FONT_LARGE  = 16;   ///< 大文字（标题、强调）
constexpr int FONT_TITLE  = 28;   ///< 页面标题
constexpr int FONT_HERO   = 72;   ///< 主视觉文字（开场画面）

// ========== 字体工具函数 ==========

/**
 * @brief  获取应用主字体（中文优先使用微软雅黑，英文使用 Segoe UI）
 * @param  pixelSize 字体像素大小
 * @param  bold      是否加粗
 * @return 配置好的 QFont 对象
 */
QFont appFont(int pixelSize = FONT_NORMAL, bool bold = false);

/**
 * @brief  获取等宽字体（用于代码/日志显示）
 * @param  pixelSize 字体像素大小
 * @param  bold      是否加粗
 * @return 等宽 QFont 对象
 */
QFont monoFont(int pixelSize = FONT_SMALL, bool bold = false);

/**
 * @brief  初始化全局字体
 *
 * 在 QApplication 创建后调用一次。
 * 会尝试加载自定义字体文件，并设置应用默认字体。
 */
void setupAppFont();
