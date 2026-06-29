/**
 * @file    Unit.cpp
 * @brief   单位基类实现
 * @author  
 * @date    2026-06-24
 */

#include "Unit.h"
#include "../units/UnitFactory.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <map>
#include <atomic>

// 6.2: 全局唯一ID计数器
int Unit::nextUnitId_ = 1;

Unit::Unit(int hp, int atk, int maxMana, int mana, Owner owner,
                     const std::string& unitType, const std::string& normalAttackName,
                     const std::string& ultimateAttackName, const std::string& equipmentName,
                     int starLevel)
        : unitId_(nextUnitId_++), hp_(hp), maxHp_(hp), atk_(atk), maxMana_(maxMana), mana_(mana),
            posX_(-1), posY_(-1), owner_(owner), state_(UnitState::Idle),
            buffAtkOffset_(0), buffArmor_(0), buffManaRegen_(0), buffLifesteal_(0),
            unitType_(unitType), normalAttackName_(normalAttackName),
            ultimateAttackName_(ultimateAttackName), equipmentName_(equipmentName),
            starLevel_(starLevel),
            originalBaseHp_(hp), originalBaseAtk_(atk), originalBaseMaxMana_(maxMana),
            baseHp_(hp), baseAtk_(atk), baseMaxMana_(maxMana), baseUltDamage_(0),
                        hasBackstab_(false) {
        for (int i = 0; i < 4; ++i) equipment_[i] = nullptr;
        initDefaultAttackRanges();
        recalculateStats();
}

int Unit::getHP() const { return hp_; }
int Unit::getMaxHP() const { return maxHp_; }
int Unit::getATK() const { return atk_; }
int Unit::getMaxMana() const { return maxMana_; }
int Unit::getMana() const { return mana_; }

bool Unit::isAlive() const { return hp_ > 0; }

void Unit::takeDamage(int amount) {
    // P3-2: 检查装备护甲效果
    int equipArmor = 0;
    for (int s = 0; s < 4; ++s) {
        if (equipment_[s] && equipment_[s]->effect == "armor") equipArmor += 2;
    }
    int effectiveArmor = buffArmor_ + equipArmor;
    int damage = std::max(0, amount - effectiveArmor);
    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        onDeath();
    }
}

void Unit::recoverMana(int amount) {
    mana_ = std::min(maxMana_, mana_ + amount);
}

bool Unit::canUseUltimate() const {
    return maxMana_ > 0 && mana_ >= maxMana_;
}

void Unit::setPosition(int x, int y) {
    posX_ = x;
    posY_ = y;
}

std::pair<int, int> Unit::getPosition() const {
    return {posX_, posY_};
}

Owner Unit::getOwner() const { return owner_; }

const TraitSet& Unit::getTraits() const { return traits_; }

void Unit::addTrait(const std::string& trait) {
    traits_.insert(trait);
}

void Unit::clearTraits() {
    traits_.clear();
}

void Unit::onTurnStart() {
    if (!isAlive()) { 
        return; 
    }
    recalculateStats();
    recoverMana(1 + buffManaRegen_);
    state_ = UnitState::Ready;
}

void Unit::onAttack(std::shared_ptr<Unit> target) {
    onAttack(target, AttackType::Normal);
}

void Unit::onAttack(std::shared_ptr<Unit> target, AttackType type) {
    if (!isAlive() || !target || !target->isAlive()) {
        return;
    }

    if (type == AttackType::Ultimate) {
        if (!canUseUltimate()) {
            type = AttackType::Normal;
        } else {
            mana_ = 0;  // 大招消耗全部法力
        }
    }

    // 状态转换交由状态机事件管理（调用方触发 StateEvent::AttackReady）
    
    // P3-2: 检查装备特效
    bool hasCrit = false;
    bool hasLifesteal = false;
    for (int s = 0; s < 4; ++s) {
        if (equipment_[s]) {
            if (equipment_[s]->effect == "crit") hasCrit = true;
            if (equipment_[s]->effect == "lifesteal") hasLifesteal = true;
        }
    }
    
    if (type == AttackType::Ultimate) {
        onUltimateAttack(target);
    } else {
        int damage = atk_;
        // 暴击20%几率双倍
        if (hasCrit && (rand() % 100 < 20)) damage *= 2;
        target->takeDamage(damage);
        recoverMana(25);  // 普攻回复25法力
        // 吸血：回复10%伤害
        if (hasLifesteal) {
            hp_ = std::min(maxHp_, hp_ + std::max(1, damage / 10));
        }
        if (buffLifesteal_ > 0) {
            recoverMana(buffLifesteal_ / 10);
        }
    }
}

