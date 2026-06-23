#pragma execution_character_set("utf-8")
#include "MarisaSpellCard.h"
#include "SpellCard.h"
#include "Enemy.h"
#include "Player.h"
#include "Bullet.h"
#include <cmath>

MarisaSpellCard::MarisaSpellCard(SpellContext& ctx, float maxDuration)
    : SpellCard(ctx, maxDuration), m_laserWidth(80.0f), m_flickerTimer(0.0f)
{
}

void MarisaSpellCard::Activate()
{
    if (m_ctx.enemyBullets && m_ctx.player) {
        float playerX = m_ctx.player->GetPosition().x;
        for (auto& b : *m_ctx.enemyBullets) {
            if (std::abs(b->GetPosition().x - playerX) < 100.0f) {
                b->Deactivate();
            }
        }
    }
    if (m_ctx.player) {
        m_ctx.player->SetInvincible(3.5f);
    }
}

void MarisaSpellCard::Update(float deltaTime)
{
    SpellCard::Update(deltaTime);
    m_flickerTimer += deltaTime;
}

void MarisaSpellCard::Render(SDL_Renderer* renderer)
{
    if (!m_ctx.player) return;

    float playerX = m_ctx.player->GetPosition().x;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    // 外光束
    int outerAlpha = static_cast<int>(std::sin(m_flickerTimer * 10.0f) * 30.0f + 225.0f);
    SDL_SetRenderDrawColor(renderer, outerAlpha, outerAlpha, 0, 200);
    SDL_Rect beamOuter = { static_cast<int>(playerX - m_laserWidth / 2), 0,
                           static_cast<int>(m_laserWidth), 1080 };
    SDL_RenderFillRect(renderer, &beamOuter);

    // 核心光束
    SDL_SetRenderDrawColor(renderer, 255, 255, 200, 220);
    SDL_Rect beamCore = { static_cast<int>(playerX - 15), 0, 30, 1080 };
    SDL_RenderFillRect(renderer, &beamCore);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

const char* MarisaSpellCard::GetName()
{
    return "\xe6\x81\x8b\xe7\xac\xa6\xe3\x80\x8cMaster Spark\xe3\x80\x8d";
}
