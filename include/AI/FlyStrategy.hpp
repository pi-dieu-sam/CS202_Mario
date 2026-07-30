#pragma once
#include "AIStrategy.hpp"

/// FlyStrategy — enemy moves in a sinusoidal wave pattern while advancing horizontally.
/// Used for flying Koopas or similar airborne enemies.
class FlyStrategy : public AIStrategy {
public:
    FlyStrategy(float amplitude = 50.0f, float frequency = 2.0f);
    void execute(Enemy& enemy, float dt) override;

private:
    float m_amplitude;
    float m_frequency;
    float m_time = 0.0f;
};