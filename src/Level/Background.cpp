#include "Level/Background.hpp"
#include "Physics/PhysicsConstants.hpp"

void Background::load(LevelTheme theme) {
  switch (theme) {
  case LevelTheme::Underground:
    m_topColor = sf::Color(10, 10, 30);
    m_bottomColor = sf::Color(0, 0, 0);
    break;
  case LevelTheme::Castle:
    m_topColor = sf::Color(40, 12, 12);
    m_bottomColor = sf::Color(10, 0, 0);
    break;
  case LevelTheme::Overworld:
  default:
    m_topColor = sf::Color(92, 148, 252);
    m_bottomColor = sf::Color(180, 220, 255);
    break;
  }
}

void Background::render(sf::RenderWindow &window, float cameraCenterX) {
  float left = cameraCenterX - WINDOW_WIDTH / 2.0f;

  sf::VertexArray gradient(sf::TriangleStrip, 4);
  gradient[0] = sf::Vertex({left, 0.0f}, m_topColor);
  gradient[1] = sf::Vertex({left + WINDOW_WIDTH, 0.0f}, m_topColor);
  gradient[2] = sf::Vertex({left, static_cast<float>(WINDOW_HEIGHT)}, m_bottomColor);
  gradient[3] = sf::Vertex({left + WINDOW_WIDTH, static_cast<float>(WINDOW_HEIGHT)},
                            m_bottomColor);

  window.draw(gradient);
}