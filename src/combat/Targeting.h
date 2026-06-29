/**
 * @file    Targeting.h
 * @brief   索敌系统，提供目标选择、优先级评分与攻击范围判断
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"

/**
 * @brief 索敌策略
 */
enum class TargetingStrategy {
    Nearest,            ///< 最近目标
    LowestHP,           ///< 生命值最低
    HighestHP,          ///< 生命值最高
    LowestATK,          ///< 攻击力最低
    HighestATK,         ///< 攻击力最高
    LowestDEF,          ///< 防御最低
    HighestThreat,      ///< 威胁度最高
    Random,             ///< 随机
    FrontRow,           ///< 前排优先
    BackRow,            ///< 后排优先
    NearestToAlly,      ///< 离友方最近
    MostDangerous,      ///< 最危险（综合评估）
    Custom              ///< 自定义
};

/**
 * @brief 索敌过滤器
 */
enum class TargetingFilter {
    None,               ///< 无过滤
    Alive,              ///< 只选择存活
    InRange,            ///< 在攻击范围内
    Visible,            ///< 在视野范围内
    SameRow,            ///< 同一行
    SameColumn,         ///< 同一列
    InAOE,              ///< 在 AOE 范围内
    NotTaunting,        ///< 非嘲讽状态
    Taunting            ///< 嘲讽状态
};

/**
 * @brief 索敌优先级配置
 */
struct TargetingPriority {
    TargetingStrategy strategy;
    float weight;                      ///< 权重（多策略组合时使用）
    std::vector<TargetingFilter> filters;
    bool reverseOrder;                 ///< 是否反转排序
};

/**
 * @brief 目标评估信息
 */
struct TargetInfo {
    std::shared_ptr<Unit> unit;
    float distance;             ///< 距离
    float score;                ///< 综合评分
    int row;                    ///< 行坐标
    int column;                 ///< 列坐标
    bool inRange;               ///< 是否在攻击范围内
    bool inSight;               ///< 是否在视野内
};

/**
 * @class   TargetingSystem
 * @brief   索敌系统，根据策略和优先级为施法者选择最优目标
 */
class TargetingSystem {
public:
    TargetingSystem();

    // ========== 策略配置 ==========
    void setStrategy(TargetingStrategy strategy);
    void addPriority(const TargetingPriority& priority);
    void clearPriorities();

    // ========== 范围设置 ==========
    void setRange(int range);
    int getRange() const;
    void setSightRange(int range);

    // ========== 核心索敌 ==========
    std::shared_ptr<Unit> findTarget(
        std::shared_ptr<Unit> source,
        const std::vector<std::shared_ptr<Unit>>& potentialTargets,
        const std::vector<std::shared_ptr<Unit>>& allies = {});

    std::vector<TargetInfo> findTargets(
        std::shared_ptr<Unit> source,
        const std::vector<std::shared_ptr<Unit>>& potentialTargets,
        int maxTargets = -1);

    // ========== 范围查询 ==========
    std::vector<std::shared_ptr<Unit>> getTargetsInRange(
        std::shared_ptr<Unit> source,
        const std::vector<std::shared_ptr<Unit>>& targets);

    bool isInRange(std::shared_ptr<Unit> source, std::shared_ptr<Unit> target) const;
    bool isInRange(int sourceX, int sourceY, int targetX, int targetY) const;

    bool isInRange(std::shared_ptr<Unit> source, int targetX, int targetY, AttackType type) const;

    std::vector<std::shared_ptr<Unit>> getTargetsInAttackRange(
        std::shared_ptr<Unit> source,
        const std::vector<std::shared_ptr<Unit>>& targets,
        AttackType type) const;

    // ========== 自定义索敌 ==========
    void setCustomTargetingFunction(
        std::function<std::shared_ptr<Unit>(
            std::shared_ptr<Unit>,
            const std::vector<std::shared_ptr<Unit>>&,
            const std::vector<std::shared_ptr<Unit>>&)> func);

    // ========== 静态工具方法 ==========
    static float calculateThreatLevel(std::shared_ptr<Unit> unit);
    static float calculateDistanceScore(float distance, int range);
    static float calculateHPBasedScore(std::shared_ptr<Unit> unit, bool lowestHP);

    static float distance(int x1, int y1, int x2, int y2);
    static float manhattanDistance(int x1, int y1, int x2, int y2);
    static bool isInSight(int sourceX, int sourceY, int targetX, int targetY, int sightRange);

private:
    TargetingStrategy strategy_;
    std::vector<TargetingPriority> priorities_;
    int range_;
    int sightRange_;

    std::function<std::shared_ptr<Unit>(
        std::shared_ptr<Unit>,
        const std::vector<std::shared_ptr<Unit>>&,
        const std::vector<std::shared_ptr<Unit>>&)> customFunc_;

    float calculateScore(std::shared_ptr<Unit> source,
                         std::shared_ptr<Unit> target,
                         const TargetingPriority& priority);

    bool applyFilter(std::shared_ptr<Unit> source,
                     std::shared_ptr<Unit> target,
                     TargetingFilter filter);

    bool isFrontRow(std::shared_ptr<Unit> unit) const;
    bool isBackRow(std::shared_ptr<Unit> unit) const;
};

/**
 * @class   TargetingManager
 * @brief   索敌管理器，维护所有单位的索敌系统
 */
class TargetingManager {
public:
    TargetingManager() = default;

    TargetingSystem& createTargetingSystem(std::shared_ptr<Unit> unit);
    void removeTargetingSystem(std::shared_ptr<Unit> unit);
    TargetingSystem* getTargetingSystem(std::shared_ptr<Unit> unit);
    void setDefaultStrategy(TargetingStrategy strategy);

private:
    std::unordered_map<size_t, TargetingSystem> systems_;
    TargetingStrategy defaultStrategy_ = TargetingStrategy::Nearest;
    size_t nextId_ = 0;
};