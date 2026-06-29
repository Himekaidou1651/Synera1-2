/**
 * @file    Shop.h
 * @brief   商店核心逻辑，提供单位购买、刷新、装备掉落等玩家与敌方共用功能
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"
#include "Player.h"

/**
 * @brief 商店装备槽位
 */
struct ShopEquipmentSlot {
    Equipment eq;   ///< 装备数据
    int price;      ///< 价格
    bool sold;      ///< 是否已售出
};

/**
 * @class   Shop
 * @brief   商店系统，提供单位购买、刷新、装备掉落与合成等功能
 *
 * 玩家和敌方共用同一套逻辑，仅通过 owner 参数区分归属。
 */
class Shop {
public:
    Shop();

    /**
     * @brief  刷新商店，生成新的可购买单位列表和装备
     * @param  owner  购买单位的归属（PlayerCtrl 或 EnemyCtrl）
     * @param  round  当前轮次，影响单位质量
     * @param  buyer  买方玩家引用
     * @param  cost   刷新消耗的金币（默认1）
     * @return 刷新是否成功（金币不足返回 false）
     */
    bool refresh(Owner owner, int round, Player& buyer, int cost = 1);

    /**
     * @brief  购买指定索引的单位
     * @param  index 商店槽位索引
     * @param  buyer 买方玩家引用
     * @param  cost  购买消耗的金币（默认2）
     * @return 购买是否成功
     */
    bool buyUnit(int index, Player& buyer, int cost = 2);

    /**
     * @brief  购买指定索引的装备
     * @param  index 装备槽位索引
     * @param  buyer 买方玩家引用
     * @return 购买是否成功
     */
    bool buyEquipment(int index, Player& buyer);

    std::vector<std::shared_ptr<Unit>>& getUnits();
    const std::vector<std::shared_ptr<Unit>>& getUnits() const;

    std::vector<ShopEquipmentSlot>& getEquipmentSlots();
    const std::vector<ShopEquipmentSlot>& getEquipmentSlots() const;

    int getSlotCount() const;
    int getEquipSlotCount() const;

    /**
     * @brief  掉落装备（根据星级随机生成）
     * @param  starLevel 星级
     * @return 新分配的 Equipment 指针（调用者负责释放）
     */
    static Equipment* dropEquipment(int starLevel);

    /**
     * @brief 装备应用结果
     */
    struct ApplyEquipmentResult {
        bool success;           ///< 是否成功
        int benchIndex;         ///< 装备到的备战区槽位索引
        std::string message;    ///< 结果描述信息
    };

    /**
     * @brief  为指定玩家的备战区单位应用装备
     * @param  droppedEquipment 要装备的装备（传入 unique_ptr，成功后被释放）
     * @param  player           目标玩家
     * @return 装备应用结果
     */
    static ApplyEquipmentResult applyEquipment(std::unique_ptr<Equipment>& droppedEquipment, Player& player);

    /**
     * @brief  自动三合一合成（在指定玩家的备战区中查找可合成的单位）
     * @param  player  目标玩家
     * @param  message 合成结果描述（输出）
     * @return 是否成功合成
     */
    static bool autoCombine(Player& player, std::string& message);

    /**
     * @brief  升级人口限制
     * @param  player 目标玩家
     * @param  cost   升级消耗的金币（默认5）
     * @return 是否升级成功
     */
    static bool upgradePopulation(Player& player, int cost = 5);

    /**
     * @brief  装备合成（3件同星同类型 → 1件高1星）
     * @param  player  目标玩家
     * @param  eqId1   装备ID1
     * @param  eqId2   装备ID2
     * @param  eqId3   装备ID3
     * @param  message 合成结果描述（输出）
     * @return 是否合成成功
     */
    static bool combineEquipment(Player& player, int eqId1, int eqId2, int eqId3, std::string& message);

    /**
     * @brief  售卖装备
     * @param  player 目标玩家
     * @param  eqId   装备ID
     * @return 售卖获得的金币
     */
    static int sellEquipment(Player& player, int eqId);

    /**
     * @brief  为指定玩家生成初始商店（新游戏时调用）
     * @param  owner 归属方
     * @param  round 当前轮次
     */
    void initializeForPlayer(Owner owner, int round);

    /**
     * @brief  重置商店状态
     */
    void reset();

private:
    static constexpr int SHOP_SLOT_COUNT = 5;        ///< 单位槽位数量
    static constexpr int EQUIP_SHOP_SLOT_COUNT = 3;  ///< 装备槽位数量
    std::vector<std::shared_ptr<Unit>> units_;
    std::vector<ShopEquipmentSlot> equipmentSlots_;
    std::mt19937 rng_;

    /**
     * @brief  创建随机单位（用于商店刷新）
     * @param  owner     归属方
     * @param  slotIndex 槽位索引
     * @param  round     当前轮次
     * @return 创建的单位
     */
    std::shared_ptr<Unit> createRandomUnit(Owner owner, int slotIndex, int round);
};