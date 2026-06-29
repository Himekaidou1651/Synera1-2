/**
 * @file    Board.cpp
 * @brief   棋盘系统实现
 * @author  
 * @date    2026-06-24
 */

#include "Board.h"

Board::Board() : grid_(BOARD_M, std::vector<std::shared_ptr<Unit>>(BOARD_N, nullptr))
{
}

bool Board::isValidPosition(int x, int y) const
{
    return x >= 0 && x < BOARD_M && y >= 0 && y < BOARD_N;
}

bool Board::isOccupied(int x, int y) const
{
    if (!isValidPosition(x, y))
    {
        return false;
    }
    return grid_[x][y] != nullptr;
}

bool Board::placeUnit(std::shared_ptr<Unit> unit, int x, int y)
{
    // 检查坐标合法性、空闲状态，然后设置单位位置并放置
    if (!unit || !isValidPosition(x, y) || grid_[x][y])
    {
        return false;
    }
    unit->setPosition(x, y);
    grid_[x][y] = std::move(unit);
    return true;
}

bool Board::removeUnit(int x, int y)
{
    // 移除棋盘上的单位，清除该格引用
    if (!isValidPosition(x, y))
    {
        return false;
    }
    if (!grid_[x][y])
    {
        return false;
    }
    grid_[x][y] = nullptr;
    return true;
}

bool Board::moveUnit(int fromX, int fromY, int toX, int toY)
{
    // 处理拖拽/移动单位，含非法放置检测
    if ((!isValidPosition(fromX, fromY)) || (!isValidPosition(toX, toY)))
    {
        return false;
    }
    if (!grid_[fromX][fromY] || grid_[toX][toY])
    {
        return false;
    }
    grid_[toX][toY] = std::move(grid_[fromX][fromY]);
    grid_[fromX][fromY] = nullptr;
    if (grid_[toX][toY])
    {
        grid_[toX][toY]->setPosition(toX, toY);
    }
    return true;
}

std::shared_ptr<Unit> Board::getUnit(int x, int y) const
{
    if (!isValidPosition(x, y))
    {
        return nullptr;
    }
    return grid_[x][y];
}

void Board::clear()
{
    for (auto &row : grid_)
    {
        for (auto &cell : row)
        {
            cell = nullptr;
        }
    }
}

std::vector<std::tuple<int, int, std::shared_ptr<Unit>>> Board::getAllUnits() const
{
    std::vector<std::tuple<int, int, std::shared_ptr<Unit>>> result;
    for (int i = 0; i < BOARD_M; i++)
    {
        for (int j = 0; j < BOARD_N; j++)
        {
            if (grid_[i][j])
            {
                result.emplace_back(i, j, grid_[i][j]);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<Unit>> Board::getUnitsForOwner(Owner owner) const
{
    std::vector<std::shared_ptr<Unit>> result;
    // 遍历棋盘，收集指定归属方的单位
    for (int i = 0; i < BOARD_M; i++)
    {
        for (int j = 0; j < BOARD_N; j++)
        {
            if (grid_[i][j] && grid_[i][j]->getOwner() == owner)
            {
                result.push_back(grid_[i][j]);
            }
        }
    }
    return result;
}

bool Board::isPlayerHalf(int x, int y) const {
    // 玩家单位在下半部分（x >= 4）
    return x >= BOARD_M / 2;
}

bool Board::isEnemyHalf(int x, int y) const {
    // 敌方单位在上半部分（x < 4）
    return x < BOARD_M / 2;
}
