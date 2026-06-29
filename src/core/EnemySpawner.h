/**
 * @file    EnemySpawner.h
 * @brief   敌方生成系统，负责按轮次配置随机生成敌方阵容
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"

using json = nlohmann::json;

/**
 * @brief 敌方波次配置，定义每轮的生成参数
 */
struct EnemyWaveConfig {
    int round;                  ///< 轮次编号
    int minCount;               ///< 最少敌方数量
    int maxCount;               ///< 最多敌方数量
    float attributeMultiplier;  ///< 属性倍数
};

/**
 * @class   EnemySpawner
 * @brief   敌方生成器，从 JSON 配置加载波次并随机生成敌方单位
 */
class EnemySpawner {
public:
    EnemySpawner();
    
    /**
     * @brief  从 JSON 配置文件加载波次信息
     * @param  configPath 配置文件路径
     * @return 是否加载成功
     */
    bool loadStageConfig(const std::string& configPath);
    
    /**
     * @brief  根据当前轮次随机生成敌方阵容
     * @param  round 当前轮次
     * @return 生成的敌方单位列表
     */
    std::vector<std::shared_ptr<Unit>> spawnWave(int round);
    
    /**
     * @brief  计算属性增长倍数（每3轮+20%）
     * @param  round 当前轮次
     * @return 属性倍数值
     */
    float getAttributeMultiplier(int round) const;
    
    /**
     * @brief  获取无限模式下的属性倍数（每轮+3%）
     * @param  round 当前轮次
     * @return 无限模式倍数值
     */
    float getInfiniteModeMultiplier(int round) const;
    
    /**
     * @brief  根据配置创建随机敌方单位（供 AI 购买使用）
     * @param  starLevel  星级
     * @param  multiplier 属性倍数
     * @return 创建的单位
     */
    std::shared_ptr<Unit> createRandomEnemyUnit(int starLevel, float multiplier);

private:
    std::vector<EnemyWaveConfig> stages_;       ///< 波次配置列表
    json baseUnitConfigs_;                      ///< 基础单位配置（JSON）
    std::vector<std::string> unitTypes_;        ///< 可用的单位类型列表
    
    /**
     * @brief  获取给定轮次的波次配置
     * @param  round 当前轮次
     * @return 波次配置
     */
    EnemyWaveConfig getWaveConfig(int round) const;
};

