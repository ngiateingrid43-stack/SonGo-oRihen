#pragma once
#include "Entity.h"
#include <string>
#include <optional>
#include <SDL3_ttf/SDL_ttf.h>
#include "StoryAnimation.h"
#include <SFML/Audio.hpp>
#include <vector>

enum class GameState { 
    MENU, 
    PLAYING,
    STORY, 
    GAME_OVER,
    CREDITS_SCREEN,
    OPTIONS_SCREEN,
};

struct AnimState {
    bool     active       = false;
    int      srcIdx       = 0;
    int      curIdx       = 0;
    int      grainesLeft  = 0;
    float    handX        = 0.f;
    float    handY        = 0.f;
    float    targetX      = 0.f;
    float    targetY      = 0.f;
    float    speed        = 700.f;
    float    pauseTimer   = 0.f;
    bool     dropping     = false;
};

// Particule de lueur pour le joueur actif
struct GlowPulse {
    float alpha    = 0.f;
    float phase    = 0.f;   // 0..2PI
    float speed    = 3.5f;
};

struct Fonts {
    TTF_Font* huge   = nullptr;
    TTF_Font* large  = nullptr;
    TTF_Font* medium = nullptr;
    TTF_Font* small  = nullptr;
    TTF_Font* tiny   = nullptr;
    bool load(const std::string& path);
    void free();
};

class Game {
private:
    bool isRunning;
    bool isRunningLogo;
    GameState     state;
    GameState     prevState;    // Pour revenir depuis Options/Credits
    SDL_Window*   window;
    SDL_Renderer* renderer;
    Fonts         m_fonts;
    GameOptions   options;

    Joueur* joueur1;
    Joueur* joueur2;
    Trou*   trou[MAX_TROU];
    TTF_Font* font;
    SDL_Texture* texture;
    SDL_Texture* handTexture;

    // Audio SFML
    sf::SoundBuffer voiceBuffer;
    sf::SoundBuffer depositBuffer;    // son dépôt graine
    sf::SoundBuffer pickupBuffer;     // son ramassage graine
    sf::SoundBuffer drumBuffer;       // tambour tour joueur

    sf::SoundBuffer storyVoiceBuffer; // Charger la voix de l'histoire

    std::optional<sf::Sound> storyVoice;
    std::optional<sf::Sound> depositSound;
    std::optional<sf::Sound> pickupSound;
    std::optional<sf::Sound> drumSound;
    sf::Music       bgMusic;          // musique de fond africaine
    sf::Music       creditMusic;          // musique de fond africaine credit
    
    bool m_creditMusicLoaded = false;

    // Animation de glow pour joueur actif
    GlowPulse     glowJ1, glowJ2;
    float         glowTime = 0.f;

    int  currentPlayer;
    std::string statusMsg;
    // Sous-titre anglais (mode histoire)
    std::string storySubtitle;
    int         storySubtitleFrame = -1;

    AnimState   anim;
    Uint64      lastTick;
    bool        lastDropping = false;  // détection bord dépôt

    StoryAnimation storyAnim;

    // Crédits défilants
    float       m_creditsScrollY = 0.f;
    float       m_creditsSpeed   = 40.f;  // px/s
    SDL_Texture* m_creditsTexture = nullptr;
    int          m_creditsTexH   = 0;

    //logo RIHEN
    std::vector<SDL_Texture*> frames;

    int frameCount;
    int currentFrame;
    double frameDuration; // durée par image
    double accumulator;

    // Résolution réelle de la fenêtre (plein écran)
    int     m_winW = WIN_W;
    int     m_winH = WIN_H;

    void InitSDL();
    void InitImGui();
    void InitEntities();
    void InitAudio();

    void HandleClick(int x, int y, Joueur* joueur1, Joueur* joueur2);
    void SwitchPlayer();
    void CollectRemainingGraines();

    void UpdateAnimation();
    void FinishAnimation();
    void RenderHand();

    void RenderNumberOfGraines(float x, float y);
    void RenderBackground();
    void RenderImGuiUI();
    void RenderPlayerGlow();          // halo lumineux sur le camp actif
    void RenderStorySubtitle();       // sous-titres anglais sur l'histoire

    void RenderCreditsScreen();       // écran crédits défilants
    void RenderOptionsScreen();       // écran options

    void PlayDepositSound();
    void PlayPickupSound();
    void PlayDrumForPlayer(int player);

    bool loadFrames(const std::string& folder, int firstFrame, int lastFrame);
    void update(double deltaTime);
    void render();

    // IA simpliste
    int  ComputeIAMove();

    void Cleanup();

public:
    Game();
    void Run();
    void init();
};