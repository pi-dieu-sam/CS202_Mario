#pragma once
#include <SFML/Graphics.hpp>

/// Camera — follows the player horizontally with clamping to level bounds.
/// Uses sf::View to offset rendering.
class Camera {
public:
    Camera();

    /// Set the level boundaries (in pixels).
    void setLevelBounds(float levelWidth, float levelHeight);

    /// Update the camera to follow a target position.
    void update(const sf::Vector2f& targetPos);

    /// Apply this camera's view to the window.
    void applyTo(sf::RenderWindow& window) const;

    /// Get the current view (for coordinate conversions).
    const sf::View& getView() const;

    /// Reset to default view.
    void reset();

private:
    sf::View m_view;
    float    m_levelWidth  = 0.0f;
    float    m_levelHeight = 0.0f;
};
