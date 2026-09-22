#pragma once
#include"Utils.h"

class SoundManager {

    private:

        sf::Music music[MAX_MUSIC];
        sf::SoundBuffer buffer[MAX-BUFFER];
        sf::Sound sound[MAX_SOUNG];

    public:

        SoundManager();


};