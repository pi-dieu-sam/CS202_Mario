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
        // Keep ONLY pure white cloud fill or pure black outlines
        bool isWhite = (px.r > 220 && px.g > 220 && px.b > 220);
        bool isBlack = (px.r < 30  && px.g < 30  && px.b < 30);
        if (!isWhite && !isBlack) {
          strip.setPixel(x, y, sf::Color::Transparent);
        }
      } else {
        // Mask out blue sheet background, header bars, and border artifacts
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

void Background::load(LevelTheme theme, float levelWidth) {
  m_elements.clear();

  const float groundTopY = static_cast<float>(WINDOW_HEIGHT) - 2.0f * TILE_SIZE; // 544px

  switch (theme) {
  case LevelTheme::Underground: {
    // Atmospheric Dark Fantasy World background for Level 2
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
    // Rich NES Castle fortress gradient (dark charcoal to deep crimson glow)
    m_topColor    = sf::Color(15, 2, 2);
    m_bottomColor = sf::Color(55, 12, 12);

    // Create upper castle battlements & window arch scenery banner (Y = 30..130px)
    sf::Image castleBanner;
    castleBanner.create(512, 100, sf::Color::Transparent);

    sf::Image castleBrick;
    if (castleBrick.loadFromFile("assets/textures/SMB_Castle_Brick_Block.png")) {
      for (int x = 0; x < 512; x += 16) {
        castleBanner.copy(castleBrick, x, 0); // Top rim
      }
      // Add fortress window arch pillars
      for (int x = 48; x < 460; x += 112) {
        for (int y = 16; y < 80; y += 16) {
          castleBanner.copy(castleBrick, x, y);
          castleBanner.copy(castleBrick, x + 16, y);
        }
      }
    }

    SceneryElement castleBattlements;
    if (castleBattlements.texture.loadFromImage(castleBanner)) {
      castleBattlements.worldY   = 35.0f; // Upper sky region
      castleBattlements.parallax = 0.4f;
      m_elements.push_back(std::move(castleBattlements));
    }
    return;
  }

  case LevelTheme::Overworld:
  default:
    // Authentic NES Overworld sky blue gradient
    m_topColor    = sf::Color(92, 148, 252);
    m_bottomColor = sf::Color(92, 148, 252);
    break;
  }

  // ── Overworld Scenery Elements (Level 1) ──────────────────────────────────

  // 1. Upper Sky Clouds (Background 3)
  {
    SceneryElement clouds;
    if (loadStrip(clouds,
                  "assets/textures/NES - Super Mario Bros. - Backgrounds - Background 3 (Clouds).png",
                  sf::IntRect(0, 40, 734, 42))) {
      clouds.worldY   = 55.0f;    // Upper sky
      clouds.parallax = 0.35f;
      m_elements.push_back(std::move(clouds));
    }
  }

  // 2. Green Hills (Background 1: positioned nicely at worldY = 499px, flush on ground line)
  {
    SceneryElement hills;
    if (loadStrip(hills,
                  "assets/textures/NES - Super Mario Bros. - Backgrounds - Background 1 (Mountains).png",
                  sf::IntRect(0, 170, 734, 45))) {
      hills.worldY   = groundTopY - 45.0f; // 499px (Flush on ground)
      hills.parallax = 0.55f;
      m_elements.push_back(std::move(hills));
    }
  }

  // 3. Green Bushes / Trees (Background 2: slightly offset horizontally and positioned at worldY = 499px)
  {
    SceneryElement bushes;
    if (loadStrip(bushes,
                  "assets/textures/NES - Super Mario Bros. - Backgrounds - Background 2 (Trees).png",
                  sf::IntRect(0, 170, 734, 45))) {
      bushes.worldY   = groundTopY - 45.0f; // 499px (Flush on ground)
      bushes.parallax = 0.75f;
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