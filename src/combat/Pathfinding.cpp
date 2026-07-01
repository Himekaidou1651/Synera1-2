/**
 * @file    Pathfinding.cpp
 * @brief   寻路系统实现
 * @author  
 * @date    2026-06-24
 */

#include "Pathfinding.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

int Pathfinding::heuristic(int x1, int y1, int x2, int y2) {
    // 欧氏距离启发式函数
    double dx = static_cast<double>(x1 - x2);
    double dy = static_cast<double>(y1 - y2);
    return static_cast<int>(std::round(std::hypot(dx, dy)));
}

std::vector<std::pair<int, int>> Pathfinding::getWalkableNeighbors(const Board& board,
                                                                    int x, int y) {
    std::vector<std::pair<int, int>> neighbors;
    
    // 8 个方向（包括对角线）
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    
    for (int i = 0; i < 8; ++i) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        
        // 检查位置是否有效且未被占据
        if (board.isValidPosition(nx, ny) && !board.isOccupied(nx, ny)) {
            neighbors.push_back({nx, ny});
        }
    }
    
    return neighbors;
}

std::vector<std::pair<int, int>> Pathfinding::astarPath(const Board& board,
                                                        int startX, int startY,
                                                        int targetX, int targetY) {
    std::vector<std::pair<int, int>> path;
    
    if (!board.isValidPosition(startX, startY) || !board.isValidPosition(targetX, targetY)) {
        return path;
    }
    
    if (startX == targetX && startY == targetY) {
        return path;
    }
    
    auto cmp = [](const std::shared_ptr<PathNode>& a, const std::shared_ptr<PathNode>& b) {
        return a->getFCost() > b->getFCost();
    };
    std::priority_queue<std::shared_ptr<PathNode>, 
                       std::vector<std::shared_ptr<PathNode>>,
                       decltype(cmp)> openSet(cmp);
    
    std::unordered_set<int> closedSet;
    auto hashPos = [](int x, int y) { return x * BOARD_N + y; };
    
    auto startNode = std::make_shared<PathNode>();
    startNode->x = startX;
    startNode->y = startY;
    startNode->gCost = 0;
    startNode->hCost = heuristic(startX, startY, targetX, targetY);
    
    openSet.push(startNode);
    
    while (!openSet.empty()) {
        auto current = openSet.top();
        openSet.pop();
        
        int hash = hashPos(current->x, current->y);
        if (closedSet.find(hash) != closedSet.end()) {
            continue;
        }
        
        closedSet.insert(hash);
        
        if (current->x == targetX && current->y == targetY) {
            // 回溯构建路径
            auto node = current;
            while (node) {
                path.push_back({node->x, node->y});
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());
            path.erase(path.begin());  // 移除起点
            return path;
        }
        
        auto neighbors = getWalkableNeighbors(board, current->x, current->y);
        for (auto [nx, ny] : neighbors) {
            int nHash = hashPos(nx, ny);
            if (closedSet.find(nHash) == closedSet.end()) {
                int newGCost = current->gCost + 1;
                int hCost = heuristic(nx, ny, targetX, targetY);
                
                auto neighbor = std::make_shared<PathNode>();
                neighbor->x = nx;
                neighbor->y = ny;
                neighbor->gCost = newGCost;
                neighbor->hCost = hCost;
                neighbor->parent = current;
                
                openSet.push(neighbor);
            }
        }
        
        // 若目标点被占据且当前节点与目标相邻，允许将目标作为可达节点
        if (board.isValidPosition(targetX, targetY) && (current->x != targetX || current->y != targetY)) {
            int nHash = hashPos(targetX, targetY);
            if (closedSet.find(nHash) == closedSet.end() &&
                std::abs(current->x - targetX) <= 1 &&
                std::abs(current->y - targetY) <= 1) {
                auto neighbor = std::make_shared<PathNode>();
                neighbor->x = targetX;
                neighbor->y = targetY;
                neighbor->gCost = current->gCost + 1;
                neighbor->hCost = 0;
                neighbor->parent = current;
                openSet.push(neighbor);
            }
        }
    }
    
    return path;  // 无路径可达
}