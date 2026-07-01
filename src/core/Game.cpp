/**
 * @file    Game.cpp
 * @brief   游戏主循环与阶段管理实现
 * @author  
 * @date    2026-06-24
 */

#include "Game.h"
#include "SaveManager.h"
#include "../combat/Pathfinding.h"
#include "../units/UnitFactory.h"
#include "../nlohmann/json.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <random>
#include <limits>

Game::Game()
    : currentRound_(1), maxRounds_(100), inCombatPhase_(false), gameOver_(false),
      playerWonGame_(false), winStreak_(0), lossStreak_(0) {
}

void Game::initialize() {
    currentRound_ = 1;
    gameOver_ = false;
    playerWonGame_ = false;
    inCombatPhase_ = false;
    winStreak_ = 0;
    lossStreak_ = 0;
    battleLog_.clear();
    board_.clear();
    enemyUnits_.clear();

    // Unit ID计数器
    Unit::resetUnitIdCounter();

    // 重建 Player，彻底重置等级/人口/经验/阶段
    player_ = Player();
    player_.setGold(96);
    player_.setHealth(100);
    player_.setLevel(1);
    player_.setPopulationLevel(1);

    // 重建敌方玩家
    enemyPlayer_ = Player();
    enemyPlayer_.setGold(96);
    enemyPlayer_.setHealth(100);
    enemyPlayer_.setLevel(1);
    enemyPlayer_.setPopulationLevel(1);

    // 初始化双方商店
    shop_.reset();
    enemyShop_.reset();
    shop_.initializeForPlayer(Owner::PlayerCtrl, currentRound_);
    enemyShop_.initializeForPlayer(Owner::EnemyCtrl, currentRound_);
}

void Game::resetGame() {
    currentRound_ = 1;
    gameOver_ = false;
    playerWonGame_ = false;
    inCombatPhase_ = false;
    winStreak_ = 0;
    lossStreak_ = 0;
    enemyUnits_.clear();
    board_.clear();

    // 重置Unit ID计数器
    Unit::resetUnitIdCounter();

    player_ = Player();
    player_.setGold(96);
    player_.setHealth(100);
    player_.setLevel(1);
    player_.setPopulationLevel(1);

    enemyPlayer_ = Player();
    enemyPlayer_.setGold(96);
    enemyPlayer_.setHealth(100);
    enemyPlayer_.setLevel(1);
    enemyPlayer_.setPopulationLevel(1);

    shop_.reset();
    enemyShop_.reset();
    shop_.initializeForPlayer(Owner::PlayerCtrl, 1);
    enemyShop_.initializeForPlayer(Owner::EnemyCtrl, 1);
}

void Game::startPhaseOne() {
    inCombatPhase_ = false;
    // 利息已移至 resolveBattle 发放，此处仅记录基准线
    goldAtRoundStart_ = player_.getGold();
    player_.prepareRecruitment();
}

void Game::dragUnitFromBenchToBoard(int benchIndex, int boardX, int boardY) {
    // 处理从备战区拖拽单位到棋盘的交互，含非法放置处理和交换逻辑
    if (inCombatPhase_ || !board_.isValidPosition(boardX, boardY) || !player_.getBench().isValidPosition(benchIndex)) {
        handleIllegalPlacement(benchIndex, -1, boardX, boardY);
        return;
    }

    if (!board_.isPlayerHalf(boardX, boardY)) {
        handleIllegalPlacement(benchIndex, -1, boardX, boardY);
        return;
    }

    auto &bench = player_.getBench();
    auto unit = bench.getUnit(benchIndex);
    if (!unit) {
        return;
    }

    auto existingBoardUnit = board_.getUnit(boardX, boardY);
    bench.removeUnit(benchIndex);

    if (!existingBoardUnit) {
        if (!board_.placeUnit(unit, boardX, boardY)) {
            bench.setUnit(benchIndex, unit);
        }
        return;
    }

    // 如果目标格已有单位，则与它交换位置
    if (!board_.removeUnit(boardX, boardY) || !board_.placeUnit(unit, boardX, boardY)) {
        bench.setUnit(benchIndex, unit);
        return;
    }

    bench.setUnit(benchIndex, existingBoardUnit);
}

void Game::dragUnitOnBoard(int fromX, int fromY, int toX, int toY) {
    // 处理棋盘内单位交换、移动或不可放置时回弹，考虑目标格子是否可站位
    if (inCombatPhase_ || !board_.isValidPosition(fromX, fromY) || !board_.isValidPosition(toX, toY)) {
        handleIllegalPlacement(fromX, fromY, toX, toY);
        return;
    }
    if (fromX == toX && fromY == toY) {
        return;
    }

    auto sourceUnit = board_.getUnit(fromX, fromY);
    if (!sourceUnit || sourceUnit->getOwner() != Owner::PlayerCtrl) {
        return;
    }
    if (!board_.isPlayerHalf(toX, toY)) {
        return;
    }

    auto targetUnit = board_.getUnit(toX, toY);
    if (!targetUnit) {
        board_.moveUnit(fromX, fromY, toX, toY);
        return;
    }

    // 允许交换位置
    board_.removeUnit(fromX, fromY);
    board_.removeUnit(toX, toY);
    if (board_.placeUnit(sourceUnit, toX, toY) && board_.placeUnit(targetUnit, fromX, fromY)) {
        return;
    }

    board_.removeUnit(toX, toY);
    board_.removeUnit(fromX, fromY);
    board_.placeUnit(sourceUnit, fromX, fromY);
    board_.placeUnit(targetUnit, toX, toY);
}

bool Game::moveBenchUnit(int fromIndex, int toIndex) {
    return player_.getBench().moveUnit(fromIndex, toIndex);
}

bool Game::swapBenchUnits(int fromIndex, int toIndex) {
    return player_.getBench().swapUnits(fromIndex, toIndex);
}

