#pragma once
#include "GameState.hpp"
#include "../Core/InputHandler.hpp"
#include "../Core/Camera.hpp"
#include "../Level/LevelTheme.hpp"
#include "../Entities/Player.hpp"
#include "../Observers/EventManager.hpp"
#include <memory>

// Forward declarations
class Level;
class HUD;

/// PlayingState — the main gameplay state.
/// Loads and runs a level, handles input, physics, and rendering.
enum class LevelTransitionStage {
    Inactive,
    FlagSlide,
    CastleEntry,
    FlagpoleScoreCount,
    TimeBonusCount,
    Finished,
    PipeEnter,
    PipeReturn
};

class PlayingState : public GameState {
public:
    PlayingState();
    ~PlayingState();

    void onEnter() override;
    void onExit() override;
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
    void startLevelTransition();
    void updateLevelTransition(float dt);
    void finishLevelTransition();
    void onPlayerDeath();

    std::string getLevelPath(int levelNumber, bool secretRoom) const;
    LevelTheme   getLevelTheme(int levelNumber, bool secretRoom) const;

    std::unique_ptr<Level>  m_level;
    Player*                 m_player = nullptr; // owned by m_level
    std::unique_ptr<HUD>    m_hud;
    InputHandler            m_input;
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
    int m_transitionDisplayScore = 0;
    int m_mainLevelNumber = 1;
    bool m_inSecretRoom = false;
    sf::Vector2f m_pipeReturnPosition = {0.0f, 0.0f};
    PowerUpState m_pipeReturnPowerUp = PowerUpState::Small;
};
