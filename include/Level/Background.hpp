#pragma once
#include "LevelTheme.hpp"
#include <SFML/Graphics.hpp>
#include <vector>

/// SceneryElement — a single tiling strip of background art drawn at a
/// given world Y offset with a parallax scroll factor relative to the camera.
struct SceneryElement {
  sf::Texture texture;  ///< Owns the loaded texture (one row from a sheet)
  float worldY = 0.0f;  ///< Y position in screen space (pixels from top)
  float parallax = 1.0f;///< 0=fixed, 1=scroll 1:1 with camera, 0.5=half speed
  float repeatWidth = 0.f; ///< horizontal tile repeat width (texture natural width if 0)
};

/// Background — themed sky backdrop for a Level, drawn before everything
/// else each frame. Renders a flat gradient plus per-theme parallax scenery
/// strips (clouds, hills, bushes) extracted from the NES background sheets.
class Background {
public:
  void load(LevelTheme theme, float levelWidth);
  void render(sf::RenderWindow &window, float cameraCenterX);

private:
  sf::Color m_topColor;
  sf::Color m_bottomColor;
  std::vector<SceneryElement> m_elements;

  /// Extract a horizontal strip from a sprite-sheet image already loaded in
  /// memory and store it in `elem.texture`. `srcRect` is the pixel region
  /// inside `sheetPath` to crop; `elem.worldY` and `elem.parallax` are set
  /// by the caller after this returns.
  bool loadStrip(SceneryElement &elem,
                 const std::string &sheetPath,
                 sf::IntRect srcRect);
};