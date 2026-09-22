#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

//  StoryAnimation
//  Animation d'intro : 6 images sur 56 secondes avec intervalles irréguliers.
//  Résolution cible : 1600 × 900 px.
//
//  Utilisation typique :
//      StoryAnimation story;
//      story.Load(renderer);          // à appeler une fois après InitSDL
//      // dans la boucle :
//      story.Update();                // chaque frame
//      story.Render(renderer);        // chaque frame, avant SDL_RenderPresent
//      if (story.IsFinished()) { state = GameState::MENU; }
//      // libérer quand on n'en a plus besoin :
//      story.Unload();

struct StoryFrame {
    const char*  path;          // chemin vers l'image (PNG )
    float        displayTime;   // durée d'affichage de ce frame (secondes)
    float        fadeIn;        // durée du fondu entrant  (≤ displayTime)
    float        fadeOut;       // durée du fondu sortant  (≤ displayTime)
};

// 6 images – total = 56 secondes
// Intervalles irréguliers choisis pour rythmer le récit :
//   frame 0 : 12 s  
//   frame 1 :  7 s
//   frame 2 : 11 s
//   frame 3 :  8 s
//   frame 4 :  10 s
//   frame 5 : 9 s  
static const int STORY_FRAME_COUNT = 6;

static const StoryFrame STORY_FRAMES[STORY_FRAME_COUNT] = {
    { "assets/story_01.png", 11.f, 0.3f, 1.0f },
    { "assets/story_02.png",  8.f, 0.8f, 0.8f },
    { "assets/story_03.png", 11.f, 1.2f, 1.0f },
    { "assets/story_04.png",  9.f, 0.8f, 0.8f },
    { "assets/story_05.png",  9.f, 1.0f, 1.0f },
    { "assets/story_06.png",  8.5f, 1.0f, 1.5f },
};

class StoryAnimation {
    
    private:
        SDL_Texture* textures[STORY_FRAME_COUNT];
        int          currentFrame;      // index du frame affiché 
        float        frameTimer;        // temps passé sur le frame courant (s)
        Uint64       lastTick;          // timestamp SDL du dernier Update()
        bool         finished;

        // Calcule l'alpha (0–255) en tenant compte des fondus entrant/sortant.
        Uint8 ComputeAlpha(int frameIdx, float elapsed) const;
    public:
        StoryAnimation();

        // Charge les 6 textures. À appeler après la création du renderer.
        void Load(SDL_Renderer* renderer);

        // Libère les textures.
        void Unload();

        // Avance la timeline (à appeler chaque frame, avant Render).
        void Update();

        // Dessine le frame courant avec fondu en 1600×900.
        void Render(SDL_Renderer* renderer) const;

        // Retourne true quand les 56 secondes sont écoulées.
        bool IsFinished() const;

        //modifie la fin de l'animation
        void SetFinished();

        // Remet l'animation à zéro (pour la rejouer).
        void Reset();


};
