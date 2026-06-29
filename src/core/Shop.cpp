/**
 * @file    Shop.cpp
 * @brief   商店核心逻辑实现
 * @author  
 * @date    2026-06-24
 */

#include "Shop.h"
#include "../units/UnitFactory.h"
#include <algorithm>
#include <sstream>

Shop::Shop() : rng_(std::random_device{}()) {
    units_.reserve(SHOP_SLOT_COUNT);
}

bool Shop::refresh(Owner owner, int round, Player& buyer, int cost) {
    // 检查金币是否足够刷新
    if (buyer.getGold() < cost) {
        return false;
    }
    
    buyer.changeGold(-cost);
    
    units_.clear();
    units_.reserve(SHOP_SLOT_COUNT);
    
    for (int i = 0; i < SHOP_SLOT_COUNT; ++i) {
        auto unit = createRandomUnit(owner, i, round);
        if (unit) {
            units_.push_back(unit);
        }
    }
    
    // ===== P0-1: 刷新装备槽位 =====
    equipmentSlots_.clear();
    equipmentSlots_.reserve(EQUIP_SHOP_SLOT_COUNT);
    
    // 根据轮次决定可生成的装备品质上限
    // 轮次1-3: ≤2★; 轮次4+: ≤3★
    int maxStar = (round <= 3) ? 2 : 3;
    static std::mt19937 equipRng(std::random_device{}());
    
    for (int i = 0; i < EQUIP_SHOP_SLOT_COUNT; ++i) {
        // 70%概率生成装备（轮次越高概率越大）
        int dropChance = std::min(70 + (round - 1) * 5, 95);
        std::uniform_int_distribution<int> chanceDist(0, 99);
        if (chanceDist(equipRng) >= dropChance) continue;
        
        auto templates = GetEquipmentTemplatesByStar(maxStar);
        if (templates.empty()) continue;
        
        std::uniform_int_distribution<size_t> tmplDist(0, templates.size() - 1);
        const auto& tmpl = templates[tmplDist(equipRng)];
        
        std::uniform_int_distribution<int> atkDist(tmpl.atkMin, tmpl.atkMax);
        std::uniform_int_distribution<int> hpDist(tmpl.hpMin, tmpl.hpMax);
        
        ShopEquipmentSlot slot;
        slot.eq = Equipment(0, tmpl.name, atkDist(equipRng), hpDist(equipRng),
                            tmpl.type, tmpl.restriction, tmpl.starLevel, 0, tmpl.effect, tmpl.tag);
        // 价格：1★=2金, 2★=6金, 3★=12金
        switch (tmpl.starLevel) {
            case 1: slot.price = 2; break;
            case 2: slot.price = 6; break;
            case 3: slot.price = 12; break;
            default: slot.price = 2;
        }
        slot.sold = false;
        equipmentSlots_.push_back(slot);
    }
    
    return true;
}

bool Shop::buyUnit(int index, Player& buyer, int cost) {
    if (index < 0 || index >= static_cast<int>(units_.size()) || !units_[index]) {
        return false;
    }
    
    auto unit = units_[index];
    if (!unit) {
        return false;
    }
    
    if (!buyer.canRecruit()) {
        return false;
    }
    
    if (buyer.getGold() < cost) {
        return false;
    }
    
    // 执行购买
    if (!buyer.getBench().addUnit(unit)) {
        return false;
    }
    
    buyer.changeGold(-cost);
    buyer.addPopulation(1);
    units_[index] = nullptr;
    return true;
}

std::vector<std::shared_ptr<Unit>>& Shop::getUnits() {
    return units_;
}

const std::vector<std::shared_ptr<Unit>>& Shop::getUnits() const {
    return units_;
}

int Shop::getSlotCount() const {
    return SHOP_SLOT_COUNT;
}

Equipment* Shop::dropEquipment(int starLevel) {
    return Unit::DropEquipment(starLevel);
}

