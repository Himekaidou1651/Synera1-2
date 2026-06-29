/**
 * @file    Unit.h
 * @brief   单位基类，定义角色属性、装备、攻击范围与羁绊系统
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include <vector>
#include <string>
#include <set>
#include <memory>
#include <utility>

enum class Owner { PlayerCtrl, EnemyCtrl };
enum class UnitState { Idle, Ready, Attacking, Casting, Moving, Dead };
enum class AttackType { Normal, Ultimate };
using TraitSet = std::set<std::string>;
enum class Race { Ningen, Youkai };
enum class ClassType { Miko, Mahoushi, Maid, Journalist };

struct UnitStats {
    int hp;
    int atk;
    int maxMana;
    int mana;
};

/**
 * @brief 装备数据结构
 */
struct Equipment {
    int id;                  ///< 装备唯一ID
    std::string name;        ///< 装备名称
    int atkBonus;            ///< 攻击加成
    int hpBonus;             ///< 生命加成
    int type;                ///< 装备类型 0:武器, 1:防具, 2:饰品1, 3:饰品2
    std::string restriction; ///< 职业限制（空表示无限制）
    int starLevel;           ///< 品质星级
    int ownerUnitId;         ///< 所属单位ID（0=暂存未装备）
    std::string effect;      ///< 特殊效果 "lifesteal"/"crit"/"armor"/""
    int tag;                 ///< 装备编号 1-20, 0=无套装
    Equipment(int i = 0, const std::string& n = "", int a = 0, int h = 0, int t = 0, const std::string& r = "", int s = 0, int o = 0, const std::string& e = "", int tg = 0)
        : id(i), name(n), atkBonus(a), hpBonus(h), type(t), restriction(r), starLevel(s), ownerUnitId(o), effect(e), tag(tg) {}
};

/**
 * @class   Unit
 * @brief   单位基类，封装角色属性、装备槽位、攻击范围、羁绊等核心机制
 */
class Unit {
public:
    Unit(int hp, int atk, int maxMana, int mana, Owner owner,
         const std::string& unitType = "Unit",
         const std::string& normalAttackName = "普攻",
         const std::string& ultimateAttackName = "大招",
         const std::string& equipmentName = "装备",
         int starLevel = 1);
    virtual ~Unit() = default;

    // ========== 唯一不变 ID ==========
    int getUnitId() const { return unitId_; }
    static void resetUnitIdCounter() { nextUnitId_ = 1; }
    static int getNextUnitIdCounter() { return nextUnitId_; }
    static void setNextUnitIdCounter(int id) { nextUnitId_ = id; }

    int getHP() const;
    int getMaxHP() const;     ///< 获取最大生命值
    int getATK() const;
    int getMaxMana() const;
    int getMana() const;
    bool isAlive() const;
    bool canUseUltimate() const;

    const std::string& getUnitType() const;
    virtual std::string getSpriteName() const;
    const std::string& getNormalAttackName() const;
    const std::string& getUltimateAttackName() const;
    const std::string& getEquipmentName() const;
    const std::string& getAttackName(AttackType type) const;
    int getStarLevel() const;
    int getEquipmentUpgradeCost() const;

    // ========== 攻击范围系统（基于坐标偏移表格） ==========
    virtual const std::vector<std::pair<int,int>>& getNormalAttackRangeMap() const;   ///< 获取普攻攻击范围偏移表
    virtual const std::vector<std::pair<int,int>>& getUltimateAttackRangeMap() const; ///< 获取大招攻击范围偏移表
    const std::vector<std::pair<int,int>>& getAttackRangeMap(AttackType type) const;  ///< 根据攻击类型获取偏移表
    bool isPositionInAttackRange(int targetX, int targetY, AttackType type) const;    ///< 检查目标是否在攻击范围内
    std::vector<std::pair<int,int>> getAttackablePositions(AttackType type) const;    ///< 获取攻击范围内的所有棋盘坐标

    // ========== 旧版攻击范围兼容 ==========
    virtual int getNormalAttackRange() const { return 1; }
    virtual int getUltimateAttackRange() const { return 2; }
    int getAttackRange(AttackType type) const;

    void setUnitType(const std::string& typeName);
    void setNormalAttackName(const std::string& name);
    void setUltimateAttackName(const std::string& name);
    void setEquipmentName(const std::string& equipmentName);

