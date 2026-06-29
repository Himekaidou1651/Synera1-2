/**
 * @file    IzayoiSakuya.h
 * @brief   十六夜咲夜角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Sakuya
 * @brief   十六夜咲夜（Maid），扇形普攻，全场大招+时停额外行动
 */
class Unit_Sakuya : public Unit {
public:
    Unit_Sakuya(Owner owner);
    ~Unit_Sakuya();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};