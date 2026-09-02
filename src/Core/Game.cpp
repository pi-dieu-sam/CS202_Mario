#include "Core/Game.hpp"
#include "States/StateManager.hpp"
#include "States/MenuState.hpp"
#include "Core/AssetManager.hpp"
#include "Core/SoundManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <iostream>

namespace {
bool supportsFullscreenMode(unsigned int width, unsigned int height) {
    for (const sf::VideoMode& mode : sf::VideoMode::getFullscreenModes()) {
        if (mode.width == width && mode.height == height) {
            return true;
        }
    }
    return false;
}
} // namespace

Game& Game::getInstance() {
    static Game instance;
    return instance;
}

Game::Game()
    : m_uiView(sf::FloatRect(0.0f, 0.0f,
                             static_cast<float>(WINDOW_WIDTH),
                             static_cast<float>(WINDOW_HEIGHT)))
    , m_stateManager(std::make_unique<StateManager>()) {
    m_settings.loadFromFile();
    applyGraphicsSettings(m_settings.graphics());
    applyAudioSettings();
}

void Game::run() {
    // Push the initial state (main menu)
    m_stateManager->pushState(std::make_unique<MenuState>());

    sf::Clock clock;
    float accumulator = 0.0f;

    while (m_running && m_window.isOpen()) {
        float frameTime = clock.restart().asSeconds();
        // Clamp to avoid spiral of death
        if (frameTime > 0.25f) frameTime = 0.25f;
        accumulator += frameTime;

        // Process pending state changes
        m_stateManager->processPending();

        if (m_stateManager->isEmpty()) {
            // StateManager::popState() refuses to empty the stack, so this
            // should be unreachable in normal play — it means every state
            // was removed via clearStates() without pushing a new root.
            // Treat it as a bug, not a valid way to quit: requestExit() (or
            // the window-close event) is the only intended exit path.
            std::cerr << "Game: state stack became empty; forcing exit.\n";
            m_running = false;
            break;
        }

        processEvents();

        // Fixed timestep updates
        while (accumulator >= FIXED_DT) {
            update(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        render();
    }

    m_window.close();
}

void Game::requestExit() {
    m_running = false;
}

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_running = false;
            return;
        }
        if (event.type == sf::Event::Resized) {
            updateUiView();
        }
        m_stateManager->handleEvent(event);
    }
}

void Game::update(float dt) {
    m_stateManager->update(dt);
}

void Game::render() {
    m_window.clear(sf::Color::Black);
    m_stateManager->render(m_window);
    m_window.display();
}

sf::RenderWindow& Game::getWindow() {
    return m_window;
}

const sf::View& Game::getUiView() const {
    return m_uiView;
}

sf::Vector2f Game::mapPixelToUiCoords(const sf::Vector2i& pixel) const {
    return m_window.mapPixelToCoords(pixel, m_uiView);
}

GameSettings& Game::getSettings() {
    return m_settings;
}

const GameSettings& Game::getSettings() const {
    return m_settings;
}

bool Game::applyGraphicsSettings(GraphicsSettings settings) {
    settings = GameSettings::normalize(settings);
    bool usedFallback = false;

    if (settings.mode == DisplayMode::Fullscreen &&
        !supportsFullscreenMode(settings.width, settings.height)) {
        settings = GraphicsSettings{};
        usedFallback = true;
    }

    const sf::Uint32 style = settings.mode == DisplayMode::Fullscreen
        ? sf::Style::Fullscreen
        : sf::Style::Titlebar | sf::Style::Close;
    m_window.create(sf::VideoMode(settings.width, settings.height), "Super Mario", style);

    // A failed OS-level creation is rare, but a safe window is always better
    // than leaving the game with no render target.
    if (!m_window.isOpen()) {
        settings = GraphicsSettings{};
        m_window.create(sf::VideoMode(settings.width, settings.height), "Super Mario",
                        sf::Style::Titlebar | sf::Style::Close);
        usedFallback = true;
    }

    m_window.setFramerateLimit(settings.maxFps);
    m_window.setKeyRepeatEnabled(false);
    m_settings.setGraphics(settings);
    updateUiView();
    return !usedFallback;
}

void Game::applyAudioSettings() {
    SoundManager::getInstance().applySettings(m_settings.audio());
}

std::vector<sf::VideoMode> Game::getFullscreenModes() const {
    return sf::VideoMode::getFullscreenModes();
}

void Game::updateUiView() {
    const sf::Vector2u size = m_window.getSize();
    if (size.x == 0 || size.y == 0) return;

    const float logicalWidth = static_cast<float>(WINDOW_WIDTH);
    const float logicalHeight = static_cast<float>(WINDOW_HEIGHT);
    const float windowRatio = static_cast<float>(size.x) / static_cast<float>(size.y);
    const float logicalRatio = logicalWidth / logicalHeight;

    sf::FloatRect viewport(0.0f, 0.0f, 1.0f, 1.0f);
    if (windowRatio > logicalRatio) {
        viewport.width = logicalRatio / windowRatio;
        viewport.left = (1.0f - viewport.width) * 0.5f;
    } else if (windowRatio < logicalRatio) {
        viewport.height = windowRatio / logicalRatio;
        viewport.top = (1.0f - viewport.height) * 0.5f;
    }

    m_uiView.reset(sf::FloatRect(0.0f, 0.0f, logicalWidth, logicalHeight));
    m_uiView.setViewport(viewport);
}

StateManager& Game::getStateManager() {
    return *m_stateManager;
}

PlayerProgress& Game::getProgress() {
    return m_progress;
}
