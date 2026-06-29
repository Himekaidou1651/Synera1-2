/**
 * @file    AliceMargatroid.h
 * @brief   爱丽丝·玛格特罗伊德角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Alice
 * @brief   爱丽丝（Mahoushi/Youkai），直线普攻，矩形AOE大招
 */
class Unit_Alice : public Unit {
public:
    Unit_Alice(Owner owner);
    ~Unit_Alice();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};