Shop::ApplyEquipmentResult Shop::applyEquipment(std::unique_ptr<Equipment>& droppedEquipment, Player& player) {
    ApplyEquipmentResult result;
    result.success = false;
    result.benchIndex = -1;
    
    if (!droppedEquipment) {
        result.message = "没有可装备的装备。";
        return result;
    }
    
    int slot = droppedEquipment->type;
    std::string restriction = droppedEquipment->restriction;
    
    // P2-4: 先尝试备战区
    auto& bench = player.getBench();
    for (int i = 0; i < BENCH_SIZE; ++i) {
        auto unit = bench.getUnit(i);
        if (!unit) continue;
        int actualSlot = slot;
        if ((slot == 2 || slot == 3) && unit->GetEquipmentSlot(2)) actualSlot = 3;
        if (unit->GetEquipmentSlot(actualSlot)) continue;
        if (!restriction.empty() && restriction != Unit::ClassToString(unit->GetClassType())) continue;
        if (unit->Equip(droppedEquipment.get(), actualSlot)) {
            result.success = true;
            result.benchIndex = i;
            player.equipToUnit(droppedEquipment->id, unit->getUnitId());
            droppedEquipment.release();
            droppedEquipment.reset();
            std::ostringstream ss;
            ss << "装备成功，已穿戴到备战区单位 " << (i + 1);
            result.message = ss.str();
            return result;
        }
    }
    
    // P2-4: 棋盘单位通过拖拽(P0-4)实现，applyEquipment仅限备战区
    
    result.message = "没有可装备目标，或槽位已满/不符合限制。";
    return result;
}

// ===== P2-2: 装备合成 =====
bool Shop::combineEquipment(Player& player, int eqId1, int eqId2, int eqId3, std::string& message) {
    Equipment* eq1 = player.getEquipmentById(eqId1);
    Equipment* eq2 = player.getEquipmentById(eqId2);
    Equipment* eq3 = player.getEquipmentById(eqId3);
    
    if (!eq1 || !eq2 || !eq3) { message = "装备不存在"; return false; }
    // 允许来自备战区/场上的装备参与合成（不再限制 ownerUnitId == 0）
    if (eq1->name != eq2->name || eq2->name != eq3->name) {
        message = "需要三件同名装备";
        return false;
    }
    if (eq1->starLevel != eq2->starLevel || eq2->starLevel != eq3->starLevel) {
        message = "需要三件同星级装备";
        return false;
    }
    if (eq1->starLevel >= 3) {
        message = "已达最高星级";
        return false;
    }
    
    // 如果装备在单位身上，先卸下
    player.unequipFromUnitByEquipId(eqId1);
    player.unequipFromUnitByEquipId(eqId2);
    player.unequipFromUnitByEquipId(eqId3);
    
    int newStar = eq1->starLevel + 1;
    
    // 升星装备：同名，星级+1，属性等比2倍
    Equipment newEq(0, eq1->name, eq1->atkBonus * 2, eq1->hpBonus * 2,
                    eq1->type, eq1->restriction, newStar, 0, eq1->effect, eq1->tag);
    message = "合成成功！获得 " + newEq.name + " ★" + std::to_string(newStar)
            + " ATK+" + std::to_string(newEq.atkBonus)
            + " HP+" + std::to_string(newEq.hpBonus);
    
    auto& table = player.getEquipmentTable();
    table.remove_if([&](const Equipment& e) { return e.id == eqId1 || e.id == eqId2 || e.id == eqId3; });
    player.addStoredEquipment(newEq);
    return true;
}

// ===== P2-3: 售卖装备 =====
int Shop::sellEquipment(Player& player, int eqId) {
    Equipment* eq = player.getEquipmentById(eqId);
    if (!eq || eq->ownerUnitId != 0) return 0;
    
    int price = 0;
    switch (eq->starLevel) {
        case 1: price = 1; break;
        case 2: price = 3; break;
        case 3: price = 6; break;
        default: price = 1;
    }
    
    player.changeGold(price);
    auto& table = player.getEquipmentTable();
    table.remove_if([&](const Equipment& e) { return e.id == eqId; });
    return price;
}

