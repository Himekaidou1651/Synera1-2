/**
 * @file    HakureiReimu.h
 * @brief   博丽灵梦角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Reimu
 * @brief   博丽灵梦（Miko），十字范围普攻，周围AOE大招
 */
class Unit_Reimu : public Unit {
public:
    Unit_Reimu(Owner owner);
    ~Unit_Reimu();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};