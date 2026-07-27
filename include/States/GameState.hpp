#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <SFML/Graphics.hpp>
#include <memory>

class GameStateManager;

/**
 * @brief Abstract base class for all game states (State Pattern).
 */
class GameState {
public:
    explicit GameState(GameStateManager& stateManager) : m_stateManager(stateManager) {}
    virtual ~GameState() = default;

    virtual void init() = 0;
    virtual void handleInput(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderWindow& window) = 0;

    virtual void pause() {}
    virtual void resume() {}

protected:
    GameStateManager& m_stateManager;
};

#endif // GAME_STATE_HPP
