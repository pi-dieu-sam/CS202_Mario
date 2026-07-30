#pragma once
#include "LevelTheme.hpp"
#include <SFML/Graphics.hpp>

/// Background — themed sky backdrop for a Level, drawn before everything
/// else each frame. The asset pack has no wide seamless parallax sheets
/// (single-sprite files only), so this is a flat top-to-bottom gradient per
/// theme rather than scrolling art.
class Background {
public:
  void load(LevelTheme theme);
  void render(sf::RenderWindow &window, float cameraCenterX);

private:
  sf::Color m_topColor;
  sf::Color m_bottomColor;
};