/**
 * @file    Skill.cpp
 * @brief   技能系统实现
 * @author  
 * @date    2026-06-24
 */

#include "Skill.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>

// ========== Skill 基类 ==========

Skill::Skill(const std::string& name,
             SkillType type,
             SkillTargetType targetType,
             int manaCost,
             float cooldown,
             int maxLevel)
    : name_(name), type_(type), targetType_(targetType),
      manaCost_(manaCost), maxCooldown_(cooldown),
      currentCooldown_(0.0f), level_(1), maxLevel_(maxLevel) {
}

bool Skill::canActivate(std::shared_ptr<Unit> caster) const {
    if (!caster || !caster->isAlive()) return false;
    if (isOnCooldown()) return false;

    // 检查法力值
    if (manaCost_ > 0 && caster->getMana() < manaCost_) {
        return false;
    }

    return true;
}

void Skill::activate(std::shared_ptr<Unit> caster,
                     std::vector<std::shared_ptr<Unit>>& targets) {
    if (!canActivate(caster)) return;

    // 消耗法力
    if (manaCost_ > 0) {
        // 法力消耗由外部系统处理
    }

    // 应用所有效果
    for (auto& effect : effects_) {
        switch (effect.type) {
            case SkillEffectType::Damage:
            case SkillEffectType::Heal:
            case SkillEffectType::Buff:
            case SkillEffectType::Debuff:
            case SkillEffectType::Stun:
            case SkillEffectType::Freeze:
                // 对每个目标应用效果
                for (auto& target : targets) {
                    if (target && target->isAlive()) {
                        applyEffect(effect, caster, target);
                    }
                }
                break;

            case SkillEffectType::AOE:
                // AOE 效果需要额外处理
                applyAOE(effect, caster, targets, targets);
                break;

            default:
                break;
        }
    }

    // 开始冷却
    startCooldown();
}

void Skill::onCooldownTick() {
    if (currentCooldown_ > 0) {
        currentCooldown_ -= 1.0f;  // 每回合减少1
        if (currentCooldown_ < 0) {
            currentCooldown_ = 0;
        }
    }
}

std::vector<std::shared_ptr<Unit>> Skill::selectTargets(
    std::shared_ptr<Unit> caster,
    const std::vector<std::shared_ptr<Unit>>& allies,
    const std::vector<std::shared_ptr<Unit>>& enemies) const {

    std::vector<std::shared_ptr<Unit>> targets;

    switch (targetType_) {
        case SkillTargetType::None:
            break;

        case SkillTargetType::Self:
            if (caster) targets.push_back(caster);
            break;

        case SkillTargetType::SingleEnemy:
            if (!enemies.empty()) {
                // 选择最近的敌方
                auto [cx, cy] = caster->getPosition();
                float minDist = std::numeric_limits<float>::max();
                std::shared_ptr<Unit> closest = nullptr;

                for (auto& enemy : enemies) {
                    if (enemy && enemy->isAlive()) {
                        auto [ex, ey] = enemy->getPosition();
                        float dist = std::sqrt((cx - ex) * (cx - ex) + (cy - ey) * (cy - ey));
                        if (dist < minDist) {
                            minDist = dist;
                            closest = enemy;
                        }
                    }
                }
                if (closest) targets.push_back(closest);
            }
            break;

        case SkillTargetType::SingleAlly:
            if (!allies.empty()) {
                // 选择生命值最低的友方
                auto lowest = std::min_element(allies.begin(), allies.end(),
                    [](const std::shared_ptr<Unit>& a, const std::shared_ptr<Unit>& b) {
                        if (!a || !a->isAlive()) return false;
                        if (!b || !b->isAlive()) return true;
                        return a->getHP() < b->getHP();
                    });
                if (lowest != allies.end() && *lowest && (*lowest)->isAlive()) {
                    targets.push_back(*lowest);
                }
            }
            break;

        case SkillTargetType::AllEnemies:
            for (auto& enemy : enemies) {
                if (enemy && enemy->isAlive()) {
                    targets.push_back(enemy);
                }
            }
            break;

        case SkillTargetType::AllAllies:
            for (auto& ally : allies) {
                if (ally && ally->isAlive()) {
                    targets.push_back(ally);
                }
            }
            break;

        case SkillTargetType::LowestHpAlly:
            if (!allies.empty()) {
                auto lowest = std::min_element(allies.begin(), allies.end(),
                    [](const std::shared_ptr<Unit>& a, const std::shared_ptr<Unit>& b) {
                        if (!a || !a->isAlive()) return false;
                        if (!b || !b->isAlive()) return true;
                        return a->getHP() < b->getHP();
                    });
                if (lowest != allies.end() && *lowest && (*lowest)->isAlive()) {
                    targets.push_back(*lowest);
                }
            }
            break;

        case SkillTargetType::RandomEnemy:
            if (!enemies.empty()) {
                std::vector<std::shared_ptr<Unit>> aliveEnemies;
                for (auto& enemy : enemies) {
                    if (enemy && enemy->isAlive()) {
                        aliveEnemies.push_back(enemy);
                    }
                }
                if (!aliveEnemies.empty()) {
                    int idx = std::rand() % aliveEnemies.size();
                    targets.push_back(aliveEnemies[idx]);
                }
            }
            break;

        default:
            break;
    }

    return targets;
}

