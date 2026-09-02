#include "Core/Camera.hpp"
#include "Core/Game.hpp"
#include "Physics/PhysicsConstants.hpp"

Camera::Camera() {
    m_view.setSize(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
    m_view.setCenter(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
}

void Camera::setLevelBounds(float levelWidth, float levelHeight) {
    m_levelWidth  = levelWidth;
    m_levelHeight = levelHeight;
}

void Camera::update(const sf::Vector2f& targetPos) {
    float halfW = WINDOW_WIDTH / 2.0f;
    float halfH = WINDOW_HEIGHT / 2.0f;

    float camX = targetPos.x;
    float camY = halfH; // Fixed vertical — classic Mario style

    // Clamp horizontal to level bounds
    if (camX < halfW) camX = halfW;
    if (m_levelWidth > 0 && camX > m_levelWidth - halfW) camX = m_levelWidth - halfW;

    // Clamp vertical (if level is taller than window)
    if (m_levelHeight > 0) {
        if (camY < halfH) camY = halfH;
        if (camY > m_levelHeight - halfH) camY = m_levelHeight - halfH;
    }

    m_view.setCenter(camX, camY);
}

void Camera::update(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
    float halfW = WINDOW_WIDTH / 2.0f;
    float halfH = WINDOW_HEIGHT / 2.0f;

    float camX = (pos1.x + pos2.x) / 2.0f;
    float camY = halfH;

    // Clamp horizontal to level bounds
    if (camX < halfW) camX = halfW;
    if (m_levelWidth > 0 && camX > m_levelWidth - halfW) camX = m_levelWidth - halfW;

    if (m_levelHeight > 0) {
        if (camY < halfH) camY = halfH;
        if (camY > m_levelHeight - halfH) camY = m_levelHeight - halfH;
    }

    m_view.setCenter(camX, camY);
}

void Camera::applyTo(sf::RenderWindow& window) const {
    sf::View worldView = m_view;
    worldView.setViewport(Game::getInstance().getUiView().getViewport());
    window.setView(worldView);
}

const sf::View& Camera::getView() const {
    return m_view;
}

void Camera::reset() {
    m_view.setSize(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
    m_view.setCenter(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
}
