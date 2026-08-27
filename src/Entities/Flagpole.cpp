#include "Entities/Flagpole.hpp"
#include "Physics/PhysicsConstants.hpp"

Flagpole::Flagpole() {
    m_type = ObjectType::Flagpole;
}

Flagpole::Flagpole(float x, float y) {
    m_type = ObjectType::Flagpole;
    m_position = {x, y};

    // Pole — tall thin rectangle
    m_pole.setSize(sf::Vector2f(6.0f, TILE_SIZE * 8));
    m_pole.setPosition(x + TILE_SIZE / 2.0f - 3.0f, y - TILE_SIZE * 7);
    m_pole.setFillColor(sf::Color(180, 180, 180));

    // Flag — small green triangle pennant, tip touching the pole
    m_flag.setPointCount(3);
    m_flag.setPoint(0, {0.0f, 0.0f});
    m_flag.setPoint(1, {TILE_SIZE * 0.8f, TILE_SIZE * 0.25f});
    m_flag.setPoint(2, {0.0f, TILE_SIZE * 0.5f});
    m_flag.setFillColor(sf::Color(60, 180, 75));
    m_flagPos = {x + TILE_SIZE / 2.0f + 3.0f, y - TILE_SIZE * 7};
    m_flag.setPosition(m_flagPos);

    // Ball cap at the top of the pole
    m_ball.setRadius(5.0f);
    m_ball.setFillColor(sf::Color(230, 230, 230));
    m_ball.setOrigin(5.0f, 5.0f);
    m_ball.setPosition(m_pole.getPosition().x + 3.0f, m_pole.getPosition().y);
}

void Flagpole::update(float dt) {
    if (m_reached) {
        // Drop flag animation
        float flagTarget = m_position.y - TILE_SIZE;
        if (m_flagPos.y < flagTarget) {
            m_flagPos.y += 150.0f * dt;
            if (m_flagPos.y > flagTarget) {
                m_flagPos.y = flagTarget;
            }
            m_flag.setPosition(m_flagPos);
        }
    }
}

void Flagpole::draw(sf::RenderWindow& window) {
    if (!m_active) return;
    window.draw(m_pole);
    window.draw(m_ball);
    window.draw(m_flag);
}

sf::FloatRect Flagpole::getBounds() const {
    return sf::FloatRect(m_position.x, m_position.y - TILE_SIZE * 7,
                         TILE_SIZE, TILE_SIZE * 8);
}

int Flagpole::calculateScore(float playerY) const {
    float poleTop    = m_position.y - TILE_SIZE * 7;
    float poleBottom = m_position.y;
    float range      = poleBottom - poleTop;
    float relPos     = (playerY - poleTop) / range;

    if (relPos < 0.1f)  return 5000;
    if (relPos < 0.3f)  return 2000;
    if (relPos < 0.5f)  return 800;
    if (relPos < 0.7f)  return 400;
    return 100;
}

float Flagpole::getSlideAnchorX() const {
    return m_position.x + TILE_SIZE / 2.0f;
}

float Flagpole::getSlideEndY() const {
    // Player positions reserve a two-tile-tall anchor even for Small form.
    // This keeps either form's feet just above the ground tile below the pole.
    return m_position.y - TILE_SIZE;
}

bool Flagpole::isReached() const { return m_reached; }
bool Flagpole::isFlagDropComplete() const {
    return m_reached && m_flagPos.y >= m_position.y - TILE_SIZE;
}
void Flagpole::setReached(bool reached) { m_reached = reached; }
