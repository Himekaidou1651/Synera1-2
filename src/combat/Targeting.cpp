/**
 * @file    Targeting.cpp
 * @brief   索敌系统实现
 * @author  
 * @date    2026-06-24
 */

#include "Targeting.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>

// ========== TargetingSystem ==========

TargetingSystem::TargetingSystem()
    : strategy_(TargetingStrategy::Nearest),
      range_(1), sightRange_(10) {
}

void TargetingSystem::setStrategy(TargetingStrategy strategy) {
    strategy_ = strategy;
    priorities_.clear();
}

void TargetingSystem::addPriority(const TargetingPriority& priority) {
    priorities_.push_back(priority);
}

void TargetingSystem::clearPriorities() {
    priorities_.clear();
}

void TargetingSystem::setRange(int range) {
    range_ = std::max(1, range);
}

int TargetingSystem::getRange() const {
    return range_;
}

void TargetingSystem::setSightRange(int range) {
    sightRange_ = std::max(1, range);
}

std::shared_ptr<Unit> TargetingSystem::findTarget(
    std::shared_ptr<Unit> source,
    const std::vector<std::shared_ptr<Unit>>& potentialTargets,
    const std::vector<std::shared_ptr<Unit>>& allies) {

    if (!source || !source->isAlive() || potentialTargets.empty()) {
        return nullptr;
    }

    // 使用自定义函数
    if (customFunc_ && strategy_ == TargetingStrategy::Custom) {
        return customFunc_(source, potentialTargets, allies);
    }

    // 使用优先级列表
    if (!priorities_.empty()) {
        auto targets = findTargets(source, potentialTargets, 1);
        if (!targets.empty()) {
            return targets[0].unit;
        }
        return nullptr;
    }

    // 使用默认策略评分
    auto [sx, sy] = source->getPosition();
    std::shared_ptr<Unit> bestTarget = nullptr;
    float bestScore = -std::numeric_limits<float>::max();

    for (const auto& target : potentialTargets) {
        if (!target || !target->isAlive()) continue;

        auto [tx, ty] = target->getPosition();
        float dist = distance(sx, sy, tx, ty);

        if (dist > sightRange_) continue;  // 超出视野范围

        float score = 0.0f;

        switch (strategy_) {
            case TargetingStrategy::Nearest:
                score = -dist;  // 距离越近分数越高
                break;

            case TargetingStrategy::LowestHP:
                score = -target->getHP();  // 生命值越低分数越高
                break;

            case TargetingStrategy::HighestHP:
                score = target->getHP();
                break;

            case TargetingStrategy::HighestATK:
                score = target->getATK();
                break;

            case TargetingStrategy::LowestATK:
                score = -target->getATK();
                break;

            case TargetingStrategy::HighestThreat:
                score = calculateThreatLevel(target);
                break;

            case TargetingStrategy::Random:
                score = static_cast<float>(std::rand());
                break;

            case TargetingStrategy::FrontRow:
                score = isFrontRow(target) ? 1000.0f : 0.0f;
                break;

            case TargetingStrategy::BackRow:
                score = isBackRow(target) ? 1000.0f : 0.0f;
                break;

            case TargetingStrategy::MostDangerous: {
                float threat = calculateThreatLevel(target);
                float hpRatio = static_cast<float>(target->getHP()) /
                               (target->getHP() + 1);
                score = threat * hpRatio;
                break;
            }

            default:
                score = -dist;
                break;
        }

        if (score > bestScore) {
            bestScore = score;
            bestTarget = target;
        }
    }

    return bestTarget;
}

std::vector<TargetInfo> TargetingSystem::findTargets(
    std::shared_ptr<Unit> source,
    const std::vector<std::shared_ptr<Unit>>& potentialTargets,
    int maxTargets) {

    std::vector<TargetInfo> results;
    if (!source || potentialTargets.empty()) return results;

    auto [sx, sy] = source->getPosition();

    for (const auto& target : potentialTargets) {
        if (!target || !target->isAlive()) continue;

        auto [tx, ty] = target->getPosition();
        float dist = distance(sx, sy, tx, ty);

        if (dist > sightRange_) continue;

        TargetInfo info;
        info.unit = target;
        info.distance = dist;
        info.score = 0.0f;
        info.row = tx;
        info.column = ty;
        info.inRange = dist <= range_;
        info.inSight = dist <= sightRange_;

        // 按优先级计算综合评分
        if (!priorities_.empty()) {
            for (const auto& priority : priorities_) {
                float score = calculateScore(source, target, priority);
                info.score += score * priority.weight;
            }
        } else {
            info.score = -dist;  // 默认按距离排序
        }

        // 应用过滤器链
        bool passAllFilters = true;
        for (const auto& priority : priorities_) {
            for (auto filter : priority.filters) {
                if (!applyFilter(source, target, filter)) {
                    passAllFilters = false;
                    break;
                }
            }
            if (!passAllFilters) break;
        }

        if (passAllFilters) {
            results.push_back(info);
        }
    }

    // 按评分降序排列
    std::sort(results.begin(), results.end(),
        [](const TargetInfo& a, const TargetInfo& b) {
            return a.score > b.score;
        });

    // 限制返回数量
    if (maxTargets > 0 && results.size() > static_cast<size_t>(maxTargets)) {
        results.resize(maxTargets);
    }

    return results;
}

