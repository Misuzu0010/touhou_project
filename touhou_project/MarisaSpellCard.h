#pragma once
#include "SpellCard.h"

class MarisaSpellCard : public SpellCard
{
public:
    MarisaSpellCard(SpellContext& ctx, float maxDuration = 3.0f);
    void Activate() override;
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;
    const char* GetName() override;

private:
    float m_laserWidth;
    float m_flickerTimer;
};
