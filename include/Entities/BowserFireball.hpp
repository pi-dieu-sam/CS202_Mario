#pragma once

#include "GameObject.hpp"

/// A hostile three-frame fireball emitted by Bowser. It travels in a straight
/// line, disappears on solid terrain, and is handled as lethal to players by
/// Level's collision loop.
class BowserFireball final : public GameObject {
public:
  BowserFireball(float x, float y, int direction = -1);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

private:
  sf::Sprite m_sprite;
  float m_lifetime = 5.0f;
  float m_animTimer = 0.0f;
  int m_animFrame = 0;
};
