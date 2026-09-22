#pragma once
#include "Utils.h"

//Joueur
class Joueur {
private:
    bool isMe;          
    int  numberGraine;
    int  gain;

public:
    Joueur();

    int  GetGain() const;
    void AddGain(int add);
    void ResetGain();

    bool IsMyRound() const;
    void SetIsMyRound(bool v);   

    void SetNumberGraine(int number);
    int  GetNumberGraine() const;

    void Destroy();   
};

//Trou
class Trou {
private:
    int         numberGraine;
    int         ID;
    StatutTrou  statutTrou;
    SDL_Texture* texture[MAX_TEXTURE]; 
    Statut      statut;
    Vector2D    positionTrou;

public:
    Trou(SDL_Renderer* renderer);

    int        GetNumberGraine() const;
    void       SetNumberGraine(int n);

    Statut     GetStatut() const;
    void       SetStatut(Statut s);

    StatutTrou GetStatutTrou() const;
    void       SetStatutTrou(StatutTrou s);

    void       SetID(int id);
    int        GetID() const;

    void       SetPosition(Vector2D pos);
    Vector2D   GetPosition() const;

    void       Render(SDL_Renderer* renderer, SDL_FRect position);
    void       Destroy();
};
