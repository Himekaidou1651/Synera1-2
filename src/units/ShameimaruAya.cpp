/**
 * @file    ShameimaruAya.cpp
 * @brief   射命丸文角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "ShameimaruAya.h"
#include <algorithm>

Unit_Aya::Unit_Aya(Owner owner):
        Unit(240, 30, 75, 0, owner,
             "Aya",
             "風神「天狗颪」",
             "「幻想風靡」",
             "团扇",
             1) {
    race_ = Race::Youkai;
    class_ = ClassType::Journalist;
    traits_.insert("A");
    baseUltDamage_ = 100;

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

Unit_Aya::~Unit_Aya() {}

void Unit_Aya::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
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

void Unit_Aya::onTurnStart() {
    if (!isAlive()) {
        return;
    }
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Aya::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Aya::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Aya::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Aya::debugName() const {
    return "射命丸 文";
}

std::string Unit_Aya::displayName() const {
    return "文";
}