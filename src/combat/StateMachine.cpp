/**
 * @file    StateMachine.cpp
 * @brief   单位状态机系统实现
 * @author  
 * @date    2026-06-24
 */

#include "StateMachine.h"
#include <algorithm>

// ========== StateMachine ==========

StateMachine::StateMachine()
    : currentState_(UnitState::Idle), stateTimer_(0.0f) {
    setupDefaultTransitions();
}

void StateMachine::init(std::shared_ptr<Unit> unit) {
    unit_ = unit;
    currentState_ = UnitState::Idle;
    stateTimer_ = 0.0f;
    onStateEnter(currentState_);
}

UnitState StateMachine::getCurrentState() const {
    return currentState_;
}

void StateMachine::addTransition(const StateTransition& transition) {
    customTransitions_.push_back(transition);
}

bool StateMachine::handleEvent(StateEvent event) {
    auto unit = unit_.lock();
    if (!unit) return false;

    // 先检查自定义转换规则
    for (const auto& transition : customTransitions_) {
        if (transition.fromState == currentState_ && transition.event == event) {
            if (!transition.condition || transition.condition(unit)) {
                onStateExit(currentState_);
                currentState_ = transition.toState;
                stateTimer_ = 0.0f;
                onStateEnter(currentState_);
                return true;
            }
        }
    }

    // 检查默认转换规则
    for (const auto& transition : defaultTransitions_) {
        if (transition.fromState == currentState_ && transition.event == event) {
            if (!transition.condition || transition.condition(unit)) {
                onStateExit(currentState_);
                currentState_ = transition.toState;
                stateTimer_ = 0.0f;
                onStateEnter(currentState_);
                return true;
            }
        }
    }

    return false;
}

void StateMachine::update(float deltaTime) {
    stateTimer_ += deltaTime;

    auto unit = unit_.lock();
    if (!unit) return;

    // 根据当前状态执行逻辑
    switch (currentState_) {
        case UnitState::Idle:
            // 等待事件触发
            break;

        case UnitState::Ready:
            // 准备就绪，等待指令
            break;

        case UnitState::Moving:
            // 移动中，由外部系统更新位置
            break;

        case UnitState::Attacking:
            // 攻击动画/冷却
            if (stateTimer_ >= 0.5f) {  // 攻击动画持续 0.5 秒
                handleEvent(StateEvent::AttackComplete);
            }
            break;

        case UnitState::Casting:
            // 施法动画/冷却
            if (stateTimer_ >= 1.0f) {  // 施法动画持续 1.0 秒
                handleEvent(StateEvent::SkillComplete);
            }
            break;

        case UnitState::Dead:
            // 死亡状态，不做任何事
            break;
    }
}

void StateMachine::reset() {
    currentState_ = UnitState::Idle;
    stateTimer_ = 0.0f;
    customTransitions_.clear();
    setupDefaultTransitions();
}

bool StateMachine::canMove() const {
    return currentState_ == UnitState::Ready || currentState_ == UnitState::Idle;
}

bool StateMachine::canAttack() const {
    return currentState_ == UnitState::Ready;
}

bool StateMachine::canCastSkill() const {
    return currentState_ == UnitState::Ready;
}

void StateMachine::setupDefaultTransitions() {
    // 清空旧规则并设置默认转换规则
    defaultTransitions_.clear();

    // Idle → Ready：回合开始
    defaultTransitions_.push_back({
        UnitState::Idle, StateEvent::TurnStart, UnitState::Ready, nullptr
    });

    // Ready → Moving：移动指令
    defaultTransitions_.push_back({
        UnitState::Ready, StateEvent::Spawn, UnitState::Moving, nullptr
    });

    // Moving → Ready：移动完成
    defaultTransitions_.push_back({
        UnitState::Moving, StateEvent::MoveComplete, UnitState::Ready, nullptr
    });

    // Ready → Attacking：攻击指令
    defaultTransitions_.push_back({
        UnitState::Ready, StateEvent::AttackReady, UnitState::Attacking, nullptr
    });

    // Attacking → Ready：攻击完成
    defaultTransitions_.push_back({
        UnitState::Attacking, StateEvent::AttackComplete, UnitState::Ready, nullptr
    });

    // Ready → Casting：施法指令
    defaultTransitions_.push_back({
        UnitState::Ready, StateEvent::SkillCast, UnitState::Casting, nullptr
    });

    // Casting → Ready：施法完成
    defaultTransitions_.push_back({
        UnitState::Casting, StateEvent::SkillComplete, UnitState::Ready, nullptr
    });

    // 任意状态 → Dead：死亡事件
    defaultTransitions_.push_back({
        UnitState::Idle, StateEvent::Death, UnitState::Dead, nullptr
    });
    defaultTransitions_.push_back({
        UnitState::Ready, StateEvent::Death, UnitState::Dead, nullptr
    });
    defaultTransitions_.push_back({
        UnitState::Moving, StateEvent::Death, UnitState::Dead, nullptr
    });
    defaultTransitions_.push_back({
        UnitState::Attacking, StateEvent::Death, UnitState::Dead, nullptr
    });
    defaultTransitions_.push_back({
        UnitState::Casting, StateEvent::Death, UnitState::Dead, nullptr
    });

    // 控制效果：眩晕/冰冻
    defaultTransitions_.push_back({
        UnitState::Ready, StateEvent::Stunned, UnitState::Idle, nullptr
    });
    defaultTransitions_.push_back({
        UnitState::Idle, StateEvent::StunEnd, UnitState::Ready, nullptr
    });
}

