#pragma once

#include "Enemy.hpp"

/// Troopa is an airborne Koopa variant. It flies horizontally without gravity
/// and reverses direction whenever it reaches a solid obstacle.
class Troopa final : public Enemy {
public:
  Troopa();

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  void onStomped() override;
  void kill() override;
};