bool Shop::autoCombine(Player& player, std::string& message) {
    auto& bench = player.getBench();
    std::vector<int> indexes;
    for (int i = 0; i < BENCH_SIZE; ++i) {
        if (bench.getUnit(i)) indexes.push_back(i);
    }
    
    bool combined = false;
    for (size_t a = 0; a < indexes.size() && !combined; ++a) {
        for (size_t b = a + 1; b < indexes.size() && !combined; ++b) {
            for (size_t c = b + 1; c < indexes.size() && !combined; ++c) {
                auto ua = bench.getUnit(indexes[a]);
                auto ub = bench.getUnit(indexes[b]);
                auto uc = bench.getUnit(indexes[c]);
                if (!ua || !ub || !uc) continue;
                if (ua->getUnitType() != ub->getUnitType() || ua->getUnitType() != uc->getUnitType()) continue;
                if (ua->getStarLevel() != ub->getStarLevel() || ua->getStarLevel() != uc->getStarLevel()) continue;
                if (player.combineBenchUnits(indexes[a], indexes[b], indexes[c])) {
                    combined = true;
                }
            }
        }
    }
    
    if (combined) {
        message = "三合一成功，单位已合成升级。";
    } else {
        message = "未找到可三合一的相同单位组合。";
    }
    
    return combined;
}

bool Shop::upgradePopulation(Player& player, int cost) {
    return player.upgradePopulationLimit(cost);
}

void Shop::initializeForPlayer(Owner owner, int round) {
    // 初始刷新不消耗金币
    units_.clear();
    units_.reserve(SHOP_SLOT_COUNT);
    
    for (int i = 0; i < SHOP_SLOT_COUNT; ++i) {
        auto unit = createRandomUnit(owner, i, round);
        if (unit) {
            units_.push_back(unit);
        }
    }

    // 初始化装备槽位
    equipmentSlots_.clear();
    equipmentSlots_.reserve(EQUIP_SHOP_SLOT_COUNT);

    int maxStar = (round <= 3) ? 2 : ((round <= 6) ? 2 : 3);
    static std::mt19937 equipRng(std::random_device{}());

    for (int i = 0; i < EQUIP_SHOP_SLOT_COUNT; ++i) {
        int dropChance = std::min(70 + (round - 1) * 5, 95);
        std::uniform_int_distribution<int> chanceDist(0, 99);
        if (chanceDist(equipRng) >= dropChance) continue;

        auto templates = GetEquipmentTemplatesByStar(maxStar);
        if (templates.empty()) continue;

        std::uniform_int_distribution<size_t> tmplDist(0, templates.size() - 1);
        const auto& tmpl = templates[tmplDist(equipRng)];

        std::uniform_int_distribution<int> atkDist(tmpl.atkMin, tmpl.atkMax);
        std::uniform_int_distribution<int> hpDist(tmpl.hpMin, tmpl.hpMax);

        ShopEquipmentSlot slot;
        slot.eq = Equipment(0, tmpl.name, atkDist(equipRng), hpDist(equipRng),
                            tmpl.type, tmpl.restriction, tmpl.starLevel, 0, tmpl.effect, tmpl.tag);
        slot.price = (tmpl.starLevel == 3) ? 12 : 6;
        slot.sold = false;
        equipmentSlots_.push_back(slot);
    }
}

void Shop::reset() {
    units_.clear();
    equipmentSlots_.clear();
}

std::shared_ptr<Unit> Shop::createRandomUnit(Owner owner, int slotIndex, int round) {
    return UnitFactory::createRandomShopUnit(owner, slotIndex, round);
}

// ===== P0-1: 装备购买 =====
bool Shop::buyEquipment(int index, Player& buyer) {
    if (index < 0 || index >= static_cast<int>(equipmentSlots_.size())) return false;
    auto& slot = equipmentSlots_[index];
    if (slot.sold) return false;
    if (buyer.getGold() < slot.price) return false;
    
    buyer.changeGold(-slot.price);
    buyer.addStoredEquipment(slot.eq);
    slot.sold = true;
    return true;
}

std::vector<ShopEquipmentSlot>& Shop::getEquipmentSlots() {
    return equipmentSlots_;
}

const std::vector<ShopEquipmentSlot>& Shop::getEquipmentSlots() const {
    return equipmentSlots_;
}

int Shop::getEquipSlotCount() const {
    return static_cast<int>(equipmentSlots_.size());
}