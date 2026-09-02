#pragma once
#include "GameObject.hpp"
#include <SFML/Graphics.hpp>

/// Fireball — projectile shot by the player in Fire state.
/// Bounces along the ground and destroys enemies on contact.
class Fireball : public GameObject {
public:
    Fireball(float x, float y, int direction);

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

    void onCollision(GameObject& other) override;

    /// Count a solid-surface hit; the fireball disappears after 3 hits.
    void noteSurfaceHit();

private:
    sf::Sprite      m_sprite;
    float           m_lifetime = 3.0f; // seconds before despawn
    int             m_direction = 1;   // 1=right, -1=left
    float           m_animTimer = 0.0f;
    int             m_animFrame = 0;
    int             m_surfaceHits = 0;
};
