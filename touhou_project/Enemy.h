#pragma once
#include "Entity.h"
#include <SDL.h>

class Enemy : public Entity
{
public:
    float Blood;
    float attack_point = 5;
    SDL_Texture* texture;

    Enemy(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 40), Blood(6000), texture(tex) {
        active = true;
    }

    ~Enemy() {
        // Texture由Game类统一管理释放，这里不需要Destroy，除非是Enemy独有的
    }

    void Render(SDL_Renderer* r) override {
        if (texture) {
            // ★修改：强制缩放 Boss
            // 也是先获取原图大小
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);

            // 设定 Boss 显示高度为 250 (比玩家大)
            float aspect = (float)w / h;
            int displayH = 250;
            int displayW = (int)(displayH * aspect);

            SDL_Rect dest = {
                (int)Position.x - displayW / 2,
                (int)Position.y - displayH / 2,
                displayW,
                displayH
            };
            SDL_RenderCopy(r, texture, NULL, &dest);
        }
        else {
            // 没有图的时候画个红框
            SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
            SDL_Rect rect = { (int)Position.x - 50, (int)Position.y - 50, 100, 100 };
            SDL_RenderFillRect(r, &rect);
        }
    }
    

    void hit(float attack) { Blood -= attack; }
    float GetBlood() { return Blood; }
};