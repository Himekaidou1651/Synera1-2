/**
 * @file    SaveManager.cpp
 * @brief   存档管理器实现
 * @author  
 * @date    2026-06-24
 */

#include "SaveManager.h"
#include "Game.h"
#include "../units/UnitFactory.h"
#include <fstream>
#include <iomanip>
#include <exception>
#include <map>

// ========== 辅助函数 ==========

static void writeQuotedString(std::ostream &out, const std::string &value) {
    out << std::quoted(value);
}

static bool readQuotedString(std::istream &in, std::string &value) {
    if (static_cast<bool>(in >> std::quoted(value))) return true;
    if (!(in >> value)) return false;
    return true;
}

// ========== 单位序列化 ==========

void SaveManager::writeUnitData(std::ostream &out, const std::shared_ptr<Unit> &unit) {
    if (!unit) return;
    out << unit->getUnitId() << ' ' << unit->getHP() << ' ' << unit->getMaxHP() << ' ' << unit->getATK() << ' '
        << unit->getMaxMana() << ' ' << unit->getMana() << ' ' << static_cast<int>(unit->getOwner()) << ' '
        << unit->getStarLevel() << ' ';
    writeQuotedString(out, unit->getUnitType());
    out << ' ';
    writeQuotedString(out, unit->getNormalAttackName());
    out << ' ';
    writeQuotedString(out, unit->getUltimateAttackName());
    out << ' ';
    writeQuotedString(out, unit->getEquipmentName());
}

std::shared_ptr<Unit> SaveManager::readUnitData(std::istream &in) {
    int unitId, hp, maxHp, atk, maxMana, mana, ownerInt, starLevel;
    if (!(in >> unitId >> hp >> maxHp >> atk >> maxMana >> mana >> ownerInt >> starLevel))
        return nullptr;

    std::string unitType, normalAttack, ultimateAttack, equipmentName;
    if (!readQuotedString(in, unitType)) return nullptr;
    if (!readQuotedString(in, normalAttack)) return nullptr;
    if (!readQuotedString(in, ultimateAttack)) return nullptr;
    if (!readQuotedString(in, equipmentName)) return nullptr;

    Owner owner = static_cast<Owner>(ownerInt);
    auto unit = Unit::CreateUnitByType(unitType, owner);
    if (!unit) return nullptr;

    unit->restoreFromSave(hp, maxHp, atk, maxMana, mana, starLevel, unitId);
    return unit;
}

// ========== Player 状态序列化 ==========

void SaveManager::writePlayerState(std::ostream &out, const Player &player) {
    out << player.getHealth() << ' ' << player.getGold() << ' ' << player.getLevel() << ' '
        << player.getPopulationLevel() << ' ' << player.getStage() << ' ' << player.getExperience() << ' '
        << player.getExperienceNextLevel() << ' ' << player.getCurrentPopulation();
}

bool SaveManager::readPlayerState(std::istream &in, Player &player) {
    int health, gold, level, popLevel, stage, experience, expNextLevel, currentPopulation;
    if (!(in >> health >> gold >> level >> popLevel >> stage >> experience >> expNextLevel >> currentPopulation))
        return false;

    player.setHealth(health);
    player.setGold(gold);
    player.setLevel(level);
    player.setPopulationLevel(popLevel);
    player.setStage(stage);
    player.setExperienceNextLevel(expNextLevel);
    player.setExperience(experience);
    player.resetRound();
    player.removePopulation(player.getCurrentPopulation());
    player.addPopulation(currentPopulation);
    return true;
}

// ========== 装备表序列化 ==========

void SaveManager::writeEquipmentTable(std::ostream &out, const std::list<Equipment> &table) {
    out << table.size() << '\n';
    for (const auto &eq : table) {
        out << eq.id << ' ' << eq.ownerUnitId << ' ';
        writeQuotedString(out, eq.name);
        out << ' ' << eq.atkBonus << ' ' << eq.hpBonus << ' ' << eq.type << ' ';
        writeQuotedString(out, eq.restriction);
        out << ' ' << eq.starLevel << ' ' << eq.tag << ' ';
        writeQuotedString(out, eq.effect);
        out << '\n';
    }
}

