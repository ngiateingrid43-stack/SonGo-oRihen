#include "../include/StoryAnimation.h"
#include <SDL3/SDL.h>

StoryAnimation::StoryAnimation()
    : currentFrame(0), frameTimer(0.f),
      lastTick(0), finished(false)
{
    for (int i = 0; i < STORY_FRAME_COUNT; i++)
        textures[i] = nullptr;
}

void StoryAnimation::Load(SDL_Renderer* renderer) {
    for (int i = 0; i < STORY_FRAME_COUNT; i++) {
        SDL_Surface* surf = IMG_Load(STORY_FRAMES[i].path);
        if (surf) {
            textures[i] = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_DestroySurface(surf);
        } else {
            SDL_Log("StoryAnimation: impossible de charger %s : %s",STORY_FRAMES[i].path, SDL_GetError());
        }
        // Activer le blending pour les fondus
        SDL_SetTextureBlendMode(textures[i], SDL_BLENDMODE_BLEND);
    }
    Reset();
}

void StoryAnimation::Unload() {
    for (int i = 0; i < STORY_FRAME_COUNT; i++) {
        if (textures[i]) {
            SDL_DestroyTexture(textures[i]);
            textures[i] = nullptr;
        }
    }
}

void StoryAnimation::Reset() {
    currentFrame = 0;
    frameTimer   = 0.f;
    lastTick     = SDL_GetTicks();
    finished     = false;
}

//  Update — avance le timer, change de frame quand la durée est atteinte
void StoryAnimation::Update() {
    if (finished) return;

    Uint64 now = SDL_GetTicks();
    float  dt  = (float)(now - lastTick) / 1000.f;   // secondes
    lastTick   = now;
    if (dt > 0.1f) dt = 0.1f;   // sécurité contre les gros deltas

    frameTimer += dt;

    // Passe au frame suivant si la durée d'affichage est dépassée
    while (currentFrame < STORY_FRAME_COUNT &&frameTimer >= STORY_FRAMES[currentFrame].displayTime) {
        frameTimer -= STORY_FRAMES[currentFrame].displayTime;
        currentFrame++;
    }

    if (currentFrame >= STORY_FRAME_COUNT) {
        currentFrame = STORY_FRAME_COUNT - 1;   // reste sur la dernière image
        finished     = true;
    }
}

//  ComputeAlpha — fondu entrant + fondu sortant
Uint8 StoryAnimation::ComputeAlpha(int frameIdx, float elapsed) const {
    const StoryFrame& f = STORY_FRAMES[frameIdx];
    float alpha = 1.f;

    // Fondu entrant
    if (elapsed < f.fadeIn && f.fadeIn > 0.f) {
        alpha = elapsed / f.fadeIn;
    }
    // Fondu sortant
    float timeBeforeEnd = f.displayTime - elapsed;
    if (timeBeforeEnd < f.fadeOut && f.fadeOut > 0.f) {
        float outAlpha = timeBeforeEnd / f.fadeOut;
        if (outAlpha < alpha) alpha = outAlpha;
    }

    // Clamp [0, 1] → [0, 255]
    if (alpha < 0.f) alpha = 0.f;
    if (alpha > 1.f) alpha = 1.f;
    return (Uint8)(alpha * 255.f);
}

//  Render — fond noir + image courante avec alpha calculé, 1600×900 px
void StoryAnimation::Render(SDL_Renderer* renderer) const {
    // 1. Fond noir (toujours visible sous les fondus)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (finished || currentFrame >= STORY_FRAME_COUNT) return;
    if (!textures[currentFrame]) return;

    // 2. Calcul de l'alpha selon la position dans le frame
    Uint8 alpha = ComputeAlpha(currentFrame, frameTimer);
    SDL_SetTextureAlphaMod(textures[currentFrame], alpha);

    // 3. Affichage plein écran 1600×900
    SDL_FRect dst = { 0.f, 0.f, 1600.f, 900.f };
    SDL_RenderTexture(renderer, textures[currentFrame], nullptr, &dst);
}

bool StoryAnimation::IsFinished() const {
    return finished;
}

void StoryAnimation::SetFinished() {
    finished = false;
}
