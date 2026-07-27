#ifndef PLAYING_STATE_HPP
#define PLAYING_STATE_HPP

#include "GameState.hpp"
#include "Entities/Player.hpp"
#include "World/Level.hpp"
#include "World/Camera.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <optional>

class InputHandler;

class PlayingState : public GameState {
public:
    PlayingState(GameStateManager& stateManager, int levelIndex = 1, const std::string& characterName = "Mario");
    ~PlayingState() override;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    void restartLevel();
    void loadNextLevel();
    void saveCurrentProgress();

    int getScore() const { return m_score; }
    int getCoins() const { return m_coins; }
    int getLives() const { return m_lives; }
    int getLevelIndex() const { return m_levelIndex; }

    void addScore(int points) { m_score += points; }
    void addCoin() { m_coins++; m_score += 100; }
    void addLife() { m_lives++; }
    void playerDied();

private:
    int m_levelIndex;
    std::string m_characterName;

    int m_score;
    int m_coins;
    int m_lives;
    float m_levelTime;

    std::unique_ptr<Player> m_player;
    std::unique_ptr<Level> m_level;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<InputHandler> m_inputHandler;

    sf::Font m_font;
    std::optional<sf::Text> m_hudText;

    void updateHUD();
};

#endif // PLAYING_STATE_HPP
