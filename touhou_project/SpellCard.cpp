#pragma execution_character_set("utf-8")
#include "SpellCard.h"
#include "Enemy.h"

void SpellCard::Update(float deltaTime) {
    m_duration += deltaTime;
    if (m_ctx.boss) m_ctx.boss->hit(600.0f * deltaTime);
    if (m_duration >= m_maxDuration) m_isFinished = true;
}