bool Game::moveBoardToBench(int boardX, int boardY, int benchIndex) {
    if (inCombatPhase_ || !board_.isValidPosition(boardX, boardY) || !player_.getBench().isValidPosition(benchIndex)) {
        return false;
    }

    auto unit = board_.getUnit(boardX, boardY);
    if (!unit || unit->getOwner() != Owner::PlayerCtrl) {
        return false;
    }

    auto &bench = player_.getBench();
    auto benchUnit = bench.getUnit(benchIndex);
    board_.removeUnit(boardX, boardY);

    if (!benchUnit) {
        bench.setUnit(benchIndex, unit);
        return true;
    }

    // 如果目标备战区槽位已有单位，则与之交换
    if (!bench.removeUnit(benchIndex)) {
        board_.placeUnit(unit, boardX, boardY);
        return false;
    }
    bench.setUnit(benchIndex, unit);
    board_.placeUnit(benchUnit, boardX, boardY);
    return true;
}

void Game::handleIllegalPlacement(int fromX, int fromY, int toX, int toY) {
    (void)fromX;
    (void)fromY;
    (void)toX;
    (void)toY;
}

bool Game::upgradePopulationLimit() {
    return player_.upgradePopulationLimit();
}

bool Game::saveGame(const std::string &filename) const {
    return SaveManager::saveGame(*this, filename);
}

bool Game::loadGame(const std::string &filename) {
    return SaveManager::loadGame(*this, filename);
}

void Game::spawnEnemyWave(int round) {
    // 死单位已由 cleanupDeadUnits() 清理，此处仅将备战区新单位部署到敌方半场空位
    // 存活单位保留在棋盘上，不重置

    // 从敌方玩家的备战区取出所有单位，随机放置到敌方半场空位
    auto &enemyBench = enemyPlayer_.getBench();
    for (int i = 0; i < BENCH_SIZE; ++i) {
        auto unit = enemyBench.getUnit(i);
        if (!unit) continue;
        
        bool placed = false;
        
        // 收集敌方半场所有空位
        std::vector<std::pair<int, int>> emptyPositions;
        for (int x = 0; x < BOARD_M / 2; ++x) {
            for (int y = 0; y < BOARD_N; ++y) {
                if (!board_.isOccupied(x, y)) {
                    emptyPositions.push_back({x, y});
                }
            }
        }
        
        // 随机选择一个空位放置
        if (!emptyPositions.empty()) {
            int idx = rand() % emptyPositions.size();
            int x = emptyPositions[idx].first;
            int y = emptyPositions[idx].second;
            board_.placeUnit(unit, x, y);
            placed = true;
        }
        
        if (placed) {
            enemyUnits_.push_back(unit);
            enemyBench.removeUnit(i);
        }
    }
}

void Game::executeEnemyPreparation() {
    // 敌方 AI 在准备阶段的操作：
    // 使用与玩家完全相同的 Shop 接口进行购买、升级、布阵
    // 注意：布阵操作由 spawnEnemyWave 统一处理，此处只做购买和合成
    
    // 清空旧日志，记录新的 AI 行为（始终记录，显示由开关控制）
    aiBehaviorLog_.clear();
    addAiBehaviorLog("=== AI 准备阶段开始 ===");
    
    // 1. 获得基础金币收入（与玩家收入机制一致）
    int baseIncome = 5 + currentRound_;
    enemyPlayer_.changeGold(baseIncome);
    addAiBehaviorLog("敌方获得基础金币 " + std::to_string(baseIncome));
    
    // 2. 刷新敌方商店
    enemyShop_.refresh(Owner::EnemyCtrl, currentRound_, enemyPlayer_, 1);
    
    // 3. AI 从商店购买单位（使用 Shop::buyUnit 接口）
    int buyAttempts = 0;
    const int maxBuyAttempts = 5;
    while (buyAttempts < maxBuyAttempts) {
        buyAttempts++;
        
        // 检查金币和人口
        if (enemyPlayer_.getGold() < 2 || !enemyPlayer_.canRecruit()) {
            break;
        }
        
        // 随机选择一个非空的商店槽位购买
        auto& shopUnits = enemyShop_.getUnits();
        std::vector<int> availableSlots;
        for (int i = 0; i < static_cast<int>(shopUnits.size()); ++i) {
            if (shopUnits[i]) {
                availableSlots.push_back(i);
            }
        }
        
        if (availableSlots.empty()) {
            break;
        }
        
        // 随机选择一个槽位购买
        std::uniform_int_distribution<int> dist(0, static_cast<int>(availableSlots.size()) - 1);
        std::mt19937 rng(std::random_device{}());
        int slotIndex = availableSlots[dist(rng)];
        
        // 记录购买信息
        if (shopUnits[slotIndex]) {
            addAiBehaviorLog("敌方购买了 " + shopUnits[slotIndex]->getUnitType()
                           + " (消耗 2 金币)");
        }
        
        // 使用 Shop::buyUnit 执行购买
        if (!enemyShop_.buyUnit(slotIndex, enemyPlayer_)) {
            break;
        }
    }
    
    // 4. AI 尝试升级人口（如果金币充足）
    int upgradeCount = 0;
    while (enemyPlayer_.getGold() >= 5) {
        if (!Shop::upgradePopulation(enemyPlayer_, 5)) {
            break;
        }
        upgradeCount++;
    }
    if (upgradeCount > 0) {
        addAiBehaviorLog("敌方升级了人口 (消耗 " + std::to_string(upgradeCount * 5) + " 金币)");
    }
    
    // 5. AI 尝试三合一合成（使用 Shop::autoCombine 接口）
    bool combined = true;
    int combineIterations = 0;
    const int maxCombineIterations = 10;
    std::string combineMessage;
    while (combined && combineIterations < maxCombineIterations) {
        combineIterations++;
        combined = Shop::autoCombine(enemyPlayer_, combineMessage);
    }
    if (!combineMessage.empty()) {
        addAiBehaviorLog("敌方合成了单位: " + combineMessage);
    }
    
    // 6. AI 尝试购买装备（P1-1：从敌方商店购买装备）
    auto& enemyEquipSlots = enemyShop_.getEquipmentSlots();
    for (int i = 0; i < static_cast<int>(enemyEquipSlots.size()); ++i) {
        if (enemyEquipSlots[i].sold) continue;
        if (enemyPlayer_.getGold() >= enemyEquipSlots[i].price) {
            if (enemyShop_.buyEquipment(i, enemyPlayer_)) {
                addAiBehaviorLog("敌方购买了装备 " + enemyEquipSlots[i].eq.name
                    + " (消耗 " + std::to_string(enemyEquipSlots[i].price) + " 金币)");
            }
        }
    }
    
    // 7. 尝试将暂存装备应用到备战区
    {
        auto stored = enemyPlayer_.getStoredEquipments();
        for (auto* eq : stored) {
            if (eq->ownerUnitId != 0) continue;
            auto& bench = enemyPlayer_.getBench();
            for (int i = 0; i < BENCH_SIZE; ++i) {
                auto u = bench.getUnit(i);
                if (!u) continue;
                int chkSlot2 = eq->type;
                if ((chkSlot2 == 2 || chkSlot2 == 3) && u->GetEquipmentSlot(2) && u->GetEquipmentSlot(3)) continue;
                if (chkSlot2 < 2 && u->GetEquipmentSlot(chkSlot2)) continue;
                if (!eq->restriction.empty() && eq->restriction != Unit::ClassToString(u->GetClassType())) continue;
                int useSlot2 = eq->type;
                if ((useSlot2 == 2 || useSlot2 == 3) && u->GetEquipmentSlot(2)) useSlot2 = 3;
                if (u->Equip(eq, useSlot2)) {
                    enemyPlayer_.equipToUnit(eq->id, u->getUnitId());
                    break;
                }
            }
        }
    }
    
    {
        auto& enemyBench = enemyPlayer_.getBench();
        int unitCount = 0;
        for (int i = 0; i < BENCH_SIZE; ++i) {
            if (enemyBench.getUnit(i)) unitCount++;
        }
        addAiBehaviorLog("敌方备战区共有 " + std::to_string(unitCount) + " 个单位，即将部署");
    }
}

