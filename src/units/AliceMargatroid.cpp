/**
 * @file    AliceMargatroid.cpp
 * @brief   爱丽丝角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "AliceMargatroid.h"
#include <algorithm>

Unit_Alice::Unit_Alice(Owner owner):
        Unit(300, 40, 50, 0, owner,
             "Alice",
             "操符「Marionette Parrar」",
             "咒符「上海人形」",
             "七色人形",
             1) {
    race_ = Race::Youkai;
    class_ = ClassType::Mahoushi;
    traits_.insert("A");
    baseUltDamage_ = 75;

    // 普攻范围：上方4格直线
    normalAttackRangeMap_.clear();
    for (int dx = -4; dx <= -1; ++dx) {
        if (dx == 0) continue;
        normalAttackRangeMap_.push_back({dx, 0});
    }

    // 大招范围：上方4×3矩形
    ultimateAttackRangeMap_.clear();
    for (int dx = -4; dx <= -1; ++dx) {
        if (dx == 0) continue;
        ultimateAttackRangeMap_.push_back({dx, -1});
        ultimateAttackRangeMap_.push_back({dx, 0});
        ultimateAttackRangeMap_.push_back({dx, 1});
    }
}

Unit_Alice::~Unit_Alice() {}

void Unit_Alice::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
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
    int pick = std::max(4, (int)inRange.size());
    for (int i = 0; i < std::min(pick, (int)inRange.size()); ++i)
        dealUltDamage(inRange[i]);
}

void Unit_Alice::onTurnStart() {
    if (!isAlive()) {
        return;
    }
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Alice::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Alice::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Alice::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Alice::debugName() const {
    return "アリス・マーガトロイド";
}

std::string Unit_Alice::displayName() const {
    return "アリス";
}