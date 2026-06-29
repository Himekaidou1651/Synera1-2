/**
 * @file    Bench.h
 * @brief   备战区系统，管理棋盘外的单位暂存槽位
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Unit.h"

/**
 * @class   Bench
 * @brief   备战区，提供单位暂存、交换、移动与三合一合成操作
 */
class Bench {
public:
    Bench();

    /**
     * @brief  检查索引是否在有效范围内
     * @param  index 备战区槽位索引
     * @return 是否有效
     */
    bool isValidPosition(int index) const;

    /**
     * @brief  向备战区添加单位（找到首个空槽位填入）
     * @param  unit 要添加的单位
     * @return 是否添加成功（已满或无单位则返回 false）
     */
    bool addUnit(std::shared_ptr<Unit> unit);

    /**
     * @brief  移除指定槽位的单位
     * @param  index 备战区槽位索引
     * @return 是否移除成功
     */
    bool removeUnit(int index);

    /**
     * @brief  交换两个备战区槽位中的单位
     * @param  indexA 槽位A
     * @param  indexB 槽位B
     * @return 是否交换成功
     */
    bool swapUnits(int indexA, int indexB);

    /**
     * @brief  将单位从源槽位移动到目标槽位
     * @param  fromIndex 源槽位索引
     * @param  toIndex   目标槽位索引
     * @return 是否移动成功（目标有单位时返回 false）
     */
    bool moveUnit(int fromIndex, int toIndex);

    /**
     * @brief  设置指定槽位的单位（覆盖写入）
     * @param  index 槽位索引
     * @param  unit  要设置的单位
     * @return 是否设置成功
     */
    bool setUnit(int index, std::shared_ptr<Unit> unit);

    /**
     * @brief  获取指定槽位的单位
     * @param  index 槽位索引
     * @return 单位指针（无单位时返回 nullptr）
     */
    std::shared_ptr<Unit> getUnit(int index) const;

    /**
     * @brief  三合一合成：消耗 B 和 C 槽位单位为 A 单位升星
     * @param  indexA 主体单位槽位
     * @param  indexB 材料1槽位
     * @param  indexC 材料2槽位
     * @return 是否合成成功
     */
    bool combineUnits(int indexA, int indexB, int indexC);

    /**
     * @brief  检查备战区是否已满
     * @return 是否所有槽位都有单位
     */
    bool isFull() const;

    /**
     * @brief  清空所有备战区槽位
     */
    void clear();

private:
    std::vector<std::shared_ptr<Unit>> slots_;  ///< 备战区槽位数组
};