void Game::executeEnemyTurn() {
    auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
    
    for (auto& enemy : enemyUnits_) {
        if (enemy && enemy->isAlive()) {
            executeEnemyUnitAction(enemy);
        }
    }
}

void Game::executeEnemyUnitAction(std::shared_ptr<Unit> enemy) {
    if (!enemy || !enemy->isAlive()) return;
    
    auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
    
    // 敌方 AI 决策
    auto decision = EnemyAI::makeDecision(enemy, board_, playerUnits);
    
    auto [ex, ey] = enemy->getPosition();
    
    switch (decision.action) {
        case AIAction::Move: {
            board_.moveUnit(ex, ey, decision.targetX, decision.targetY);
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "敌方 %s 从 (%d,%d) 移动到 (%d,%d)",
                         enemy->getUnitType().c_str(), ex, ey, decision.targetX, decision.targetY);
                addAiBehaviorLog(buf);
            }
            // 设置意图信息（供棋盘图标显示）
            {
                AiIntentInfo intent;
                intent.action = "移动";
                intent.targetX = decision.targetX;
                intent.targetY = decision.targetY;
                if (decision.targetUnit) intent.targetName = decision.targetUnit->getUnitType();
                setAiIntent(enemy, intent);
            }
            break;
        }
        case AIAction::Attack: {
            if (decision.targetUnit && decision.targetUnit->isAlive()) {
                auto [tx, ty] = decision.targetUnit->getPosition();
                std::string attackName = (decision.attackType == AttackType::Ultimate) ?
                    enemy->getUltimateAttackName() : enemy->getNormalAttackName();
                if (decision.attackType == AttackType::Ultimate) {
                    auto [eux, euy] = enemy->getPosition();
                    setUltimateUsed(attackName, enemy->getUnitType(), eux, euy);
                    enemy->setCombatTargets(board_.getUnitsForOwner(Owner::PlayerCtrl));
                }
                enemy->onAttack(decision.targetUnit, decision.attackType);
                // 通过状态机触发攻击状态转换
                if (auto* sm = stateMachineManager_.getStateMachine(enemy)) {
                    sm->handleEvent(StateEvent::AttackReady);
                }
                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "敌方 %s 使用 %s 攻击 %s (%d,%d)",
                             enemy->getUnitType().c_str(), attackName.c_str(),
                             decision.targetUnit->getUnitType().c_str(), tx, ty);
                    addAiBehaviorLog(buf);
                }
                // 设置意图信息
                {
                    AiIntentInfo intent;
                    intent.action = "攻击";
                    intent.targetX = tx;
                    intent.targetY = ty;
                    intent.targetName = decision.targetUnit->getUnitType();
                    setAiIntent(enemy, intent);
                }
            }
            break;
        }
        case AIAction::CastSkill:
        case AIAction::None:
        default:
            addAiBehaviorLog("敌方 " + enemy->getUnitType() + " 待机中");
            {
                AiIntentInfo intent;
                intent.action = "待机";
                setAiIntent(enemy, intent);
            }
            break;
    }
}