void StateMachine::onStateEnter(UnitState state) {
    auto unit = unit_.lock();
    if (!unit) return;

    switch (state) {
        case UnitState::Dead:
            unit->onDeath();
            break;
        default:
            break;
    }
}

void StateMachine::onStateExit(UnitState state) {
    // 退出状态时的清理工作（预留）
    (void)state;
}

bool StateMachine::checkTransition(const StateTransition& transition, std::shared_ptr<Unit> unit) {
    if (!transition.condition) return true;
    return transition.condition(unit);
}

// ========== StateMachineManager ==========

StateMachine& StateMachineManager::createStateMachine(std::shared_ptr<Unit> unit) {
    size_t id = nextId_++;
    auto result = stateMachines_.emplace(id, StateMachine());
    result.first->second.init(unit);
    return result.first->second;
}

void StateMachineManager::removeStateMachine(std::shared_ptr<Unit> unit) {
    if (!unit) return;
    
    // 遍历查找并移除对应单位的状态机
    for (auto it = stateMachines_.begin(); it != stateMachines_.end(); ) {
        auto smUnit = it->second.getUnit().lock();
        if (smUnit == unit) {
            it = stateMachines_.erase(it);
        } else {
            ++it;
        }
    }
}

StateMachine* StateMachineManager::getStateMachine(std::shared_ptr<Unit> unit) {
    if (!unit) return nullptr;
    
    // 遍历查找与单位关联的状态机
    for (auto& [id, sm] : stateMachines_) {
        auto smUnit = sm.getUnit();
        if (smUnit.lock() == unit) {
            return &sm;
        }
    }
    return nullptr;
}

void StateMachineManager::updateAll(float deltaTime) {
    for (auto& [id, sm] : stateMachines_) {
        (void)id;
        sm.update(deltaTime);
    }
}

void StateMachineManager::broadcastEvent(StateEvent event) {
    for (auto& [id, sm] : stateMachines_) {
        (void)id;
        sm.handleEvent(event);
    }
}

void StateMachineManager::resetAll() {
    for (auto& [id, sm] : stateMachines_) {
        (void)id;
        sm.reset();
    }
}

// ========== AttackOrderManager ==========

AttackOrderManager::AttackOrderManager() {}

void AttackOrderManager::registerAttacker(std::shared_ptr<Unit> unit, std::shared_ptr<Unit> target, AttackType type) {
    if (!unit || !target || !unit->isAlive() || !target->isAlive()) {
        return;
    }

    // 检查目标是否在攻击范围内
    auto targetPos = target->getPosition();
    if (!unit->isPositionInAttackRange(targetPos.first, targetPos.second, type)) {
        return;
    }

    PendingAttack attack;
    attack.attacker = unit;
    attack.target = target;
    attack.type = type;
    attack.isPlayerUnit = (unit->getOwner() == Owner::PlayerCtrl);
    pendingAttacks_.push_back(attack);
}

void AttackOrderManager::executeAllAttacks() {
    // 先执行所有玩家单位的攻击
    executeAttacksByOwner(Owner::PlayerCtrl);

    // 再执行所有敌方单位的攻击
    executeAttacksByOwner(Owner::EnemyCtrl);

    // 清空队列
    clear();
}

void AttackOrderManager::executeAttacksByOwner(Owner owner) {
    for (auto& attack : pendingAttacks_) {
        if (attack.attacker->getOwner() != owner) continue;

        // 再次检查目标是否存活且在范围内
        if (!attack.attacker->isAlive() || !attack.target->isAlive()) continue;

        auto targetPos = attack.target->getPosition();
        if (!attack.attacker->isPositionInAttackRange(targetPos.first, targetPos.second, attack.type)) {
            continue;
        }

        // 执行攻击
        attack.attacker->onAttack(attack.target, attack.type);
    }
}

void AttackOrderManager::clear() {
    pendingAttacks_.clear();
}

int AttackOrderManager::getPendingAttackCount() const {
    return static_cast<int>(pendingAttacks_.size());
}

bool AttackOrderManager::hasPendingAttacks() const {
    return !pendingAttacks_.empty();
}