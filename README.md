# Synera: Synergy Auto-Arena

> 一个基于 **Qt6** 和 **C++17** 的自走棋风格策略游戏 20260701

---

## 目录

- [1. 基本信息](#1-基本信息)
- [2. 文件树结构](#2-文件树结构)
- [3. 核心类和数据结构](#3-核心类和数据结构)
- [4. 算法描述](#4-算法描述)
- [5. 辅助函数与工具](#5-辅助函数与工具)
- [6. 已实现扩展功能](#6-已实现扩展功能)
- [7. 构建与运行](#7-构建与运行)
- [8. 设计说明与已知限制](#8-设计说明与已知限制)
- [9. 参考资料与致谢](#9-参考资料与致谢)

---

## 1. 基本信息

| 项目 | 内容 |
|------|------|
| **项目名称** | Synera — Synergy Auto-Arena |
| **开发环境** | Qt 6.11.1 + CMake 3.20 + C++17（MinGW） |
| **依赖库** | nlohmann-json（vcpkg） |

---

## 2. 文件树结构

```text
Synera2026/
├── assets/                        # 资源文件
│   ├── button/                    #   按钮图标资源
│   ├── effects/                   #   特效资源（掉落动画等）
│   ├── fonts/                     #   字体
│   ├── genzonmikuji/              #   关于
│   ├── icons/                     #   通用图标
│   ├── music/                     #   音乐
│   ├── sounds/                    #   音效
│   ├── ui/                        #   界面元素
│   ├── units/                     #   单位绘制
│   └── ulti/                      #   大招特效
│
├── src/                           # 源代码
│   ├── main.cpp                   #   程序入口
│   ├── core/                      #   核心逻辑层
│   │   ├── Game.cpp/h             #     游戏主循环、阶段管理、回合推进
│   │   ├── Board.cpp/h            #     棋盘逻辑（放置/移动/占用/半场判定）
│   │   ├── Bench.cpp/h            #     备战区（8槽位管理/交换/合成）
│   │   ├── Player.cpp/h           #     玩家属性（血量/金币/等级/人口/装备表）
│   │   ├── Unit.cpp/h             #     单位基类（属性/羁绊/装备/升星/攻击范围）
│   │   ├── Shop.cpp/h             #     商店逻辑（招募/刷新/装备购买/合成）
│   │   ├── EnemySpawner.cpp/h     #     敌方生成器（波次配置/随轮次增强）
│   │   ├── EnemyAI.cpp/h          #     敌方AI（决策/索敌/行动）
│   │   └── Commondata.h           #     全局宏定义与通用类型头文件
│   │
│   ├── combat/                    #   战斗系统层
│   │   ├── StateMachine.cpp/h     #     单位状态机（状态转换/事件驱动）
│   │   ├── Skill.cpp/h            #     技能系统（主动/被动/终极/触发）
│   │   ├── Pathfinding.cpp/h      #     寻路系统（A* 算法）
│   │   └── Targeting.cpp/h        #     索敌系统（多策略/优先级/范围判定）
│   │
│   ├── units/                     #   角色派生类（9名角色）
│   │   ├── UnitFactory.cpp/h      #     单位工厂（按类型名创建/随机生成）
│   │   ├── HakureiReimu.cpp/h     #     博丽灵梦（人/巫女，十字普攻+周围大招）
│   │   ├── KirisameMarisa.cpp/h   #     雾雨魔理沙（人/魔法使，纵向普攻+横向魔炮）
│   │   ├── KomeijiKoishi.cpp/h    #     古明地恋（妖/魔法使，近战AOE混乱）
│   │   ├── KonpakuYoumu.cpp/h     #     魂魄妖梦（人/女仆，双刀二连击）
│   │   ├── AliceMargatroid.cpp/h  #     爱丽丝（妖/魔法使，上方4格直线+4×3矩形大招）
│   │   ├── ShameimaruAya.cpp/h    #     射命丸文（妖/记者，上方2×3矩形+全屏大招）
│   │   ├── HimekaidouHatate.cpp/h #     姬海棠果（妖/记者，上方2×3矩形+全屏大招）
│   │   ├── IzayoiSakuya.cpp/h     #     十六夜咲夜（人/女仆，全屏时停飞刀）
│   │   └── KochiyaSanae.cpp/h     #     东风谷早苗（人/巫女，周围24格5×5包围盒AOE）
│   │
│   ├── ui/                        #   图形界面层（Qt6 Widgets）
│   │   ├── MainWindow.cpp/h       #     主窗口（开场动画/主菜单/游戏面板/存档管理）
│   │   ├── BoardWidget.cpp/h      #     棋盘控件（拖拽/渲染/动画）
│   │   ├── ShopWidget.cpp/h       #     商店控件（购买/刷新/装备/图鉴）
│   │   ├── ShopWindow.cpp/h       #     商店弹窗
│   │   ├── BattleLogWidget.cpp/h  #     战斗日志面板
│   │   ├── StatsWidget.cpp/h      #     统计信息面板（羁绊/经济/属性）
│   │   ├── ChartWidget.cpp/h      #     图表控件（战斗记录可视化）
│   │   ├── MusicPlayerWidget.cpp/h#     音乐播放器控件
│   │   ├── FontConfig.cpp/h       #     字体全局配置
│   │   └── StartWindow.cpp/h      #     开始界面
│   │
│   └── nlohmann/                  #   第三方 JSON 库
│       └── json.hpp
│
├── docs/                          # 设计文档
│   ├── 装备数值.md                #   装备模板/特效/星级/价格/合成规则
│   ├── 角色数值.md                #   角色基础属性与星级倍率
│   ├── 大招逻辑.md                #   角色大招/符卡机制设计
│   ├── 经济结算系统.md            #   金币/利息/连胜连败结算规则
│   ├── 装备系统待补充逻辑.md       #   装备系统遗留逻辑
│   ├── 装备系统与角色装备名分离方案.md
│   ├── 完成情况.md                  #   功能完成度逐条核查
│   └── UI改进建议.md               #   界面交互优化建议
│
├── markdowns/                     # 帮助/角色介绍文档
│   ├── about.md                   #   关于页面
│   ├── help.md                    #   帮助主页
│   ├── help_intro.md              #   玩法介绍
│   ├── help_operation.md          #   操作说明
│   └── characters/                #   角色介绍
│
├── saves/                         # 存档文件（.sav 格式）
├── build/                         # CMake 构建输出
├── vcpkg_installed/               # vcpkg 依赖
├── CMakeLists.txt                 # CMake 构建配置
├── CMakeLists.bak                 # CMake 构建配置备份
├── vcpkg.json                     # vcpkg 依赖清单
├── vcpkg-configuration.json       # vcpkg 配置
├── launcher_main.cpp              # 独立启动器源码（纯 Win32）
├── launcher.rc / launcher.res     # 启动器资源文件
├── build.ps1                      # 构建脚本（PowerShell）
├── run.bat                        # 运行脚本（批处理）
├── AI_usage.md                    # AI 辅助开发使用说明
└── README.md                      # 项目说明文档（本文件）
```

### 核心文件夹说明

| 文件夹 | 说明 |
|--------|------|
| `src/core/` | 游戏核心逻辑：棋盘、备战区、单位基类、玩家属性、商店、AI、敌人生成 |
| `src/combat/` | 战斗子系统：状态机驱动回合、A\* 寻路、多策略索敌、技能多态 |
| `src/units/` | 9 名角色派生类 + 工厂模式，各角色独有攻击范围偏移表和符卡技能 |
| `src/ui/` | Qt6 Widgets 图形界面：主窗口、棋盘渲染、商店面板、战斗日志、统计图表、音乐播放 |
| `assets/` | 所有图片、音效、字体、UI 素材资源；`ulti/` 按角色分子目录存放大招特效 |
| `docs/` | 项目设计文档（装备/角色数值/大招逻辑/经济结算）+ 功能完成度核查 |
| `markdowns/` | 游戏内帮助页面与 9 名角色 Markdown 介绍文档 |
| `QTreadme/` | 开发过程备注：实现总结、升星装备规则、UI 改进记录、PvP 设计说明 |
| `saves/` | 游戏存档文件（`.sav` 格式，JSON 序列化） |

---

## 3. 核心类和数据结构

### 3.1 核心逻辑层（`src/core/`）

| 类名 | 文件 | 主要功能 |
|------|------|----------|
| **`Unit`** | `Unit.h/cpp` | 单位基类。封装 HP/ATK/Range/MaxMana/Mana 基础属性；管理种族（`Race`，2种：人/妖）与职业（`ClassType`，4种：巫女/魔法使/女仆/记者）羁绊标签；4 槽位装备系统（weapon/armor/acc×2）；3 合 1 升星（`levelUpStar`）；攻击范围偏移表（`normalAttackRangeMap_` / `ultimateAttackRangeMap_`）；全局羁绊计算（`CheckTraits`）；装备掉落（`DropEquipment`）。成员变量包含原始基准属性（`originalBase*`）和光环调整后基准属性（`base*`），支持羁绊光环叠加后再 `recalculateStats()` |
| **`Board`** | `Board.h/cpp` | 8×8 棋盘。管理单位放置/移动/移除/占用查询；提供 `isPlayerHalf()` / `isEnemyHalf()` 半场判定；`getUnitsForOwner()` 按归属获取所有单位；底层为 `vector<vector<shared_ptr<Unit>>>` |
| **`Bench`** | `Bench.h/cpp` | 备战区（8 槽位）。管理单位添加/移除/交换/合成；`combineUnits` 实现 3 合 1 升星 |
| **`Player`** | `Player.h/cpp` | 玩家实体。管理血量/金币/等级/经验/人口上限；持有 `Bench` 和全局装备表 `equipmentTable_`（`list<Equipment>`）；提供装备增删查改 API |
| **`Game`** | `Game.h/cpp` | 游戏主控制器。管理回合循环（准备→战斗→结算）；逐步战斗动画推进（`CombatStep` 枚举）；敌方波次生成与 AI 行动调度；存档/读档（加载前全面清理旧状态，序列化棋盘/备战区/装备表等全部状态）；战斗结算（扣血/金币/连胜连败） |
| **`Shop`** | `Shop.h/cpp` | 商店系统。提供 5 招募位 + 3 装备槽位；`refresh()` 生成新单位/装备；`buyUnit()` / `buyEquipment()` 购买逻辑；装备合成树（3 同名同星 → 1 同名高星，属性等比×2叠加，支持暂存区+备战区+棋盘装备） |
| **`EnemySpawner`** | `EnemySpawner.h/cpp` | 敌方生成器。加载 JSON 波次配置（`EnemyWaveConfig`）；`spawnWave(round)` 按轮次随机生成敌方阵容；属性每 3 轮 +20% |
| **`EnemyAI`** | `EnemyAI.h/cpp` | 敌方 AI 决策。`makeDecision()` 为每个敌方单位决定移动/攻击/技能动作；`selectTarget()` 基于威胁度评估选择目标；A\* 寻路到攻击位（非目标格）；无路可走时绝境攻击 |

### 3.2 战斗系统层（`src/combat/`）

| 类名 | 文件 | 主要功能 |
|------|------|----------|
| **`StateMachine`** | `StateMachine.h/cpp` | 单位状态机。管理 `UnitState`（Idle/Ready/Attacking/Casting/Moving/Dead）之间的转换；事件驱动（`StateEvent`）；支持自定义转换规则（`addTransition`） |
| **`Skill`** | `Skill.h/cpp` | 技能系统基类。支持 Active/Passive/Ultimate/Trigger 四种类型；`SkillEffect` 结构体定义伤害/治疗/增益/减益/眩晕/冰冻/击退/护盾等效果 |
| **`Pathfinding`** | `Pathfinding.h/cpp` | 寻路系统。提供 **A\*** 寻路算法静态方法；`getWalkableNeighbors()` 获取 8 方向可行走邻格；支持棋盘占用碰撞检测 |
| **`TargetingSystem`** | `Targeting.h/cpp` | 索敌系统。支持 12 种索敌策略（Nearest/LowestHP/HighestHP/LowestATK/HighestATK/FrontRow/BackRow/MostDangerous 等）；多策略优先级组合（`TargetingPriority` + 权重）；支持普攻/大招不同范围判定 |

### 3.3 单位派生类（`src/units/`）

| 类名 | 角色名 | 种族 | 职业 | 特色 |
|------|--------|:--:|:--:|------|
| `Unit_Reimu` | 博丽灵梦 | 人 | 巫女 | 2 格十字普攻 + 周围大招 |
| `Unit_Marisa` | 雾雨魔理沙 | 人 | 魔法使 | 全列纵向普攻 + 全行横向魔炮 |
| `Unit_Koishi` | 古明地恋 | 妖 | 魔法使 | 近战 AOE 混乱攻击 |
| `Unit_Youmu` | 魂魄妖梦 | 人 | 女仆 | 双刀二连击 |
| `Unit_Alice` | 爱丽丝 | 妖 | 魔法使 | 上方 4 格直线普攻 + 4×3 矩形大招（75 伤害/最多 4 人） |
| `Unit_Aya` | 射命丸文 | 妖 | 记者 | 上方 2×3 矩形普攻 + 全屏大招（100 伤害/最多 8 人） |
| `Unit_Hatate` | 姬海棠果 | 妖 | 记者 | 上方 2×3 矩形普攻 + 全屏大招（80 伤害/最多 8 人） |
| `Unit_Sakuya` | 十六夜咲夜 | 人 | 女仆 | 全屏时停飞刀 + 3 次额外行动（90 伤害/最多 5 人） |
| `Unit_Sanae` | 东风谷早苗 | 人 | 巫女 | 十字 2 格普攻 + 5×5 包围盒大招（85 伤害/最多 3 人） |

### 3.4 UI 层（`src/ui/`）

| 类名 | 文件 | 主要功能 |
|------|------|----------|
| **`MainWindow`** | `MainWindow.h/cpp` | 主窗口。管理开场动画、主菜单、游戏面板切换、存档管理界面、音量/亮度设置 |
| **`BoardWidget`** | `BoardWidget.h/cpp` | 棋盘渲染控件。拖拽摆放/交换；渲染单位图标/血条/蓝条/星级；点击信息面板（HP/ATK/Mana/装备/羁绊）；拖动时攻击范围预览（橙红高亮）；战斗倍速按钮；移动/伤害/死亡动画；掉落闪烁动画 |
| **`ShopWidget`** | `ShopWidget.h/cpp` | 商店面板。购买/刷新/锁定单位；装备购买/合成；装备图鉴标签页 |
| **`BattleLogWidget`** | `BattleLogWidget.h/cpp` | 战斗日志面板。展示每回合收支/胜负/掉落 |
| **`StatsWidget`** | `StatsWidget.h/cpp` | 统计面板。展示羁绊激活状态/经济/属性信息 |
| **`ChartWidget`** | `ChartWidget.h/cpp` | 图表控件。战斗记录可视化（收支/血量曲线） |

### 3.5 关键数据结构

```cpp
// 单位属性结构
struct UnitStats { int hp; int atk; int maxMana; int mana; };

// 装备结构
struct Equipment {
    int id;              // 全局唯一 ID
    std::string name;    // 装备名称
    int atkBonus;        // 攻击力加成
    int hpBonus;         // 生命值加成
    int type;            // 0:武器 1:防具 2:饰品1 3:饰品2
    std::string restriction; // 职业限制（空=无限制）
    int starLevel;       // 星级品质（1/2/3）
    int ownerUnitId;     // 装备到的角色ID（0=暂存区未装备）
    std::string effect;  // 特殊效果："crit"/"lifesteal"/"armor"/""
};

// 装备模板（Commondata.h）
struct EquipmentTemplate {
    std::string name;         // 装备名称
    int type;                 // 0:武器 1:防具 2:饰品
    int atkMin, atkMax;       // 攻击加成随机范围
    int hpMin, hpMax;         // 生命加成随机范围
    std::string restriction;  // 职业限制
    int starLevel;            // 品质（1/2/3）
    std::string effect;       // 特殊效果（"crit"/"lifesteal"/"armor"/""）
    int tag;                  // 装备编号（1-20，0=无套装）
};

// 战斗日志条目
struct BattleLogEntry {
    int round;           // 回合数
    bool playerWon;      // 胜负
    int remainingUnits;  // 存活单位数
    int totalGoldEarned; // 总收入
    int healthLost;      // 损失生命
};
```

---

## 4. 算法描述

### 4.1 寻路算法（`Pathfinding`）

提供 **A\*** 寻路算法：

- **A\*（启发式搜索）**：
  - 使用欧氏距离作为启发函数 $h(n)$：`hypot(dx, dy)`
  - 优先队列按 $f(n) = g(n) + h(n)$ 排序（`PathNode::getFCost()`）
  - 时间复杂度优于 BFS，适合大型网格

- **碰撞检测**：`getWalkableNeighbors()` 检查 `board.isValidPosition()` 和 `!board.isOccupied()` 确保不穿过己方单位

### 4.2 目标锁定算法（`TargetingSystem`）

支持 **12 种索敌策略** 和 **多策略优先级组合**：

- **默认策略**（`Nearest`）：选择欧氏距离最近的存活敌方单位
- **优先级评分系统**：
  - 对每个候选目标计算综合评分 $score = \sum (weight_i \times score_i)$
  - 策略包括：`LowestHP`（血量最低优先）、`HighestThreat`（威胁度最高优先）、`FrontRow`（前排优先）等
- **过滤器链**：`Alive`（存活）、`InRange`（范围内）、`SameRow`（同行）等
- **范围判定**：支持基于攻击范围偏移表的普攻/大招不同范围判定（`isInRange(source, targetX, targetY, type)`）
- **威胁度计算**：综合 ATK × HP × range 评估目标危险程度

### 4.3 羁绊计算算法（`Unit::CheckTraits`）

静态方法，在战斗开始前调用，对上阵单位按种族和职业统计并应用光环：

1. **重置基准属性**：将所有单位的 `base*` 恢复为 `originalBase*`，清除 `hasBackstab_` 等机制标志
2. **统计计数**：`map<Race, int>` 和 `map<ClassType, int>` 分别统计种族和职业出现次数
3. **光环应用**（当前全部为属性光环类，机制改变类尚未实现）：

   | 羁绊 | 类型 | 触发条件 | 效果 |
   |------|:--:|:--:|------|
   | 人（Ningen） | 属性光环 | ≥2 | ATK +10%；≥4 人时 +20% |
   | 妖（Youkai） | 属性光环 | ≥2 | HP +10%；≥4 妖时 +20% |
   | 巫女（Miko） | 属性光环 | ≥2 | 最大法力 -25（更快放大招） |
   | 魔法使（Mahoushi） | 属性光环 | ≥2 | ATK +16% |
   | 女仆（Maid） | 属性光环 | ≥2 | HP +12%（`hasBackstab_` 字段预留但未激活） |
   | 记者（Journalist） | 属性光环 | ≥2 | ATK +24%（法力回复机制预留但未激活） |

4. **重新计算**：对所有单位调用 `recalculateStats()`，将光环后的 `base*` 和装备加成合并为最终属性
5. **返回激活描述**：返回描述字符串列表供 UI 展示

### 4.4 分步战斗系统（`Game::doEnemyMoveStep`）

战斗采用逐步推进机制，每个定时器 tick（默认 100ms）执行一个单位的行动，**敌我交替轮询**：

1. **交替行动**：敌→我→敌→我... 交替执行，而非敌方全动完再轮到我方。避免人数劣势时被围殴至死无法还手。
2. **每微回合**：双方所有存活单位各行动一次后进入下一微回合，最多 100 微回合强制平局
3. **清理**（`cleanupDeadUnits`）：死亡单位从棋盘和敌方列表中移除
4. **动画超时保护**（`MainWindow::onCombatStep`）：若动画阻塞超 30 tick（≈3秒），强制清除动画继续战斗
5. **绝境攻击**：单位被夹击无路可走且目标不在正常攻击范围时，强制攻击最近敌人
6. **敌方单位存活保留**：回合结束后敌方存活单位不再被清除，与新部署单位一同留在棋盘上

### 4.5 点击信息面板（`BoardWidget::drawInfoPanel`）

点击棋盘或备战区单位时，在其右侧弹出半透明信息面板，显示：
- **HP/ATK/Mana 滑动条**：直观展示当前/最大值比例
- **专属武器**：角色固定武器名
- **穿戴装备列表**：4 槽位的名称、星级、ATK/HP 加成，每件装备右侧 [×] 按钮可卸下回背包
- **激活羁绊**：统计当前场上种族/职业人数，显示该单位享受的光环效果

---

## 5. 辅助函数与工具

### 5.1 全局宏（`Commondata.h`）

| 宏 | 值 | 用途 |
|----|-----|------|
| `BOARD_M`, `BOARD_N` | 8, 8 | 棋盘行×列 |
| `BENCH_SIZE` / `BENCH_SLOTS` | 8 | 备战区槽位数 |
| `WINDOW_DEFAULT_WIDTH/HEIGHT` | 1600/900 | 默认窗口尺寸 |
| 各类 UI 尺寸宏 | 见文件 | `BOARD_BASE_CELL_SIZE`(96)、`CELL_SIZE_MIN/MAX` 等数十个布局常量 |

### 5.2 单位工厂（`UnitFactory`）

- `createUnitByType(unitType, owner)` — 按角色类型名（"Reimu"/"Marisa" 等）创建对应派生类实例并返回 `shared_ptr<Unit>`；各角色在各自构造函数中设置固定属性值
- `createRandomShopUnit(owner, slotIndex, round)` — 按轮次随机选择类型生成商店单位（参数预留给属性随槽位/轮次变化）
- `getAvailableUnitTypes()` — 返回 9 种可用单位类型名列表

### 5.3 存档序列化辅助函数（`Game.cpp` 内部）

| 函数 | 用途 |
|------|------|
| `writeQuotedString(ostream, string)` | 写入带引号的字符串（处理含空格的名词） |
| `readQuotedString(istream, string)` | 读取带引号的字符串 |
| `writeUnitData(ostream, unit)` | 将单位完整状态序列化为一行（HP/MaxHP/ATK/Range/MaxMana/Mana/Owner/Star/EquipmentStar + 4 字符串字段） |
| `readUnitData(istream)` | 反序列化一行数据并调用 `UnitFactory` 重建单位 + `restoreFromSave` 恢复精确值 |

### 5.4 `Unit` 内部辅助

| 方法 | 用途 |
|------|------|
| `recalculateStats()` | 根据 `base*` + 装备 `atkBonus/hpBonus` + `equipmentStar_` + `starLevel_` 折算后写入 `atk_/hp_` 等运行时属性 |
| `RaceToString(Race)` / `ClassToString(ClassType)` | 枚举转字符串，供 UI 显示 |
| `EquipmentTypeToString(int)` | 装备槽位类型转中文（武器/防具/饰品1/饰品2） |
| `initDefaultAttackRanges()` | 初始化默认攻击范围偏移表（相邻 4 格普攻 + 曼哈顿距离 2 内大招） |

### 5.5 `TargetingSystem` 工具方法

| 方法 | 用途 |
|------|------|
| `distance(x1,y1, x2,y2)` | 欧氏距离 |
| `manhattanDistance(x1,y1, x2,y2)` | 曼哈顿距离 |
| `calculateThreatLevel(unit)` | 综合威胁度（ATK × HP × range） |
| `calculateDistanceScore(distance, range)` | 距离评分（范围内线性衰减） |
| `calculateHPBasedScore(unit, lowestHP)` | 血量评分 |

---

## 6. 已实现扩展功能

### 已实现的扩展功能

| # | 扩展功能 | 实现说明 |
|:--:|----------|----------|
| 1 | **利息机制** | `Game::startPhaseOne()` 中 `max(5, gold/15)` 每回合发放；`resolveBattle()` 中连胜/连败额外奖励 |
| 2 | **装备合成** | `Shop::combineEquipment()` — 3 件同名同星→1 件同名高1星（属性等比×2叠加）；搜遍暂存区+备战区+棋盘；商店合成按钮触发 |
| 3 | **敌方 AI 装备** | `Game::executeEnemyPreparation()` — 敌方从商店购买装备并优先装备高星单位 |
| 4 | **敌方失败扣血** | `Game::resolveBattle()` 中 `max(5, 存活×3 + 轮次)` 扣敌方血量 |
| 5 | **装备特效** | `Unit::onAttack()` / `takeDamage()` 中检测 crit（20%双倍）、lifesteal（10%吸血）、armor（+2护甲） |
| 6 | **装备系统** | 1★/2★/3★ 全装备模板（20件），4 职业各有对应装备，商店按轮次解锁星级 |
| 7 | **装备图鉴** | `ShopWidget` 标签页展示全部装备模板 |
| 8 | **掉落动画** | `BoardWidget::startEquipDropAnim()` — 金色闪烁衰减效果 |
| 9 | **战斗倍速** | `BoardWidget` 中 1×/2×/4× 循环切换，修改战斗定时器间隔 |
| 10 | **全角色符卡** | 9 角色普攻+大招均为东方原作符卡格式 |
| 11 | **存档删除** | `MainWindow::onDeleteSave()` — 加载界面每个存档右侧的删除按钮 |
| 12 | **点击信息面板** | 点击棋盘/备战区单位 → 右侧弹出半透明面板（HP/ATK/Mana 滑动条 + 装备列表 + 激活羁绊） |
| 13 | **拖动攻击范围预览** | 拖动单位时以橙红色高亮该单位放置后普攻覆盖的格子 |
| 14 | **战斗动画超时保护** | `onCombatStep` 中动画阻塞超 3 秒自动清除，防止战斗永久冻结 |
| 15 | **装备面板卸载** | 点击单位信息面板中装备条目的 [×] 按钮即可卸下装备 |
| 16 | **死亡掉落装备** | 敌方单位死亡时身上装备按星级掉落（1★35%/2★25%/3★15%），归玩家 |
| 17 | **交替战斗行动** | 战斗改为敌我交替轮询（敌→我→敌→我），而非敌全动完再轮到我方 |
| 18 | **敌方存活保留** | 回合结束后敌方存活单位保留在棋盘上，与新部署单位一同再战 |
| 19 | **装备页刷新按钮** | 装备标签页增加刷新按钮，与招募页功能一致 |

---

## 7. 构建与运行

### 7.1 环境要求

| 组件 | 版本要求 | 说明 |
|------|----------|------|
| **操作系统** | Windows 10/11 64-bit | 开发与运行平台 |
| **CMake** | ≥ 3.20 | 构建系统生成器 |
| **Qt** | 6.11.1 | GUI 框架（Core / Gui / Widgets / Multimedia / Svg） |
| **编译器** | MSVC 2022 或 MinGW-w64 13.1+ | Qt 6.11.1 自带 MinGW 工具链 |
| **vcpkg** | 最新版 | C++ 包管理器，用于安装 nlohmann-json |
| **Git** | 任意版本 | vcpkg 需要 Git 下载依赖 |

### 7.2 安装 Qt 6.11.1

1. 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
2. 安装时选择 Qt 6.11.1，勾选以下组件：
   - **MinGW 路径**（推荐）：`Qt 6.11.1` → `MinGW 6.11.1 64-bit`
   - **MSVC 路径**（备选）：`Qt 6.11.1` → `MSVC 2022 64-bit`
   - 附加库：`Qt Multimedia`、`Qt SVG`
3. 默认安装到 `D:\Qt\6.11.1\`（MinGW 子目录为 `mingw_64`，MSVC 为 `msvc2022_64`）

> 项目 `CMakeLists.txt` 自动探测两条路径：先尝试 `D:/Qt/6.11.1/msvc2022_64`，再尝试 `D:/Qt/6.11.1/mingw_64`。若安装在其他位置，需修改 `CMakeLists.txt` 中 `Qt6_DIR` 的路径

### 7.3 安装 vcpkg 与依赖

```powershell
# 1. 克隆 vcpkg（在任意目录，如 C:\）
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# 2. 运行引导脚本
.\bootstrap-vcpkg.bat

# 3. 安装项目依赖
.\vcpkg install nlohmann-json

# 4. （可选）全局集成，使 CMake 自动找到 vcpkg
.\vcpkg integrate install
```

> 若项目根目录已有 `vcpkg_installed/` 文件夹（即已通过 vcpkg manifest 模式安装），可跳过第 3-4 步。CMake 会自动识别 `vcpkg-configuration.json` 和 `vcpkg.json`。

### 7.4 构建项目

#### 方式一：PowerShell 脚本（MSVC，一键构建）

```powershell
# 在项目根目录执行
.\build.ps1                # Release 构建
.\build.ps1 -Debug          # Debug 构建
.\build.ps1 -Clean          # 清理后重新构建
```

脚本自动执行：查找 CMake → 检查 Visual Studio 2022 → 验证 Qt → 配置 CMake（`Visual Studio 17 2022` 生成器）→ 编译 → 输出 `build\bin\Synera.exe`。

#### 方式二：CMake 命令行（MinGW，推荐）

```powershell
# 在项目根目录执行
mkdir build -Force
cd build

# 配置（使用 MinGW 工具链）
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/mingw_64

# 编译
mingw32-make -j4

# 输出：build\bin\Synera.exe
```

> 若使用 MSVC 生成器：
> ```powershell
> cmake .. -G "Visual Studio 17 2022" -DQt6_DIR=D:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6
> cmake --build . --config Release
> ```

#### 构建产物

| 文件 | 路径 | 说明 |
|------|------|------|
| `Synera.exe` | `build/bin/` | 主游戏可执行文件 |
| `SyneraLauncher.exe` | `build/bin/` | 轻量级启动器（纯 Win32，检查环境后拉起主程序） |

### 7.5 运行游戏

#### 方式一：双击启动器

直接双击项目根目录的 `SyneraLauncher.exe`（构建后自动复制到根目录），它会自动定位并启动 `build\bin\Synera.exe`。

#### 方式二：批处理脚本

```batch
# 在项目根目录执行
.\run.bat
```

脚本自动检查 CMake、Qt、项目文件，查找已有可执行文件或自动构建，然后运行。

#### 方式三：直接运行

```powershell
# 确保 Qt bin 目录在 PATH 中，或使用 windeployqt 部署
D:\Qt\6.11.1\mingw_64\bin\windeployqt.exe build\bin\Synera.exe --qmldir src\ui

# 然后双击
.\build\bin\Synera.exe
```

> **注意**：直接运行前必须用 `windeployqt` 将 Qt 运行时 DLL 复制到 exe 同目录，否则会报缺失 DLL 错误。`run.bat` 脚本已自动处理此步骤。

### 7.6 Qt 部署（分发用）

若需将程序打包为独立可分发版本：

```powershell
# 进入构建输出目录
cd build\bin

# MinGW 版本
D:\Qt\6.11.1\mingw_64\bin\windeployqt.exe Synera.exe --qmldir ..\..\src\ui

# MSVC 版本
D:\Qt\6.11.1\msvc2022_64\bin\windeployqt.exe Synera.exe --qmldir ..\..\src\ui
```

> `windeployqt` 自动复制所需的 Qt DLL、插件、QML 模块到可执行文件目录。

### 7.7 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `CMake Error: Could not find Qt6` | Qt 路径不正确 | 检查 `CMakeLists.txt` 中 `Qt6_DIR` 是否指向实际安装路径 |
| `missing nlohmann/json.hpp` | vcpkg 依赖未安装 | 运行 `vcpkg install nlohmann-json` |
| `mingw32-make: command not found` | MinGW 不在 PATH | 将 `D:\Qt\6.11.1\mingw_64\bin` 加入系统 PATH |
| 运行时缺少 DLL | Qt 运行时未部署 | 运行 `windeployqt Synera.exe` |
| 编译时 `MOC` 错误 | Qt MOC 未运行 | 确保 `CMAKE_AUTOMOC ON`（`CMakeLists.txt` 已设置） |

---

## 8. 设计说明与已知限制

### 8.1 类无限模式

本游戏采用**类无限模式**而非固定关卡制：游戏在达到预设的最大回合数 100 前，只有在以下条件之一满足时游戏才会终止：

- 我方生命值（玩家血量）归零 → 游戏失败
- 敌方生命值（Boss 血量）归零 → 游戏胜利

随轮次推进，敌方单位数量、属性和种类持续增强，玩家需尽可能存活更多轮次。

### 8.2 回合制战斗

本游戏采用**回合制（大回合中有若干个微回合）的战斗系统**：每个定时器 tick（默认 100ms）执行一个单位的行动，敌我交替轮询。单位的攻击/移动/施法均为离散回合事件，由 `Game::doEnemyMoveStep` 统一调度。

### 8.3 三维羁绊系统（种族-职业-装备）

本游戏实现了**种族 × 职业 × 装备**的三维羁绊体系：

| 维度 | 说明 |
|------|------|
| **种族（Race）** | 2 种：人（Ningen）、妖（Youkai），≥2 人时 +ATK，≥2 妖时 +HP，≥4 时效果翻倍 |
| **职业（ClassType）** | 4 种：巫女（Miko）、魔法使（Mahoushi）、女仆（Maid）、记者（Journalist），各职业 ≥2 人时触发专属属性光环 |
| **装备套装（Equipment Set）** | 20 件东方主题装备，按 tag 分属 4 大套装（神道/魔法/庭师/天狗套装），同一单位穿戴多件同套装装备触发额外套装效果 |

羁绊存在等级细分：种族/职业羁绊按人数阈值分档；装备星级分 1★/2★/3★ 三档，属性等比翻倍；装备合成树为 3 件同名同星 → 1 件同名高 1 星。

### 8.4 装备星级双重机制

装备星级由双重因素共同决定：

- **固有星级**：不同装备模板本身即具有不同星级（如低阶装备 1★，高阶装备 3★），直接影响属性范围
- **升星叠加**：3 件同名同星装备可在商店合成 1 件同名高星装备（属性等比 ×2 叠加）

此外，单位升星（1★→2★→3★）时，其穿戴装备的 `equipmentStar_` 字段也会参与 `recalculateStats()` 的最终属性折算，形成单位星级 × 装备星级的交叉影响矩阵。

### 8.5 经济结算系统

本游戏实现了丰富的经济结算规则（详见 `docs/经济结算系统.md`）：

| 结算项 | 规则 |
|--------|------|
| **基础金币** | 每轮固定基础收入（随轮次递增） |
| **利息机制** | 每 15 金币产生 1 利息（下限 5 金币），鼓励存钱运营 |
| **首功奖励** | 按我方单位击杀数额外奖励金币 |
| **连胜/连败** | 连胜或连败均有额外金币加成，鼓励不同运营策略 |
| **存活奖励** | 结算时按我方存活单位数额外奖励 |

此外，`ChartWidget` 提供了**经济系统统计报表**，以折线图形式可视化每回合的金币收支、血量变化，辅助玩家复盘决策。

### 8.6 UI 已知限制

由于 Qt6 Widgets 框架本身的特性，当前 UI 存在以下已知限制，短期内难以修复：

| 问题 | 原因 |
|------|------|
| **跨页面拖拽装备不可行** | 备战时装备暂存区在 `ShopWidget` 中，而棋盘在 `BoardWidget` 中，两者为独立的 Qt 控件树。Qt 的 Drag & Drop 机制在跨 Widget 层级传递自定义 MIME 数据时存在兼容性问题 |
| **页面排版与背景缩放冲突** | 使用 `setStyleSheet` 设置主窗口背景图时，`border-image` 的拉伸规则与 `QSplitter`/`QGridLayout` 的自动缩放行为存在冲突 |

> 这些问题不影响核心游戏逻辑，属于 UI 体验层面的已知限制。

---

## 9. 参考资料与致谢

### 9.1 技术参考

| 资源 | 用途 |
|------|------|
| [Qt 6 Documentation](https://doc.qt.io/qt-6/) | Qt GUI 框架 API 参考 |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 序列化/反序列化（存档系统） |
| [CMake Documentation](https://cmake.org/documentation/) | 跨平台构建系统 |
| [vcpkg](https://github.com/microsoft/vcpkg) | C++ 包管理器 |

### 9.2 素材来源

| 来源 | 用途 | 许可 |
|------|------|------|
| Kenney · 1-bit Pack | 部分 UI 与单位图标参考 | CC0 |
| OpenGameArt | 部分角色精灵图参考 | CC-BY / CC0 |
| 东方 Project 原作 | 角色设计灵感与符卡命名参考 | — |

> 项目中所用素材均为免费可商用或已获授权的资源。角色设计灵感来源于东方 Project 系列作品，符卡名称参考原作设定

### 9.3 致谢

- 感谢 Qt 开源社区提供的优秀 C++ GUI 框架
- 感谢 GitHub Copilot 在开发过程中提供的代码辅助
- 感谢东方 Project 原作团队（上海爱丽丝幻乐团）的角色与世界观设计灵感

---