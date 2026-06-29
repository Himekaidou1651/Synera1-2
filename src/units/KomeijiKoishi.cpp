/**
 * @file    KomeijiKoishi.cpp
 * @brief   古明地恋角色单位实现
 * @author  
 * @date    2026-06-24
 */

#include "KomeijiKoishi.h"
#include <algorithm>

Unit_Koishi::Unit_Koishi(Owner owner):
        Unit(350, 36, 75, 0, owner,
             "Koishi",
             "心符「无意识弹幕」",
             "本能「イドの解放」",
             "恋之瞳",
             1) {
    race_ = Race::Youkai;
    class_ = ClassType::Mahoushi;
    traits_.insert("A");
    baseUltDamage_ = 90;

    // 普攻范围：周围8格
    normalAttackRangeMap_.clear();
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;
            normalAttackRangeMap_.push_back({dx, dy});
        }
    }

    // 大招范围：曼哈顿距离≤4 与 上方6格直线 取并集
    ultimateAttackRangeMap_.clear();
    std::set<std::pair<int,int>> ultSet;
    // 曼哈顿距离 ≤ 4
    for (int dx = -4; dx <= 4; ++dx) {
        for (int dy = -4; dy <= 4; ++dy) {
            if (dx == 0 && dy == 0) continue;
            if (std::abs(dx) + std::abs(dy) <= 4) {
                ultSet.insert({dx, dy});
            }
        }
    }
    // 上方6格直线（dx负方向）
    for (int d = 1; d <= 6; ++d) {
        ultSet.insert({-d, 0});
    }
    for (const auto& p : ultSet) {
        ultimateAttackRangeMap_.push_back(p);
    }
}

Unit_Koishi::~Unit_Koishi() {}

void Unit_Koishi::onUltimateAttack(std::shared_ptr<Unit> /*target*/) {
    auto [ux, uy] = getPosition();
    const auto& rangeMap = getUltimateAttackRangeMap();
    std::shared_ptr<Unit> center = nullptr;
    float best = 1e9f;
    for (auto& e : combatTargets_) {
        if (!e || !e->isAlive()) continue;
        auto [ex, ey] = e->getPosition();
        for (auto& off : rangeMap)
            if (ux + off.first == ex && uy + off.second == ey) {
                float d = std::hypot(ex - ux, ey - uy);
                if (d < best) { best = d; center = e; }
                break;
            }
    }
    if (!center) return;
    auto [cx, cy] = center->getPosition();
    dealUltDamage(center);
    for (auto& e : combatTargets_) {
        if (!e || !e->isAlive() || e == center) continue;
        auto [ex, ey] = e->getPosition();
        if (std::abs(ex - cx) + std::abs(ey - cy) <= 1) dealUltDamage(e);
    }
}

void Unit_Koishi::onTurnStart() {
    if (!isAlive()) {
        return;
    }
    state_ = UnitState::Ready;
    recalculateStats();
}

void Unit_Koishi::onDeath() {
    state_ = UnitState::Dead;
}

void Unit_Koishi::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit_Koishi::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

std::string Unit_Koishi::debugName() const {
    return "古明地 こいし";
}

std::string Unit_Koishi::displayName() const {
    return "こいし";
}