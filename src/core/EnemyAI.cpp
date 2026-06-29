/**
 * @file    EnemyAI.cpp
 * @brief   敌方 AI 决策系统实现
 * @author  
 * @date    2026-06-24
 */

#include "EnemyAI.h"
#include "../combat/Pathfinding.h"
#include <algorithm>
#include <cmath>
#include <limits>

float EnemyAI::euclideanDistance(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

float EnemyAI::calculateThreatLevel(const std::shared_ptr<Unit>& unit) {
    if (!unit || !unit->isAlive()) {
        return 0.0f;
    }
    
    // 威胁程度 = 攻击力 × 生命值比例 + 技能威胁
    int maxHp = unit->getMaxHP();
    float hpRatio = (maxHp > 0) ? static_cast<float>(unit->getHP()) / maxHp : 0.0f;
    float threatLevel = unit->getATK() * hpRatio;
    
    // 有大招的单位威胁度更高
    if (unit->canUseUltimate()) {
        threatLevel *= 1.5f;
    }
    
    return threatLevel;
}

std::shared_ptr<Unit> EnemyAI::selectTarget(const std::shared_ptr<Unit>& enemy,
                                            const Board& board,
                                            const std::vector<std::shared_ptr<Unit>>& playerUnits) {
    if (!enemy || playerUnits.empty()) {
        return nullptr;
    }
    
    auto [enemyX, enemyY] = enemy->getPosition();
    
    std::shared_ptr<Unit> bestTarget = nullptr;
    float bestScore = -std::numeric_limits<float>::max();
    
    for (const auto& unit : playerUnits) {
        if (!unit || !unit->isAlive()) {
            continue;
        }
        
        auto [unitX, unitY] = unit->getPosition();
        float distance = euclideanDistance(enemyX, enemyY, unitX, unitY);
        
        // 目标距离超出视野范围（暂定20），忽略
        if (distance > 20.0f) {
            continue;
        }
        
        // 索敌优先级：
        // 1. 优先最近的目标
        // 2. 距离相同则按生命值低 → 从左到右 → 从下到上
        float score = -distance;  // 距离越近，score 越高
        
        // 同距离时，优先生命值低的
        if (bestTarget != nullptr) {
            auto [bestX, bestY] = bestTarget->getPosition();
            float bestDistance = euclideanDistance(enemyX, enemyY, bestX, bestY);
            
            if (std::abs(distance - bestDistance) < 0.01f) {  // 距离相同
                if (unit->getHP() < bestTarget->getHP()) {
                    score += 100;  // 生命值低加分
                } else if (unit->getHP() == bestTarget->getHP()) {
                    // 生命值相同，比较 x（从左到右）
                    if (unitX < bestX) {
                        score += 50;
                    } else if (unitX == bestX) {
                        // 比较 y（从下到上）
                        if (unitY > bestY) {
                            score += 25;
                        }
                    }
                }
            }
        }
        
        // 威胁度作为次要因素（仅在距离非常接近时生效）
        score += calculateThreatLevel(unit) * 0.01f;
        
        if (score > bestScore) {
            bestScore = score;
            bestTarget = unit;
        }
    }
    
    return bestTarget;
}

AttackType EnemyAI::selectAttackType(const std::shared_ptr<Unit>& enemy) {
    if (!enemy) {
        return AttackType::Normal;
    }
    
    // 蓝满释放大招
    if (enemy->canUseUltimate()) {
        return AttackType::Ultimate;
    }
    
    return AttackType::Normal;
}

AIDecision EnemyAI::makeDecision(const std::shared_ptr<Unit>& enemy,
                                 const Board& board,
                                 const std::vector<std::shared_ptr<Unit>>& playerUnits) {
    AIDecision decision;
    decision.action = AIAction::None;
    decision.targetUnit = nullptr;
    decision.attackType = AttackType::Normal;
    
    if (!enemy || !enemy->isAlive()) {
        return decision;
    }
    
    auto [enemyX, enemyY] = enemy->getPosition();
    
    // 第一步：选择目标
    auto target = selectTarget(enemy, board, playerUnits);
    if (!target) {
        return decision;  // 无目标，待机
    }
    
    decision.targetUnit = target;
    auto [targetX, targetY] = target->getPosition();
    
    // 第二步：判断是否在攻击范围内
    float distance = euclideanDistance(enemyX, enemyY, targetX, targetY);
    
    if (enemy->isPositionInAttackRange(targetX, targetY, AttackType::Normal)) {
        // 在范围内，攻击
        decision.action = AIAction::Attack;
        decision.targetX = targetX;
        decision.targetY = targetY;
        decision.attackType = selectAttackType(enemy);
    } else {
        // 不在范围内，寻路移动到目标附近可攻击的位置
        int bestMoveX = -1;
        int bestMoveY = -1;
        size_t bestPathLength = std::numeric_limits<size_t>::max();

        for (int tx = 0; tx < BOARD_M; ++tx) {
            for (int ty = 0; ty < BOARD_N; ++ty) {
                if (!board.isValidPosition(tx, ty) || board.isOccupied(tx, ty)) {
                    continue;
                }
                
                // 检查从候选位置 (tx,ty) 是否能攻击到目标
                bool canAttackFromHere = false;
                const auto& rangeMap = enemy->getNormalAttackRangeMap();
                for (const auto& offset : rangeMap) {
                    if (tx + offset.first == targetX && ty + offset.second == targetY) {
                        canAttackFromHere = true;
                        break;
                    }
                }
                if (!canAttackFromHere) {
                    continue;
                }

                auto path = Pathfinding::astarPath(board, enemyX, enemyY, tx, ty);
                if (!path.empty() && path.size() < bestPathLength) {
                    bestPathLength = path.size();
                    bestMoveX = path[0].first;
                    bestMoveY = path[0].second;
                }
            }
        }

        if (bestMoveX >= 0 && bestMoveY >= 0) {
            decision.action = AIAction::Move;
            decision.targetX = bestMoveX;
            decision.targetY = bestMoveY;
        } else {
            // 无路可走到攻击位（被夹击/无空位）→ 强行攻击最近目标
            decision.action = AIAction::Attack;
            decision.targetX = targetX;
            decision.targetY = targetY;
            decision.attackType = selectAttackType(enemy);
        }
    }
    
    return decision;
}