void Unit::onUltimateAttack(std::shared_ptr<Unit> target) {
    dealUltDamage(target);
}

int Unit::calcUltDamage() const {
    double factor = 1.0;
    if (starLevel_ == 2) factor = 1.8;
    else if (starLevel_ >= 3) factor = 3.5;
    int equipAtk = 0;
    for (int i = 0; i < 4; ++i) {
        if (equipment_[i]) equipAtk += equipment_[i]->atkBonus;
    }
    return static_cast<int>(std::round(baseUltDamage_ * factor)) + buffAtkOffset_ + equipAtk;
}

void Unit::dealUltDamage(std::shared_ptr<Unit> target) {
    if (target && target->isAlive()) {
        target->takeDamage(calcUltDamage());
    }
}

void Unit::onDeath() {
    state_ = UnitState::Dead;
}

void Unit::applyBuff(const std::string& buffName) {
    if (buffName.empty()) {
        return;
    }
    activeBuffs_.insert(buffName);
    recalculateStats();
}

void Unit::removeBuff(const std::string& buffName) {
    activeBuffs_.erase(buffName);
    recalculateStats();
}

void Unit::recalculateStats() {
    // 重置简单 buff 偏移量
    buffAtkOffset_ = 0;
    buffArmor_ = 0;
    buffManaRegen_ = 0;
    buffLifesteal_ = 0;

    for (const auto& buff : activeBuffs_) {
        if (buff == "AttackUp") {
            buffAtkOffset_ += 2;
        } else if (buff == "Fortify") {
            buffArmor_ += 2;
        } else if (buff == "ManaFlow") {
            buffManaRegen_ += 1;
        } else if (buff == "Vampiric") {
            buffLifesteal_ += 20;
        }
    }

    // 计算星级倍率：等比叠加，1★ = 1.0, 2★ = 2.0, 3★ = 4.0
    double factor = 1.0;
    if (starLevel_ == 2) factor = 2.0;
    else if (starLevel_ >= 3) factor = 4.0;

    int equipAtk = 0;
    int equipHp = 0;
    for (int i = 0; i < 4; ++i) {
        if (equipment_[i]) {
            equipAtk += equipment_[i]->atkBonus;
            equipHp += equipment_[i]->hpBonus;
        }
    }

    // 保存旧的最大血量，用于按比例缩放当前血量
    int oldMaxHp = maxHp_;

    // 按星级倍率缩放基础属性，叠加装备和 buff 加成
    int newMaxHp = static_cast<int>(std::round(baseHp_ * factor)) + equipHp;
    int newAtk = static_cast<int>(std::round(baseAtk_ * factor)) + buffAtkOffset_ + equipAtk;
    int newMaxMana = static_cast<int>(std::round(baseMaxMana_ * factor));

    // 应用计算结果
    maxHp_ = newMaxHp;
    maxMana_ = newMaxMana;
    atk_ = newAtk;

    // 按比例缩放当前血量（装备变更时保持血量比例不变）
    if (oldMaxHp > 0 && maxHp_ != oldMaxHp && isAlive()) {
        hp_ = static_cast<int>(std::round(1.0 * hp_ * maxHp_ / oldMaxHp));
    }

    // 保持当前 HP 不超过新的最大值
    if (hp_ > maxHp_) hp_ = maxHp_;
    // 若 hp_ 为 0（已死亡）保持不变；存活单位保证 hp_ 至少为 1
    if (isAlive() && hp_ == 0) hp_ = std::min(1, maxHp_);
}

