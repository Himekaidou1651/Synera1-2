/**
 * @file    Skill.h
 * @brief   技能系统，定义技能基类与伤害/治疗/增益/大招/被动等派生技能
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"

/**
 * @brief 技能类型
 */
enum class SkillType {
    Active,     ///< 主动技能（需要手动施放）
    Passive,    ///< 被动技能（自动触发）
    Ultimate,   ///< 终极技能（消耗法力）
    Trigger     ///< 触发技能（满足条件时自动触发）
};

/**
 * @brief 技能目标类型
 */
enum class SkillTargetType {
    None,           ///< 无目标（被动/光环）
    SingleEnemy,    ///< 单个敌方
    SingleAlly,     ///< 单个友方
    Self,           ///< 自身
    AllEnemies,     ///< 所有敌方
    AllAllies,      ///< 所有友方
    AOE,            ///< 范围（圆形/矩形）
    RandomEnemy,    ///< 随机敌方
    LowestHpAlly,   ///< 生命值最低的友方
    HighestAtkAlly  ///< 攻击力最高的友方
};

/**
 * @brief 技能效果类型
 */
enum class SkillEffectType {
    Damage,         ///< 伤害
    Heal,           ///< 治疗
    Buff,           ///< 增益
    Debuff,         ///< 减益
    Stun,           ///< 眩晕
    Freeze,         ///< 冰冻
    Knockback,      ///< 击退
    Pull,           ///< 拉拽
    Shield,         ///< 护盾
    ManaRecover,    ///< 法力回复
    Summon,         ///< 召唤
    Teleport,       ///< 传送
    Resurrection,   ///< 复活
    AOE
};

/**
 * @brief 技能效果数据
 */
struct SkillEffect {
    SkillEffectType type;
    float value;                ///< 数值（伤害量/治疗量/持续时间等）
    float radius;               ///< 范围半径（对 AOE 有效）
    int targetCount;            ///< 目标数量（-1 表示所有）
    std::string buffName;       ///< 增益/减益名称
    int duration;               ///< 持续时间（回合数）
    bool canCrit;               ///< 是否可以暴击
};

/**
 * @brief 技能等级配置
 */
struct SkillLevelConfig {
    int requiredLevel;          ///< 所需单位等级
    float valueMultiplier;      ///< 数值倍率
    float cooldownReduction;    ///< 冷却缩减（秒）
    int manaCostReduction;      ///< 法力消耗减少
};

/**
 * @class   Skill
 * @brief   技能基类，封装冷却、法力消耗、目标选择和效果应用
 */
class Skill {
public:
    Skill(const std::string& name,
          SkillType type,
          SkillTargetType targetType,
          int manaCost = 0,
          float cooldown = 1.0f,
          int maxLevel = 1);
    virtual ~Skill() = default;

    // ========== 核心方法 ==========
    virtual bool canActivate(std::shared_ptr<Unit> caster) const;
    virtual void activate(std::shared_ptr<Unit> caster,
                          std::vector<std::shared_ptr<Unit>>& targets);
    virtual void onCooldownTick();

    // ========== 目标选择 ==========
    virtual std::vector<std::shared_ptr<Unit>> selectTargets(
        std::shared_ptr<Unit> caster,
        const std::vector<std::shared_ptr<Unit>>& allies,
        const std::vector<std::shared_ptr<Unit>>& enemies) const;

    // ========== 升级系统 ==========
    bool levelUp();
    int getLevel() const;
    int getMaxLevel() const;
    void setLevel(int level);

    // ========== 访问器 ==========
    const std::string& getName() const;
    SkillType getType() const;
    SkillTargetType getTargetType() const;
    bool isOnCooldown() const;
    float getCurrentCooldown() const;
    float getMaxCooldown() const;
    int getManaCost() const;

    // ========== 冷却管理 ==========
    void startCooldown();
    void resetCooldown();
    void reduceCooldown(float amount);

    // ========== 效果管理 ==========
    void addEffect(const SkillEffect& effect);
    const std::vector<SkillEffect>& getEffects() const;
    void clearEffects();

