/**
 * @file    IzayoiSakuya.cpp
 * @brief   十六夜咲夜角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "IzayoiSakuya.h"
#include <algorithm>

Unit_Sakuya::Unit_Sakuya(Owner owner):
        Unit(310, 42, 75, 0, owner,
             "Sakuya",
             "時符「Perfect Square」",
             "幻世「The World」",
             "飞刀",
             1) {
    race_ = Race::Ningen;
    class_ = ClassType::Maid;
    traits_.insert("A");
    baseUltDamage_ = 90;

    // 普攻范围：上方2格扇形
    normalAttackRangeMap_.clear();
    for (int d = 1; d <= 2; ++d) {
        normalAttackRangeMap_.push_back({-d, 0});
        normalAttackRangeMap_.push_back({-d, -d});
        normalAttackRangeMap_.push_back({-d, d});
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

Unit_Sakuya::~Unit_Sakuya() {}

void Unit_Sakuya::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
    auto [ux, uy] = getPosition();
    std::vector<std::shared_ptr<Unit>> alive;
    for (auto& e : combatTargets_)
        if (e && e->isAlive()) alive.push_back(e);
    std::sort(alive.begin(), alive.end(), [&](auto& a, auto& b) {
        auto [ax, ay] = a->getPosition(); auto [bx, by] = b->getPosition();
        return std::hypot(ax - ux, ay - uy) < std::hypot(bx - ux, by - uy);
    });
    int pick = std::max(5, (int)alive.size());
    for (int i = 0; i < std::min(pick, (int)alive.size()); ++i)
        dealUltDamage(alive[i]);
    extraActions_ = 3;  // 时停：三步额外行动
}

void Unit_Sakuya::onTurnStart() {
    if (!isAlive()) {
        return;
    }
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Sakuya::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Sakuya::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Sakuya::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Sakuya::debugName() const {
    return "十六夜 咲夜";
}

std::string Unit_Sakuya::displayName() const {
    return "咲夜";
}