#pragma once
#include "GameObject.hpp"
#include <SFML/Graphics.hpp>

/// Flagpole — end-of-level trigger. Score depends on grab height.
class Flagpole : public GameObject {
public:
    Flagpole();
    Flagpole(float x, float y);

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

    /// Calculate score bonus based on where the player grabs the pole.
    int calculateScore(float playerY) const;

    /// World-space target coordinates for the player completion cutscene.
    float getSlideAnchorX() const;
    float getSlideEndY() const;

    bool isReached() const;
    void setReached(bool reached);

private:
    // The asset pack has no standalone small flag/ball icon (only tall
    // pre-assembled pole+flag renders), so — like the pole already was —
    // the flag and its ball cap are drawn as plain shapes rather than
    // sprites.
    sf::RectangleShape m_pole;
    sf::ConvexShape m_flag;
    sf::CircleShape m_ball;
    sf::Vector2f m_flagPos;
    bool  m_reached   = false;
    float m_flagDropY = 0.0f;
};
