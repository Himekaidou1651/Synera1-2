/**
 * @file    KonpakuYoumu.h
 * @brief   魂魄妖梦角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Youmu
 * @brief   魂魄妖梦（Maid），扇形普攻，锥形大招二连击
 */
class Unit_Youmu : public Unit {
public:
    Unit_Youmu(Owner owner);
    ~Unit_Youmu();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};