const std::string& Unit::getUnitType() const {
    return unitType_;
}

std::string Unit::getSpriteName() const {
    return unitType_;
}

const std::string& Unit::getNormalAttackName() const {
    return normalAttackName_;
}

const std::string& Unit::getUltimateAttackName() const {
    return ultimateAttackName_;
}

const std::string& Unit::getEquipmentName() const {
    return equipmentName_;
}

const std::string& Unit::getAttackName(AttackType type) const {
    return type == AttackType::Ultimate ? ultimateAttackName_ : normalAttackName_;
}

// === 攻击范围实现（基于坐标偏移表）===

void Unit::initDefaultAttackRanges() {
    // 默认普攻范围：相邻4格（上下左右）
    normalAttackRangeMap_.clear();
    normalAttackRangeMap_.push_back({-1, 0}); // 上
    normalAttackRangeMap_.push_back({1, 0});  // 下
    normalAttackRangeMap_.push_back({0, -1}); // 左
    normalAttackRangeMap_.push_back({0, 1});  // 右

    // 默认大招范围：曼哈顿距离 <= 2 的所有格子（不包括自身）
    ultimateAttackRangeMap_.clear();
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            if (dx == 0 && dy == 0) continue;
            if (std::abs(dx) + std::abs(dy) <= 2) {
                ultimateAttackRangeMap_.push_back({dx, dy});
            }
        }
    }
}

const std::vector<std::pair<int,int>>& Unit::getNormalAttackRangeMap() const {
    return normalAttackRangeMap_;
}

const std::vector<std::pair<int,int>>& Unit::getUltimateAttackRangeMap() const {
    return ultimateAttackRangeMap_;
}

const std::vector<std::pair<int,int>>& Unit::getAttackRangeMap(AttackType type) const {
    return type == AttackType::Ultimate ? getUltimateAttackRangeMap() : getNormalAttackRangeMap();
}