void Game::executePlayerUnitAction(std::shared_ptr<Unit> unit) {
    if (!unit || !unit->isAlive()) return;
    
    int maxActions = 1 + unit->getExtraActions();
    for (int action = 0; action < maxActions; ++action) {
        if (!unit->isAlive()) break;
        
        // 找最近敌人
        std::shared_ptr<Unit> nearest = nullptr;
        float minDist = std::numeric_limits<float>::max();
        for (auto& enemy : enemyUnits_) {
            if (enemy && enemy->isAlive()) {
                auto [ux, uy] = unit->getPosition();
                auto [ex, ey] = enemy->getPosition();
                float dist = std::sqrt(float((ux - ex) * (ux - ex) + (uy - ey) * (uy - ey)));
                if (dist < minDist) { minDist = dist; nearest = enemy; }
            }
        }
        
        if (!nearest) break;
        
        auto [ux, uy] = unit->getPosition();
        auto [ex, ey] = nearest->getPosition();
        
        // 额外行动时跳过移动（直接攻击或继续移动）
        if (action == 0 && !unit->isPositionInAttackRange(ex, ey, AttackType::Normal)) {
            // 尝试寻路移动到能攻击到目标的位置，而非目标所在格子（目标格被占据，A*永远不可达）
            int bestMoveX = -1, bestMoveY = -1;
            size_t bestPathLen = std::numeric_limits<size_t>::max();
            
            for (int tx = 0; tx < BOARD_M; ++tx) {
                for (int ty = 0; ty < BOARD_N; ++ty) {
                    if (!board_.isValidPosition(tx, ty) || board_.isOccupied(tx, ty)) continue;
                    
                    bool canAttackFromHere = false;
                    const auto& rangeMap = unit->getNormalAttackRangeMap();
                    for (const auto& offset : rangeMap) {
                        if (tx + offset.first == ex && ty + offset.second == ey) {
                            canAttackFromHere = true;
                            break;
                        }
                    }
                    if (!canAttackFromHere) continue;
                    
                    auto path = Pathfinding::astarPath(board_, ux, uy, tx, ty);
                    if (!path.empty() && path.size() < bestPathLen) {
                        bestPathLen = path.size();
                        bestMoveX = path[0].first;
                        bestMoveY = path[0].second;
                    }
                }
            }
            
            if (bestMoveX >= 0 && bestMoveY >= 0) {
                board_.moveUnit(ux, uy, bestMoveX, bestMoveY);
            } else {
                // 无法寻路到攻击位，向目标方向移动一步
                int dx = (ex > ux) ? 1 : ((ex < ux) ? -1 : 0);
                int dy = (ey > uy) ? 1 : ((ey < uy) ? -1 : 0);
                bool moved = false;
                if (dx != 0) {
                    int nx = ux + dx, ny = uy;
                    if (board_.isValidPosition(nx, ny) && !board_.isOccupied(nx, ny)) {
                        board_.moveUnit(ux, uy, nx, ny);
                        moved = true;
                    }
                }
                if (!moved && dy != 0) {
                    int nx = ux, ny = uy + dy;
                    if (board_.isValidPosition(nx, ny) && !board_.isOccupied(nx, ny)) {
                        board_.moveUnit(ux, uy, nx, ny);
                    }
                }
            }
            std::tie(ux, uy) = unit->getPosition();
        }
        
        // 攻击（如果在范围内），或无法移动时强制攻击最近敌人
        if (unit->isAlive() && nearest && nearest->isAlive()) {
            auto [nux, nuy] = unit->getPosition();
            auto [nex, ney] = nearest->getPosition();
            bool canNormalAttack = unit->isPositionInAttackRange(nex, ney, AttackType::Normal);
            
            // 不能正常攻击但也不能移动（已被夹击/无路可走）→ 强制攻击最近敌人
            if (!canNormalAttack && action == 0) {
                // 确认无法移动到任何攻击位
                bool canMoveSomewhere = false;
                for (int tx = 0; tx < BOARD_M && !canMoveSomewhere; ++tx) {
                    for (int ty = 0; ty < BOARD_N && !canMoveSomewhere; ++ty) {
                        if (!board_.isValidPosition(tx, ty) || board_.isOccupied(tx, ty)) continue;
                        const auto& rangeMap = unit->getNormalAttackRangeMap();
                        for (const auto& offset : rangeMap) {
                            if (tx + offset.first == nex && ty + offset.second == ney) {
                                canMoveSomewhere = true;
                                break;
                            }
                        }
                    }
                }
                if (!canMoveSomewhere) {
                    canNormalAttack = true;  // 无路可走，强制攻击
                }
            }
            
            if (canNormalAttack) {
                auto attackType = AttackType::Normal;
                if (action == 0 && unit->canUseUltimate() && unit->isPositionInAttackRange(nex, ney, AttackType::Ultimate)) {
                    attackType = AttackType::Ultimate;
                }
                if (attackType == AttackType::Ultimate) {
                    setUltimateUsed(unit->getUltimateAttackName(), unit->getUnitType(), ux, uy);
                    unit->setCombatTargets(enemyUnits_);
                }
                unit->onAttack(nearest, attackType);
                // 击杀赏金
                if (!nearest->isAlive()) {
                    switch (nearest->getStarLevel()) {
                        case 1: combatKillGold_ += 2; break;
                        case 2: combatKillGold_ += 5; break;
                        case 3: combatKillGold_ += 12; break;
                        default: combatKillGold_ += 2;
                    }
                    combatKilledCount_++;
                }
                if (auto* sm = stateMachineManager_.getStateMachine(unit)) {
                    sm->handleEvent(StateEvent::AttackReady);
                }
            }
        }
    }
    // 消耗额外行动
    while (unit->getExtraActions() > 0) unit->consumeExtraAction();
}

