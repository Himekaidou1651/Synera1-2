/**
 * @file    KirisameMarisa.cpp
 * @brief   雾雨魔理沙角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "KirisameMarisa.h"
#include <algorithm>

Unit_Marisa::Unit_Marisa(Owner owner):
        Unit(280, 45, 50, 0, owner,
             "Marisa",
             "魔符「Stardust Reverie」",
             "恋符「Master Spark」",
             "八卦炉",
             1) {
    race_ = Race::Ningen;
    class_ = ClassType::Mahoushi;
    traits_.insert("A");
    baseUltDamage_ = 75;

    // 普攻范围：同行全屏（dy遍历整列）
    normalAttackRangeMap_.clear();
    for (int dy = -BOARD_N; dy <= BOARD_N; ++dy) {
        if (dy == 0) continue;
        normalAttackRangeMap_.push_back({0, dy});
    }

    // 大招范围：同列全屏（dx遍历整行）
    ultimateAttackRangeMap_.clear();
    for (int dx = -BOARD_M; dx <= BOARD_M; ++dx) {
        if (dx == 0) continue;
        ultimateAttackRangeMap_.push_back({dx, 0});
    }
}

Unit_Marisa::~Unit_Marisa() {}

void Unit_Marisa::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
    auto [ux, uy] = getPosition();
    const auto& rangeMap = getUltimateAttackRangeMap();
    for (auto& e : combatTargets_) {
        if (!e || !e->isAlive()) continue;
        auto [ex, ey] = e->getPosition();
        for (auto& off : rangeMap)
            if (ux + off.first == ex && uy + off.second == ey) { dealUltDamage(e); break; }
    }
}

void Unit_Marisa::onTurnStart() {
    if (!isAlive()) return;
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Marisa::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Marisa::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Marisa::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Marisa::debugName() const {
    return "霧雨 魔理沙";
}

std::string Unit_Marisa::displayName() const {
    return "魔理沙";
}