    void takeDamage(int amount);
    void recoverMana(int amount);
    void setPosition(int x, int y);
    std::pair<int, int> getPosition() const;

    Owner getOwner() const;
    const TraitSet& getTraits() const;
    void addTrait(const std::string& trait);
    void clearTraits();
    
    // ========== 三合一升星 ==========
    bool levelUpStar(Unit* other1, Unit* other2);
    bool upgradeEquipment();

    virtual void onTurnStart();
    virtual void onAttack(std::shared_ptr<Unit> target);
    virtual void onAttack(std::shared_ptr<Unit> target, AttackType type);
    virtual void onUltimateAttack(std::shared_ptr<Unit> target);

    // ========== AOE 大招辅助 ==========
    void setCombatTargets(const std::vector<std::shared_ptr<Unit>>& targets) { combatTargets_ = targets; }
    int calcUltDamage() const;
    void dealUltDamage(std::shared_ptr<Unit> target);
    int getExtraActions() const { return extraActions_; }
    void consumeExtraAction() { if (extraActions_ > 0) extraActions_--; }
    virtual void onDeath();
    virtual void applyBuff(const std::string& buffName);
    virtual void removeBuff(const std::string& buffName);
    virtual std::string debugName() const;
    virtual std::string displayName() const;  ///< 仅返回名（不含姓），用于 UI 显示

    // ========== 装备 API ==========
    bool Equip(Equipment* eq, int slot);
    Equipment* Unequip(int slot);
    static Equipment* DropEquipment(int starLevel);
    static std::string EquipmentTypeToString(int type);
    static std::string ClassToString(ClassType c);

    // ========== 羁绊系统 ==========
    static std::vector<std::string> CheckTraits(const std::vector<Unit*>& units);

    // ========== 存档辅助 ==========
    static std::shared_ptr<Unit> CreateUnitByType(const std::string& unitType, Owner owner);
    void restoreFromSave(int hp, int maxHp, int atk, int maxMana, int mana, int starLevel, int unitId = -1);

protected:
    void recalculateStats();

    // 种族/职业到字符串的映射辅助
    static std::string RaceToString(Race r);

    int unitId_;       ///< 唯一不变ID，构造时自动分配，终生不变
    static int nextUnitId_;
    int hp_;
    int maxHp_;        ///< 最大生命值
    int atk_;
    int maxMana_;
    int mana_;
    int posX_;
    int posY_;
    Owner owner_;
    UnitState state_; 
    TraitSet traits_;

    std::set<std::string> activeBuffs_;
    int buffAtkOffset_;
    int buffArmor_;
    int buffManaRegen_;
    int buffLifesteal_;

    std::string unitType_;
    std::string normalAttackName_;
    std::string ultimateAttackName_;
    std::string equipmentName_;
    int starLevel_;
    int extraActions_ = 0;  ///< Sakuya 式：大招后额外行动次数

    // 种族与职业（羁绊系统用）
    Race race_;
    ClassType class_;

    // 原始基础属性（不可变基线）和可变基础属性（光环用）
    int originalBaseHp_;
    int originalBaseAtk_;
    int originalBaseMaxMana_;
    int baseHp_;
    int baseAtk_;
    int baseMaxMana_;
    int baseUltDamage_;   ///< 大招基础伤害（文档《角色数值》指定，受星级倍率缩放）

    // 羁绊机制标志
    bool hasBackstab_;

    // 装备槽位 0:武器, 1:防具, 2:饰品1, 3:饰品2
    Equipment* equipment_[4];

    std::vector<std::shared_ptr<Unit>> combatTargets_;  ///< AOE 大招：战斗中的敌方单位列表

    // ========== 攻击范围偏移表（相对于单位自身坐标） ==========
    // 派生类应在构造函数中填充这些向量
    std::vector<std::pair<int,int>> normalAttackRangeMap_;
    std::vector<std::pair<int,int>> ultimateAttackRangeMap_;

    // 初始化默认攻击范围（相邻4格普攻，曼哈顿距离2内大招）
    void initDefaultAttackRanges();

public:
    // ========== UI 访问器 ==========
    Equipment* GetEquipmentSlot(int slot) const;
    bool HasBackstab() const;
    Race GetRace() const;
    ClassType GetClassType() const;
};