void Game::cleanupDeadUnits() {
    // 从棋盘上移除所有死亡单位，处理装备掉落
    for (int x = 0; x < BOARD_M; ++x) {
        for (int y = 0; y < BOARD_N; ++y) {
            auto u = board_.getUnit(x, y);
            if (u && !u->isAlive()) {
                // 死亡装备掉落：1★=35%, 2★=25%, 3★=15%
                // 己方死→敌方得，敌方死→己方得
                Player* recipient = (u->getOwner() == Owner::EnemyCtrl) ? &player_ : &enemyPlayer_;
                for (int s = 0; s < 4; ++s) {
                    Equipment* eq = u->GetEquipmentSlot(s);
                    if (!eq) continue;
                    int roll = rand() % 100;
                    bool drop = false;
                    if (eq->starLevel == 1 && roll < 35) drop = true;
                    else if (eq->starLevel == 2 && roll < 25) drop = true;
                    else if (eq->starLevel == 3 && roll < 15) drop = true;
                    if (drop) {
                        recipient->addStoredEquipment(*eq);
                        recipient->unequipFromUnit(eq->id);
                        u->Unequip(s);
                    }
                }
                
                if (u->getOwner() == Owner::PlayerCtrl) {
                    player_.removePopulation(1);
                }
                board_.removeUnit(x, y);
            }
        }
    }
    // 从敌人列表中移除死亡单位
    enemyUnits_.erase(std::remove_if(enemyUnits_.begin(), enemyUnits_.end(),
        [](const std::shared_ptr<Unit>& e){ return !e || !e->isAlive(); }), enemyUnits_.end());
}

void Game::registerAllUnitsToStateMachine() {
    // 为场上所有单位创建状态机
    auto allUnits = board_.getAllUnits();
    for (auto& entry : allUnits) {
        int x, y;
        std::shared_ptr<Unit> unit;
        std::tie(x, y, unit) = entry;
        if (unit) {
            // 检查是否已有状态机
            if (!stateMachineManager_.getStateMachine(unit)) {
                stateMachineManager_.createStateMachine(unit);
            }
        }
    }
}

void Game::transitionToNextRound() {
    // 战后清理
    cleanupDeadUnits();
    
    // 重置击杀赏金计数器
    combatKillGold_ = 0;
    combatKilledCount_ = 0;
    
    // 战斗阶段结束
    inCombatPhase_ = false;
    
    if (!gameOver_) {
        currentRound_ += 1;
        shop_.refresh(Owner::PlayerCtrl, currentRound_, player_, 1);
        startPhaseOne();
        stateMachineManager_.resetAll();
    }
    
    // 修正结算日志当前金币为扣费后的实际值
    if (!battleLog_.empty()) {
        battleLog_.back().currentGold = player_.getGold();
    }
    
    if (currentRound_ > maxRounds_) gameOver_ = true;
}

void Game::addAiBehaviorLog(const std::string& msg) {
    aiBehaviorLog_.push_back(msg);
    // 限制日志数量，防止无限增长
    while (aiBehaviorLog_.size() > 200) {
        aiBehaviorLog_.erase(aiBehaviorLog_.begin());
    }
}

void Game::setAiIntent(std::shared_ptr<Unit> unit, const AiIntentInfo& info) {
    if (unit) {
        aiIntentMap_[unit] = info;
    }
}

void Game::deployEnemyUnits() {
    // 先执行敌方准备阶段（AI 使用 Shop 接口进行购买、布阵）
    executeEnemyPreparation();
    
    // 生成敌方阵容（从敌方备战区取单位放到棋盘）
    spawnEnemyWave(currentRound_);
    
    // ===== 应用羁绊与装备套装效果 =====
    {
        auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
        std::vector<Unit*> playerRaw;
        for (auto& u : playerUnits) if (u && u->isAlive()) playerRaw.push_back(u.get());
        Unit::CheckTraits(playerRaw);
    }
    {
        std::vector<Unit*> enemyRaw;
        for (auto& e : enemyUnits_) if (e && e->isAlive()) enemyRaw.push_back(e.get());
        Unit::CheckTraits(enemyRaw);
    }
    
    // 标记进入战斗阶段
    inCombatPhase_ = true;
    
    // 回合开始：通知场上单位
    {
        auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
        for (auto& unit : playerUnits) if (unit && unit->isAlive()) unit->onTurnStart();
        for (auto& enemy : enemyUnits_) if (enemy && enemy->isAlive()) enemy->onTurnStart();
    }
    
    // 为所有单位注册状态机
    registerAllUnitsToStateMachine();
    
    // 广播回合开始事件
    stateMachineManager_.broadcastEvent(StateEvent::TurnStart);
    
    // 重置战斗动画状态
    currentCombatStep_ = CombatStep::DeployEnemy;
    combatPlayerIdx_ = 0;
    combatEnemyIdx_ = 0;
    combatBattleRound_ = 0;
    roundRobinIsEnemy_ = true;
    
    // 清空旧 AI 意图
    aiIntentMap_.clear();
}

