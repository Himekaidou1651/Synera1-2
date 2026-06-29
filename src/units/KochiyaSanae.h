/**
 * @file    KochiyaSanae.h
 * @brief   东风谷早苗角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Sanae
 * @brief   东风谷早苗（Miko），十字范围普攻，Chebyshev距离2大招
 */
class Unit_Sanae : public Unit {
public:
    Unit_Sanae(Owner owner);
    ~Unit_Sanae();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};