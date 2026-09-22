#include "../include/Logic.h"

// Ordre de parcours horaire
static const int clockwise[MAX_TROU] = {
    0, 1, 2, 3, 4, 5, 6,      // ID 1 → 7
    13, 12, 11, 10, 9, 8, 7   // ID 14 → 8
};

// Table inverse : pour un idx donné, sa position dans clockwise
static int cwPos[MAX_TROU];

void InitClockwise() {
    for (int i = 0; i < MAX_TROU; i++) {
        cwPos[clockwise[i]] = i;
    }
        
}

// Retourne l'index du prochain trou dans le sens horaire
int NextIdx(int idx) {
    int pos = cwPos[idx];
    return clockwise[(pos + 1) % MAX_TROU];
}

// Retourne l'index du trou precedent sans anti horaire
int PrevIdx(int idx) {
    int pos = cwPos[idx];
    return clockwise[(pos - 1 + MAX_TROU) % MAX_TROU];
}

// TrouCenter — retourne le centre en pixels du trou d'ID donné (1..14)
Vector2D TrouCenter(int ID) {
    SDL_FRect r = TrouPosition(ID);
    return { r.x + r.w * 0.5f, r.y + r.h * 0.5f };
}

// CheckPrise
void CheckPrise(Trou** trou) {
    for (int i = 0; i < MAX_TROU; i++) {
        int n = trou[i]->GetNumberGraine();
        trou[i]->SetStatut((n == 2 || n == 3) ? PRISE : NOT_PRISE);
    }
}

// CheckGain
void CheckGain(Trou** trou, int lastIdx, Joueur* joueur, int currentPlayer) {
    int i = lastIdx;
    CheckPrise(trou);

    while (trou[i]->GetStatut() == PRISE) {
        bool campAdverse = (currentPlayer == 1 && i > 6) ||
                            (currentPlayer == 2 && i <= 6);

        if (!campAdverse) break; // on ne capture jamais dans son propre camp -> arrêt de la chaîne

        joueur->AddGain(trou[i]->GetNumberGraine());
        trou[i]->SetNumberGraine(0);
        trou[i]->SetStatut(NOT_PRISE);

        i = PrevIdx(i);      // on remonte dans le sens ANTI-horaire réel
        CheckPrise(trou);    // recalculer les statuts après la capture
    }
}

// LastIdx
int LastIdx(Trou** trou, int idx) {
    int graines = trou[idx]->GetNumberGraine();
    if (graines == 0) return idx;

    int last = idx;
    int count = graines;
    while (count > 0) {
        last = NextIdx(last);
        if (last == idx) continue;
        count--;
    }
    return last;
}


// PlayGain
void PlayGain(int idx, Trou** trou) {
    int graines = trou[idx]->GetNumberGraine();
    if (graines == 0) return;

    trou[idx]->SetNumberGraine(0);

    int current = idx;
    for (int k = 0; k < graines; k++) {
        current = NextIdx(current);
        if (current == idx)            
            current = NextIdx(current);
        trou[current]->SetNumberGraine(trou[current]->GetNumberGraine() + 1);
    }
}


// TrouPosition
SDL_FRect TrouPosition(int ID, float DX, float DY, int w, int h, float gap) {
    SDL_FRect r;
    float steep;
    steep = w + 30.0f;
    if (ID >= 1 && ID <= 7) {

        if (ID == 1) {
          
            r = { (float)((DX + (ID - 1) * w) - 10), (float)(DY), (float)w, (float)h };  
        } else {
            
            r = { (float)((DX + (ID - 1) * steep)), (float)(DY), (float)w, (float)h };

        }
        
        
    } else if (ID >= 8 && ID <= 14) {
        if (ID == 8) {
            
            r = { (float)((DX + (ID - 8) * w) - 10), (float)(DY + h + gap ), (float)w, (float)h };
        } else {
            
            r = { (float)((DX + (ID - 8) * steep)), (float)(DY + h + gap ), (float)w, (float)h };
        } 
        
        
    }
    return r;
}

// GetClickedID
int GetClickedID(int x, int y, float DX, float DY, int w, int h, float gap) {
    // Rangée du haut : ID 1..7
    w = w + 30.0f;
    if (y >= DY && y < DY + h) {
        if (x >= DX && x < DX + 7 * w) {

            int id = (x - DX) / w + 1;

            if (id >= 1 && id <= 7) {
                return id;
            }
            
        }
    }
    // Rangée du bas : ID 8..14
    int yBot = DY + h + gap;
    if (y >= yBot && y < yBot + h) {

        if (x >= DX && x < DX + 7 * w) {

            int id = (x - DX) / w + 8;

            if (id >= 8 && id <= 14) {

                return id;
            }
            
        }
    }
    return -1;
}

//  RenderMap
void RenderMap(SDL_Renderer* renderer, Trou** trou) {

    SDL_Surface* bg = IMG_Load("assets/Background.png");
    SDL_Texture* bgTex = SDL_CreateTextureFromSurface(renderer, bg);
    SDL_DestroySurface(bg);
    SDL_FRect positionBg = {50.f, 225.f, 1500.f, 551.f};
    SDL_RenderTexture(renderer, bgTex, NULL, &positionBg);

    for (int i = 0; i < MAX_TROU; i++) {
        SDL_FRect pos = TrouPosition(i + 1);
        trou[i]->Render(renderer, pos);
    }
}

// IsGameOver
bool IsGameOver(Trou** trou, Joueur* joueur1, Joueur* joueur2) {
    bool camp1Vide = true, camp2Vide = true, hasWinner = false;
    for (int i = 0; i < 7;  i++) {
        if (trou[i]->GetNumberGraine() > 0) { 
            camp1Vide = false; 
            break;
        }    
        if (joueur1->GetGain() >= 37) {
            hasWinner = true;
        }
    }
    for (int i = 7; i < 14; i++) { 
        if (trou[i]->GetNumberGraine() > 0) { 
            camp2Vide = false; 
            break; 
        }
        if (joueur2->GetGain() >= 37) {
            hasWinner = true;
        }
    }
    
    return camp1Vide || camp2Vide || hasWinner;
}
