/**
 * @file    Game.h
 * @brief   游戏主循环与阶段管理，协调棋盘、玩家、商店与战斗流程
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include "Commondata.h"
#include "Board.h"
#include "Player.h"
#include "Shop.h"
#include "EnemyAI.h"
#include "../combat/StateMachine.h"

/**
 * @brief 战斗动画步骤枚举
 */
enum class CombatStep {
    None,           ///< 无动作
    DeployEnemy,    ///< 部署敌方单位到棋盘
    EnemyTurn,      ///< 敌方行动（移动/攻击）
    PlayerTurn,     ///< 我方行动
    CheckEnd,       ///< 检查战斗是否结束
    Finished        ///< 战斗结束
};

/**
 * @brief 战斗日志条目，记录每回合的经济与战斗明细
 */
struct BattleLogEntry {
    int round;                      ///< 回合数
    bool playerWon;                 ///< 玩家是否胜利
    int remainingUnits;             ///< 剩余单位数量
    int baseGold;                   ///< 基础金币
    int victoryGold;                ///< 胜利奖励金币
    int streakGold;                 ///< 连胜/连败奖励金币
    int killGold;                   ///< 击杀赏金（按星级 2/5/12）
    int interestGold;               ///< 利息金币
    int totalGoldEarned;            ///< 本回合总收入（含利息+战斗奖励）
    int goldBeforeIncome;           ///< 战斗收入前的金币数（即花费后的余额）
    int goldAtRoundStart;           ///< 回合开始时的金币数（发放利息后、花费前）
    int currentGold;                ///< 当前金币（收入后）
    int healthLost;                 ///< 本回合损失生命
    int currentHealth;              ///< 当前生命
    std::string equipmentDropped;   ///< 掉落的装备名称
    std::string summary;            ///< 简短总结文字

    // 金币流：goldAtRoundStart → (花费) → goldBeforeIncome → (战斗收入) → currentGold
    int getGoldSpent() const { return std::max(0, goldAtRoundStart - goldBeforeIncome); }
    int getNetIncome() const { return currentGold - goldAtRoundStart; }

    BattleLogEntry()
        : round(0), playerWon(false), remainingUnits(0)
        , baseGold(0), victoryGold(0), streakGold(0), killGold(0), interestGold(0)
        , totalGoldEarned(0), goldBeforeIncome(0), goldAtRoundStart(0), currentGold(0)
        , healthLost(0), currentHealth(0) {}
};

/**
 * @class   Game
 * @brief   游戏主控制器，管理回合流程、阶段切换、双方操作与战斗动画
 */
class Game {
public:
    Game();

    void initialize();
    void resetGame();
    void startPhaseOne();
    void dragUnitFromBenchToBoard(int benchIndex, int boardX, int boardY);
    void dragUnitOnBoard(int fromX, int fromY, int toX, int toY);
    bool moveBenchUnit(int fromIndex, int toIndex);
    bool swapBenchUnits(int fromIndex, int toIndex);
    bool moveBoardToBench(int boardX, int boardY, int benchIndex);
    void handleIllegalPlacement(int fromX, int fromY, int toX, int toY);
    void spawnEnemyWave(int round);
    void executeEnemyTurn();
    void executeEnemyPreparation();
    void endRound();
    bool isGameOver() const;
    bool isPlayerWonGame() const { return playerWonGame_; }  ///< 玩家是否赢得了整局游戏
    bool isInCombatPhase() const;
    int getCurrentRound() const;
    bool saveGame(const std::string &filename) const;
    bool loadGame(const std::string &filename);
    bool upgradePopulationLimit();
    const Player& getPlayer() const;
    Player& getPlayer();
    const Player& getEnemyPlayer() const;
    Player& getEnemyPlayer();
    const Board& getBoard() const;
    Board& getBoard();           ///< 非 const 版（供 SaveManager 使用）
    const std::vector<std::shared_ptr<Unit>>& getEnemyUnits() const;
    
