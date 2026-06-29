/**
 * @file    EnemyAI.h
 * @brief   敌方 AI 决策系统，负责索敌、攻击方式选择与移动决策
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"
#include "Board.h"

/**
 * @brief AI 可执行的行动类型
 */
enum class AIAction {
    None,       ///< 无行动
    Move,       ///< 移动
    Attack,     ///< 攻击
    CastSkill   ///< 释放技能
};

/**
 * @brief AI 决策结果
 */
struct AIDecision {
    AIAction action;                          ///< 行动类型
    int targetX, targetY;                     ///< 目标坐标（移动目标或攻击目标）
    std::shared_ptr<Unit> targetUnit;         ///< 目标单位
    AttackType attackType;                    ///< 攻击类型
};

/**
 * @class   EnemyAI
 * @brief   敌方 AI，为每个敌方单位生成行动决策
 */
class EnemyAI {
public:
    /**
     * @brief  为敌方单位做出行动决策
     * @param  enemy        敌方单位
     * @param  board        当前棋盘状态
     * @param  playerUnits  玩家方单位列表
     * @return AI 决策结果
     */
    static AIDecision makeDecision(const std::shared_ptr<Unit>& enemy,
                                   const Board& board,
                                   const std::vector<std::shared_ptr<Unit>>& playerUnits);
    
private:
    /**
     * @brief  索敌：从玩家单位中选择最优攻击目标
     * @param  enemy        敌方单位
     * @param  board        当前棋盘状态
     * @param  playerUnits  玩家方单位列表
     * @return 选中的目标单位（无目标时返回 nullptr）
     */
    static std::shared_ptr<Unit> selectTarget(const std::shared_ptr<Unit>& enemy,
                                              const Board& board,
                                              const std::vector<std::shared_ptr<Unit>>& playerUnits);
    
    /**
     * @brief  计算两点之间的欧氏距离
     * @param  x1, y1 点1坐标
     * @param  x2, y2 点2坐标
     * @return 欧氏距离
     */
    static float euclideanDistance(int x1, int y1, int x2, int y2);
    
    /**
     * @brief  计算单位的威胁程度（用于智能索敌优先级）
     * @param  unit 目标单位
     * @return 威胁值
     */
    static float calculateThreatLevel(const std::shared_ptr<Unit>& unit);
    
    /**
     * @brief  选择攻击方式（普攻/大招）
     * @param  enemy 敌方单位
     * @return 攻击类型
     */
    static AttackType selectAttackType(const std::shared_ptr<Unit>& enemy);
};