std::vector<std::shared_ptr<Unit>> TargetingSystem::getTargetsInRange(
    std::shared_ptr<Unit> source,
    const std::vector<std::shared_ptr<Unit>>& targets) {

    std::vector<std::shared_ptr<Unit>> inRange;
    if (!source) return inRange;

    auto [sx, sy] = source->getPosition();

    for (const auto& target : targets) {
        if (target && target->isAlive() && isInRange(sx, sy, target->getPosition().first, target->getPosition().second)) {
            inRange.push_back(target);
        }
    }

    return inRange;
}

bool TargetingSystem::isInRange(std::shared_ptr<Unit> source, std::shared_ptr<Unit> target) const {
    if (!source || !target) return false;
    auto [sx, sy] = source->getPosition();
    auto [tx, ty] = target->getPosition();
    return isInRange(sx, sy, tx, ty);
}

bool TargetingSystem::isInRange(int sourceX, int sourceY, int targetX, int targetY) const {
    // 使用简单的距离检查作为fallback
    return distance(sourceX, sourceY, targetX, targetY) <= range_;
}

// 基于攻击范围偏移表检查目标是否在范围内
bool TargetingSystem::isInRange(std::shared_ptr<Unit> source, int targetX, int targetY, AttackType type) const {
    if (!source) return false;
    return source->isPositionInAttackRange(targetX, targetY, type);
}

// 获取在攻击范围内的目标（基于单位的攻击范围偏移表）
std::vector<std::shared_ptr<Unit>> TargetingSystem::getTargetsInAttackRange(
    std::shared_ptr<Unit> source,
    const std::vector<std::shared_ptr<Unit>>& targets,
    AttackType type) const {
    std::vector<std::shared_ptr<Unit>> inRange;
    if (!source) return inRange;

    for (const auto& target : targets) {
        if (target && target->isAlive()) {
            auto [tx, ty] = target->getPosition();
            if (source->isPositionInAttackRange(tx, ty, type)) {
                inRange.push_back(target);
            }
        }
    }
    return inRange;
}

void TargetingSystem::setCustomTargetingFunction(
    std::function<std::shared_ptr<Unit>(
        std::shared_ptr<Unit>,
        const std::vector<std::shared_ptr<Unit>>&,
        const std::vector<std::shared_ptr<Unit>>&)> func) {
    customFunc_ = func;
}

float TargetingSystem::calculateThreatLevel(std::shared_ptr<Unit> unit) {
    if (!unit || !unit->isAlive()) return 0.0f;

    float threat = static_cast<float>(unit->getATK());

    // 生命值比例影响威胁度
    float hpRatio = static_cast<float>(unit->getHP()) / (unit->getMaxMana() + 1);
    threat *= hpRatio;

    // 有大招时威胁度更高
    if (unit->canUseUltimate()) {
        threat *= 1.5f;
    }

    // 星级加成
    threat *= (1.0f + (unit->getStarLevel() - 1) * 0.3f);

    return threat;
}

float TargetingSystem::calculateDistanceScore(float distance, int range) {
    if (distance <= range) {
        return 100.0f;  // 在攻击范围内得分最高
    }
    return -distance;  // 距离越远得分越低
}

float TargetingSystem::calculateHPBasedScore(std::shared_ptr<Unit> unit, bool lowestHP) {
    if (!unit || !unit->isAlive()) return 0.0f;

    float hp = static_cast<float>(unit->getHP());
    float maxHp = static_cast<float>(unit->getMaxMana());

    if (lowestHP) {
        return -hp / maxHp;  // 生命值比例越低得分越高
    } else {
        return hp / maxHp;   // 生命值比例越高得分越高
    }
}

