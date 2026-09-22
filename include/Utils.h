#pragma once
#include <iostream>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SFML/Audio.hpp>
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_sdl3.h"
#include "../imgui/backends/imgui_impl_sdlrenderer3.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#define MAX_SURFACE 16
#define MAX_TEXTURE 16
#define MAX_TROU    14

// Résolution de base (sera ajustée en plein écran)
#define WIN_W 1600
#define WIN_H 900

constexpr int   SCREEN_W       = 1600;
constexpr int   SCREEN_H       = 900;
constexpr float PI             = 3.14159265f;
constexpr int   FPS            = 60;
constexpr float FRAME_DURATION = 1000.0f / FPS;

// Constantes audio
#define MAX_MUSIC   8
#define MAX_BUFFER  8
#define MAX_SOUNG   8

enum Statut {
    PRISE = 0,
    NOT_PRISE,
};

struct Vector2D {
    float x;
    float y;
};

enum StatutTrou {
    ZERO=0, UN, DEUX, TROIS, QUATRE, CINQ,
    SIX, SEPT, HUIT, NEUF, DIX,
    ONZE, DOUZE, TREIZE, QUATORZE, QUINZE,
};

struct Color {
    Uint8 r, g, b, a;
    SDL_FColor toFColor() const {
        return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
    }
};

// Palette africaine camerounaise
constexpr Color COL_BG           = {  10,  10,  25, 255 };
constexpr Color COL_ORANGE       = { 255, 140,   0, 255 };
constexpr Color COL_ORANGE_GLOW  = { 255, 100,   0, 120 };
constexpr Color COL_WHITE        = { 255, 255, 255, 255 };
constexpr Color COL_GOLD         = { 255, 215,   0, 255 };
constexpr Color COL_CYAN         = {   0, 230, 255, 255 };
constexpr Color COL_GRAY         = { 160, 160, 160, 255 };

// Couleurs africaines
constexpr Color COL_TERRE_CUITE  = { 180,  70,  15, 255 };
constexpr Color COL_VERT_FORET   = {  30, 100,  40, 255 };
constexpr Color COL_OR_CHAUD     = { 210, 160,  30, 255 };
constexpr Color COL_BRUN_CACAO   = {  90,  45,  10, 255 };

// Mode de jeu
enum class GameMode {
    JOUEUR_VS_JOUEUR,
    IA_VS_JOUEUR,
};

// Options globales du jeu
struct GameOptions {
    bool     soundEnabled     = true;
    bool     musicEnabled     = true;
    bool     drumEnabled      = true;
    float    musicVolume      = 10.f;
    float    sfxVolume        = 100.f;
    GameMode gameMode         = GameMode::JOUEUR_VS_JOUEUR;
    bool     fullscreen       = true;
};
