/**
 * @file    EnemySpawner.cpp
 * @brief   敌方生成系统实现
 * @author  
 * @date    2026-06-24
 */

#include "EnemySpawner.h"
#include <fstream>
#include <cmath>
#include <cstdlib>

EnemySpawner::EnemySpawner() {}

bool EnemySpawner::loadStageConfig(const std::string& configPath) {
    try {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            return false;
        }
        json config;
        file >> config;
        
        // 解析基础单位配置
        if (config.contains("baseUnits")) {
            baseUnitConfigs_ = config["baseUnits"];
            // 收集所有可用的单位类型
            for (auto& [unitType, unitConfig] : baseUnitConfigs_.items()) {
                unitTypes_.push_back(unitType);
            }
        }
        
        // 解析波次配置（新格式：minCount, maxCount）
        if (config.contains("stages")) {
            for (const auto& stage : config["stages"]) {
                EnemyWaveConfig wave;
                wave.round = stage.value("round", 0);
                wave.minCount = stage.value("minCount", 2);
                wave.maxCount = stage.value("maxCount", 4);
                wave.attributeMultiplier = stage.value("attributeMultiplier", 1.0f);
                
                stages_.push_back(wave);
            }
        }
        
        return !stages_.empty() && !unitTypes_.empty();
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<std::shared_ptr<Unit>> EnemySpawner::spawnWave(int round) {
    std::vector<std::shared_ptr<Unit>> enemies;
    
    // 获取当前轮次的波次配置
    EnemyWaveConfig waveConfig = getWaveConfig(round);
    
    // 计算属性倍数
    float multiplier = getAttributeMultiplier(round);
    
    // 随机生成敌方数量（在 minCount 和 maxCount 之间）
    int enemyCount = waveConfig.minCount + (rand() % (waveConfig.maxCount - waveConfig.minCount + 1));
    
    // 根据轮数决定敌方星级范围
    // 轮1-3: 1星，轮4-6: 1-2星混合，轮7-10: 2-3星混合
    int minStarLevel = 1;
    int maxStarLevel = 1;
    
    if (round <= 3) {
        minStarLevel = 1;
        maxStarLevel = 1;
    } else if (round <= 6) {
        minStarLevel = 1;
        maxStarLevel = 2;
    } else {
        minStarLevel = 2;
        maxStarLevel = 3;
    }
    
    // 随机生成敌方单位
    for (int i = 0; i < enemyCount; ++i) {
        int starLevel = minStarLevel + (rand() % (maxStarLevel - minStarLevel + 1));
        auto enemy = createRandomEnemyUnit(starLevel, multiplier);
        if (enemy) {
            enemies.push_back(enemy);
        }
    }
    
    return enemies;
}

EnemyWaveConfig EnemySpawner::getWaveConfig(int round) const {
    // 查找对应轮次的配置
    for (const auto& stage : stages_) {
        if (stage.round == round) {
            return stage;
        }
    }
    
    // 没有找到特定配置时使用无限模式
    if (!stages_.empty()) {
        EnemyWaveConfig infiniteConfig = stages_.back();
        // 无限模式下稍微增加敌方数量
        infiniteConfig.minCount = std::min(7, infiniteConfig.minCount + (round - 10) / 5);
        infiniteConfig.maxCount = std::min(8, infiniteConfig.maxCount + (round - 10) / 5);
        return infiniteConfig;
    }
    
    // 默认配置
    return EnemyWaveConfig{round, 2, 4, 1.0f};
}

float EnemySpawner::getAttributeMultiplier(int round) const {
    // 每3轮属性+20%
    // 轮次 1-3: 1.0x, 轮次 4-6: 1.2x, 轮次 7-9: 1.44x, ...
    int cycles = (round - 1) / 3;
    return std::pow(1.2f, cycles);
}

float EnemySpawner::getInfiniteModeMultiplier(int round) const {
    // 无限模式下敌人强度每轮增长 3%
    return std::pow(1.03f, round - 1);
}

std::shared_ptr<Unit> EnemySpawner::createRandomEnemyUnit(int starLevel, float multiplier) {
    try {
        // 从可用单位中随机选择一个
        if (unitTypes_.empty()) {
            return nullptr;
        }
        
        int randomIndex = rand() % unitTypes_.size();
        const std::string& unitType = unitTypes_[randomIndex];
        
        if (!baseUnitConfigs_.contains(unitType)) {
            // 无配置时创建默认敌方单位
            int hp = static_cast<int>(50 * multiplier);
            int atk = static_cast<int>(8 * multiplier);
            int maxMana = 100;
            
            return std::make_shared<Unit>(hp, atk, maxMana, 0, Owner::EnemyCtrl,
                                         unitType, "普攻", "大招", "装备", starLevel);
        }
        
        const auto& config = baseUnitConfigs_[unitType];
        
        // 根据星阶增加属性
        float starMultiplier = 1.0f + (starLevel - 1) * 0.3f;
        
        int hp = static_cast<int>(config.value("hp", 50) * multiplier * starMultiplier);
        int atk = static_cast<int>(config.value("atk", 8) * multiplier * starMultiplier);
        int maxMana = config.value("maxMana", 100);
        
        auto unit = std::make_shared<Unit>(hp, atk, maxMana, 0, Owner::EnemyCtrl,
                                          unitType, "普攻", "大招", "装备", starLevel);
        
        // 应用特性
        if (config.contains("traits")) {
            for (const auto& trait : config["traits"]) {
                unit->addTrait(trait);
            }
        }
        
        return unit;
    } catch (const std::exception& e) {
        return nullptr;
    }
}
