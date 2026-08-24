#pragma once
#include "GameObject.hpp"
#include "../Level/LevelTheme.hpp"
#include <string>

// Forward declaration
class Player;

/// Item — Abstract base class for collectible items.
/// Each item defines its own activate() behavior on the player.
class Item : public GameObject {
public:
    Item();
    virtual ~Item() = default;

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

    /// Apply this item's effect to the player.
    virtual void activate(Player& player) = 0;

    /// Mark as collected (triggers observer event and deactivates).
    void collect(Player& player);

    /// Is this item currently moving (e.g., mushroom pops out of block)?
    bool isMoving() const;

    /// Which environment palette this item's sprite should use, set by
    /// EntityFactory/Block right after construction (theme isn't known
    /// inside the zero-arg subclass constructors).
    void setTheme(LevelTheme theme);

protected:
    /// Subclasses override to (re)apply their themed texture rect once
    /// m_theme is known (can't be done in the constructor).
    virtual void refreshSprite() {}

    sf::Sprite m_sprite;
    std::string m_texturePath;
    bool       m_moving   = false;
    bool       m_collected = false;
    LevelTheme m_theme = LevelTheme::Overworld;

    /// Set to true by subclasses whose texture is an animated .gif — draw()
    /// will then call applyGifFrame(m_texturePath, m_animFrame) instead of
    /// applyFrame(m_texturePath).
    bool m_isGif = false;

    /// Optional horizontal PNG sprite sheet support. When m_sheetFrameWidth
    /// is > 0, draw() crops cell `m_animFrame` (each cell m_sheetFrameWidth
    /// wide, cells m_sheetGap px apart) via applySheetFrame instead of
    /// drawing the whole image.
    int m_sheetFrameWidth = 0;
    int m_sheetGap = 0;

    // Animation
    float m_animTimer = 0.0f;
    int   m_animFrame = 0;
};

