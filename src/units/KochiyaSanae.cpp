/**
 * @file    KochiyaSanae.cpp
 * @brief   东风谷早苗角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "KochiyaSanae.h"
#include <algorithm>

Unit_Sanae::Unit_Sanae(Owner owner):
        Unit(380, 36, 75, 0, owner,
             "Sanae",
             "奇跡「白昼の客星」",
             "大奇跡「八坂の神風」",
             "守矢御币",
             1) {
    race_ = Race::Ningen;
    class_ = ClassType::Miko;
    traits_.insert("A");
    baseUltDamage_ = 85;

    // 普攻范围：十字2格
    normalAttackRangeMap_.clear();
    for (int d = 1; d <= 2; ++d) {
        normalAttackRangeMap_.push_back({-d, 0});
        normalAttackRangeMap_.push_back({d, 0});
        normalAttackRangeMap_.push_back({0, -d});
        normalAttackRangeMap_.push_back({0, d});
    }

    // 大招范围：周围24格（Chebyshev距离≤2，5×5包围盒不含自身）
    ultimateAttackRangeMap_.clear();
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            if (dx == 0 && dy == 0) continue;
            ultimateAttackRangeMap_.push_back({dx, dy});
        }
    }
}

Unit_Sanae::~Unit_Sanae() {}

void Unit_Sanae::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
    auto [ux, uy] = getPosition();
    const auto& rangeMap = getUltimateAttackRangeMap();
    std::vector<std::shared_ptr<Unit>> inRange;
    for (auto& e : combatTargets_) {
        if (!e || !e->isAlive()) continue;
        auto [ex, ey] = e->getPosition();
        for (auto& off : rangeMap)
            if (ux + off.first == ex && uy + off.second == ey) { inRange.push_back(e); break; }
    }
    std::sort(inRange.begin(), inRange.end(), [&](auto& a, auto& b) {
        auto [ax, ay] = a->getPosition(); auto [bx, by] = b->getPosition();
        return std::hypot(ax - ux, ay - uy) < std::hypot(bx - ux, by - uy);
    });
    int pick = std::max(3, (int)inRange.size());
    for (int i = 0; i < std::min(pick, (int)inRange.size()); ++i) dealUltDamage(inRange[i]);
}

void Unit_Sanae::onTurnStart() {
    if (!isAlive()) {
        return;
    }
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Sanae::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Sanae::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Sanae::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Sanae::debugName() const {
    return "東風谷 早苗";
}

std::string Unit_Sanae::displayName() const {
    return "早苗";
}