bool Skill::levelUp() {
    if (level_ >= maxLevel_) return false;
    level_++;
    return true;
}

int Skill::getLevel() const { return level_; }
int Skill::getMaxLevel() const { return maxLevel_; }

void Skill::setLevel(int level) {
    level_ = std::clamp(level, 1, maxLevel_);
}

const std::string& Skill::getName() const { return name_; }
SkillType Skill::getType() const { return type_; }
SkillTargetType Skill::getTargetType() const { return targetType_; }

bool Skill::isOnCooldown() const {
    return currentCooldown_ > 0;
}

float Skill::getCurrentCooldown() const { return currentCooldown_; }
float Skill::getMaxCooldown() const { return maxCooldown_; }
int Skill::getManaCost() const { return manaCost_; }

void Skill::startCooldown() {
    currentCooldown_ = maxCooldown_;
}

void Skill::resetCooldown() {
    currentCooldown_ = 0;
}

void Skill::reduceCooldown(float amount) {
    currentCooldown_ = std::max(0.0f, currentCooldown_ - amount);
}

void Skill::addEffect(const SkillEffect& effect) {
    effects_.push_back(effect);
}

const std::vector<SkillEffect>& Skill::getEffects() const {
    return effects_;
}

void Skill::clearEffects() {
    effects_.clear();
}

std::string Skill::getDescription() const {
    return name_ + " (Lv." + std::to_string(level_) + ")";
}

void Skill::applyEffect(SkillEffect& effect, std::shared_ptr<Unit> caster,
                        std::shared_ptr<Unit> target) {
    if (!target || !target->isAlive()) return;

    switch (effect.type) {
        case SkillEffectType::Damage: {
            int damage = static_cast<int>(effect.value);
            target->takeDamage(damage);
            break;
        }
        case SkillEffectType::Heal: {
            // 治疗：通过负伤害值实现
            int heal = static_cast<int>(effect.value);
            // 负伤害即治疗
            target->takeDamage(-heal);
            break;
        }
        case SkillEffectType::Buff:
            if (!effect.buffName.empty()) {
                target->applyBuff(effect.buffName);
            }
            break;

        case SkillEffectType::Debuff:
            // 减益效果由外部状态机处理
            break;

        case SkillEffectType::Stun:
        case SkillEffectType::Freeze:
            // 控制效果（眩晕/冰冻）由状态机处理
            break;

        default:
            break;
    }
}

void Skill::applyAOE(SkillEffect& effect, std::shared_ptr<Unit> caster,
                     std::vector<std::shared_ptr<Unit>>& targets,
                     const std::vector<std::shared_ptr<Unit>>& allTargets) {
    if (!caster) return;

    auto [cx, cy] = caster->getPosition();
    float radius = effect.radius;

    for (auto& target : allTargets) {
        if (!target || !target->isAlive()) continue;

        auto [tx, ty] = target->getPosition();
        float dist = std::sqrt((cx - tx) * (cx - tx) + (cy - ty) * (cy - ty));

        if (dist <= radius) {
            applyEffect(effect, caster, target);
        }
    }
}

// ========== DamageSkill ==========

