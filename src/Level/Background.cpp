#include "Level/Background.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>

bool Background::loadStrip(SceneryElement &elem,
                            const std::string &sheetPath,
                            sf::IntRect srcRect) {
  sf::Image sheet;
  if (!sheet.loadFromFile(sheetPath)) return false;

  sf::Image strip;
  strip.create(static_cast<unsigned>(srcRect.width),
               static_cast<unsigned>(srcRect.height),
               sf::Color::Transparent);

  strip.copy(sheet, 0, 0, srcRect, true);

  // Filter out all non-graphic backdrop pixels from the sprite sheet:
  // - For Clouds sheet (Background 3): keep ONLY white (255,255,255) cloud fills and black (0,0,0) outlines.
  // - For Mountains/Trees sheets: keep ONLY green/brown graphics and black outlines; mask out all blue/purple sheet tiles.
  bool isCloudsSheet = (sheetPath.find("Background 3") != std::string::npos);

  unsigned w = strip.getSize().x;
  unsigned h = strip.getSize().y;

  for (unsigned y = 0; y < h; ++y) {
    for (unsigned x = 0; x < w; ++x) {
      sf::Color px = strip.getPixel(x, y);

      if (isCloudsSheet) {
        // Keep ONLY pure white cloud fill or pure black outlines
        bool isWhite = (px.r == 255 && px.g == 255 && px.b == 255);
        bool isBlack = (px.r == 0   && px.g == 0   && px.b == 0);
        if (!isWhite && !isBlack) {
          strip.setPixel(x, y, sf::Color::Transparent);
        }
      } else {
        // Mask out any blue/purple sheet grid tiles or header bar colors
        bool isBlueish = (px.b > 200 && px.b > px.g) || (px.r < 180 && px.b > 180);
        bool isHeaderOrDivider = (px.r == 0 && px.g == 41 && px.b == 140) ||
                                 (px.r == 16 && px.g == 148 && px.b == 0) ||
                                 (px.r == 173 && px.g == 173 && px.b == 173);
        if (isBlueish || isHeaderOrDivider) {
          strip.setPixel(x, y, sf::Color::Transparent);
        }
      }
    }
  }

  return elem.texture.loadFromImage(strip);
}

void Background::load(LevelTheme theme, float levelWidth) {
  m_elements.clear();

  switch (theme) {
  case LevelTheme::Underground:
    // Authentic dark cave backdrop for Level 2
    m_topColor    = sf::Color(12, 12, 32);
    m_bottomColor = sf::Color(0, 0, 8);
    // Underground cave has no sky clouds or hills — pure dark cave atmosphere
    return;

  case LevelTheme::Castle:
    // Authentic deep red fortress backdrop for Level 3
    m_topColor    = sf::Color(36, 8, 8);
    m_bottomColor = sf::Color(12, 0, 0);
    // Castle fortress interior has no sky clouds or hills — atmospheric fortress
    return;

  case LevelTheme::Overworld:
  default:
    // Authentic Overworld sky blue for Level 1
    m_topColor    = sf::Color(92, 148, 252);
    m_bottomColor = sf::Color(92, 148, 252);
    break;
  }

  // ── Overworld Scenery Elements (Level 1) ──────────────────────────────────

  // 1. Clouds (Background 3: cropped precisely to y=40..82 to skip top header bar)
  {
    SceneryElement clouds;
    if (loadStrip(clouds,
                  "assets/textures/NES - Super Mario Bros. - Backgrounds - Background 3 (Clouds).png",
                  sf::IntRect(0, 40, 734, 42))) {
      clouds.worldY   = 50.f;     // Positioned gracefully in upper sky
      clouds.parallax = 0.35f;    // Parallax scrolling
      m_elements.push_back(std::move(clouds));
    }
  }

  // 2. Green Hills (Background 1: cropped precisely to y=170..215)
  {
    SceneryElement hills;
    if (loadStrip(hills,
                  "assets/textures/NES - Super Mario Bros. - Backgrounds - Background 1 (Mountains).png",
                  sf::IntRect(0, 170, 734, 45))) {
      hills.worldY   = static_cast<float>(WINDOW_HEIGHT) - 3.5f * TILE_SIZE - 45.f;
      hills.parallax = 0.55f;
      m_elements.push_back(std::move(hills));
    }
  }

  // 3. Green Bushes (Background 2: cropped precisely to y=170..215)
  {
    SceneryElement bushes;
    if (loadStrip(bushes,
                  "assets/textures/NES - Super Mario Bros. - Backgrounds - Background 2 (Trees).png",
                  sf::IntRect(0, 170, 734, 45))) {
      bushes.worldY   = static_cast<float>(WINDOW_HEIGHT) - 2.5f * TILE_SIZE - 45.f;
      bushes.parallax = 0.7f;
      m_elements.push_back(std::move(bushes));
    }
  }
}

void Background::render(sf::RenderWindow &window, float cameraCenterX) {
  float left = cameraCenterX - static_cast<float>(WINDOW_WIDTH) / 2.0f;

  // ── Sky / Cave / Castle Gradient ─────────────────────────────────────────
  sf::VertexArray gradient(sf::TriangleStrip, 4);
  gradient[0] = sf::Vertex({left, 0.0f},                                         m_topColor);
  gradient[1] = sf::Vertex({left + static_cast<float>(WINDOW_WIDTH), 0.0f},      m_topColor);
  gradient[2] = sf::Vertex({left, static_cast<float>(WINDOW_HEIGHT)},            m_bottomColor);
  gradient[3] = sf::Vertex({left + static_cast<float>(WINDOW_WIDTH),
                             static_cast<float>(WINDOW_HEIGHT)},                  m_bottomColor);
  window.draw(gradient);

  // ── Scenery Strips ───────────────────────────────────────────────────────
  for (const SceneryElement &elem : m_elements) {
    if (elem.texture.getSize().x == 0) continue;

    const float texW = static_cast<float>(elem.texture.getSize().x);

    // Apply parallax: strip scrolls slower than the camera by `parallax` factor.
    float scrollX = cameraCenterX * elem.parallax;

    // Tile the strip to cover the full window width seamlessly.
    float startX = left + std::fmod(left - scrollX, texW);
    if (startX > left) startX -= texW;

    sf::Sprite sprite(elem.texture);

    float x = startX;
    while (x < left + static_cast<float>(WINDOW_WIDTH)) {
      sprite.setPosition(x, elem.worldY);
      window.draw(sprite);
      x += texW;
    }
  }
}