/**
 * @file    Bench.cpp
 * @brief   备战区系统实现
 * @author  
 * @date    2026-06-24
 */

#include "Bench.h"

Bench::Bench() : slots_(BENCH_SIZE, nullptr) {
}

bool Bench::isValidPosition(int index) const {
    return index >= 0 && index < BENCH_SIZE;
}

bool Bench::addUnit(std::shared_ptr<Unit> unit) {
    // 找到首个空槽位填入；若已满则返回 false
    if (!unit) {
        return false;
    }
    for (auto& slot : slots_) {
        if (!slot) {
            slot = std::move(unit);
            return true;
        }
    }
    return false;
}

bool Bench::removeUnit(int index) {
    // 验证索引有效后清空对应槽位
    if (!isValidPosition(index)) {
        return false;
    }
    slots_[index] = nullptr;
    return true;
}

bool Bench::swapUnits(int indexA, int indexB) {
    // 交换两个备战区槽位中的单位
    if ((!isValidPosition(indexA)) || (!isValidPosition(indexB))) {
        return false;
    }
    if (indexA == indexB) {
        return true;
    }
    std::swap(slots_[indexA], slots_[indexB]);
    return true;
}

bool Bench::moveUnit(int fromIndex, int toIndex) {
    // 从源槽位移动到目标槽位，目标被占用时返回 false
    if ((!isValidPosition(fromIndex)) || (!isValidPosition(toIndex))) {
        return false;
    }
    if (slots_[toIndex]) {
        return false;
    }
    if (!slots_[fromIndex]) {
        return false;
    }
    slots_[toIndex] = std::move(slots_[fromIndex]);
    slots_[fromIndex] = nullptr;
    return true;
}

bool Bench::setUnit(int index, std::shared_ptr<Unit> unit) {
    if (!isValidPosition(index) || !unit || slots_[index]) {
        return false;
    }
    slots_[index] = std::move(unit);
    return true;
}

bool Bench::combineUnits(int indexA, int indexB, int indexC) {
    if (indexA == indexB || indexB == indexC || indexA == indexC) {
        return false;
    }
    if (!isValidPosition(indexA) || !isValidPosition(indexB) || !isValidPosition(indexC)) {
        return false;
    }
    auto unitA = slots_[indexA];
    auto unitB = slots_[indexB];
    auto unitC = slots_[indexC];
    if (!unitA || !unitB || !unitC) {
        return false;
    }
    if (unitA->getUnitType() != unitB->getUnitType() || unitA->getUnitType() != unitC->getUnitType()) {
        return false;
    }
    if (unitA->getStarLevel() != unitB->getStarLevel() || unitA->getStarLevel() != unitC->getStarLevel()) {
        return false;
    }

    unitA->levelUpStar(unitB.get(), unitC.get());
    slots_[indexB] = nullptr;
    slots_[indexC] = nullptr;
    return true;
}

std::shared_ptr<Unit> Bench::getUnit(int index) const {
    // 返回指定槽位中的单位，无效索引或空槽位返回 nullptr
    if (!isValidPosition(index)) {
        return nullptr;
    }
    return slots_[index];
}

bool Bench::isFull() const {
    // 检查备战区是否所有槽位都已占用
    for (auto a : slots_) {
        if (!a)
            return false;
    }
    return true;
}

void Bench::clear() {
    // 清空所有备战区槽位
    for (auto a : slots_) {
        a = nullptr;
    }
}
