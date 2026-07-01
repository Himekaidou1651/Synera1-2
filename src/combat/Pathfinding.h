/**
 * @file    Pathfinding.h
 * @brief   寻路系统，提供 A* 路径搜索算法
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Commondata.h"
#include "../core/Board.h"

/**
 * @brief A* 寻路节点
 */
struct PathNode {
    int x, y;
    int gCost;  ///< 从起点的实际成本
    int hCost;  ///< 到目标的启发式估计成本
    std::shared_ptr<PathNode> parent;

    int getFCost() const { return gCost + hCost; }
};

/**
 * @class   Pathfinding
 * @brief   寻路系统，提供 A* 网格路径搜索算法
 */
class Pathfinding {
public:
    /**
     * @brief  使用 A* 算法寻找最优路径
     * @param  board    棋盘实例
     * @param  startX   起点行坐标
     * @param  startY   起点列坐标
     * @param  targetX  目标行坐标
     * @param  targetY  目标列坐标
     * @return 从起点到目标的路径坐标列表（不含起点）
     */
    static std::vector<std::pair<int, int>> astarPath(const Board& board,
                                                       int startX, int startY,
                                                       int targetX, int targetY);
    
    /**
     * @brief  获取相邻可行走的位置
     * @param  board 棋盘实例
     * @param  x     当前行坐标
     * @param  y     当前列坐标
     * @return 可通行的相邻坐标列表
     */
    static std::vector<std::pair<int, int>> getWalkableNeighbors(const Board& board,
                                                                  int x, int y);

private:
    /**
     * @brief  启发式函数（欧氏距离）
     * @param  x1, y1 点1坐标
     * @param  x2, y2 点2坐标
     * @return 启发式估计值
     */
    static int heuristic(int x1, int y1, int x2, int y2);
};