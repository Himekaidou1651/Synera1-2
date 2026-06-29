/**
 * @file    Board.h
 * @brief   棋盘系统，管理棋盘格上的单位放置、移动和查询
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"

/**
 * @class   Board
 * @brief   8×8 棋盘网格，提供单位放置、移除、移动及归属方查询
 */
class Board {
public:
    Board();

    /**
     * @brief  检查坐标是否在棋盘范围内
     * @param  x 行坐标
     * @param  y 列坐标
     * @return 是否有效
     */
    bool isValidPosition(int x, int y) const;

    /**
     * @brief  检查指定坐标是否已有单位占用
     * @param  x 行坐标
     * @param  y 列坐标
     * @return 是否被占用
     */
    bool isOccupied(int x, int y) const;

    /**
     * @brief  在指定坐标放置单位
     * @param  unit 要放置的单位
     * @param  x    行坐标
     * @param  y    列坐标
     * @return 是否放置成功
     */
    bool placeUnit(std::shared_ptr<Unit> unit, int x, int y);

    /**
     * @brief  移除指定坐标的单位
     * @param  x 行坐标
     * @param  y 列坐标
     * @return 是否移除成功
     */
    bool removeUnit(int x, int y);

    /**
     * @brief  将单位从一个坐标移动到另一个坐标
     * @param  fromX 源行坐标
     * @param  fromY 源列坐标
     * @param  toX   目标行坐标
     * @param  toY   目标列坐标
     * @return 是否移动成功
     */
    bool moveUnit(int fromX, int fromY, int toX, int toY);

    /**
     * @brief  获取指定坐标的单位
     * @param  x 行坐标
     * @param  y 列坐标
     * @return 单位指针（无单位时返回 nullptr）
     */
    std::shared_ptr<Unit> getUnit(int x, int y) const;

    /**
     * @brief  清空棋盘上所有单位
     */
    void clear();

    /**
     * @brief  获取棋盘上所有单位的列表（含坐标）
     * @return 三元组 (x, y, unit) 的列表
     */
    std::vector<std::tuple<int, int, std::shared_ptr<Unit>>> getAllUnits() const;

    /**
     * @brief  获取指定归属方的所有单位
     * @param  owner 归属方（玩家/敌方）
     * @return 单位列表
     */
    std::vector<std::shared_ptr<Unit>> getUnitsForOwner(Owner owner) const;

    /**
     * @brief  检查坐标是否在玩家半场（x >= BOARD_M/2）
     * @param  x 行坐标
     * @param  y 列坐标
     * @return 是否在玩家半场
     */
    bool isPlayerHalf(int x, int y) const;

    /**
     * @brief  检查坐标是否在敌方半场（x < BOARD_M/2）
     * @param  x 行坐标
     * @param  y 列坐标
     * @return 是否在敌方半场
     */
    bool isEnemyHalf(int x, int y) const;

private:
    std::vector<std::vector<std::shared_ptr<Unit>>> grid_;  ///< 棋盘二维网格
};