DamageSkill::DamageSkill(const std::string& name,
                         SkillTargetType targetType,
                         int baseDamage,
                         float damageMultiplier,
                         int manaCost,
                         float cooldown)
    : Skill(name, SkillType::Active, targetType, manaCost, cooldown),
      baseDamage_(baseDamage), damageMultiplier_(damageMultiplier) {

    SkillEffect effect;
    effect.type = SkillEffectType::Damage;
    effect.value = static_cast<float>(baseDamage_);
    addEffect(effect);
}

void DamageSkill::activate(std::shared_ptr<Unit> caster,
                           std::vector<std::shared_ptr<Unit>>& targets) {
    if (!canActivate(caster)) return;

    int atk = caster->getATK();
    int damage = static_cast<int>(baseDamage_ + atk * damageMultiplier_);

    for (auto& target : targets) {
        if (target && target->isAlive()) {
            target->takeDamage(damage);
        }
    }

    startCooldown();
}

// ========== HealSkill ==========

HealSkill::HealSkill(const std::string& name,
                     SkillTargetType targetType,
                     int baseHeal,
                     float healMultiplier,
                     int manaCost,
                     float cooldown)
    : Skill(name, SkillType::Active, targetType, manaCost, cooldown),
      baseHeal_(baseHeal), healMultiplier_(healMultiplier) {
}

void HealSkill::activate(std::shared_ptr<Unit> caster,
                         std::vector<std::shared_ptr<Unit>>& targets) {
    if (!canActivate(caster)) return;

    int healAmount = static_cast<int>(baseHeal_ + caster->getATK() * healMultiplier_);

    for (auto& target : targets) {
        if (target && target->isAlive()) {
            // 治疗：负伤害
            target->takeDamage(-healAmount);
        }
    }

    startCooldown();
}

// ========== BuffSkill ==========

BuffSkill::BuffSkill(const std::string& name,
                     SkillTargetType targetType,
                     const std::string& buffName,
                     int duration,
                     int manaCost,
                     float cooldown)
    : Skill(name, SkillType::Active, targetType, manaCost, cooldown),
      buffName_(buffName), duration_(duration) {
}

void BuffSkill::activate(std::shared_ptr<Unit> caster,
                         std::vector<std::shared_ptr<Unit>>& targets) {
    if (!canActivate(caster)) return;

    for (auto& target : targets) {
        if (target && target->isAlive()) {
            target->applyBuff(buffName_);
        }
    }

    startCooldown();
}

// ========== UltimateSkill ==========

UltimateSkill::UltimateSkill(const std::string& name,
                             SkillTargetType targetType,
                             int baseDamage,
                             float damageMultiplier,
                             int manaCost,
                             float cooldown)
    : Skill(name, SkillType::Ultimate, targetType, manaCost, cooldown),
      baseDamage_(baseDamage), damageMultiplier_(damageMultiplier) {
}

bool UltimateSkill::canActivate(std::shared_ptr<Unit> caster) const {
    if (!Skill::canActivate(caster)) return false;
    return caster->canUseUltimate();
}

void UltimateSkill::activate(std::shared_ptr<Unit> caster,
                             std::vector<std::shared_ptr<Unit>>& targets) {
    if (!canActivate(caster)) return;

    int atk = caster->getATK();
    int damage = static_cast<int>((baseDamage_ + atk) * damageMultiplier_);

    for (auto& target : targets) {
        if (target && target->isAlive()) {
            target->takeDamage(damage);
        }
    }

    startCooldown();
}

std::string UltimateSkill::getDescription() const {
    return name_ + " [大招] (消耗" + std::to_string(manaCost_) + "法力, " +
           "冷却" + std::to_string(static_cast<int>(maxCooldown_)) + "回合)";
}

// ========== PassiveSkill ==========

PassiveSkill::PassiveSkill(const std::string& name,
                           const std::string& buffName,
                           SkillTargetType targetType)
    : Skill(name, SkillType::Passive, targetType, 0, 0.0f),
      buffName_(buffName), activated_(false) {
}

bool PassiveSkill::canActivate(std::shared_ptr<Unit> caster) const {
    if (!caster || !caster->isAlive()) return false;
    return !activated_;
}

void PassiveSkill::activate(std::shared_ptr<Unit> caster,
                            std::vector<std::shared_ptr<Unit>>& targets) {
    if (!canActivate(caster)) return;

    for (auto& target : targets) {
        if (target && target->isAlive()) {
            target->applyBuff(buffName_);
        }
    }

    activated_ = true;
}