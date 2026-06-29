/**
 * @file    KonpakuYoumu.cpp
 * @brief   魂魄妖梦角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "KonpakuYoumu.h"
#include <algorithm>

Unit_Youmu::Unit_Youmu(Owner owner):
        Unit(320, 40, 50, 0, owner,
             "Youmu",
             "人符「現世斬」",
             "六道剣「一念無量劫」",
             "楼观白楼剑",
             1) {
    race_ = Race::Ningen;
    class_ = ClassType::Maid;
    traits_.insert("A");
    baseUltDamage_ = 70;

    // 普攻范围：上方2格扇形
    normalAttackRangeMap_.clear();
    for (int d = 1; d <= 2; ++d) {
        normalAttackRangeMap_.push_back({-d, 0});
        normalAttackRangeMap_.push_back({-d, -d});
        normalAttackRangeMap_.push_back({-d, d});
    }

    // 大招范围：上方5格锥形
    ultimateAttackRangeMap_.clear();
    for (int dist = 1; dist <= 5; ++dist) {
        for (int dy = -dist; dy <= dist; ++dy) {
            ultimateAttackRangeMap_.push_back({-dist, dy});
        }
    }
}

Unit_Youmu::~Unit_Youmu() {}

void Unit_Youmu::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
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
    for (int i = 0; i < std::min(pick, (int)inRange.size()); ++i) {
        dealUltDamage(inRange[i]);
        dealUltDamage(inRange[i]);  // 二连击
    }
}

void Unit_Youmu::onTurnStart() {
    if (!isAlive()) {
        return;
    }
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Youmu::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Youmu::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Youmu::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Youmu::debugName() const {
    return "魂魄 妖夢";
}

std::string Unit_Youmu::displayName() const {
    return "妖夢";
}