float TargetingSystem::distance(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return std::sqrt(static_cast<float>(dx * dx + dy * dy));
}

float TargetingSystem::manhattanDistance(int x1, int y1, int x2, int y2) {
    return static_cast<float>(std::abs(x1 - x2) + std::abs(y1 - y2));
}

bool TargetingSystem::isInSight(int sourceX, int sourceY, int targetX, int targetY, int sightRange) {
    return distance(sourceX, sourceY, targetX, targetY) <= sightRange;
}

float TargetingSystem::calculateScore(std::shared_ptr<Unit> source,
                                      std::shared_ptr<Unit> target,
                                      const TargetingPriority& priority) {
    if (!source || !target) return 0.0f;

    auto [sx, sy] = source->getPosition();
    auto [tx, ty] = target->getPosition();
    float dist = distance(sx, sy, tx, ty);
    float score = 0.0f;

    switch (priority.strategy) {
        case TargetingStrategy::Nearest:
            score = -dist;
            break;
        case TargetingStrategy::LowestHP:
            score = -static_cast<float>(target->getHP());
            break;
        case TargetingStrategy::HighestHP:
            score = static_cast<float>(target->getHP());
            break;
        case TargetingStrategy::HighestATK:
            score = static_cast<float>(target->getATK());
            break;
        case TargetingStrategy::LowestATK:
            score = -static_cast<float>(target->getATK());
            break;
        case TargetingStrategy::HighestThreat:
            score = calculateThreatLevel(target);
            break;
        case TargetingStrategy::FrontRow:
            score = isFrontRow(target) ? 10.0f : 0.0f;
            break;
        case TargetingStrategy::BackRow:
            score = isBackRow(target) ? 10.0f : 0.0f;
            break;
        default:
            score = -dist;
            break;
    }

    if (priority.reverseOrder) {
        score = -score;
    }

    return score;
}

bool TargetingSystem::applyFilter(std::shared_ptr<Unit> source,
                                  std::shared_ptr<Unit> target,
                                  TargetingFilter filter) {
    if (!source || !target) return false;

    switch (filter) {
        case TargetingFilter::None:
            return true;

        case TargetingFilter::Alive:
            return target->isAlive();

        case TargetingFilter::InRange: {
            auto [sx, sy] = source->getPosition();
            auto [tx, ty] = target->getPosition();
            return distance(sx, sy, tx, ty) <= range_;
        }

        case TargetingFilter::Visible: {
            auto [sx, sy] = source->getPosition();
            auto [tx, ty] = target->getPosition();
            return distance(sx, sy, tx, ty) <= sightRange_;
        }

        case TargetingFilter::SameRow: {
            auto [sx, sy] = source->getPosition();
            auto [tx, ty] = target->getPosition();
            return sx == tx;
        }

        case TargetingFilter::SameColumn: {
            auto [sx, sy] = source->getPosition();
            auto [tx, ty] = target->getPosition();
            return sy == ty;
        }

        default:
            return true;
    }
}

bool TargetingSystem::isFrontRow(std::shared_ptr<Unit> unit) const {
    if (!unit) return false;
    auto [x, y] = unit->getPosition();
    (void)y;
    // 前排：靠近敌方半场
    return x < BOARD_M / 2;
}

bool TargetingSystem::isBackRow(std::shared_ptr<Unit> unit) const {
    if (!unit) return false;
    auto [x, y] = unit->getPosition();
    (void)y;
    // 后排：靠近己方半场
    return x >= BOARD_M / 2;
}

// ========== TargetingManager ==========

TargetingSystem& TargetingManager::createTargetingSystem(std::shared_ptr<Unit> unit) {
    size_t id = nextId_++;
    auto result = systems_.emplace(id, TargetingSystem());
    result.first->second.setStrategy(defaultStrategy_);
    return result.first->second;
}

void TargetingManager::removeTargetingSystem(std::shared_ptr<Unit> unit) {
    for (auto it = systems_.begin(); it != systems_.end(); ) {
        it = systems_.erase(it);
    }
}

TargetingSystem* TargetingManager::getTargetingSystem(std::shared_ptr<Unit> unit) {
    for (auto& [id, system] : systems_) {
        (void)id;
        (void)system;
        // 简化实现
    }
    return nullptr;
}

void TargetingManager::setDefaultStrategy(TargetingStrategy strategy) {
    defaultStrategy_ = strategy;
}