bool Unit::isPositionInAttackRange(int targetX, int targetY, AttackType type) const {
    const auto& rangeMap = getAttackRangeMap(type);
    for (const auto& offset : rangeMap) {
        int checkX = posX_ + offset.first;
        int checkY = posY_ + offset.second;
        if (checkX == targetX && checkY == targetY) {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<int,int>> Unit::getAttackablePositions(AttackType type) const {
    const auto& rangeMap = getAttackRangeMap(type);
    std::vector<std::pair<int,int>> positions;
    for (const auto& offset : rangeMap) {
        int absX = posX_ + offset.first;
        int absY = posY_ + offset.second;
        // 确保在棋盘范围内
        if (absX >= 0 && absX < BOARD_M && absY >= 0 && absY < BOARD_N) {
            positions.push_back({absX, absY});
        }
    }
    return positions;
}

// === 旧版攻击范围兼容实现（定义已移至头文件内联）===

int Unit::getAttackRange(AttackType type) const {
    return type == AttackType::Ultimate ? getUltimateAttackRange() : getNormalAttackRange();
}

int Unit::getStarLevel() const {
    return starLevel_;
}

int Unit::getEquipmentUpgradeCost() const {
    return 0;
}

void Unit::setUnitType(const std::string& typeName) {
    unitType_ = typeName;
}

void Unit::setNormalAttackName(const std::string& name) {
    normalAttackName_ = name;
}

void Unit::setUltimateAttackName(const std::string& name) {
    ultimateAttackName_ = name;
}

void Unit::setEquipmentName(const std::string& equipmentName) {
    equipmentName_ = equipmentName;
}

bool Unit::levelUpStar(Unit* other1, Unit* other2) {
    if (!other1 || !other2) return false;
    // 要求三单位同类型且同星级
    if (other1->unitType_ != unitType_ || other2->unitType_ != unitType_) return false;
    if (other1->starLevel_ != starLevel_ || other2->starLevel_ != starLevel_) return false;
    if (starLevel_ >= 3) return false; // 已达最高星级

    // 升星
    starLevel_ += 1;

    // 合并装备：保留最高星级的装备，星级相同时按 this → other1 → other2 优先级
    for (int i = 0; i < 4; ++i) {
        Equipment* chosen = nullptr;
        int bestStar = -1;
        // 检查自身装备
        if (equipment_[i]) { chosen = equipment_[i]; bestStar = equipment_[i]->starLevel; }
        // 检查 other1 装备
        if (other1->equipment_[i]) {
            int s = other1->equipment_[i]->starLevel;
            if (s > bestStar) { chosen = other1->equipment_[i]; bestStar = s; }
        }
        // 检查 other2 装备
        if (other2->equipment_[i]) {
            int s = other2->equipment_[i]->starLevel;
            if (s > bestStar) { chosen = other2->equipment_[i]; bestStar = s; }
        }

        // 分配选中的装备；若取自捐赠者则从其身上清除
        if (chosen != equipment_[i]) {
            // 若选中的装备来自 other1/other2，则从原持有者分离
            if (other1->equipment_[i] == chosen) other1->equipment_[i] = nullptr;
            if (other2->equipment_[i] == chosen) other2->equipment_[i] = nullptr;
            equipment_[i] = chosen;
        }
    }

    // 合并基础属性：保持自身原始值不变，重新计算
    recalculateStats();
    // 确保法力值不超出上限
    mana_ = std::min(mana_, maxMana_);
    return true;
}

// ========== 装备 API ==========
bool Unit::Equip(Equipment* eq, int slot) {
    if (!eq) return false;
    if (slot < 0 || slot >= 4) return false;
    // 职业限制检查
    if (!eq->restriction.empty()) {
        if (eq->restriction != ClassToString(class_)) {
            return false;
        }
    }
    if (equipment_[slot]) return false; // 槽位已被占用
    // 饰品(type 2/3)可进槽位2或3，type跟随实际槽位
    if ((eq->type == 2 || eq->type == 3) && (slot == 2 || slot == 3)) {
        eq->type = slot;
    }
    equipment_[slot] = eq;
    recalculateStats();
    return true;
}

Equipment* Unit::Unequip(int slot) {
    if (slot < 0 || slot >= 4) return nullptr;
    Equipment* prev = equipment_[slot];
    equipment_[slot] = nullptr;
    recalculateStats();
    return prev;
}

Equipment* Unit::DropEquipment(int starLevel) {
    static std::mt19937_64 rng((unsigned)std::random_device{}());

    // 从模板池中筛选符合条件的装备
    auto templates = GetEquipmentTemplatesByStar(std::max(1, starLevel));
    if (templates.empty()) {
        // 回退：如果模板池为空，生成一个基础装备
        Equipment* e = new Equipment(static_cast<int>(rng()), "装备★1", 1, 2, 0, "", 1);
        return e;
    }

    // 随机选取一个模板
    std::uniform_int_distribution<size_t> templateDist(0, templates.size() - 1);
    const auto& tmpl = templates[templateDist(rng)];

    // 在模板的属性范围内随机生成具体数值
    std::uniform_int_distribution<int> atkDist(tmpl.atkMin, tmpl.atkMax);
    std::uniform_int_distribution<int> hpDist(tmpl.hpMin, tmpl.hpMax);
    int atk = atkDist(rng);
    int hp = hpDist(rng);

    int id = static_cast<int>(rng());
    Equipment* e = new Equipment(id, tmpl.name, atk, hp, tmpl.type, tmpl.restriction, tmpl.starLevel, 0, tmpl.effect, tmpl.tag);
    return e;
}

std::string Unit::EquipmentTypeToString(int type) {
    switch (type) {
        case 0: return "武器";
        case 1: return "防具";
        case 2: return "饰品1";
        case 3: return "饰品2";
        default: return "未知装备";
    }
}

// ========== 辅助函数 ==========
std::string Unit::ClassToString(ClassType c) {
    switch (c) {
        case ClassType::Miko: return "Miko";
        case ClassType::Mahoushi: return "Mahoushi";
        case ClassType::Maid: return "Maid";
        case ClassType::Journalist: return "Journalist";
        default: return "";
    }
}

std::string Unit::RaceToString(Race r) {
    switch (r) {
        case Race::Ningen: return "Ningen";
        case Race::Youkai: return "Youkai";
        default: return "";
    }
}

std::vector<std::string> Unit::CheckTraits(const std::vector<Unit*>& units) {
    std::vector<std::string> activated;
    if (units.empty()) return activated;

    // 重置基础属性和标志位
    for (auto u : units) {
        if (!u) continue;
        u->baseAtk_ = u->originalBaseAtk_;
        u->baseHp_ = u->originalBaseHp_;
        u->baseMaxMana_ = u->originalBaseMaxMana_;
        u->hasBackstab_ = false;
    }

    // 按种族/职业统计数量
    std::map<Race, int> raceCount;
    std::map<ClassType, int> classCount;
    for (auto u : units) {
        if (!u) continue;
        raceCount[u->GetRace()]++;
        classCount[u->GetClassType()]++;
    }

        // 种族羁绊 (根据 Unit.h 中的 Race 枚举：Ningen / Youkai)
    if (raceCount[Race::Ningen] >= 2) {
        int cnt = raceCount[Race::Ningen];
        double factor = cnt >= 4 ? 1.2 : 1.1;
        int affectedCount = 0;
        for (auto u : units) if (u && u->GetRace() == Race::Ningen) {
            u->baseAtk_ = static_cast<int>(std::round(u->originalBaseAtk_ * factor));
            affectedCount++;
        }
        int percent = static_cast<int>((factor - 1.0) * 100);
        std::ostringstream ss;
        ss << "【人】(" << cnt << ")";
        if (cnt >= 4) ss << " ★第2阶";
        else ss << " ☆第1阶";
        ss << "\n  效果: 人单位攻击力 +" << percent << "%";
        ss << "\n  受影响的单位: " << affectedCount << "个";
        activated.push_back(ss.str());
    }

    if (raceCount[Race::Youkai] >= 2) {
        int cnt = raceCount[Race::Youkai];
        double factor = cnt >= 4 ? 1.2 : 1.1;
        int affectedCount = 0;
        for (auto u : units) if (u && u->GetRace() == Race::Youkai) {
            u->baseHp_ = static_cast<int>(std::round(u->originalBaseHp_ * factor));
            affectedCount++;
        }
        int percent = static_cast<int>((factor - 1.0) * 100);
        std::ostringstream ss;
        ss << "【妖】(" << cnt << ")";
        if (cnt >= 4) ss << " ★第2阶";
        else ss << " ☆第1阶";
        ss << "\n  效果: 妖单位生命 +" << percent << "%";
        ss << "\n  受影响的单位: " << affectedCount << "个";
        activated.push_back(ss.str());
    }

    // 职业羁绊 (根据 Unit.h 中的 ClassType 枚举)
    if (classCount[ClassType::Miko] >= 2) {
        int cnt = classCount[ClassType::Miko];
        int affectedCount = 0;
        for (auto u : units) if (u && u->GetClassType() == ClassType::Miko) {
            u->baseMaxMana_ = u->originalBaseMaxMana_ - 25;
            affectedCount++;
        }
        std::ostringstream ss;
        ss << "【巫女】(" << cnt << ")";
        ss << "\n  效果: 巫女 最大法力 -25";
        ss << "\n  受影响的单位: " << affectedCount << "个";
        activated.push_back(ss.str());
    }

    if (classCount[ClassType::Mahoushi] >= 2) {
        int cnt = classCount[ClassType::Mahoushi];
        int affectedCount = 0;
        for (auto u : units) if (u && u->GetClassType() == ClassType::Mahoushi) {
            u->baseAtk_ = static_cast<int>(std::round(u->originalBaseAtk_ * 1.16));
            affectedCount++;
        }
        std::ostringstream ss;
        ss << "【魔法使】(" << cnt << ")";
        ss << "\n  效果: 魔法使 攻击力 +16%";
        ss << "\n  受影响的单位: " << affectedCount << "个";
        activated.push_back(ss.str());
    }

    if (classCount[ClassType::Maid] >= 2) {
        int cnt = classCount[ClassType::Maid];
        int affectedCount = 0;
        for (auto u : units) if (u && u->GetClassType() == ClassType::Maid) {
            u->baseHp_ = static_cast<int>(std::round(u->originalBaseHp_ * 1.12));
            affectedCount++;
        }
        std::ostringstream ss;
        ss << "【女仆】(" << cnt << ")";
        ss << "\n  效果: 女仆 生命 +12%";
        ss << "\n  受影响的单位: " << affectedCount << "个";
        activated.push_back(ss.str());
    }

    if (classCount[ClassType::Journalist] >= 2) {
        int cnt = classCount[ClassType::Journalist];
        int affectedCount = 0;
        for (auto u : units) if (u && u->GetClassType() == ClassType::Journalist) {
            u->baseAtk_ = static_cast<int>(std::round(u->originalBaseAtk_ * 1.24));
            affectedCount++;
        }
        std::ostringstream ss;
        ss << "【记者】(" << cnt << ")";
        ss << "\n  效果: 记者 攻击力 +24%";
        ss << "\n  受影响的单位: " << affectedCount << "个";
        activated.push_back(ss.str());
    }

    // 光环计算后重新应用属性
    for (auto u : units) if (u) u->recalculateStats();

    // P3-1: 装备套装效果（recalculateStats 之后直接改最终属性）
    // 每件装备有唯一 tag(1-20)，套装由 tag 组合精准匹配
    static const std::vector<std::tuple<std::set<int>, int, int, const char*>> setDefs = {
        {{5,9},      3, 3, "神道(2件) ATK+3"},
        {{5,9,13},   5, 8, "神道(3件) ATK+5 HP+8"},
        {{6,10},     3, 3, "魔法(2件) ATK+3"},
        {{6,10,14},  5, 8, "魔法(3件) ATK+5 HP+8"},
        {{7,12},     0, 5, "庭师(2件) HP+5"},
        {{8,11},     8, 2, "天狗(2件) ATK+8 HP+2"},
    };
    
    for (auto u : units) {
        if (!u) continue;
        std::set<int> tags;
        for (int s = 0; s < 4; ++s) {
            auto* eq = u->GetEquipmentSlot(s);
            if (eq && eq->tag > 0) tags.insert(eq->tag);
        }
        if (tags.size() < 2) continue;
        for (const auto& [required, atk, hp, desc] : setDefs) {
            bool match = true;
            for (int t : required) if (!tags.count(t)) { match = false; break; }
            if (match) {
                u->atk_ += atk;
                u->maxHp_ += hp;
                u->hp_ = std::min(u->hp_ + hp, u->maxHp_);
                std::ostringstream ss;
                ss << "【" << desc << "】— " << u->displayName();
                activated.push_back(ss.str());
            }
        }
    }

    return activated;
}

// ========== 访问器 ==========
Equipment* Unit::GetEquipmentSlot(int slot) const {
    if (slot < 0 || slot >= 4) return nullptr;
    return equipment_[slot];
}

bool Unit::HasBackstab() const {
    return hasBackstab_;
}

Race Unit::GetRace() const { return race_; }

ClassType Unit::GetClassType() const { return class_; }

bool Unit::upgradeEquipment() {
    recalculateStats();
    return true;
}

std::string Unit::debugName() const {
    return unitType_ + "★" + std::to_string(starLevel_);
}

std::string Unit::displayName() const {
    return unitType_;
}

void Unit::restoreFromSave(int hp, int maxHp, int atk, int maxMana, int mana, int starLevel, int unitId) {
    if (unitId >= 0) {
        unitId_ = unitId;
        // 确保 nextUnitId_ 大于已恢复的ID，避免碰撞
        if (unitId >= nextUnitId_) nextUnitId_ = unitId + 1;
    }
    hp_ = hp;
    maxHp_ = maxHp;
    atk_ = atk;
    maxMana_ = maxMana;
    mana_ = mana;
    starLevel_ = starLevel;
}

std::shared_ptr<Unit> Unit::CreateUnitByType(const std::string& unitType, Owner owner) {
    return UnitFactory::createUnitByType(unitType, owner);
}
