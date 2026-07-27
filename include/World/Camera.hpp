#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <SFML/Graphics.hpp>

/**
 * @brief Dynamic game camera following the active player character across the level.
 */
class Camera {
public:
    Camera(float width = 800.0f, float height = 600.0f)
        : m_view({400.0f, 300.0f}, {width, height}) {}

    ~Camera() = default;

    void update(const sf::Vector2f& targetPos, float levelWidth = 3200.0f) {
        float cameraX = targetPos.x;
        // Clamp camera so it doesn't view past left edge
        if (cameraX < 400.0f) cameraX = 400.0f;
        if (cameraX > levelWidth - 400.0f) cameraX = levelWidth - 400.0f;

        m_view.setCenter({cameraX, 300.0f});
    }

    const sf::View& getView() const { return m_view; }

private:
    sf::View m_view;
};

#endif // CAMERA_HPP
