#pragma once
#include "GameObject.hpp"
#include "../Level/LevelTheme.hpp"
#include <SFML/Graphics.hpp>
#include <memory>

class Item;

/// BlockType — identifies what kind of block this is.
enum class BlockType {
  Brick,   // Breakable when player is Big
  Question // Contains an item, becomes empty after hit
};

/// Block — interactive blocks that can be hit from below.
/// QuestionBlocks spawn items; BrickBlocks break when the player is Big.
class Block : public GameObject {
  friend class SnapshotAccess;
public:
  Block();
  Block(BlockType blockType, float x, float y, LevelTheme theme,
        bool startUsed = false);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  /// Hit this block from below. Returns a spawned item (or nullptr).
  std::unique_ptr<Item> hit(bool playerIsBig);

  BlockType getBlockType() const;
  bool isUsed() const;

  /// Set what item this block contains (for QuestionBlocks).
  void setContainedItem(ObjectType itemType);

private:
  BlockType m_blockType = BlockType::Question;
  ObjectType m_containedItem = ObjectType::Coin;
  bool m_used = false;
  LevelTheme m_theme = LevelTheme::Overworld;
  sf::Sprite m_sprite;

  // Bump animation
  float m_bumpOffset = 0.0f;
  float m_bumpTimer = 0.0f;
  bool m_bumping = false;
};
