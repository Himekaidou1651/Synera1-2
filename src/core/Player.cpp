/**
 * @file    Player.cpp
 * @brief   玩家属性管理实现
 * @author  
 * @date    2026-06-24
 */

#include "Player.h"

Player::Player()
    : health_(100), gold_(0), level_(1), populationLevel_(1), stage_(1), experience_(0), experienceNextLevel_(100),
      populationLimit_(0), currentPopulation_(0) {
    updatePopulationLimit();
}

int Player::getHealth() const { return health_; }
int Player::getGold() const { return gold_; }
int Player::getLevel() const { return level_; }
int Player::getPopulationLevel() const { return populationLevel_; }
int Player::getStage() const { return stage_; }
int Player::getExperience() const { return experience_; }
int Player::getPopulationLimit() const { return populationLimit_; }
int Player::getCurrentPopulation() const { return currentPopulation_; }

void Player::changeHealth(int delta) {
    health_ += delta;
    if (health_ < 0) {
        health_ = 0;
    }
}

void Player::changeGold(int delta) {
    gold_ += delta;
    if (gold_ < 0) {
        gold_ = 0;
    }
}

void Player::setHealth(int health) {
    health_ = health;
    if (health_ < 0) {
        health_ = 0;
    }
}

void Player::setGold(int gold) {
    gold_ = std::max(0, gold);
}

void Player::gainExperience(int amount) {
    experience_ += amount;
    while (experience_ >= experienceNextLevel_) {
        // 允许多次升级溢出经验或保留进度
        levelUp(experience_-experienceNextLevel_);
    }
}

void Player::levelUp(int exp) {
    level_ += 1;
    experience_ = std::max(0, exp);
    changeGold(1);
    // 经验升级不影响人口上限
    updateExperienceThreshold();
}

bool Player::canRecruit() const {
    return currentPopulation_ < populationLimit_ && !bench_.isFull();
}

void Player::addPopulation(int amount) {
    currentPopulation_ += amount;
    if (currentPopulation_ < 0) {
        currentPopulation_ = 0;
    }
}

void Player::removePopulation(int amount) {
    currentPopulation_ -= amount;
    if (currentPopulation_ < 0) {
        currentPopulation_ = 0;
    }
}

void Player::resetRound() {
    experience_ = std::max(0, experience_);
    updatePopulationLimit();
}

void Player::advanceStage() {
    stage_ += 1;
    changeGold(1);
}

void Player::prepareRecruitment() {
    if (gold_ >= 2) {
        changeGold(-2);
    }
    if (stage_ % 3 == 0) {
        changeGold(1);
    }
}

bool Player::upgradePopulationLimit(int cost) {
    if (gold_ < cost) {
        return false;
    }
    changeGold(-cost);
    populationLevel_ += 1;
    // 人口升级不影响经验等级
    updatePopulationLimit();
    return true;
}

void Player::setLevel(int level) {
    level_ = std::max(1, level);
    updateExperienceThreshold();
}

void Player::setPopulationLevel(int level) {
    populationLevel_ = std::max(1, level);
    updatePopulationLimit();
}

void Player::setStage(int stage) {
    stage_ = std::max(1, stage);
}

void Player::setExperience(int experience) {
    experience_ = std::max(0, experience);
}

bool Player::combineBenchUnits(int indexA, int indexB, int indexC) {
    bool result = bench_.combineUnits(indexA, indexB, indexC);
    if (result) {
        removePopulation(2);
    }
    return result;
}

bool Player::upgradeUnitEquipment(std::shared_ptr<Unit> unit) {
    if (!unit) {
        return false;
    }
    int cost = unit->getEquipmentUpgradeCost();
    if (gold_ < cost) {
        return false;
    }
    changeGold(-cost);
    return unit->upgradeEquipment();
}

bool Player::upgradeUnitEquipmentInBench(int index) {
    auto unit = bench_.getUnit(index);
    return upgradeUnitEquipment(unit);
}

void Player::updatePopulationLimit() {
    populationLimit_ = std::max(1, populationLevel_ + 1);
}

void Player::updateExperienceThreshold() {
    experienceNextLevel_ = (level_ + 1) * 50;
}

Bench& Player::getBench() { return bench_; }
const Bench& Player::getBench() const { return bench_; }

std::list<Equipment>& Player::getEquipmentTable() {
    return equipmentTable_;
}

const std::list<Equipment>& Player::getEquipmentTable() const {
    return equipmentTable_;
}

int Player::addStoredEquipment(const Equipment& eq) {
    Equipment newEq = eq;
    newEq.id = nextEquipId_++;
    newEq.ownerUnitId = 0;
    equipmentTable_.push_back(newEq);
    return newEq.id;
}

Equipment* Player::getEquipmentById(int id) {
    for (auto& eq : equipmentTable_) {
        if (eq.id == id) {
            return &eq;
        }
    }
    return nullptr;
}

bool Player::equipToUnit(int equipId, int unitId) {
    Equipment* eq = getEquipmentById(equipId);
    if (!eq) return false;
    eq->ownerUnitId = unitId;
    return true;
}

bool Player::unequipFromUnit(int equipId) {
    Equipment* eq = getEquipmentById(equipId);
    if (!eq) return false;
    eq->ownerUnitId = 0;
    return true;
}

std::vector<Equipment*> Player::getStoredEquipments() {
    std::vector<Equipment*> result;
    for (auto& eq : equipmentTable_) {
        if (eq.ownerUnitId == 0) {
            result.push_back(&eq);
        }
    }
    return result;
}

std::vector<Equipment*> Player::getEquipmentsForUnit(int unitId) {
    std::vector<Equipment*> result;
    for (auto& eq : equipmentTable_) {
        if (eq.ownerUnitId == unitId) {
            result.push_back(&eq);
        }
    }
    return result;
}

std::vector<Equipment*> Player::findAllEquipmentByNameAndStar(const std::string& name, int star) {
    std::vector<Equipment*> result;
    // 1. 搜索暂存区装备（ownerUnitId == 0）
    for (auto& eq : equipmentTable_) {
        if (eq.ownerUnitId == 0 && eq.name == name && eq.starLevel == star) {
            result.push_back(&eq);
        }
    }
    // 2. 搜索备战区单位身上的装备
    for (int i = 0; i < BENCH_SIZE; ++i) {
        auto unit = bench_.getUnit(i);
        if (!unit) continue;
        for (int s = 0; s < 4; ++s) {
            Equipment* eq = unit->GetEquipmentSlot(s);
            if (eq && eq->name == name && eq->starLevel == star) {
                result.push_back(eq);
            }
        }
    }
    return result;
}

bool Player::unequipFromUnitByEquipId(int equipId) {
    Equipment* eq = getEquipmentById(equipId);
    if (!eq) return false;
    int unitId = eq->ownerUnitId;
    if (unitId == 0) return true; // 已在暂存区
    
    // 在备战区中查找持有该装备的单位
    for (int i = 0; i < BENCH_SIZE; ++i) {
        auto unit = bench_.getUnit(i);
        if (!unit || unit->getUnitId() != unitId) continue;
        for (int s = 0; s < 4; ++s) {
            if (unit->GetEquipmentSlot(s) == eq) {
                unit->Unequip(s);
                eq->ownerUnitId = 0;
                return true;
            }
        }
    }
    return false;
}

void Player::setNextEquipId(int id) {
    nextEquipId_ = id;
}