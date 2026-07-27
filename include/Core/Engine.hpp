#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// Forward declaration
class GameStateManager;

/**
 * @brief Core Game Engine class managing window initialization, main loop, fixed timing, and states.
 */
class Engine {
public:
    Engine(unsigned int width = 800, unsigned int height = 600, const std::string& title = "Super Mario Bros - CS202");
    ~Engine();

    /**
     * @brief Start and run the main game loop.
     */
    void run();

    sf::RenderWindow& getWindow() { return m_window; }
    GameStateManager& getStateManager() { return *m_stateManager; }

private:
    void processEvents();
    void update(float dt);
    void render();

    sf::RenderWindow m_window;
    std::unique_ptr<GameStateManager> m_stateManager;
    bool m_isRunning;
    sf::Clock m_clock;
};

#endif // ENGINE_HPP