bool Game::doEnemyMoveStep() {
    if (!inCombatPhase_) return false;
    
    // ===== 检查战斗是否结束 =====
    if (checkBattleEnd()) {
        resolveBattle();
        transitionToNextRound();
        currentCombatStep_ = CombatStep::Finished;
        return false;
    }
    
    // 更新所有状态机
    stateMachineManager_.updateAll(0.1f);
    
    // ===== 交替行动（敌→我→敌→我...） =====
    // 每微回合开始：重新获取双方单位快照并轮询
    if (currentCombatStep_ == CombatStep::DeployEnemy) {
        combatPlayerSnap_ = board_.getUnitsForOwner(Owner::PlayerCtrl);
        combatEnemyIdx_ = 0;
        combatPlayerIdx_ = 0;
        combatBattleRound_ = 0;
        roundRobinIsEnemy_ = true;  // 敌方先手
        currentCombatStep_ = CombatStep::EnemyTurn;
    }
    
    // 找一个可行动的单位（交替搜索敌方/己方）
    bool acted = false;
    int searchSides = 0;
    while (!acted && searchSides < 2) {
        searchSides++;
        
        if (roundRobinIsEnemy_) {
            // 尝试从敌方找一个活单位行动
            while (combatEnemyIdx_ < static_cast<int>(enemyUnits_.size())) {
                auto& enemy = enemyUnits_[combatEnemyIdx_];
                combatEnemyIdx_++;
                if (enemy && enemy->isAlive()) {
                    aiIntentMap_.clear();
                    executeEnemyUnitAction(enemy);
                    acted = true;
                    break;
                }
            }
        } else {
            // 尝试从己方找一个活单位行动
            while (combatPlayerIdx_ < static_cast<int>(combatPlayerSnap_.size())) {
                auto& unit = combatPlayerSnap_[combatPlayerIdx_];
                combatPlayerIdx_++;
                if (unit && unit->isAlive()) {
                    executePlayerUnitAction(unit);
                    acted = true;
                    break;
                }
            }
        }
        
        // 轮到另一方
        roundRobinIsEnemy_ = !roundRobinIsEnemy_;
    }
    
    // 检查战斗是否结束
    if (checkBattleEnd()) {
        resolveBattle();
        transitionToNextRound();
        currentCombatStep_ = CombatStep::Finished;
        return false;
    }
    
    // 双方都遍历完了 → 进入下一微回合
    if (!acted) {
        // 清理死亡单位
        enemyUnits_.erase(std::remove_if(enemyUnits_.begin(), enemyUnits_.end(),
            [](const std::shared_ptr<Unit>& e){ return !e || !e->isAlive(); }), enemyUnits_.end());
        cleanupDeadUnits();
        
        combatBattleRound_++;
        
        // 最大微回合限制 → 平局
        if (combatBattleRound_ >= 100) {
            resolveBattle();
            transitionToNextRound();
            currentCombatStep_ = CombatStep::Finished;
            return false;
        }
        
        // 重新获取快照，开始新微回合
        combatPlayerSnap_ = board_.getUnitsForOwner(Owner::PlayerCtrl);
        combatEnemyIdx_ = 0;
        combatPlayerIdx_ = 0;
        roundRobinIsEnemy_ = true;
        
        // 如果双方都没活单位了，直接结束
        if (enemyUnits_.empty() || combatPlayerSnap_.empty()) {
            resolveBattle();
            transitionToNextRound();
            currentCombatStep_ = CombatStep::Finished;
            return false;
        }
        
        return true;  // 继续
    }
    
    return true;
}

bool Game::checkBattleEnd() {
    auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
    
    bool playerAlive = false;
    for (auto& unit : playerUnits) {
        if (unit && unit->isAlive()) {
            playerAlive = true;
            break;
        }
    }
    
    // 玩家棋盘上无单位但备战区有 → 强制部署到玩家半场再打
    if (!playerAlive) {
        auto& bench = player_.getBench();
        bool benchHasUnit = false;
        for (int i = 0; i < BENCH_SIZE; ++i) {
            if (bench.getUnit(i)) { benchHasUnit = true; break; }
        }
        if (benchHasUnit) {
            // 强制部署备战区单位到玩家半场空位
            for (int i = 0; i < BENCH_SIZE; ++i) {
                auto unit = bench.getUnit(i);
                if (!unit || !unit->isAlive()) continue;
                for (int x = BOARD_M / 2; x < BOARD_M; ++x) {
                    for (int y = 0; y < BOARD_N; ++y) {
                        if (!board_.isOccupied(x, y)) {
                            board_.placeUnit(unit, x, y);
                            bench.removeUnit(i);
                            goto nextBenchUnit;
                        }
                    }
                }
                nextBenchUnit:;
            }
            // 重新检查玩家存活
            auto newPlayerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
            for (auto& u : newPlayerUnits) {
                if (u && u->isAlive()) { playerAlive = true; break; }
            }
        }
    }
    
    bool anyEnemyAlive = false;
    // 双重检查：enemyUnits_ 列表 + 棋盘（防止列表与棋盘不同步）
    for (auto& enemy : enemyUnits_) {
        if (enemy && enemy->isAlive()) { anyEnemyAlive = true; break; }
    }
    if (!anyEnemyAlive) {
        auto boardEnemies = board_.getUnitsForOwner(Owner::EnemyCtrl);
        for (auto& enemy : boardEnemies) {
            if (enemy && enemy->isAlive()) { anyEnemyAlive = true; break; }
        }
    }
    
    // 棋盘上无敌方但备战区有 → 尝试强制部署，不直接判全灭
    if (!anyEnemyAlive) {
        auto& enemyBench = enemyPlayer_.getBench();
        bool benchHasUnit = false;
        for (int i = 0; i < BENCH_SIZE; ++i) {
            if (enemyBench.getUnit(i)) { benchHasUnit = true; break; }
        }
        if (benchHasUnit) {
            // 尝试把备战区单位部署到棋盘
            spawnEnemyWave(currentRound_);
            // 重新检查
            for (auto& enemy : enemyUnits_) {
                if (enemy && enemy->isAlive()) { anyEnemyAlive = true; break; }
            }
        }
    }
    
    return !playerAlive || !anyEnemyAlive;
}