    // ========== 状态设置器（供 SaveManager 使用） ==========
    void setCurrentRound(int r) { currentRound_ = r; }
    void setInCombatPhase(bool v) { inCombatPhase_ = v; }
    void setGameOver(bool v) { gameOver_ = v; }
    void setPlayerWonGame(bool v) { playerWonGame_ = v; }
    void setWinStreak(int v) { winStreak_ = v; }
    void setLossStreak(int v) { lossStreak_ = v; }
    
    // ========== 存档辅助 ==========
    void rebuildEnemyUnitsFromBoard();  ///< 从棋盘重建 enemyUnits_ 向量
    void clearAllState();               ///< 清理所有游戏状态（用于加载失败回退）
    
    // 商店访问（玩家商店，由 UI 操作）
    Shop& getShop();
    const Shop& getShop() const;
    
    // 敌方商店（由 AI 操作）
    Shop& getEnemyShop();
    const Shop& getEnemyShop() const;
    
    // 战斗结算
    bool checkBattleEnd();
    void resolveBattle();

    // ========== 逐步战斗动画支持 ==========
    void startCombatAnimation();           ///< 开始逐步战斗动画
    bool advanceCombatAnimation();         ///< 推进到下一步，返回 true 表示还有更多步骤
    CombatStep getCurrentCombatStep() const { return currentCombatStep_; }
    bool isEnemyTurnNow() const { return roundRobinIsEnemy_; }           ///< 当前是否为敌方行动轮
    int getCombatEnemyIdx() const { return combatEnemyIdx_; }            ///< 当前敌方行动索引
    int getCombatPlayerIdx() const { return combatPlayerIdx_; }          ///< 当前玩家行动索引
    const std::vector<std::shared_ptr<Unit>>& getCombatPlayerSnap() const { return combatPlayerSnap_; }
    void setCombatStep(CombatStep step) { currentCombatStep_ = step; }
    void setCombatSubStep(int sub) { combatPlayerIdx_ = sub; }          ///< 兼容旧存档
    void setCombatUnitIndex(int idx) { combatEnemyIdx_ = idx; }         ///< 兼容旧存档
    
    // 分步战斗
    void deployEnemyUnits();
    bool doEnemyMoveStep();
    
    // ========== 战斗日志 ==========
    const std::vector<BattleLogEntry>& getBattleLog() const { return battleLog_; }
    void addBattleLogEntry(const BattleLogEntry& entry) { battleLog_.push_back(entry); }
    int getWinStreak() const { return winStreak_; }
    int getLossStreak() const { return lossStreak_; }
    
    // ========== AI 行为日志 ==========
    void addAiBehaviorLog(const std::string& msg);
    const std::vector<std::string>& getAiBehaviorLog() const { return aiBehaviorLog_; }
    void clearAiBehaviorLog() { aiBehaviorLog_.clear(); }
    void clearBattleLog() { battleLog_.clear(); aiBehaviorLog_.clear(); aiIntentMap_.clear(); }
    
    /**
     * @brief AI 意图信息（用于在棋盘上显示图标）
     */
    struct AiIntentInfo {
        std::string action;      ///< "移动"、"攻击"、"待机"
        std::string targetName;  ///< 目标单位名称
        int targetX = -1;        ///< 目标行坐标
        int targetY = -1;        ///< 目标列坐标
    };
    const std::map<std::shared_ptr<Unit>, AiIntentInfo>& getAiIntentMap() const { return aiIntentMap_; }
    void setAiIntent(std::shared_ptr<Unit> unit, const AiIntentInfo& info);
    void clearAiIntentMap() { aiIntentMap_.clear(); }
    
    // 显示 AI 信息开关
    void setShowAiInfo(bool show) { showAiInfo_ = show; }
    bool getShowAiInfo() const { return showAiInfo_; }

    // ========== 战斗倍速 ==========
    int getCombatSpeed() const { return combatSpeed_; }
    void setCombatSpeed(int s) { combatSpeed_ = s; }
    int cycleCombatSpeed() { combatSpeed_ = (combatSpeed_ == 1) ? 2 : (combatSpeed_ == 2) ? 4 : 1; return combatSpeed_; }
    
