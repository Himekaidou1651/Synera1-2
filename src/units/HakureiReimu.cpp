/**
 * @file    HakureiReimu.cpp
 * @brief   博丽灵梦角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "HakureiReimu.h"
#include <algorithm>

Unit_Reimu::Unit_Reimu(Owner owner):
        Unit(400, 36, 75, 0, owner,
             "Reimu",
             "宝符「陰陽宝玉」",
             "霊符「夢想封印」",
             "阴阳玉",
             1) {
    race_ = Race::Ningen;
    class_ = ClassType::Miko;
    traits_.insert("A");
    baseUltDamage_ = 90;

    // 普攻范围：十字2格
    normalAttackRangeMap_.clear();
    for (int d = 1; d <= 2; ++d) {
        normalAttackRangeMap_.push_back({-d, 0});
        normalAttackRangeMap_.push_back({d, 0});
        normalAttackRangeMap_.push_back({0, -d});
        normalAttackRangeMap_.push_back({0, d});
    }

    // 大招范围：周围8格
    ultimateAttackRangeMap_.clear();
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;
            ultimateAttackRangeMap_.push_back({dx, dy});
        }
    }
}

Unit_Reimu::~Unit_Reimu() {}

void Unit_Reimu::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
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
    for (int i = 0; i < std::min(pick, (int)inRange.size()); ++i) dealUltDamage(inRange[i]);
}

void Unit_Reimu::onTurnStart() {
    if (!isAlive()) return;
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Reimu::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Reimu::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Reimu::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Reimu::debugName() const {
    return "博麗 霊夢";
}

std::string Unit_Reimu::displayName() const {
    return "霊夢";
}