bool SaveManager::readEquipmentTable(std::istream &in, std::list<Equipment> &table, int &nextId) {
    size_t equipCount;
    if (!(in >> equipCount)) return false;
    table.clear();

    for (size_t i = 0; i < equipCount; ++i) {
        int id, ownerUnitId, atkBonus, hpBonus, type, starLevel, tag;
        std::string name, restriction, effect;
        if (!(in >> id >> ownerUnitId)) return false;
        if (!readQuotedString(in, name)) return false;
        if (!(in >> atkBonus >> hpBonus >> type)) return false;
        if (!readQuotedString(in, restriction)) return false;
        if (!(in >> starLevel)) return false;
        if (!(in >> tag)) return false;
        if (!readQuotedString(in, effect)) return false;
        table.push_back(Equipment(id, name, atkBonus, hpBonus, type, restriction, starLevel, ownerUnitId, effect, tag));
    }
    nextId = static_cast<int>(equipCount) + 1;
    return true;
}

// ========== 主入口 ==========

bool SaveManager::saveGame(const Game &game, const std::string &filename) {
    try {
        std::ofstream out(filename);
        if (!out) return false;

        // === 第1行: Game级别状态 + Unit ID计数器 ===
        out << game.getCurrentRound() << ' ' << game.isInCombatPhase() << ' '
            << game.isGameOver() << ' ' << game.isPlayerWonGame() << ' '
            << game.getWinStreak() << ' ' << game.getLossStreak() << ' '
            << Unit::getNextUnitIdCounter() << '\n';

        // ========== 我方数据 ==========
        writePlayerState(out, game.getPlayer());
        out << '\n';

        // 棋盘所有单位（含我方和敌方，通过 owner 区分）
        const auto &board = game.getBoard();
        auto units = board.getAllUnits();
        out << units.size() << '\n';
        for (const auto &entry : units) {
            int x, y;
            std::shared_ptr<Unit> unit;
            std::tie(x, y, unit) = entry;
            out << x << ' ' << y << ' ';
            writeUnitData(out, unit);
            out << '\n';
        }

        // 我方备战区
        const auto &bench = game.getPlayer().getBench();
        std::vector<int> benchIndices;
        for (int i = 0; i < BENCH_SIZE; ++i) {
            if (bench.getUnit(i)) benchIndices.push_back(i);
        }
        out << benchIndices.size() << '\n';
        for (int index : benchIndices) {
            auto unit = bench.getUnit(index);
            out << index << ' ';
            writeUnitData(out, unit);
            out << '\n';
        }

        // 我方装备表
        writeEquipmentTable(out, game.getPlayer().getEquipmentTable());

        // ========== 敌方数据（与我方完全对称） ==========
        writePlayerState(out, game.getEnemyPlayer());
        out << '\n';

        // 敌方备战区
        const auto &enemyBench = game.getEnemyPlayer().getBench();
        std::vector<int> enemyBenchIndices;
        for (int i = 0; i < BENCH_SIZE; ++i) {
            if (enemyBench.getUnit(i)) enemyBenchIndices.push_back(i);
        }
        out << enemyBenchIndices.size() << '\n';
        for (int index : enemyBenchIndices) {
            auto unit = enemyBench.getUnit(index);
            out << index << ' ';
            writeUnitData(out, unit);
            out << '\n';
        }

        // 敌方装备表
        writeEquipmentTable(out, game.getEnemyPlayer().getEquipmentTable());

        return true;
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool SaveManager::loadGame(Game &game, const std::string &filename) {
    try {
        std::ifstream in(filename);
        if (!in) return false;

        // === 第1行: Game级别状态 ===
        int currentRound, unitIdCounter;
        bool inCombatPhase, gameOver, playerWonGame = false;
        int winStreak, lossStreak;
        if (!(in >> currentRound >> inCombatPhase >> gameOver >> playerWonGame >> winStreak >> lossStreak >> unitIdCounter))
            return false;

        // 恢复Unit ID计数器
        Unit::setNextUnitIdCounter(unitIdCounter);

        game.setCurrentRound(currentRound);
        game.setInCombatPhase(inCombatPhase);
        game.setGameOver(gameOver);
        game.setPlayerWonGame(playerWonGame);
        game.setWinStreak(winStreak);
        game.setLossStreak(lossStreak);

        // ========== 读取我方数据 ==========
        Player &player = game.getPlayer();
        Player &enemyPlayer = game.getEnemyPlayer();
        Board &board = game.getBoard();

        board.clear();
        player.resetRound();
        player.getBench().clear();
        player.getEquipmentTable().clear();
        enemyPlayer.getBench().clear();
        enemyPlayer.getEquipmentTable().clear();

        if (!readPlayerState(in, player)) return false;

        // 读取棋盘所有单位
        size_t boardCount;
        if (!(in >> boardCount)) return false;
        for (size_t i = 0; i < boardCount; ++i) {
            int x, y;
            if (!(in >> x >> y)) return false;
            in.ignore();
            auto unit = readUnitData(in);
            if (!unit) return false;
            if (!board.placeUnit(unit, x, y)) return false;
        }

        // 读取我方备战区
        size_t benchCount;
        if (!(in >> benchCount)) return false;
        auto &bench = player.getBench();
        for (size_t i = 0; i < benchCount; ++i) {
            int index;
            if (!(in >> index)) return false;
            if (index < 0 || index >= BENCH_SIZE) return false;
            in.ignore();
            auto unit = readUnitData(in);
            if (!unit) return false;
            if (!bench.setUnit(index, unit)) return false;
        }

        // 读取我方装备表
        int playerNextId;
        if (!readEquipmentTable(in, player.getEquipmentTable(), playerNextId)) return false;
        player.setNextEquipId(playerNextId);

        // ========== 读取敌方数据（对称结构） ==========
        if (!readPlayerState(in, enemyPlayer)) return false;

        // 读取敌方备战区
        size_t enemyBenchCount;
        if (!(in >> enemyBenchCount)) return false;
        auto &enemyBench = enemyPlayer.getBench();
        for (size_t i = 0; i < enemyBenchCount; ++i) {
            int index;
            if (!(in >> index)) return false;
            if (index < 0 || index >= BENCH_SIZE) return false;
            in.ignore();
            auto unit = readUnitData(in);
            if (!unit) return false;
            if (!enemyBench.setUnit(index, unit)) return false;
        }

        // 读取敌方装备表
        int enemyNextId;
        if (!readEquipmentTable(in, enemyPlayer.getEquipmentTable(), enemyNextId)) return false;
        enemyPlayer.setNextEquipId(enemyNextId);

        // === 使用 unitId 为所有单位装备 ===
        std::map<int, std::shared_ptr<Unit>> unitById;
        for (int x = 0; x < BOARD_M; ++x) {
            for (int y = 0; y < BOARD_N; ++y) {
                auto u = board.getUnit(x, y);
                if (u) unitById[u->getUnitId()] = u;
            }
        }
        for (int i = 0; i < BENCH_SIZE; ++i) {
            auto u = bench.getUnit(i);
            if (u) unitById[u->getUnitId()] = u;
        }
        for (int i = 0; i < BENCH_SIZE; ++i) {
            auto u = enemyBench.getUnit(i);
            if (u) unitById[u->getUnitId()] = u;
        }

        for (auto &eq : player.getEquipmentTable()) {
            if (eq.ownerUnitId == 0) continue;
            auto it = unitById.find(eq.ownerUnitId);
            if (it != unitById.end()) it->second->Equip(&eq, eq.type);
        }
        for (auto &eq : enemyPlayer.getEquipmentTable()) {
            if (eq.ownerUnitId == 0) continue;
            auto it = unitById.find(eq.ownerUnitId);
            if (it != unitById.end()) it->second->Equip(&eq, eq.type);
        }

        // 重建 enemyUnits_
        game.rebuildEnemyUnitsFromBoard();

        return true;
    } catch (const std::exception &) {
        game.clearAllState();
        return false;
    } catch (...) {
        game.clearAllState();
        return false;
    }
}
