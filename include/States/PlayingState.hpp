#pragma once
#include "GameState.hpp"
#include "../Core/InputHandler.hpp"
#include "../Core/Camera.hpp"
#include "../Observers/EventManager.hpp"
#include <memory>

// Forward declarations
class Level;
class Player;
class HUD;

/// PlayingState — the main gameplay state.
/// Loads and runs a level, handles input, physics, and rendering.
enum class LevelTransitionStage {
    Inactive,
    FlagSlide,
    CastleEntry,
    ScoreCount,
    Finished
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
    void startLevelTransition();
    void updateLevelTransition(float dt);
    void finishLevelTransition();
    void onPlayerDeath();

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
    int m_transitionBonusScore = 0;
    int m_transitionDisplayScore = 0;
};
