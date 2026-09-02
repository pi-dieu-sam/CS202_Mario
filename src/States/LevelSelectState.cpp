#include "States/LevelSelectState.hpp"
#include "Core/AssetManager.hpp"
#include "Core/Game.hpp"
#include "Entities/Player.hpp"
#include "Entities/Tile.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Level/LevelTheme.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "States/Navigator.hpp"

#include <cmath>

LevelSelectState::LevelSelectState() {}

void LevelSelectState::onEnter() {
  sf::Font &font =
      AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

  m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
  m_background.setFillColor(sf::Color(30, 30, 50));

  m_title.setFont(font);
  m_title.setString("SELECT LEVEL");
  m_title.setCharacterSize(36);
  m_title.setFillColor(sf::Color::White);
  auto tb = m_title.getLocalBounds();
  m_title.setOrigin(tb.width / 2.0f, tb.height / 2.0f);
  m_title.setPosition(WINDOW_WIDTH / 2.0f, 50.0f);

  m_modeBadge.setFont(font);
  m_modeBadge.setCharacterSize(13);
  m_modeBadge.setFillColor(sf::Color(205, 221, 255));
  const PlayerProgress& progress = Game::getInstance().getProgress();
  if (progress.isCoop()) {
    const std::string p1 = progress.getSelectedCharacter();
    const std::string p2 = p1 == "Mario" ? "Luigi" : "Mario";
    m_modeBadge.setString("CO-OP - P1 " + p1 + " / P2 " + p2);
  } else {
    m_modeBadge.setString("1 PLAYER - " + progress.getSelectedCharacter());
  }
  auto mb = m_modeBadge.getLocalBounds();
  m_modeBadge.setOrigin(mb.left + mb.width / 2.0f, mb.top + mb.height / 2.0f);
  m_modeBadge.setPosition(WINDOW_WIDTH / 2.0f, 88.0f);

  float boxW = 210.0f, boxH = 360.0f;
  float gap = 30.0f;
  float startX = (WINDOW_WIDTH - (3 * boxW + 2 * gap)) / 2.0f;

  std::string titles[] = {"WORLD 1-1", "WORLD 2-1", "WORLD 3-1"};
  std::string themes[] = {"GRASSLAND", "UNDERGROUND", "CASTLE"};
  std::string descs[] = {
      "Bright outdoor\nlevel.\n\nIntroductory\ndifficulty.",
      "Dark cave\nwith narrow\npassages.\n\nMedium\ndifficulty.",
      "Lava fortress\nfilled with\nhazards.\n\nHardest\ndifficulty."};

  sf::Color boxColors[] = {
      sf::Color(40, 140, 60, 200), // Green for Grassland
      sf::Color(50, 60, 140, 200), // Blue/Purple for Underground
      sf::Color(160, 40, 40, 200)  // Red for Castle
  };

  for (int i = 0; i < 3; i++) {
    float posX = startX + i * (boxW + gap);
    float posY = 120.0f;

    m_levelBoxes[i].setSize(sf::Vector2f(boxW, boxH));
    m_levelBoxes[i].setPosition(posX, posY);
    m_levelBoxes[i].setFillColor(boxColors[i]);
    m_levelBoxes[i].setOutlineThickness(4.0f);
    m_levelBoxes[i].setOutlineColor(sf::Color::Transparent);

    // Title
    m_levelTitles[i].setFont(font);
    m_levelTitles[i].setString(titles[i]);
    m_levelTitles[i].setCharacterSize(20);
    m_levelTitles[i].setFillColor(sf::Color::Yellow);
    auto ltb = m_levelTitles[i].getLocalBounds();
    m_levelTitles[i].setOrigin(ltb.width / 2.0f, 0.0f);
    m_levelTitles[i].setPosition(posX + boxW / 2.0f, posY + 20.0f);

    // Theme
    m_levelThemes[i].setFont(font);
    m_levelThemes[i].setString(themes[i]);
    m_levelThemes[i].setCharacterSize(16);
    m_levelThemes[i].setFillColor(sf::Color::White);
    auto thb = m_levelThemes[i].getLocalBounds();
    m_levelThemes[i].setOrigin(thb.width / 2.0f, 0.0f);
    m_levelThemes[i].setPosition(posX + boxW / 2.0f, posY + 60.0f);

    // Desc
    m_levelDescs[i].setFont(font);
    m_levelDescs[i].setString(descs[i]);
    m_levelDescs[i].setCharacterSize(12);
    m_levelDescs[i].setFillColor(sf::Color(220, 220, 220));
    m_levelDescs[i].setPosition(posX + 15.0f, posY + 182.0f);

    // Small themed tile-strip preview along the bottom of the box, plus
    // a pipe accent — built from the same Tileset sprites used in-level.
    static const TileType groundTypes[] = {TileType::Ground, TileType::Underground,
                                            TileType::CastleBlock};
    static const LevelTheme levelThemes[] = {
        LevelTheme::Overworld, LevelTheme::Underground, LevelTheme::Castle};

    m_previewTiles[i].clear();
    const float tileSize = 26.0f;
    const int tileCount = 6;
    float rowY = posY + boxH - 40.0f;
    float rowStartX = posX + (boxW - tileCount * tileSize) / 2.0f;

    for (int t = 0; t < tileCount; t++) {
      sf::Sprite sprite;
      SpriteRegistry::applyFrame(
          sprite, SpriteRegistry::tilePath(groundTypes[i], levelThemes[i]),
          sf::FloatRect(rowStartX + t * tileSize, rowY, tileSize, tileSize));
      m_previewTiles[i].push_back(sprite);
    }

    sf::Sprite pipeL, pipeR;
    float pipeY = rowY - tileSize;
    float pipeX = posX + boxW / 2.0f - tileSize;
    SpriteRegistry::applyFrame(
        pipeL, SpriteRegistry::tilePath(TileType::PipeTopLeft, levelThemes[i]),
        sf::FloatRect(pipeX, pipeY, tileSize, tileSize));
    SpriteRegistry::applyFrame(
        pipeR, SpriteRegistry::tilePath(TileType::PipeTopRight, levelThemes[i]),
        sf::FloatRect(pipeX + tileSize, pipeY, tileSize, tileSize));
    m_previewTiles[i].push_back(pipeL);
    m_previewTiles[i].push_back(pipeR);
  }

  m_selected = 0;
  m_animationTime = 0.0f;
  updateSelectionVisuals();
  updatePreviewAnimations(0.0f);

  m_helpText.setFont(font);
  m_helpText.setCharacterSize(12);
  m_helpText.setString("LEFT/RIGHT: SELECT LEVEL   ENTER: PLAY   ESC: BACK");
  m_helpText.setFillColor(sf::Color(190, 207, 255));
  auto hb = m_helpText.getLocalBounds();
  m_helpText.setOrigin(hb.left + hb.width / 2.0f, hb.top + hb.height / 2.0f);
  m_helpText.setPosition(WINDOW_WIDTH / 2.0f, 548.0f);
}

