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

  bool isCloudsSheet = (sheetPath.find("Background 3") != std::string::npos);

  unsigned w = strip.getSize().x;
  unsigned h = strip.getSize().y;

  for (unsigned y = 0; y < h; ++y) {
    for (unsigned x = 0; x < w; ++x) {
      sf::Color px = strip.getPixel(x, y);

      if (isCloudsSheet) {
        bool isWhite = (px.r > 220 && px.g > 220 && px.b > 220);
        bool isBlack = (px.r < 30  && px.g < 30  && px.b < 30);
        if (!isWhite && !isBlack) {
          strip.setPixel(x, y, sf::Color::Transparent);
        }
      } else {
        bool isBlueSky = (px.b > 180) || (px.b > px.r + 30 && px.b > px.g + 30) ||
                         (px.r == 136 && px.g == 134 && px.b == 255) ||
                         (px.r == 108 && px.g == 106 && px.b == 255) ||
                         (px.r == 146 && px.g == 144 && px.b == 255) ||
                         (px.r == 92  && px.g == 148 && px.b == 252);

        bool isHeaderOrDivider = (px.r == 0   && px.g == 41  && px.b == 140) ||
                                 (px.r == 16  && px.g == 148 && px.b == 0)   ||
                                 (px.r == 173 && px.g == 173 && px.b == 173);

        if (isBlueSky || isHeaderOrDivider) {
          strip.setPixel(x, y, sf::Color::Transparent);
        }
      }
    }
  }

  return elem.texture.loadFromImage(strip);
}

void Background::load(LevelTheme theme, float levelWidth, bool useLavaBackground) {
  m_elements.clear();

  if (useLavaBackground) {
    m_topColor    = sf::Color(42, 10, 8);
    m_bottomColor = sf::Color(18, 3, 2);

    SceneryElement lavaBackground;
    if (lavaBackground.texture.loadFromFile("assets/textures/lava_background.png")) {
      lavaBackground.worldY   = 0.0f;
      lavaBackground.parallax = 0.35f;
      const float scale = static_cast<float>(WINDOW_HEIGHT) /
                          lavaBackground.texture.getSize().y;
      lavaBackground.scale = {scale, scale};
      m_elements.push_back(std::move(lavaBackground));
    }
    return;
  }

  switch (theme) {
  case LevelTheme::Underground: {
    // Level 2: Dark Fantasy World background
    m_topColor    = sf::Color(20, 10, 35);
    m_bottomColor = sf::Color(8, 4, 16);

    SceneryElement darkFantasyBg;
    if (darkFantasyBg.texture.loadFromFile("assets/textures/dark_fantasy_bg.png")) {
      darkFantasyBg.worldY   = 0.0f;  // Spans full window height
      darkFantasyBg.parallax = 0.35f; // Mystical slow parallax drift
      m_elements.push_back(std::move(darkFantasyBg));
    }
    return;
  }

  case LevelTheme::Castle: {
    // Level 3: Romance Fantasy World background
    m_topColor    = sf::Color(255, 160, 140);
    m_bottomColor = sf::Color(255, 200, 150);

    SceneryElement romanceFantasyBg;
    if (romanceFantasyBg.texture.loadFromFile("assets/textures/romance_fantasy_bg.png")) {
      romanceFantasyBg.worldY   = 0.0f;  // Spans full window height
      romanceFantasyBg.parallax = 0.35f; // Dreamlike slow parallax drift
      m_elements.push_back(std::move(romanceFantasyBg));
    }
    return;
  }

  case LevelTheme::Overworld:
  default: {
    // Level 1: Dystopian Fantasy World background
    m_topColor    = sf::Color(35, 18, 40);
    m_bottomColor = sf::Color(65, 30, 20);

    SceneryElement dystopianFantasyBg;
    if (dystopianFantasyBg.texture.loadFromFile("assets/textures/dystopian_fantasy_bg.png")) {
      dystopianFantasyBg.worldY   = 0.0f;  // Spans full window height
      dystopianFantasyBg.parallax = 0.35f; // Atmospheric slow parallax drift
      m_elements.push_back(std::move(dystopianFantasyBg));
    }
    return;
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

    const float texW = static_cast<float>(elem.texture.getSize().x) * elem.scale.x;

    // Apply parallax: strip scrolls slower than the camera by `parallax` factor.
    float scrollX = cameraCenterX * elem.parallax;

    // Tile the strip to cover the full window width seamlessly.
    float startX = left + std::fmod(left - scrollX, texW);
    if (startX > left) startX -= texW;

    sf::Sprite sprite(elem.texture);
    sprite.setScale(elem.scale);
    // Soften/dim background scenery so foreground character, blocks, and enemies stand out clearly
    sprite.setColor(sf::Color(150, 150, 165, 200));

    float x = startX;
    while (x < left + static_cast<float>(WINDOW_WIDTH)) {
      sprite.setPosition(x, elem.worldY);
      window.draw(sprite);
      x += texW;
    }
  }
}
