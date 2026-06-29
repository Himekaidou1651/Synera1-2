/**
 * @file    HimekaidouHatate.cpp
 * @brief   姬海棠果角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "HimekaidouHatate.h"
#include <algorithm>

Unit_Hatate::Unit_Hatate(Owner owner):
        Unit(220, 30, 50, 0, owner,
             "Hatate",
             "連写「Rapid Shot」",
             "写真「Full Panorama Shot」",
             "翻盖机",
             1) {
    race_ = Race::Youkai;
    class_ = ClassType::Journalist;
    traits_.insert("A");
    baseUltDamage_ = 80;

    // 普攻范围：上方2×3矩形
    normalAttackRangeMap_.clear();
    for (int dx = 1; dx <= 2; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            normalAttackRangeMap_.push_back({-dx, dy});
        }
    }

    // 大招范围：全场（所有棋盘格子，不含自身）
    ultimateAttackRangeMap_.clear();
    for (int dx = -BOARD_M; dx <= BOARD_M; ++dx) {
        for (int dy = -BOARD_N; dy <= BOARD_N; ++dy) {
            if (dx == 0 && dy == 0) continue;
            ultimateAttackRangeMap_.push_back({dx, dy});
        }
    }
}

Unit_Hatate::~Unit_Hatate() {}

void Unit_Hatate::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
    auto [ux, uy] = getPosition();
    std::vector<std::shared_ptr<Unit>> alive;
    for (auto& e : combatTargets_)
        if (e && e->isAlive()) alive.push_back(e);
    std::sort(alive.begin(), alive.end(), [&](auto& a, auto& b) {
        auto [ax, ay] = a->getPosition(); auto [bx, by] = b->getPosition();
        return std::hypot(ax - ux, ay - uy) < std::hypot(bx - ux, by - uy);
    });
    int pick = std::max(8, (int)alive.size());
    for (int i = 0; i < std::min(pick, (int)alive.size()); ++i)
        dealUltDamage(alive[i]);
}

void Unit_Hatate::onTurnStart() {
    if (!isAlive()) {
        return;
    }
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Hatate::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Hatate::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Hatate::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Hatate::debugName() const {
    return "姫海棠 はたて";
}

std::string Unit_Hatate::displayName() const {
    return "はたて";
}