void LevelSelectState::onExit() {}

void LevelSelectState::handleEvent(const sf::Event &event) {
  sf::RenderWindow &window = Game::getInstance().getWindow();

  // Mouse Move
  if (event.type == sf::Event::MouseMoved) {
    sf::Vector2f mousePos =
        Game::getInstance().mapPixelToUiCoords({event.mouseMove.x, event.mouseMove.y});
    for (int i = 0; i < 3; i++) {
      if (m_levelBoxes[i].getGlobalBounds().contains(mousePos)) {
        if (m_selected != i) {
          m_selected = i;
          updateSelectionVisuals();
        }
        break;
      }
    }
    return;
  }

  // Mouse Click
  if (event.type == sf::Event::MouseButtonPressed &&
      event.mouseButton.button == sf::Mouse::Left) {
    sf::Vector2f mousePos =
        Game::getInstance().mapPixelToUiCoords({event.mouseButton.x, event.mouseButton.y});
    for (int i = 0; i < 3; i++) {
      if (m_levelBoxes[i].getGlobalBounds().contains(mousePos)) {
        m_selected = i;
        Game::getInstance().getProgress().setCurrentLevel(m_selected + 1);
        Navigator::apply(
            ScreenFlow::onConfirm(ScreenFlow::Screen::LevelSelect,
                                  Game::getInstance().getProgress().getGameMode()),
            Game::getInstance().getStateManager(),
            Game::getInstance().getProgress().getGameMode());
        return;
      }
    }
  }

  // Keyboard
  if (event.type == sf::Event::KeyPressed) {
    switch (event.key.code) {
    case sf::Keyboard::Left:
    case sf::Keyboard::A:
      m_selected = (m_selected - 1 + 3) % 3;
      updateSelectionVisuals();
      break;

    case sf::Keyboard::Right:
    case sf::Keyboard::D:
      m_selected = (m_selected + 1) % 3;
      updateSelectionVisuals();
      break;

    case sf::Keyboard::Return:
    case sf::Keyboard::Space:
      Game::getInstance().getProgress().setCurrentLevel(m_selected + 1);
      Navigator::apply(
          ScreenFlow::onConfirm(ScreenFlow::Screen::LevelSelect,
                                Game::getInstance().getProgress().getGameMode()),
          Game::getInstance().getStateManager(),
          Game::getInstance().getProgress().getGameMode());
      break;

    case sf::Keyboard::Escape:
      Navigator::apply(ScreenFlow::onBack(ScreenFlow::Screen::LevelSelect),
                        Game::getInstance().getStateManager(),
                        Game::getInstance().getProgress().getGameMode());
      break;

    default:
      break;
    }
  }
}

