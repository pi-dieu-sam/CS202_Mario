#pragma once
#include "GameState.hpp"
#include "../Core/InputHandler.hpp"
#include "../Core/Camera.hpp"
#include "../Level/LevelTheme.hpp"
#include "../Entities/Player.hpp"
#include "../Observers/EventManager.hpp"
#include "../UI/ScorePopup.hpp"
#include <memory>

// Forward declarations
class Level;
class HUD;

/// PlayingState — the main gameplay state.
/// Loads and runs a level, handles input, physics, and rendering.
enum class LevelTransitionStage {
    Inactive,
    DeathAnimation,
    FlagSlide,
    CastleEntry,
    TimeBonusCount,
    Finished,
    PipeEnter,
    PipeReturn,
    PipeExit,
    PipeFadeOut,
    PipeFadeIn
};

class PlayingState : public GameState {
public:
    PlayingState();
    ~PlayingState();

    void onEnter() override;
    void onExit() override;
    void onPause() override;
    void onResume() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void loadLevel(int levelNumber);
    void checkLevelComplete();
    bool tryEnterPipe();
    bool tryExitPipe();
    void startPipeTransition(bool enteringSecret);
    void updatePipeTransition(float dt);
    void swapPipeMap();
    void startLevelTransition();
    void beginCastleEntry();
    void updateLevelTransition(float dt);
    void finishLevelTransition();
    void beginPlayerDeath();
    void finishPlayerDeath();

    std::string getLevelPath(int levelNumber, bool secretRoom) const;
    LevelTheme   getLevelTheme(int levelNumber, bool secretRoom) const;

    std::unique_ptr<Level>  m_level;
    Player*                 m_player = nullptr; // owned by m_level
    Player*                 m_player2 = nullptr; // owned by m_level
    std::unique_ptr<HUD>    m_hud;
    ScorePopupManager       m_scorePopups;
    InputHandler            m_inputP1;
    InputHandler            m_inputP2;
    Camera                  m_camera;

    ScopedEventSubscription m_coinSub;
    ScopedEventSubscription m_enemyDefeatedSub;
    ScopedEventSubscription m_playerDiedSub;
    ScopedEventSubscription m_powerUpSub;
    ScopedEventSubscription m_blockHitSub;
    ScopedEventSubscription m_playerDamagedSub;
    ScopedEventSubscription m_levelCompletedSub;

    float m_levelTimer = 0.0f;
    bool  m_levelComplete = false;
    LevelTransitionStage m_transitionStage = LevelTransitionStage::Inactive;
    float m_transitionTimer = 0.0f;
    float m_transitionScoreTimer = 0.0f;
    int m_transitionStartScore = 0;
    int m_transitionFlagpoleBonus = 0;
    int m_transitionTimeBonus = 0;
    int m_transitionRemainingSeconds = 0;
    int m_transitionConvertedTimeScore = 0;
    int m_transitionConvertedFlagpoleScore = 0;
    int m_transitionDisplayScore = 0;
    float m_castleDoorTargetX = 0.0f;
    int m_mainLevelNumber = 1;
    bool m_inSecretRoom = false;
    bool m_pipeTransitionEnteringSecret = false;
    sf::Vector2f m_pipeReturnPosition = {0.0f, 0.0f};
    PowerUpState m_pipeReturnPowerUp = PowerUpState::Small;
    PowerUpState m_pipeReturnPowerUp2 = PowerUpState::Small;
};
