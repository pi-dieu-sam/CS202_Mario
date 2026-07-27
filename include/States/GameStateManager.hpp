#ifndef GAME_STATE_MANAGER_HPP
#define GAME_STATE_MANAGER_HPP

#include "GameState.hpp"
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

class Engine;

/**
 * @brief Manages the state stack and controls current active state (State Pattern).
 */
class GameStateManager {
public:
    explicit GameStateManager(Engine& engine);
    ~GameStateManager() = default;

    void pushState(std::unique_ptr<GameState> state);
    void popState();
    void changeState(std::unique_ptr<GameState> state);
    void clearStates();

    void handleInput(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderWindow& window);

    bool isEmpty() const { return m_states.empty(); }
    GameState* getCurrentState() const { return m_states.empty() ? nullptr : m_states.back().get(); }

    Engine& getEngine() { return m_engine; }

private:
    Engine& m_engine;
    std::vector<std::unique_ptr<GameState>> m_states;
};

#endif // GAME_STATE_MANAGER_HPP
