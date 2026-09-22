#include "../include/Entity.h"

//  JOUEUR
Joueur::Joueur(): isMe(false), numberGraine(0), gain(0) {}

int  Joueur::GetGain() const { 
    return gain; 
}

void Joueur::AddGain(int add) { 
    gain += add; 
}

void Joueur::ResetGain() { 
    gain = 0; 
}

bool Joueur::IsMyRound() const { 
    return isMe; 
}

void Joueur::SetIsMyRound(bool v) { 
    isMe = v; 
}
    
void Joueur::SetNumberGraine(int n)  { 
    numberGraine = n; 
}

int  Joueur::GetNumberGraine() const { 
    return numberGraine; 
}

void Joueur::Destroy() { 

}


//  TROU
Trou::Trou(SDL_Renderer* renderer)
    : numberGraine(5), ID(0), statutTrou(CINQ), statut(NOT_PRISE)
{
    positionTrou = {0.f, 0.f};

    for (int i = 0; i < MAX_TEXTURE; i++) {
        std::string path = "assets/trou" + std::to_string(i) + ".png";
        SDL_Surface* surf = IMG_Load(path.c_str());
        if (surf) {
            texture[i] = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_DestroySurface(surf);
        } else {
            texture[i] = nullptr;
            SDL_Log("WARN: impossible de charger ", SDL_GetError());
        }
    }
}

int  Trou::GetNumberGraine() const { 
    return numberGraine; 
}

Statut    Trou::GetStatut() const { 
    return statut; 
}

StatutTrou Trou::GetStatutTrou() const { 
    return statutTrou;
}

int  Trou::GetID() const { 
    return ID; 
}

Vector2D  Trou::GetPosition() const { 
    return positionTrou; 
}


void Trou::SetNumberGraine(int n) {
    numberGraine = n;
    int clamped = (n > 15) ? 15 : (n < 0 ? 0 : n);
    statutTrou  = static_cast<StatutTrou>(clamped);
}

void Trou::SetStatut(Statut s) { 
    statut = s; 
}
void Trou::SetStatutTrou(StatutTrou s) { 
    statutTrou = s; 
}

void Trou::SetID(int id) {
    ID = id; 
}

void Trou::SetPosition(Vector2D pos) { 
    positionTrou = pos; 
}

void Trou::Render(SDL_Renderer* renderer, SDL_FRect position) {

    int idx = (numberGraine > 15) ? 15 : numberGraine;
    if (texture[idx]) {
        SDL_RenderTexture(renderer, texture[idx], nullptr, &position);
    }

}

void Trou::Destroy() {
    for (int i = 0; i < MAX_TEXTURE; i++) {
        if (texture[i]) {
            SDL_DestroyTexture(texture[i]);
            texture[i] = nullptr;
        }
    }
}
