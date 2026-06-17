#pragma execution_character_set("utf-8")
#include "ReimuSpellCard.h"
#include "SpellCard.h"
#include "Enemy.h"
#include "Player.h"
#include "Bullet.h"
#include <cmath>

ReimuSpellCard::ReimuSpellCard(SpellContext& ctx, float maxDuration)
    : SpellCard(ctx, maxDuration), m_rotateSpeed(180.0f), m_orbitRadius(150.0f)
{
    float startAngles[8] = { 0, 45, 90, 135, 180, 225, 270, 315 };
    for (int i = 0; i < 8; i++) {
        m_angles[i] = startAngles[i];
    }
}

void ReimuSpellCard::Activate()
{
    if (m_ctx.enemyBullets) {
        for (auto& b : *m_ctx.enemyBullets) {
            b->Deactivate();
        }
    }
    if (m_ctx.player) {
        m_ctx.player->SetInvincible(3.5f);
    }
}

void ReimuSpellCard::Update(float deltaTime)
{
    SpellCard::Update(deltaTime);
    for (int i = 0; i < 8; i++) {
        m_angles[i] += m_rotateSpeed * deltaTime;
        if (m_angles[i] >= 360.0f) m_angles[i] -= 360.0f;
    }
}

void ReimuSpellCard::Render(SDL_Renderer* renderer)
{
    if (!m_ctx.boss) return;

    float bossX = m_ctx.boss->GetPosition().x;
    float bossY = m_ctx.boss->GetPosition().y;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 100, 200, 200);

    for (int i = 0; i < 8; i++) {
        float rad = m_angles[i] * 3.14159265f / 180.0f;
        int tx = static_cast<int>(bossX + std::cos(rad) * m_orbitRadius);
        int ty = static_cast<int>(bossY + std::sin(rad) * m_orbitRadius);
        SDL_Rect orb = { tx - 10, ty - 10, 20, 20 };
        SDL_RenderFillRect(renderer, &orb);
    }
}

const char* ReimuSpellCard::GetName()
{
    return "\xe7\x81\xb5\xe7\xac\xa6\xe3\x80\x8c\xe6\xa2\xa6\xe6\x83\xb3\xe5\xb0\x81\xe5\x8d\xb0\xe3\x80\x8d";
}