void Game::resolveBattle() {
    auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
    
    bool playerAlive = false;
    int aliveUnitCount = 0;
    for (auto& unit : playerUnits) {
        if (unit && unit->isAlive()) {
            playerAlive = true;
            aliveUnitCount++;
        }
    }
    
    bool anyEnemyAlive = false;
    for (auto& enemy : enemyUnits_) {
        if (enemy && enemy->isAlive()) { anyEnemyAlive = true; break; }
    }
    if (!anyEnemyAlive) {
        auto boardEnemies = board_.getUnitsForOwner(Owner::EnemyCtrl);
        for (auto& enemy : boardEnemies) {
            if (enemy && enemy->isAlive()) { anyEnemyAlive = true; break; }
        }
    }

    int preIncomeGold = player_.getGold();

    // 利息在战斗结束后发放（非回合初）
    int interestGold = std::max(5, preIncomeGold / 15);
    player_.changeGold(interestGold);
    lastGoldBreakdown_.interestGold = interestGold;

    BattleLogEntry entry;
    entry.round = currentRound_;
    entry.remainingUnits = aliveUnitCount;
    entry.goldBeforeIncome = preIncomeGold;
    entry.goldAtRoundStart = goldAtRoundStart_;
    entry.interestGold = interestGold;
    lastGoldBreakdown_.totalBeforeGold = player_.getGold();

    // 战斗结算
    if (!anyEnemyAlive && playerAlive) {
        // 玩家胜利
        int baseGold = 5;
        int victoryGold = 20;
        int streakBonus = std::max(0, winStreak_ * 5);
        int totalReward = baseGold + victoryGold + streakBonus + combatKillGold_;
        
        player_.changeGold(totalReward);
        winStreak_ += 1;
        lossStreak_ = 0;
        
        // 经验发放：15 + 击杀数 × 5
        int xp = 15 + combatKilledCount_ * 5;
        player_.gainExperience(xp);
        
        entry.playerWon = true;
        entry.baseGold = baseGold;
        entry.victoryGold = victoryGold;
        entry.streakGold = streakBonus;
        entry.killGold = combatKillGold_;
        entry.totalGoldEarned = totalReward + interestGold;
        entry.currentGold = player_.getGold();
        entry.healthLost = 0;
        entry.currentHealth = player_.getHealth();
        
        lastGoldBreakdown_.baseGold = baseGold;
        lastGoldBreakdown_.victoryGold = victoryGold;
        lastGoldBreakdown_.streakGold = streakBonus;
        lastGoldBreakdown_.interestGold = interestGold;
        
        // 对敌方造成伤害（存活单位越多伤害越高）
        int enemyDamage = std::max(5, aliveUnitCount * 3 + currentRound_);
        enemyPlayer_.changeHealth(-enemyDamage);
        
        // 敌方生命值归零 → 玩家赢得整局游戏
        if (enemyPlayer_.getHealth() <= 0) {
            enemyPlayer_.setHealth(0);
            gameOver_ = true;
            playerWonGame_ = true;
        }
        
        // 敌方经验
        enemyPlayer_.gainExperience(5);
    } else if (!playerAlive) {
        // 玩家失败
        int baseGold = 3;
        int lossPenalty = -10;
        int healthDamage = 15 + 3 * lossStreak_;
        int totalReward = baseGold + lossPenalty;
        
        player_.changeGold(totalReward);
        player_.changeHealth(-healthDamage);
        lossStreak_ += 1;
        winStreak_ = 0;
        
        // 经验发放：失败不给经验
        player_.gainExperience(0);
        // 敌方经验：失败也给敌方经验
        enemyPlayer_.gainExperience(3);
        
        entry.playerWon = false;
        entry.baseGold = baseGold;
        entry.victoryGold = 0;
        entry.streakGold = lossPenalty;
        entry.killGold = 0;
        entry.totalGoldEarned = totalReward + interestGold;
        entry.currentGold = player_.getGold();
        entry.healthLost = healthDamage;
        entry.currentHealth = player_.getHealth();
        
        lastGoldBreakdown_.baseGold = baseGold;
        lastGoldBreakdown_.victoryGold = 0;
        lastGoldBreakdown_.streakGold = lossPenalty;
        lastGoldBreakdown_.interestGold = interestGold;
        
        if (player_.getHealth() <= 0) {
            player_.setHealth(0);
            gameOver_ = true;
            playerWonGame_ = false;
        }
    } else {
        // 平局独立结算，不视为玩家失败
        int aliveEnemyCount = 0;
        for (auto& enemy : enemyUnits_) {
            if (enemy && enemy->isAlive()) aliveEnemyCount++;
        }
        double survivalRatio = (aliveEnemyCount > 0)
            ? (double)aliveUnitCount / (aliveUnitCount + aliveEnemyCount)
            : 0.5;
        int drawGold = 3 + (int)(survivalRatio * 5);  // 3~8 金币
        
        player_.changeGold(drawGold);
        // 平局不扣血，不给胜利奖励，连胜/连败不变
        // winStreak_ 和 lossStreak_ 保持不变
        
        player_.gainExperience(10);
        enemyPlayer_.gainExperience(5);
        
        entry.playerWon = false;
        entry.baseGold = drawGold;
        entry.victoryGold = 0;
        entry.streakGold = 0;
        entry.killGold = 0;
        entry.interestGold = interestGold;
        entry.totalGoldEarned = drawGold + interestGold;
        entry.currentGold = player_.getGold();
        entry.healthLost = 0;
        entry.currentHealth = player_.getHealth();
        
        lastGoldBreakdown_.baseGold = drawGold;
        lastGoldBreakdown_.victoryGold = 0;
        lastGoldBreakdown_.streakGold = 0;
        lastGoldBreakdown_.interestGold = interestGold;
    }
    
    // 构建总结文字
    {
        char buf[512];
        if (entry.playerWon) {
            snprintf(buf, sizeof(buf),
                "[回合 %d] 战斗胜利！剩余 %d 个单位\n"
                "[回合 %d] 获得金币: 基础%d + 胜利%d + 连胜奖励%d + 击杀%d + 利息%d = %d\n"
                "[回合 %d] 当前金币: %d",
                entry.round, entry.remainingUnits,
                entry.round, entry.baseGold, entry.victoryGold, entry.streakGold, combatKillGold_, entry.interestGold, entry.totalGoldEarned,
                entry.round, entry.currentGold);
        } else if (entry.healthLost > 0) {
            snprintf(buf, sizeof(buf),
                "[回合 %d] 战斗失败！剩余 %d 个单位\n"
                "[回合 %d] 获得金币: 基础%d + 失败%d + 利息%d = %d\n"
                "[回合 %d] 当前金币: %d  生命损失: %d  当前生命: %d",
                entry.round, entry.remainingUnits,
                entry.round, entry.baseGold, entry.streakGold, entry.interestGold, entry.totalGoldEarned,
                entry.round, entry.currentGold, entry.healthLost, entry.currentHealth);
        } else {
            snprintf(buf, sizeof(buf),
                "[回合 %d] 平局！剩余 %d 个单位\n"
                "[回合 %d] 获得金币: 平局%d + 利息%d = %d\n"
                "[回合 %d] 当前金币: %d  连胜/连败不变",
                entry.round, entry.remainingUnits,
                entry.round, entry.baseGold, entry.interestGold, entry.totalGoldEarned,
                entry.round, entry.currentGold);
        }
        entry.summary = buf;
    }
    
    battleLog_.push_back(entry);
}

