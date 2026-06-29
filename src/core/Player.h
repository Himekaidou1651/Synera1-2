/**
 * @file    Player.h
 * @brief   玩家属性管理（血量、金币、等级、人口、装备表）
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Bench.h"
#include "Unit.h"

/**
 * @class   Player
 * @brief   玩家数据模型，管理生命值、金币、等级、人口、备战区和装备表
 */
class Player {
public:
    Player();

    int getHealth() const;
    int getGold() const;
    int getLevel() const;
    int getPopulationLevel() const;
    int getStage() const;
    int getExperience() const;
    int getExperienceNextLevel() const { return experienceNextLevel_; }
    int getPopulationLimit() const;
    int getCurrentPopulation() const;

    void changeHealth(int delta);
    void changeGold(int delta);
    void setHealth(int health);
    void setGold(int gold);
    void setLevel(int level);
    void setPopulationLevel(int level);
    void setStage(int stage);
    void setExperience(int experience);
    void setExperienceNextLevel(int nextLevel) { experienceNextLevel_ = std::max(1, nextLevel); }
    void gainExperience(int amount);
    void levelUp(int exp);
    bool canRecruit() const;
    void addPopulation(int amount);
    void removePopulation(int amount);
    void resetRound();
    void advanceStage();
    void prepareRecruitment();
    bool upgradePopulationLimit(int cost = 5);
    bool combineBenchUnits(int indexA, int indexB, int indexC);
    bool upgradeUnitEquipment(std::shared_ptr<Unit> unit);
    bool upgradeUnitEquipmentInBench(int index);

    Bench& getBench();
    const Bench& getBench() const;

    // ========== 全局装备表管理 ==========
    std::list<Equipment>& getEquipmentTable();
    const std::list<Equipment>& getEquipmentTable() const;
    int addStoredEquipment(const Equipment& eq);
    Equipment* getEquipmentById(int id);
    bool equipToUnit(int equipId, int unitId);
    bool unequipFromUnit(int equipId);
    std::vector<Equipment*> getStoredEquipments();
    std::vector<Equipment*> getEquipmentsForUnit(int unitId);
    std::vector<Equipment*> findAllEquipmentByNameAndStar(const std::string& name, int star);
    bool unequipFromUnitByEquipId(int equipId);
    void setNextEquipId(int id);

private:
    void updatePopulationLimit();
    void updateExperienceThreshold();

    int health_;
    int gold_;
    int level_;
    int populationLevel_;
    int stage_;
    int experience_;
    int experienceNextLevel_;
    int populationLimit_;
    int currentPopulation_;
    Bench bench_;
    std::list<Equipment> equipmentTable_;  ///< 全局装备表
    int nextEquipId_ = 1;                  ///< 下一个装备 ID
};
