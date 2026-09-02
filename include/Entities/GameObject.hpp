#pragma once
#include <SFML/Graphics.hpp>

/// Enum to tag what kind of object this is, used for collision dispatch.
enum class ObjectType {
    None,
    Player,
    Enemy,
    Tile,
    Block,
    Coin,
    Mushroom,
    FireFlower,
    Star,
    Fireball,
    Flagpole,
    Pipe,
    FlowersBuff,
    FireBar,
    LavaFireball
};

/// GameObject — Abstract base class for ALL entities in the game.
/// Defines the common interface: position, velocity, bounds, update, draw.
class GameObject {
    friend class SnapshotAccess;
public:
    GameObject();
    virtual ~GameObject() = default;

    // Pure virtual — every entity must implement these
    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual sf::FloatRect getBounds() const = 0;

    // Virtual — default collision handler (can override)
    virtual void onCollision(GameObject& other);

    // ── Position & velocity ──
    sf::Vector2f getPosition() const;
    void         setPosition(float x, float y);
    void         setPosition(const sf::Vector2f& pos);

    sf::Vector2f getVelocity() const;
    void         setVelocity(float vx, float vy);
    void         setVelocity(const sf::Vector2f& vel);

    // ── Active flag ──
    bool isActive() const;
    void setActive(bool active);

    // ── Type tag ──
    ObjectType getType() const;

protected:
    sf::Vector2f m_position  = {0.0f, 0.0f};
    sf::Vector2f m_velocity  = {0.0f, 0.0f};
    bool         m_active    = true;
    ObjectType   m_type      = ObjectType::None;
};
