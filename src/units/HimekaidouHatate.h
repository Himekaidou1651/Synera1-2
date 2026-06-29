/**
 * @file    HimekaidouHatate.h
 * @brief   姬海棠果角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Hatate
 * @brief   姬海棠果（Journalist/Youkai），矩形普攻，全场大招
 */
class Unit_Hatate : public Unit {
public:
    Unit_Hatate(Owner owner);
    ~Unit_Hatate();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};