void LevelSelectState::update(float dt) {
  updatePreviewAnimations(dt);
}

void LevelSelectState::render(sf::RenderWindow &window) {
  window.setView(Game::getInstance().getUiView());
  window.draw(m_background);
  window.draw(m_title);
  window.draw(m_modeBadge);
  for (int i = 0; i < 3; i++) {
    window.draw(m_levelBoxes[i]);
    window.draw(m_previewCoins[i]);
    window.draw(m_previewEnemies[i]);
    window.draw(m_previewHeroes[i]);
    for (auto &tile : m_previewTiles[i])
      window.draw(tile);
    window.draw(m_levelTitles[i]);
    window.draw(m_levelThemes[i]);
    window.draw(m_levelDescs[i]);
  }
  window.draw(m_helpText);
}

void LevelSelectState::updateSelectionVisuals() {
  for (int i = 0; i < 3; i++) {
    if (i == m_selected) {
      m_levelBoxes[i].setOutlineColor(sf::Color::Yellow);
      m_levelBoxes[i].setOutlineThickness(5.0f);
    } else {
      m_levelBoxes[i].setOutlineColor(sf::Color::Transparent);
      m_levelBoxes[i].setOutlineThickness(3.0f);
    }
  }
}

void LevelSelectState::updatePreviewAnimations(float dt) {
  m_animationTime += dt;
  const PlayerProgress& progress = Game::getInstance().getProgress();
  const CharacterId hero = progress.getSelectedCharacter() == "Luigi"
      ? CharacterId::Luigi : CharacterId::Mario;

  for (int i = 0; i < 3; ++i) {
    const float posX = 85.0f + i * 240.0f;
    const float phase = std::fmod(m_animationTime + i * 0.42f, 2.8f);
    const bool jumping = phase > 1.7f && phase < 2.35f;
    const float jumpProgress = jumping ? (phase - 1.7f) / 0.65f : 0.0f;
    const float lift = jumping ? std::sin(jumpProgress * 3.14159265f) * 34.0f : 0.0f;
    SpriteRegistry::applyPlayerFrame(
        m_previewHeroes[i], hero, PowerUpState::Big,
        jumping ? SpriteRegistry::PlayerAnim::Jump : SpriteRegistry::PlayerAnim::Walk,
        static_cast<int>(m_animationTime * 10.0f),
        sf::FloatRect(posX + 70.0f, 278.0f - lift - 64.0f, 0.0f, 64.0f));

    SpriteRegistry::applyGoombaFrame(
        m_previewEnemies[i], static_cast<int>(m_animationTime * 11.0f + i * 3),
        sf::FloatRect(posX + 145.0f, 278.0f - 32.0f, 0.0f, 32.0f), true);

    const float coinBob = std::sin(m_animationTime * 4.0f + i) * 6.0f;
    SpriteRegistry::applyFrame(m_previewCoins[i], "assets/textures/SMBCoin.gif",
                               sf::FloatRect(posX + 105.0f, 212.0f + coinBob, 0.0f, 22.0f));
  }
}
