/**
 * @file    SaveManager.h
 * @brief   存档管理器，负责游戏数据的序列化与反序列化
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Board.h"
#include "Player.h"
#include <string>
#include <memory>

class Game;  ///< 前向声明

/**
 * @class   SaveManager
 * @brief   存档管理器，提供对称的我方/敌方存档格式的读写
 */
class SaveManager {
public:
    /**
     * @brief  保存游戏到文件
     * @param  game     游戏实例
     * @param  filename 存档文件路径
     * @return 是否保存成功
     */
    static bool saveGame(const Game& game, const std::string& filename);

    /**
     * @brief  从文件加载游戏
     * @param  game     游戏实例（输出）
     * @param  filename 存档文件路径
     * @return 是否加载成功（失败时清理状态）
     */
    static bool loadGame(Game& game, const std::string& filename);

private:
    /**
     * @brief  写入单个单位数据
     * @param  out  输出流
     * @param  unit 单位指针
     */
    static void writeUnitData(std::ostream& out, const std::shared_ptr<Unit>& unit);

    /**
     * @brief  读取单个单位数据
     * @param  in 输入流
     * @return 重建的单位指针
     */
    static std::shared_ptr<Unit> readUnitData(std::istream& in);

    /**
     * @brief  写入 Player 状态行
     * @param  out    输出流
     * @param  player 玩家实例
     */
    static void writePlayerState(std::ostream& out, const Player& player);

    /**
     * @brief  读取 Player 状态行
     * @param  in     输入流
     * @param  player 玩家实例（输出）
     * @return 是否读取成功
     */
    static bool readPlayerState(std::istream& in, Player& player);

    /**
     * @brief  写入装备表
     * @param  out   输出流
     * @param  table 装备表
     */
    static void writeEquipmentTable(std::ostream& out, const std::list<Equipment>& table);

    /**
     * @brief  读取装备表并绑定到单位
     * @param  in     输入流
     * @param  table  装备表（输出）
     * @param  nextId 下一个装备 ID（输出）
     * @return 是否读取成功
     */
    static bool readEquipmentTable(std::istream& in, std::list<Equipment>& table, int& nextId);
};
