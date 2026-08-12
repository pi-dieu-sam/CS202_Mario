#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

/// ScorePopupManager — draws short-lived score increments in world space.
/// Each score event gets its own popup so simultaneous rewards remain visible.
class ScorePopupManager {
public:
    /// Load the font used by popup text.
    void init();

    /// Add a popup at a world-space position.
    void add(int score, const sf::Vector2f& worldPosition);

    /// Advance popup movement, fading, and lifetime.
    void update(float dt);

    /// Draw popups using the render window's current view.
    void render(sf::RenderWindow& window) const;

    /// Remove all active popups, for example when changing levels.
    void clear();

private:
    struct Popup {
        sf::Text text;
        sf::Vector2f startPosition = {0.0f, 0.0f};
        sf::Vector2f position = {0.0f, 0.0f};
        float age = 0.0f;
    };

    sf::Font* m_font = nullptr;
    std::vector<Popup> m_popups;
};
