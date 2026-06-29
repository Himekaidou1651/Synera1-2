/**
 * @file    UnitFactory.cpp
 * @brief   单位工厂实现
 * @author  
 * @date    2026-06-24
 */

#include "UnitFactory.h"
#include "HakureiReimu.h"
#include "KirisameMarisa.h"
#include "KomeijiKoishi.h"
#include "KonpakuYoumu.h"
#include "AliceMargatroid.h"
#include "ShameimaruAya.h"
#include "HimekaidouHatate.h"
#include "IzayoiSakuya.h"
#include "KochiyaSanae.h"
#include <random>

std::shared_ptr<Unit> UnitFactory::createUnitByType(const std::string& unitType,
                                                     Owner owner) {
    if (unitType == "Reimu") {
        return std::make_shared<Unit_Reimu>(owner);
    } else if (unitType == "Marisa") {
        return std::make_shared<Unit_Marisa>(owner);
    } else if (unitType == "Koishi") {
        return std::make_shared<Unit_Koishi>(owner);
    } else if (unitType == "Youmu") {
        return std::make_shared<Unit_Youmu>(owner);
    } else if (unitType == "Alice") {
        return std::make_shared<Unit_Alice>(owner);
    } else if (unitType == "Aya") {
        return std::make_shared<Unit_Aya>(owner);
    } else if (unitType == "Hatate") {
        return std::make_shared<Unit_Hatate>(owner);
    } else if (unitType == "Sakuya") {
        return std::make_shared<Unit_Sakuya>(owner);
    } else if (unitType == "Sanae") {
        return std::make_shared<Unit_Sanae>(owner);
    }
    return nullptr;
}

std::shared_ptr<Unit> UnitFactory::createRandomShopUnit(Owner owner, int /*slotIndex*/, int /*round*/) {
    static std::mt19937 rng(std::random_device{}());
    
    const auto& types = getAvailableUnitTypes();
    std::uniform_int_distribution<int> choice(0, static_cast<int>(types.size()) - 1);
    const std::string& unitType = types[choice(rng)];
    
    return createUnitByType(unitType, owner);
}

std::vector<std::string> UnitFactory::getAvailableUnitTypes() {
    return {"Reimu", "Marisa", "Koishi", "Youmu", "Alice", "Aya", "Hatate", "Sakuya", "Sanae"};
}