/**
 * @file    StateMachine.h
 * @brief   单位状态机系统，管理状态转换、攻击顺序与回合调度
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"

// 状态枚举在 Unit.h 中定义为 UnitState，本状态机以此为基础

/**
 * @brief 状态转换事件
 */
enum class StateEvent {
    Spawn,          ///< 生成
    TurnStart,      ///< 回合开始
    MoveComplete,   ///< 移动完成
    AttackReady,    ///< 攻击准备就绪
    AttackComplete, ///< 攻击完成
    SkillCast,      ///< 技能施放
    SkillComplete,  ///< 技能完成
    HitTaken,       ///< 受到攻击
    Death,          ///< 死亡
    Revive,         ///< 复活
    Stunned,        ///< 被眩晕
    StunEnd,        ///< 眩晕结束
    Frozen,         ///< 被冰冻
    FreezeEnd       ///< 冰冻结束
};

/**
 * @brief 状态转换规则
 */
struct StateTransition {
    UnitState fromState;
    StateEvent event;
    UnitState toState;
    std::function<bool(std::shared_ptr<Unit>)> condition; ///< 可选条件
};

/**
 * @class   StateMachine
 * @brief   单个单位的状态机，处理事件驱动的状态转换与计时
 */
class StateMachine {
public:
    StateMachine();

    // ========== 生命周期 ==========
    void init(std::shared_ptr<Unit> unit);
    bool handleEvent(StateEvent event);
    UnitState getCurrentState() const;
    void update(float deltaTime);
    void reset();

    // ========== 动作权限检查 ==========
    bool canMove() const;
    bool canAttack() const;
    bool canCastSkill() const;

    // ========== 转换规则管理 ==========
    void addTransition(const StateTransition& transition);
    std::weak_ptr<Unit> getUnit() const { return unit_; }

private:
    std::weak_ptr<Unit> unit_;
    UnitState currentState_;
    std::vector<StateTransition> customTransitions_;
    std::vector<StateTransition> defaultTransitions_;
    float stateTimer_;  ///< 当前状态持续时间

    void setupDefaultTransitions();
    void onStateEnter(UnitState state);
    void onStateExit(UnitState state);
    bool checkTransition(const StateTransition& transition, std::shared_ptr<Unit> unit);
};

/**
 * @class   StateMachineManager
 * @brief   状态机管理器，维护所有单位的状态机并统一更新
 */
class StateMachineManager {
public:
    StateMachineManager() = default;

    StateMachine& createStateMachine(std::shared_ptr<Unit> unit);
    void removeStateMachine(std::shared_ptr<Unit> unit);
    StateMachine* getStateMachine(std::shared_ptr<Unit> unit);
    void updateAll(float deltaTime);
    void broadcastEvent(StateEvent event);
    void resetAll();

private:
    std::unordered_map<size_t, StateMachine> stateMachines_;  ///< 以 unit id 为键
    size_t nextId_ = 0;
};

/**
 * @class   AttackOrderManager
 * @brief   攻击顺序管理器，确保玩家先攻击、敌方后攻击
 */
class AttackOrderManager {
public:
    AttackOrderManager();

    void registerAttacker(std::shared_ptr<Unit> unit, std::shared_ptr<Unit> target, AttackType type);
    void executeAllAttacks();
    void clear();
    int getPendingAttackCount() const;
    bool hasPendingAttacks() const;

private:
    /**
     * @brief 待执行的攻击记录
     */
    struct PendingAttack {
        std::shared_ptr<Unit> attacker;
        std::shared_ptr<Unit> target;
        AttackType type;
        bool isPlayerUnit;  ///< 是否是玩家单位
    };

    std::vector<PendingAttack> pendingAttacks_;

    /**
     * @brief  按归属方顺序执行攻击
     * @param  owner 攻击方归属
     */
    void executeAttacksByOwner(Owner owner);
};