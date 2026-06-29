/**
 * @file    KirisameMarisa.h
 * @brief   雾雨魔理沙角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Marisa
 * @brief   雾雨魔理沙（Mahoushi），同行普攻，同列大招
 */
class Unit_Marisa : public Unit {
public:
    Unit_Marisa(Owner owner);
    ~Unit_Marisa();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};