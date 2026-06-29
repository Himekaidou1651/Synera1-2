# ============================================================================
# @file    CMakeLists.txt
# @brief   Synera 项目构建配置，包含主程序和 Win32 启动器
# @author  
# @date    2026-06-24
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(Synera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== Qt 配置 ==========
# 若未通过环境变量或 CMake 命令行指定 Qt6_DIR，则自动探测系统上已安装的 Qt6 路径
if(NOT DEFINED Qt6_DIR)
    if(EXISTS "D:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6")
        set(Qt6_DIR "D:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6")
    elseif(EXISTS "D:/Qt/6.11.1/mingw_64/lib/cmake/Qt6")
        set(Qt6_DIR "D:/Qt/6.11.1/mingw_64/lib/cmake/Qt6")
    endif()
endif()
find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia Svg REQUIRED)

# ========== 自动代码处理 ==========
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ========== 核心源文件 ==========
set(CORE_SOURCES
    src/core/Unit.cpp
    src/core/Shop.cpp
    src/core/Player.cpp
    src/core/Game.cpp
    src/core/Board.cpp
    src/core/Bench.cpp
    src/core/SaveManager.cpp
    src/core/EnemySpawner.cpp
    src/core/EnemyAI.cpp
)

set(CORE_HEADERS
    src/core/Unit.h
    src/core/Shop.h
    src/core/Player.h
    src/core/Game.h
    src/core/Board.h
    src/core/Bench.h
    src/core/SaveManager.h
    src/core/Commondata.h
)

# ========== 单位源文件 ==========
set(UNIT_SOURCES
    src/units/UnitFactory.cpp
    src/units/HakureiReimu.cpp
    src/units/KirisameMarisa.cpp
    src/units/KomeijiKoishi.cpp
    src/units/KonpakuYoumu.cpp
    src/units/AliceMargatroid.cpp
    src/units/ShameimaruAya.cpp
    src/units/HimekaidouHatate.cpp
    src/units/IzayoiSakuya.cpp
    src/units/KochiyaSanae.cpp
)

set(UNIT_HEADERS
    src/units/UnitFactory.h
    src/units/HakureiReimu.h
    src/units/KirisameMarisa.h
    src/units/KomeijiKoishi.h
    src/units/KonpakuYoumu.h
    src/units/AliceMargatroid.h
    src/units/ShameimaruAya.h
    src/units/HimekaidouHatate.h
    src/units/IzayoiSakuya.h
    src/units/KochiyaSanae.h
)

# ========== 对战系统源文件 ==========
set(COMBAT_SOURCES
    src/combat/Pathfinding.cpp
    src/combat/StateMachine.cpp
    src/combat/Skill.cpp
    src/combat/Targeting.cpp
)

set(COMBAT_HEADERS
    src/combat/Pathfinding.h
    src/combat/StateMachine.h
    src/combat/Skill.h
    src/combat/Targeting.h
)

# ========== UI 源文件 ==========
set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/BoardWidget.cpp
    src/ui/ShopWidget.cpp
    src/ui/ShopWindow.cpp
    src/ui/MusicPlayerWidget.cpp
    src/ui/BattleLogWidget.cpp
    src/ui/ChartWidget.cpp
    src/ui/StatsWidget.cpp
    src/ui/FontConfig.cpp
    src/main.cpp
)

set(UI_HEADERS
    src/ui/MainWindow.h
    src/ui/BoardWidget.h
    src/ui/ShopWidget.h
    src/ui/ShopWindow.h
    src/ui/MusicPlayerWidget.h
    src/ui/BattleLogWidget.h
    src/ui/ChartWidget.h
    src/ui/StatsWidget.h
    src/ui/FontConfig.h
)

# ========== 构建主程序 ==========
add_executable(Synera
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${UNIT_SOURCES}
    ${UNIT_HEADERS}
    ${COMBAT_SOURCES}
    ${COMBAT_HEADERS}
    ${UI_SOURCES}
    ${UI_HEADERS}
)

# ========== 链接 Qt 库 ==========
target_link_libraries(Synera
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Svg
)

# ========== 包含目录 ==========
target_include_directories(Synera PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/combat
    ${CMAKE_CURRENT_SOURCE_DIR}/src/units
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ========== 输出目录 ==========
set_target_properties(Synera PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
#  轻量级启动器 SyneraLauncher（纯 Win32，不需要 Qt）
#  输出到项目根目录，方便用户双击启动
# ============================================================================
add_executable(SyneraLauncher WIN32 launcher_main.cpp)
target_link_libraries(SyneraLauncher PRIVATE shlwapi)
target_compile_definitions(SyneraLauncher PRIVATE
    _UNICODE
    UNICODE
)

# ========== 启动器输出目录 ==========
set_target_properties(SyneraLauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)