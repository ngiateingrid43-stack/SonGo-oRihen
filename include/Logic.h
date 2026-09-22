#pragma once
#include "Utils.h"
#include "Entity.h"

//Logique de jeu
void InitClockwise();
int  NextIdx(int idx);
// Retourne l'index du trou précédent dans le sens horaire (donc capture anti-horaire)
int PrevIdx(int idx); 

//Marque les trous à 2 ou 3 graines comme PRISE, les autres NOT_PRISE
void CheckPrise(Trou** trou);

//Collecte les graines du trou cliqué + capture en chaîne vers l'arrière
void CheckGain(Trou** trou, int lastID, Joueur* joueur, int currentPlayer);

// Retourne l'index (0-based) du dernier trou semé après PlayGain
int LastIdx(Trou** trou, int idx);

// Distribue les graines du trou d'index idx dans les suivants (circulaire)
void PlayGain(int idx, Trou** trou);

// Calcule la SDL_FRect d'affichage pour un trou d'ID 1..14
SDL_FRect TrouPosition(int ID, float DX = 130.0f, float DY = 262.5f, int w  = 168, int h = 188, float gap = 98.0f);

// Retourne le centre en pixels d'un trou d'ID 1..14
Vector2D TrouCenter(int ID);

// Retourne l'ID (1..14) du trou cliqué, ou -1
int GetClickedID(int x, int y, float DX = 130.0f, float DY = 262.5f, int w  = 168, int h = 188, float gap = 98.0f);

// Dessine tous les trous
void RenderMap(SDL_Renderer* renderer, Trou** trou);

// Vérifie si la partie est terminée (un camp n'a plus de graines)
bool IsGameOver(Trou** trou, Joueur* joueur1, Joueur* joueur2);
