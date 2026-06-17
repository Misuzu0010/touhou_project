#pragma once
#include "SpellCard.h"

class ReimuSpellCard : public SpellCard
{
public:
    ReimuSpellCard(SpellContext& ctx, float maxDuration = 3.0f);
    void Activate() override;
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;
    const char* GetName() override;

private:
    float m_angles[8];
    float m_rotateSpeed;
    float m_orbitRadius;
};