    // 描述
    virtual std::string getDescription() const;

protected:
    std::string name_;
    SkillType type_;
    SkillTargetType targetType_;
    int manaCost_;
    float maxCooldown_;
    float currentCooldown_;
    int level_;
    int maxLevel_;
    std::vector<SkillEffect> effects_;
    std::vector<SkillLevelConfig> levelConfigs_;

    /**
     * @brief  应用单个效果到目标
     * @param  effect  技能效果
     * @param  caster  施法者
     * @param  target  目标单位
     */
    void applyEffect(SkillEffect& effect, std::shared_ptr<Unit> caster,
                     std::shared_ptr<Unit> target);

    /**
     * @brief  应用 AOE 范围效果
     * @param  effect     技能效果
     * @param  caster     施法者
     * @param  targets    主目标列表
     * @param  allTargets 全部可能目标
     */
    void applyAOE(SkillEffect& effect, std::shared_ptr<Unit> caster,
                  std::vector<std::shared_ptr<Unit>>& targets,
                  const std::vector<std::shared_ptr<Unit>>& allTargets);
};

// ========== 派生技能类 ==========

/**
 * @class   DamageSkill
 * @brief   伤害技能，基于攻击力与倍率造成伤害
 */
class DamageSkill : public Skill {
public:
    DamageSkill(const std::string& name,
                SkillTargetType targetType,
                int baseDamage,
                float damageMultiplier = 1.0f,
                int manaCost = 0,
                float cooldown = 1.0f);

    void activate(std::shared_ptr<Unit> caster,
                  std::vector<std::shared_ptr<Unit>>& targets) override;

private:
    int baseDamage_;
    float damageMultiplier_;
};

/**
 * @class   HealSkill
 * @brief   治疗技能，基于攻击力与倍率恢复生命
 */
class HealSkill : public Skill {
public:
    HealSkill(const std::string& name,
              SkillTargetType targetType,
              int baseHeal,
              float healMultiplier = 1.0f,
              int manaCost = 0,
              float cooldown = 1.0f);

    void activate(std::shared_ptr<Unit> caster,
                  std::vector<std::shared_ptr<Unit>>& targets) override;

private:
    int baseHeal_;
    float healMultiplier_;
};

/**
 * @class   BuffSkill
 * @brief   增益技能，为目标添加指定 buff
 */
class BuffSkill : public Skill {
public:
    BuffSkill(const std::string& name,
              SkillTargetType targetType,
              const std::string& buffName,
              int duration,
              int manaCost = 0,
              float cooldown = 1.0f);

    void activate(std::shared_ptr<Unit> caster,
                  std::vector<std::shared_ptr<Unit>>& targets) override;

private:
    std::string buffName_;
    int duration_;
};

/**
 * @class   UltimateSkill
 * @brief   终极技能（大招），消耗满法力释放高伤害
 */
class UltimateSkill : public Skill {
public:
    UltimateSkill(const std::string& name,
                  SkillTargetType targetType,
                  int baseDamage,
                  float damageMultiplier = 2.0f,
                  int manaCost = 100,
                  float cooldown = 3.0f);

    bool canActivate(std::shared_ptr<Unit> caster) const override;
    void activate(std::shared_ptr<Unit> caster,
                  std::vector<std::shared_ptr<Unit>>& targets) override;
    std::string getDescription() const override;

private:
    int baseDamage_;
    float damageMultiplier_;
};

/**
 * @class   PassiveSkill
 * @brief   被动技能，自动为自身添加永久 buff
 */
class PassiveSkill : public Skill {
public:
    PassiveSkill(const std::string& name,
                 const std::string& buffName,
                 SkillTargetType targetType = SkillTargetType::Self);

    bool canActivate(std::shared_ptr<Unit> caster) const override;
    void activate(std::shared_ptr<Unit> caster,
                  std::vector<std::shared_ptr<Unit>>& targets) override;

private:
    std::string buffName_;
    bool activated_;
};