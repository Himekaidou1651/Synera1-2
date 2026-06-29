/**
 * @file    UnitFactory.h
 * @brief   单位工厂，根据类型名称或随机创建角色单位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "../core/Unit.h"
#include "../core/Commondata.h"

/**
 * @class   UnitFactory
 * @brief   单位工厂，提供按类型创建和随机生成单位的功能
 */
class UnitFactory {
public:
    static std::shared_ptr<Unit> createUnitByType(const std::string& unitType,
                                                   Owner owner);

    static std::shared_ptr<Unit> createRandomShopUnit(Owner owner, int slotIndex, int round);
    static std::vector<std::string> getAvailableUnitTypes();
};