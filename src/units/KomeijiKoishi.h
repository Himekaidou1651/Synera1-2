/**
 * @file    KomeijiKoishi.h
 * @brief   古明地恋角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"

/**
 * @class   Unit_Koishi
 * @brief   古明地恋（Mahoushi/Youkai），周围普攻，曼哈顿距离+直线混合大招
 */
class Unit_Koishi : public Unit {
public:
    Unit_Koishi(Owner owner);
    ~Unit_Koishi();

    void onUltimateAttack(std::shared_ptr<Unit> target) override;
    virtual void onTurnStart();
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;
};