#pragma once
#include <vector>
#include <memory>
#include <SDL.h>

class Player;
class Enemy;
class Bullet;

struct SpellContext {
    std::vector<std::unique_ptr<Bullet>>* enemyBullets = nullptr;
    Enemy* boss = nullptr;
    Player* player = nullptr;
    SDL_Renderer* renderer = nullptr;
};

class SpellCard
{
public:
    SpellCard(SpellContext& ctx, float maxDuration = 3.0f)
        : m_ctx(ctx), m_maxDuration(maxDuration) {}
    virtual ~SpellCard() = default;

    virtual void Activate() = 0;
    virtual void Update(float deltaTime);
    virtual void Render(SDL_Renderer* renderer) = 0;
    virtual bool IsFinished() { return m_isFinished; }
    virtual const char* GetName() = 0;

protected:
    SpellContext m_ctx;
    float m_duration = 0;
    float m_maxDuration;
    bool m_isFinished = false;
};
