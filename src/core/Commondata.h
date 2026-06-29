/**
 * @file    Commondata.h
 * @brief   公共数据结构、宏定义、装备模板与 UI 布局常量
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <queue>
#include <map>
#include <set>
#include <tuple>
#include <list>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <functional>

#define BOARD_M 8
#define BOARD_N 8
#define BENCH_SIZE 8
#define BENCH_SLOTS 8

// ========== UI 窗口尺寸 ==========
#define WINDOW_DEFAULT_WIDTH  1600
#define WINDOW_DEFAULT_HEIGHT 900
#define WINDOW_MIN_WIDTH      1280
#define WINDOW_MIN_HEIGHT     720

// ========== UI 组件尺寸 ==========
#define BATTLELOG_WIDTH   420
#define BATTLELOG_HEIGHT  400
#define BOARD_MIN_WIDTH   480
#define BOARD_MIN_HEIGHT  540
#define BOARD_AREA_MIN_WIDTH  640
#define BOARD_AREA_MIN_HEIGHT 720
#define COMBINE_BTN_W     72
#define COMBINE_BTN_H     28
#define BUTTON_SIZE_SMALL 28
#define BUTTON_SIZE_MEDIUM 34
#define BUTTON_SIZE_LARGE 48
#define BUTTON_SIZE_ICON  22
#define BUTTON_SIZE_ICON2 20
#define BUTTON_SIZE_WIDE_W 260
#define BUTTON_SIZE_WIDE_H 48
#define BUTTON_MIN_HEIGHT 42
#define BUTTON_MIN_HEIGHT2 38
#define DIALOG_WIDTH      380
#define DIALOG_HEIGHT     320
#define STATSWIDGET_WIDTH 560
#define STATSWIDGET_HEIGHT 460
#define MUSICPLAYER_HEIGHT 100
#define MUSICPLAYER_WIDTH 620
#define SKIPBTN_WIDTH     100
#define SKIPBTN_HEIGHT    32
#define SHOPBTN_W         40
#define SHOPBTN_H         36
#define SHOP_SLOT_SIZE    90
#define LOADBTN_W         180
#define LOADBTN_H         48
#define PANEL_MIN_WIDTH   200
#define PANEL_MAX_WIDTH   320
#define TRAIT_MIN_HEIGHT  60
#define TRAIT_MAX_HEIGHT  220
#define SHOP_MIN_HEIGHT   250
#define SAVE_LIST_MAX_HEIGHT 520
#define FLIGHT_ICON_SIZE  32
#define ICON_SIZE_SMALL   18
#define ICON_SIZE_TINY    16

// ========== UI 布局参数 ==========
#define BOARD_BASE_CELL_SIZE  96
#define BOARD_BASE_MARGIN     20
#define BOARD_BENCH_MARGIN    8
#define CELL_SIZE_MIN        48
#define CELL_SIZE_MAX        120
#define MARGIN_MIN           10
#define MARGIN_MAX           30
#define CELL_RIGHT_RESERVE   20
#define SAVE_LIST_MIN_HEIGHT 400
#define SETTINGS_MARGIN_LR   80
#define SETTINGS_MARGIN_TB   60
#define SETTINGS_MARGIN_B    60
#define MENU_MARGIN_LR       60
#define MENU_MARGIN_TB       40
#define MENU_MARGIN_B        60
#define START_MARGIN_LR      60
#define START_MARGIN_TB      80
#define START_MARGIN_B       60
#define PANEL_MARGIN_LR      12
#define PANEL_MARGIN_TB      8
#define GAMEAREA_MARGIN      10
#define CARD_MARGIN_LR       6
#define CARD_MARGIN_TB       4
#define CARD_SPACING         1
#define TRAIT_MARGIN_LR      8
#define TRAIT_MARGIN_TB      4
#define DIALOG_MARGIN_LR     20
#define DIALOG_MARGIN_TB     16
#define BOTTOM_MARGIN_R      20
#define BOTTOM_MARGIN_B      20
#define LAYOUT_SPACING_DEFAULT 10
#define LAYOUT_SPACING_TIGHT 6
#define LAYOUT_SPACING_MENU  12

// ========== 装备模板系统 ==========
struct EquipmentTemplate {
    std::string name;        // 装备名称（符卡格式）
    int type;                // 0:武器, 1:防具, 2:饰品1, 3:饰品2
    int atkMin, atkMax;      // 攻击加成范围
    int hpMin, hpMax;        // 生命加成范围
    std::string restriction; // 职业限制
    int starLevel;           // 品质 1/2/3
    std::string effect;      ///< 特殊效果 "lifesteal"/"crit"/"armor"/""
    int tag;                 // 装备编号 1-20, 0=无套装
};

// 可掉落装备模板
inline std::vector<EquipmentTemplate> GetEquipmentTemplates() {
    return {
        // ===== 1★ 装备 =====
        {"蘑菇",     2, 4,  6,  2,  5,  "Mahoushi",   1, "lifesteal", 1},
        {"粗制护符", 1, 0,  1,  30, 50, "Miko",       1, "",          2},
        {"木屐",     2, 1,  2,  5,  10, "Journalist", 1, "",          3},
        {"家徽",     2, 2,  3,  4,  6,  "Maid",       1, "",          4},

        // ===== 2★ 装备 =====
        // --- 武器(0) ---
        {"御币",   0, 10, 15, 5,  10, "Miko",       2, "",          5},
        {"八卦炉", 0, 12, 18, 0,  0,  "Mahoushi",   2, "crit",      6},
        {"扫帚",   0, 8,  12, 10, 15, "Maid",       2, "",          7},
        {"风符",   0, 12, 20, 4,  8,  "Journalist", 2, "",          8},
        // --- 防具(1) ---
        {"巫女服",   1, 0, 2, 60, 90, "Miko",       2, "",          9},
        {"魔法斗篷", 1, 2, 4, 50, 80, "Mahoushi",   2, "",          10},
        {"鸦符",     1, 3, 5, 40, 70, "Journalist", 2, "",          11},
        {"庭师裙",   1, 1, 3, 55, 85, "Maid",       2, "armor",     12},
        // --- 饰品(2) ---
        {"阴阳玉", 2, 6,  10, 20, 35, "Miko",       2, "armor",     13},
        {"光撃",   2, 8,  12, 15, 25, "Mahoushi",   2, "lifesteal", 14},
        {"山伏",   2, 8,  10, 20, 25, "Journalist", 2, "",          15},
        {"魂符",   2, 7,  11, 18, 30, "Maid",       2, "",          16},

        // ===== 3★ 装备 =====
        // --- 武器(0) ---
        {"道",     0, 18, 25, 15, 25, "Miko",       3, "",          17},
        {"书",     0, 20, 30, 10, 20, "Mahoushi",   3, "crit",      18},
        {"三脚架", 0, 24, 32, 8,  15, "Journalist", 3, "",          19},
        {"半灵",   0, 20, 28, 12, 20, "Maid",       3, "",          20},
    };
}

// 按星级筛选模板
inline std::vector<EquipmentTemplate> GetEquipmentTemplatesByStar(int maxStar) {
    std::vector<EquipmentTemplate> result;
    for (const auto& t : GetEquipmentTemplates()) {
        if (t.starLevel <= maxStar) {
            result.push_back(t);
        }
    }
    return result;
}
#define LAYOUT_SPACING_PANEL 8
#define LAYOUT_SPACING_WIDE  16
#define LAYOUT_SPACING_WIDE2 18
#define LAYOUT_SPACING_BTN   20
#define LAYOUT_SPACING_SHOP  8
#define RECRUIT_SPACING      6
#define MUSIC_MARGIN_LR      14
#define MUSIC_MARGIN_TB      12
#define SHOP_MARGIN          6
#define SHOP_TAB_MARGIN      4
#define SHOP_HEADER_PADDING  3
#define CHART_MARGIN_LEFT    50
#define CHART_MARGIN_RIGHT   20
#define CHART_MARGIN_TOP     30
#define CHART_MARGIN_BOTTOM  40
#define TITLE_FONT_SIZE      72
#define SUBTITLE_FONT_SIZE   20
#define HINT_FONT_SIZE       14
#define LETTER_SPACING_TITLE 12
#define LETTER_SPACING_SUBTITLE 6

// ========== 动画/计时参数 ==========
#define TIMER_UPDATE_MS       16
#define TIMER_ANIM_MS         33
#define TIMER_COMBAT_MS       50
#define TIMER_AUTO_ADVANCE_MS 5000
#define TIMER_GLOW_MS         33

// ========== 动画计时（毫秒） ==========
#define ROUND_SETTLEMENT_DELAY_MS 1800
#define EFFECT_FADE_DELAY_MS     1200
#define EFFECT_FADEIN_DURATION_MS 300
#define EFFECT_FADEOUT_DURATION_MS 500
#define EFFECT_FADEOUT_DURATION2_MS 600
#define TITLE_FADEIN_DURATION_MS  1500
#define SUBTITLE_FADEIN_DURATION_MS 1000
#define HINT_FADEIN_DURATION_MS   1000
#define BG_TRANSITION_DELAY_MS    1500
#define ANIM_DURATION_600_MS      600
#define INTRO_TITLE_DELAY_MS     500
#define INTRO_SUBTITLE_DELAY_MS  1200
#define INTRO_GLOW_DELAY_MS      2000
#define INTRO_HINT_DELAY_MS      2500

// ========== 滑块/范围参数 ==========
#define SLIDER_VOLUME_DEFAULT   65
#define SLIDER_SFX_DEFAULT      80
#define SLIDER_BRIGHT_DEFAULT   100
#define SLIDER_BRIGHT_MIN       30
#define SLIDER_BRIGHT_MAX       100
#define SLIDER_PROGRESS_MAX     1000
#define AUDIO_INITIAL_VOLUME    0.65