    // 装备掉落动画
    bool consumeEquipDropAnim() { bool v = equipDropAnimPending_; equipDropAnimPending_ = false; return v; }

    // 大招全屏文字通知
    void setUltimateUsed(const std::string& name, const std::string& charType, int unitX, int unitY) { 
        ultimateUsedName_ = name; ultimateUsedChar_ = charType; ultimateUsedX_ = unitX; ultimateUsedY_ = unitY; 
    }
    std::string consumeUltimateName() { std::string s = ultimateUsedName_; ultimateUsedName_.clear(); return s; }
    std::string consumeUltimateChar() { std::string s = ultimateUsedChar_; ultimateUsedChar_.clear(); return s; }
    bool hasUltimateUsed() const { return !ultimateUsedName_.empty(); }
    int getUltimateUnitX() const { return ultimateUsedX_; }
    int getUltimateUnitY() const { return ultimateUsedY_; }

    /**
     * @brief 金币收入明细跟踪
     */
    struct GoldBreakdown {
        int baseGold = 5;
        int victoryGold = 0;
        int streakGold = 0;
        int interestGold = 0;
        int totalBeforeGold = 0;
    };
    GoldBreakdown lastGoldBreakdown_;

    // ========== 状态机系统 ==========
    StateMachineManager& getStateMachineManager();
    AttackOrderManager& getAttackOrderManager();

private:
    Player player_;
    Player enemyPlayer_;
    Shop shop_;           ///< 玩家商店
    Shop enemyShop_;      ///< 敌方商店
    Board board_;
    int currentRound_;
    int maxRounds_;
    bool inCombatPhase_;
    bool gameOver_;
    bool playerWonGame_ = false;  ///< 玩家是否赢得整局（敌方 HP 归零）vs 玩家损失整局（我方 HP 归零）
    int winStreak_;
    int lossStreak_;
    int goldAtRoundStart_ = 0;    ///< 回合开始时的金币（利息后），用于计算实际支出
    std::vector<BattleLogEntry> battleLog_;
    std::vector<std::shared_ptr<Unit>> enemyUnits_;

    // ========== 战斗动画状态（交替行动制） ==========
    CombatStep currentCombatStep_ = CombatStep::None;
    int combatEnemyIdx_ = 0;      ///< 当前微回合敌方单位索引
    int combatPlayerIdx_ = 0;     ///< 当前微回合玩家单位索引
    int combatBattleRound_ = 0;   ///< 当前微回合
    bool roundRobinIsEnemy_ = true;  ///< 当前是否为敌方行动轮（交替轮询）
    std::vector<std::shared_ptr<Unit>> combatPlayerSnap_;  ///< 当前微回合的玩家单位快照

    // ========== 状态机系统 ==========
    StateMachineManager stateMachineManager_;
    AttackOrderManager attackOrderManager_;
    
    // ========== AI 相关 ==========
    std::vector<std::string> aiBehaviorLog_;
    std::map<std::shared_ptr<Unit>, AiIntentInfo> aiIntentMap_;
    bool showAiInfo_ = true;
    int combatSpeed_ = 1;             ///< 战斗速度倍率 1x/2x/4x
    int combatKillGold_ = 0;          ///< 战斗中击杀敌方单位获得的金币累计
    int combatKilledCount_ = 0;       ///< 战斗中击杀敌方单位数量（用于经验计算）
    bool equipDropAnimPending_ = false;  ///< 装备掉落动画挂起标志
    std::string ultimateUsedName_;
    std::string ultimateUsedChar_;
    int ultimateUsedX_ = -1;
    int ultimateUsedY_ = -1;
    
    // ========== 辅助函数 ==========
    void cleanupDeadUnits();
    void transitionToNextRound();
    void registerAllUnitsToStateMachine();
    void executePlayerUnitAction(std::shared_ptr<Unit> unit);
    void executeEnemyUnitAction(std::shared_ptr<Unit> enemy);
};