void Game::endRound() {
    transitionToNextRound();
}

bool Game::isGameOver() const {
    return gameOver_;
}

bool Game::isInCombatPhase() const {
    return inCombatPhase_;
}

int Game::getCurrentRound() const {
    return currentRound_;
}

const Player& Game::getPlayer() const {
    return player_;
}

Player& Game::getPlayer() {
    return player_;
}

const Player& Game::getEnemyPlayer() const {
    return enemyPlayer_;
}

Player& Game::getEnemyPlayer() {
    return enemyPlayer_;
}

Shop& Game::getShop() {
    return shop_;
}

const Shop& Game::getShop() const {
    return shop_;
}

Shop& Game::getEnemyShop() {
    return enemyShop_;
}

const Shop& Game::getEnemyShop() const {
    return enemyShop_;
}

const Board& Game::getBoard() const {
    return board_;
}

Board& Game::getBoard() {
    return board_;
}

void Game::rebuildEnemyUnitsFromBoard() {
    enemyUnits_.clear();
    for (int x = 0; x < BOARD_M; ++x) {
        for (int y = 0; y < BOARD_N; ++y) {
            auto unit = board_.getUnit(x, y);
            if (unit && unit->getOwner() == Owner::EnemyCtrl) {
                enemyUnits_.push_back(unit);
            }
        }
    }
}

void Game::clearAllState() {
    board_.clear();
    enemyUnits_.clear();

    // 彻底重建双方玩家（与 initialize() 一致，确保无残留状态）
    player_ = Player();
    enemyPlayer_ = Player();

    // 重置商店（清除旧商店数据和装备槽）
    shop_.reset();
    enemyShop_.reset();

    // 清除战斗日志和 AI 日志
    battleLog_.clear();
    aiBehaviorLog_.clear();
    aiIntentMap_.clear();

    // 重置战斗动画状态
    currentCombatStep_ = CombatStep::None;
    combatEnemyIdx_ = 0;
    combatPlayerIdx_ = 0;
    combatBattleRound_ = 0;
    roundRobinIsEnemy_ = true;
    combatPlayerSnap_.clear();

    // 重置状态机和攻击顺序管理器
    stateMachineManager_.resetAll();
    attackOrderManager_.clear();

    // 重置杂项战斗变量
    combatKillGold_ = 0;
    combatKilledCount_ = 0;
    equipDropAnimPending_ = false;
    ultimateUsedName_.clear();
    ultimateUsedChar_.clear();
    ultimateUsedX_ = -1;
    ultimateUsedY_ = -1;
}

const std::vector<std::shared_ptr<Unit>>& Game::getEnemyUnits() const {
    return enemyUnits_;
}

StateMachineManager& Game::getStateMachineManager() {
    return stateMachineManager_;
}

AttackOrderManager& Game::getAttackOrderManager() {
    return attackOrderManager_;
}

void Game::startCombatAnimation() {
    // 执行敌方准备并部署
    executeEnemyPreparation();
    spawnEnemyWave(currentRound_);
    
    // ===== 应用羁绊与装备套装效果 =====
    {
        auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
        std::vector<Unit*> playerRaw;
        for (auto& u : playerUnits) if (u && u->isAlive()) playerRaw.push_back(u.get());
        Unit::CheckTraits(playerRaw);
    }
    {
        std::vector<Unit*> enemyRaw;
        for (auto& e : enemyUnits_) if (e && e->isAlive()) enemyRaw.push_back(e.get());
        Unit::CheckTraits(enemyRaw);
    }
    
    // 标记进入战斗阶段
    inCombatPhase_ = true;
    
    // 回合开始：通知场上单位
    {
        auto playerUnits = board_.getUnitsForOwner(Owner::PlayerCtrl);
        for (auto& unit : playerUnits) if (unit && unit->isAlive()) unit->onTurnStart();
        for (auto& enemy : enemyUnits_) if (enemy && enemy->isAlive()) enemy->onTurnStart();
    }
    
    // 为所有单位注册状态机
    registerAllUnitsToStateMachine();
    
    // 广播回合开始事件
    stateMachineManager_.broadcastEvent(StateEvent::TurnStart);
    
    // 重置战斗动画状态
    currentCombatStep_ = CombatStep::DeployEnemy;
    combatPlayerIdx_ = 0;
    combatEnemyIdx_ = 0;
    combatBattleRound_ = 0;
    roundRobinIsEnemy_ = true;
    combatPlayerSnap_ = board_.getUnitsForOwner(Owner::PlayerCtrl);
}

bool Game::advanceCombatAnimation() {
    // 逐步执行战斗动画
    return doEnemyMoveStep();
}