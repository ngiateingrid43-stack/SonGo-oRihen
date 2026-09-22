#include "../include/Game.h"
#include "../include/Logic.h"

// Sous-titres anglais pour chaque frame de l'histoire (6 frames)
static const char* STORY_SUBTITLES[6] = {
    "In the heart of Cameroon, under the great silk-cotton tree, the elders played Songo'o to settle disputes.",
    "The seeds, carved from baobab wood, were sown with wisdom — each move a silent prayer to the ancestors.",
    "Two warriors face each other at the board. Only patience and cunning will lead to victory.",
    "The village gathers at sunset. The rhythm of the game echoes the rhythm of life itself.",
    "To capture your opponent's seeds is to earn respect — not just from men, but from the spirits of the land.",
    "Songo'o lives on. Passed from father to child, it binds generations in a shared heritage of strategy and soul.",
};


//  Constructeur

Game::Game()
    : isRunning(false), state(GameState::STORY), prevState(GameState::MENU),
      window(nullptr), renderer(nullptr),
      joueur1(nullptr), joueur2(nullptr),
      currentPlayer(1), statusMsg("Au tour du Joueur 1"),
      font(nullptr), texture(nullptr), handTexture(nullptr),
      lastTick(0),
      m_winW(WIN_W), m_winH(WIN_H),
      frameCount(0), currentFrame(0),
      frameDuration(1.2 / 30.0), accumulator(0.0)
{
    for (int i = 0; i < MAX_TROU; i++) trou[i] = nullptr;
}


//  InitSDL — plein écran automatique

void Game::InitSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return;
    }
    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        return;
    }

    // --- Récupérer la résolution de l'écran ---
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(displayID);
    if (dm) {
        m_winW = dm->w;
        m_winH = dm->h;
    } else {
        m_winW = WIN_W;
        m_winH = WIN_H;
    }

    // Créer fenêtre en plein écran
    window = SDL_CreateWindow("SONGO'O", m_winW, m_winH,
                              SDL_WINDOW_FULLSCREEN);
    if (!window) {
        // Fallback fenêtre normale si plein écran échoue
        window = SDL_CreateWindow("SONGO'O", WIN_W, WIN_H, 0);
        m_winW = WIN_W;
        m_winH = WIN_H;
        if (!window) { SDL_Log("SDL_CreateWindow: %s", SDL_GetError()); exit(1); }
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) { SDL_Log("SDL_CreateRenderer: %s", SDL_GetError()); exit(1); }

    // Mise à l'échelle logique → rendu toujours en 1600×900 interne
    SDL_SetRenderLogicalPresentation(renderer, WIN_W, WIN_H,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    SDL_Surface* icon = IMG_Load("assets/Icon.png");
    if (icon) { SDL_SetWindowIcon(window, icon); SDL_DestroySurface(icon); }

    SDL_SetRenderVSync(renderer, 1);

    // Police
    const std::string fontPath = "C:/Windows/Fonts/arialbd.ttf";

    if (!m_fonts.load(fontPath)) {
        if (!m_fonts.load("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf")) {
            std::cerr << "Aucune police disponible.\n";
            return;
        }
    }

    // Frames numerotees de 3 a 158 (RIHEN LOGO_00003.png ... RIHEN LOGO_00158.png)
    if (!loadFrames("assets/animrihen/", 3, 158)) {
        return;
    }

}

//  InitAudio

void Game::InitAudio() {
    // Musique de fond africaine camerounaise
    if (!bgMusic.openFromFile("audio/background.mp3")) {
        SDL_Log("WARN: Musique de fond introuvable  background.mp3");
    } else {
        bgMusic.setLooping(true);
        bgMusic.setVolume(options.musicEnabled ? options.musicVolume : 0.f);
        
    }

    // Musique de fond africaine camerounaise
    if (creditMusic.openFromFile("audio/credit.mp3")) {
       creditMusic.setLooping(true);
       creditMusic.setVolume(options.musicVolume);
       m_creditMusicLoaded = true;
    } else {
        SDL_Log("WARN: Musique de fond introuvable  credit.mp3");

    }

    // Son dépôt de graine
    if (!depositBuffer.loadFromFile("audio/deposit.mp3")) {
        SDL_Log("WARN: Son dépôt introuvable");
    } else {
        depositSound.emplace(depositBuffer);
        depositSound->setVolume(options.sfxVolume);
    }

    // Son ramassage de graine
    if (!pickupBuffer.loadFromFile("audio/pickup.mp3")) {
        SDL_Log("WARN: Son ramassage introuvable");
    } else {
        pickupSound.emplace(pickupBuffer);
        pickupSound->setVolume(options.sfxVolume);
    }

    // Tambour pour les tours
    if (!drumBuffer.loadFromFile("audio/tambour.mp3")) {
        SDL_Log("WARN: Son tambour introuvable (drum.wav / tambour.wav)");
    } else {
        drumSound.emplace(drumBuffer);
        drumSound->setVolume(options.sfxVolume);
    }
}


//  Son helpers

void Game::PlayDepositSound() {
    if (!options.soundEnabled) return;
    if (depositSound.has_value()) {
        depositSound->stop();
        depositSound->play();
    }
}

void Game::PlayPickupSound() {
    if (!options.soundEnabled) return;
    if (pickupSound.has_value()) {
        pickupSound->stop();
        pickupSound->play();
    }
}

void Game::PlayDrumForPlayer(int ) {
    if (!options.drumEnabled) return;
    if (drumSound.has_value()) {
        if(drumSound->getStatus() == sf::Sound::Status::Playing){
          drumSound->stop();  
        } else {
            drumSound->play();
        } 
        
    }
}


//  InitImGui — style africain camerounais

void Game::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 14.f;
    style.FrameRounding    = 9.f;
    style.ItemSpacing      = ImVec2(10, 9);
    style.FramePadding     = ImVec2(14, 7);
    style.WindowBorderSize = 2.5f;
    style.PopupRounding    = 10.f;
    style.ScrollbarRounding= 8.f;

    ImVec4* colors = style.Colors;
    // Palette bois d'ébène & or & terre cuite
    colors[ImGuiCol_WindowBg]      = ImVec4(0.09f, 0.05f, 0.02f, 0.95f);
    colors[ImGuiCol_TitleBg]       = ImVec4(0.45f, 0.20f, 0.04f, 1.f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.62f, 0.28f, 0.05f, 1.f);
    colors[ImGuiCol_Button]        = ImVec4(0.55f, 0.22f, 0.04f, 1.f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.82f, 0.40f, 0.08f, 1.f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.36f, 0.14f, 0.02f, 1.f);
    colors[ImGuiCol_Header]        = ImVec4(0.50f, 0.22f, 0.05f, 0.85f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.72f, 0.38f, 0.10f, 1.f);
    colors[ImGuiCol_FrameBg]       = ImVec4(0.18f, 0.10f, 0.04f, 1.f);
    colors[ImGuiCol_FrameBgHovered]= ImVec4(0.28f, 0.15f, 0.05f, 1.f);
    colors[ImGuiCol_Text]          = ImVec4(1.0f,  0.92f, 0.72f, 1.f);
    colors[ImGuiCol_Border]        = ImVec4(0.72f, 0.42f, 0.10f, 0.85f);
    colors[ImGuiCol_Separator]     = ImVec4(0.72f, 0.42f, 0.10f, 0.65f);
    colors[ImGuiCol_SliderGrab]    = ImVec4(0.82f, 0.50f, 0.12f, 1.f);
    colors[ImGuiCol_SliderGrabActive]= ImVec4(1.f, 0.70f, 0.20f, 1.f);
    colors[ImGuiCol_CheckMark]     = ImVec4(0.95f, 0.72f, 0.18f, 1.f);

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}


//  InitEntities

void Game::InitEntities() {
    joueur1 = new Joueur();
    joueur2 = new Joueur();
    for (int i = 0; i < MAX_TROU; i++) {
        trou[i] = new Trou(renderer);
        trou[i]->SetID(i + 1);
    }
    InitClockwise();
    joueur1->SetIsMyRound(true);
    joueur2->SetIsMyRound(false);
    currentPlayer = 1;
    statusMsg = "Au tour du Joueur 1";

    SDL_Surface* s = IMG_Load("assets/hand.png");
    if (s) {
        handTexture = SDL_CreateTextureFromSurface(renderer, s);
        SDL_DestroySurface(s);
    } else {
        handTexture = nullptr;
    }
    storyAnim.Load(renderer);
}


//  HandleClick

void Game::HandleClick(int x, int y, Joueur* joueur1, Joueur* joueur2) {
    if (state != GameState::PLAYING) return;
    if (anim.active) return;

    float lx, ly;
    SDL_RenderCoordinatesFromWindow(renderer, (float)x, (float)y, &lx, &ly);
    x = (int)lx;
    y = (int)ly;

    // Mode IA : bloquer les clics du joueur 2 si c'est son tour
    if (options.gameMode == GameMode::IA_VS_JOUEUR && currentPlayer == 2) return;

    int id = GetClickedID(x, y);
    if (id == -1) return;
    int idx = id - 1;

    bool validClick = false;
    if (currentPlayer == 1 && idx >= 0 && idx <= 6)  validClick = true;
    if (currentPlayer == 2 && idx >= 7 && idx <= 13) validClick = true;

    if (!validClick) { statusMsg = "Ce n'est pas votre camp !"; return; }
    if (trou[idx]->GetNumberGraine() == 0) { statusMsg = "Ce trou est vide !"; return; }

    Vector2D center = TrouCenter(id);
    anim.active      = true;
    anim.srcIdx      = idx;
    anim.curIdx      = idx;
    anim.grainesLeft = trou[idx]->GetNumberGraine();
    anim.handX       = center.x;
    anim.handY       = center.y;
    anim.dropping    = true;
    anim.pauseTimer  = 0.3f;
    lastDropping     = false;

    trou[idx]->SetNumberGraine(0);
    PlayPickupSound();

    int nextIdx = NextIdx(idx);
    if (nextIdx == idx) nextIdx = NextIdx(nextIdx);
    Vector2D c   = TrouCenter(nextIdx + 1);
    anim.targetX = c.x;
    anim.targetY = c.y;

    lastTick = SDL_GetTicks();
    statusMsg = "Semis en cours...";
}



//  IA simpliste

int Game::ComputeIAMove() {
    // Choisit le premier trou non vide du camp IA (trous 7..13)
    for (int i = 7; i <= 13; i++) {
        if (trou[i]->GetNumberGraine() > 0) return i + 1; // retourne ID
    }
    return -1;
}

//Chargement des imagge de l'animation deu logo
bool Game::loadFrames(const std::string& folder, int firstFrame, int lastFrame) {
    frames.reserve(lastFrame - firstFrame + 1);

    // --- Dessiner l'animation "Noge" pendant le chargement ---
    // Texte "Noge" en noir, préparé une seule fois (texture réutilisée à chaque frame)
    SDL_Color noirNoge = { 0, 0, 0, 255 };
    TTF_Font* nogeFont = m_fonts.huge ? m_fonts.huge : m_fonts.large;
    SDL_Texture* nogeTextTex = nullptr;
    float nogeTextW = 0.f, nogeTextH = 0.f;
    if (nogeFont) {
        SDL_Surface* nogeSurf = TTF_RenderText_Blended(nogeFont, "Noge", 0, noirNoge);
        if (nogeSurf) {
            nogeTextTex = SDL_CreateTextureFromSurface(renderer, nogeSurf);
            nogeTextW = (float)nogeSurf->w;
            nogeTextH = (float)nogeSurf->h;
            SDL_DestroySurface(nogeSurf);
        }
    }

    auto renderNoge = [&](float glowAlpha) {
        float W = (float)WIN_W;
        float H = (float)WIN_H;

        // Fond (couleur au choix : bleu nuit profond)
        SDL_SetRenderDrawColor(renderer, 22, 18, 46, 255);
        SDL_RenderClear(renderer);

        // Cercle centré (couleur au choix : or) — grossit et rétrécit avec le glow
        float cx = W * 0.5f;
        float cy = H * 0.5f;
        float baseR   = H * 0.22f;
        float circleR = baseR * (0.8f + 0.2f * glowAlpha);

        // Halo lumineux pulsant derrière le cercle
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        int haloLayers = 18;
        for (int h = haloLayers; h >= 1; h--) {
            float r = circleR * 1.08f + h * 4.5f;
            Uint8 a = (Uint8)(glowAlpha * (1.f - (float)h / haloLayers) * 180.f);
            SDL_SetRenderDrawColor(renderer, 255, 196, 20, a);
            // Cercle approximé par des rectangles concentriques
            for (float angle = 0; angle < 360.f; angle += 2.f) {
                float rad = angle * 3.14159f / 180.f;
                float px = cx + cosf(rad) * r;
                float py = cy + sinf(rad) * r;
                SDL_FRect dot = { px - 1.5f, py - 1.5f, 3.f, 3.f };
                SDL_RenderFillRect(renderer, &dot);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Cercle plein blanc, rempli par triangles (fan depuis le centre)
        const int CIRCLE_POINTS = 60;
        float circleX[CIRCLE_POINTS], circleY[CIRCLE_POINTS];
        for (int p = 0; p < CIRCLE_POINTS; p++) {
            float angle = (float)p * (2.f * 3.14159f / CIRCLE_POINTS);
            circleX[p] = cx + cosf(angle) * circleR;
            circleY[p] = cy + sinf(angle) * circleR;
        }
        SDL_FColor blanc = { 1.0f, 1.0f, 1.0f, 1.0f };
        for (int p = 0; p < CIRCLE_POINTS; p++) {
            int next = (p + 1) % CIRCLE_POINTS;
            SDL_Vertex verts[3] = {
                { {cx,          cy},          blanc, {0,0} },
                { {circleX[p],  circleY[p]},  blanc, {0,0} },
                { {circleX[next],circleY[next]}, blanc, {0,0} },
            };
            SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
        }

        // Texte "Noge" en noir, centré au milieu du cercle
        if (nogeTextTex) {
            SDL_FRect dst = {
                cx - nogeTextW * 0.5f,
                cy - nogeTextH * 0.5f,
                nogeTextW, nogeTextH
            };
            SDL_RenderTexture(renderer, nogeTextTex, nullptr, &dst);
        }

        SDL_RenderPresent(renderer);
    };

    // Afficher l'animation immédiatement (glow neutre)
    renderNoge(0.7f);

    // Charger les frames en faisant pulser le cercle
    int total = lastFrame - firstFrame + 1;
    float glowPhase = 0.f;

    for (int i = firstFrame; i <= lastFrame; ++i) {
        std::ostringstream oss;
        oss << folder << "RIHEN LOGO_" << std::setw(5) << std::setfill('0') << i << ".png";
        std::string path = oss.str();

        SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
        if (!tex) {
            std::cerr << "Erreur chargement " << path << ": " << SDL_GetError() << std::endl;
            if (nogeTextTex) SDL_DestroyTexture(nogeTextTex);
            return false;
        }
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
        frames.push_back(tex);

        // Rafraîchir l'animation "Noge" toutes les 10 frames pour faire pulser le cercle
        if ((i - firstFrame) % 10 == 0) {
            glowPhase += 0.4f;
            float glow = 0.55f + 0.45f * sinf(glowPhase);
            renderNoge(glow);
        }
    }

    if (nogeTextTex) SDL_DestroyTexture(nogeTextTex);

    frameCount = static_cast<int>(frames.size());
    currentFrame = 0;
    std::cout << frameCount << " frames chargees." << std::endl;
    return true;
}

//update animation du logo
void Game::update(double deltaTime) {
    if (frameCount == 0) return;

    // Animation deja terminee : ferme la fenetre
    if (currentFrame >= frameCount - 1) {
        currentFrame = frameCount - 1;
        isRunningLogo = false;
        return;
    }

    accumulator += deltaTime;
    while (accumulator >= frameDuration && currentFrame < frameCount - 1) {
        currentFrame++;
        accumulator -= frameDuration;
    }
}

//afficharge de l'animation du logo
void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (frameCount > 0) {
        SDL_Texture* tex = frames[currentFrame];
        float texW, texH;
        SDL_GetTextureSize(tex, &texW, &texH);

        // Centrage (l'image fait 1920x1080, on l'adapte si la fenetre est plus petite)
        float scale = std::min((float)m_winW / texW, (float)m_winH / texH);
        float drawW = texW * scale;
        float drawH = texH * scale;

        SDL_FRect dst;
        dst.w = drawW;
        dst.h = drawH;
        dst.x = (m_winW - drawW) / 2.0f;
        dst.y = (m_winH - drawH) / 2.0f;

        SDL_RenderTexture(renderer, tex, nullptr, &dst);
    }

    SDL_RenderPresent(renderer);
}

//  UpdateAnimation

void Game::UpdateAnimation() {
    if (!anim.active) return;

    Uint64 now = SDL_GetTicks();
    float  dt  = (float)(now - lastTick) / 1000.f;
    lastTick   = now;
    if (dt > 0.1f) dt = 0.1f;

    if (anim.dropping) {
        // Détecter le bord descendant de "dropping" pour jouer le son
        if (!lastDropping) {
            // on vient d'entrer en phase dropping = dépôt effectué
        }
        lastDropping = true;

        anim.pauseTimer -= dt;
        if (anim.pauseTimer > 0.f) return;

        anim.dropping = false;
        lastDropping  = false;

        if (anim.grainesLeft == 0) {
            FinishAnimation();
            return;
        }

        anim.curIdx = NextIdx(anim.curIdx);
        if (anim.curIdx == anim.srcIdx)
            anim.curIdx = NextIdx(anim.curIdx);

        Vector2D c   = TrouCenter(anim.curIdx + 1);
        anim.targetX = c.x;
        anim.targetY = c.y;
        return;
    }

    lastDropping = false;

    float dx   = anim.targetX - anim.handX;
    float dy   = anim.targetY - anim.handY;
    float dist = SDL_sqrtf(dx * dx + dy * dy);
    float step = anim.speed * dt;

    if (dist <= step || dist < 1.f) {
        anim.handX = anim.targetX;
        anim.handY = anim.targetY;

        // *** Son dépôt ***
        PlayDepositSound();

        trou[anim.curIdx]->SetNumberGraine(
            trou[anim.curIdx]->GetNumberGraine() + 1);
        anim.grainesLeft--;

        anim.dropping   = true;
        anim.pauseTimer = 0.12f;
    } else {
        anim.handX += (dx / dist) * step;
        anim.handY += (dy / dist) * step;
    }
}


//  FinishAnimation

void Game::FinishAnimation() {
    anim.active = false;
    int last = anim.curIdx;

    CheckPrise(trou);
    Joueur* joueurActif = (currentPlayer == 1) ? joueur1 : joueur2;
    CheckGain(trou, last, joueurActif, currentPlayer);

    if (IsGameOver(trou, joueur1, joueur2)) {
        CollectRemainingGraines();
        state = GameState::GAME_OVER;
        return;
    }

    SwitchPlayer();
}


//  RenderHand

void Game::RenderHand() {
    if (!anim.active) return;

    if (handTexture) {
        float hw = 168.f, hh = 188.f;
        SDL_FRect dst = {
            anim.handX - hw * 0.5f,
            anim.handY - hh * 0.5f,
            hw, hh
        };
        SDL_RenderTexture(renderer, handTexture, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, 230, 120, 20, 220);
        for (int r = 0; r < 20; r++) {
            SDL_FRect dot = { anim.handX - r * 0.5f, anim.handY - r * 0.5f, (float)r, (float)r };
            SDL_RenderFillRect(renderer, &dot);
        }
    }

    if (anim.grainesLeft > 0) {
        SDL_Color blanc = { 255, 255, 200, 255 };
        TTF_Font* f = TTF_OpenFont("C:/Windows/Fonts/timesbd.ttf", 32);
        if (!f) f = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 32);
        if (!f) f = TTF_OpenFont("timesbd.ttf", 32);
        if (f) {
            std::string txt = std::to_string(anim.grainesLeft);
            SDL_Surface* surf = TTF_RenderText_Blended(f, txt.c_str(), 0, blanc);
            if (surf) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FRect dst = {
                    anim.handX - surf->w * 0.5f,
                    anim.handY - 52.f,
                    (float)surf->w, (float)surf->h
                };
                SDL_RenderTexture(renderer, t, nullptr, &dst);
                SDL_DestroyTexture(t);
                SDL_DestroySurface(surf);
            }
            TTF_CloseFont(f);
        }
    }
}


//  SwitchPlayer

void Game::SwitchPlayer() {
    currentPlayer = (currentPlayer == 1) ? 2 : 1;
    joueur1->SetIsMyRound(currentPlayer == 1);
    joueur2->SetIsMyRound(currentPlayer == 2);
    statusMsg = "Au tour du Joueur " + std::to_string(currentPlayer);

    // Tambour au changement de tour
    PlayDrumForPlayer(currentPlayer);

    // Mode IA : si c'est le tour du joueur 2, jouer automatiquement
    if (options.gameMode == GameMode::IA_VS_JOUEUR && currentPlayer == 2) {
        int iaID = ComputeIAMove();
        if (iaID != -1) {
            int idx = iaID - 1;
            Vector2D center = TrouCenter(iaID);
            anim.active      = true;
            anim.srcIdx      = idx;
            anim.curIdx      = idx;
            anim.grainesLeft = trou[idx]->GetNumberGraine();
            anim.handX       = center.x;
            anim.handY       = center.y;
            anim.dropping    = true;
            anim.pauseTimer  = 0.5f;
            lastDropping     = false;
            trou[idx]->SetNumberGraine(0);
            PlayPickupSound();
            int nextIdx = NextIdx(idx);
            if (nextIdx == idx) nextIdx = NextIdx(nextIdx);
            Vector2D c   = TrouCenter(nextIdx + 1);
            anim.targetX = c.x;
            anim.targetY = c.y;
            lastTick = SDL_GetTicks();
            statusMsg = "L'IA réfléchit...";
        }
    }
}

void Game::CollectRemainingGraines() {
    for (int i = 0; i < 7;  i++) { joueur1->AddGain(trou[i]->GetNumberGraine()); trou[i]->SetNumberGraine(0); }
    for (int i = 7; i < 14; i++) { joueur2->AddGain(trou[i]->GetNumberGraine()); trou[i]->SetNumberGraine(0); }
}


//  RenderBackground

void Game::RenderBackground() {
    SDL_SetRenderDrawColor(renderer, 15, 8, 3, 255);
    SDL_RenderClear(renderer);
    if(state == GameState::MENU || (prevState == GameState::MENU && state == GameState::OPTIONS_SCREEN)) {
        // Fond menu : texture ou couleur bois sombre
        SDL_Surface* bgM = IMG_Load("assets/Songo'oIcon.png");
        if (bgM) {
            SDL_Texture* bgMTex = SDL_CreateTextureFromSurface(renderer, bgM);
            SDL_DestroySurface(bgM);
            SDL_FRect positionBg = { 0.f, 0.f, (float)WIN_W, (float)WIN_H };
            SDL_RenderTexture(renderer, bgMTex, NULL, &positionBg);
            SDL_DestroyTexture(bgMTex);
        }
    }
    
}


//  RenderPlayerGlow  — halo lumineux sur le camp du joueur actif
void Game::RenderPlayerGlow() {
    if (state != GameState::PLAYING) return;

    glowTime += 0.016f;  // ~60fps
    float pulse = 0.55f + 0.45f * sinf(glowTime * 3.5f);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Joueur 1 : rangée du haut (trous 1..7), couleur or chaud
    // Joueur 2 : rangée du bas  (trous 8..14), couleur bleu safir

    if (currentPlayer == 1) {
        // Halo or derrière les trous 1..7
        Uint8 alpha = (Uint8)(60.f * pulse);
        SDL_SetRenderDrawColor(renderer, 255, 185, 30, alpha);
        // Rectangle englobant la rangée haute
        SDL_FRect glow = { 40.f, 215.f, 1520.f, 270.f };
        SDL_RenderFillRect(renderer, &glow);
        // Bordure lumineuse
        SDL_SetRenderDrawColor(renderer, 255, 200, 50, (Uint8)(120.f * pulse));
        SDL_RenderRect(renderer, &glow);
    } else {
        // Halo bleu derrière les trous 8..14
        Uint8 alpha = (Uint8)(60.f * pulse);
        SDL_SetRenderDrawColor(renderer, 60, 160, 255, alpha);
        SDL_FRect glow = { 40.f, 511.f, 1520.f, 270.f };
        SDL_RenderFillRect(renderer, &glow);
        SDL_SetRenderDrawColor(renderer, 100, 180, 255, (Uint8)(120.f * pulse));
        SDL_RenderRect(renderer, &glow);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}


//  RenderStorySubtitle — sous-titres anglais
void Game::RenderStorySubtitle() {
    if (state != GameState::STORY) return;
    // Synchroniser l'index avec storyAnim (approximation via temps)
    // On utilise un texte fixe par frame
    // On récupère l'index de frame courant via la méthode de StoryAnimation
    // Pour l'instant on affiche toujours le dernier texte chargé

    // Texte en bas de l'écran
    TTF_Font* subFont = m_fonts.small;
    if (!subFont) return;

    // Fond semi-transparent derrière le sous-titre
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
    SDL_FRect bg = { 0.f, 820.f, (float)WIN_W, 70.f };
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Texte centré
    int frameIdx = storySubtitleFrame;
    if (frameIdx < 0 || frameIdx >= 6) frameIdx = 0;
    const char* txt = STORY_SUBTITLES[frameIdx];

    SDL_Color col = { 255, 240, 200, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended_Wrapped(subFont, txt, 0, col, WIN_W - 80);
    if (surf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        float tw = (float)surf->w;
        float th = (float)surf->h;
        SDL_FRect dst = { (WIN_W - tw) * 0.5f, 828.f, tw, th };
        SDL_RenderTexture(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
        SDL_DestroySurface(surf);
    }
}


//  RenderCreditsScreen — crédits défilants

void Game::RenderCreditsScreen() {
    // Fond dégradé noir/brun africain
    SDL_SetRenderDrawColor(renderer, 8, 4, 2, 255);
    SDL_RenderClear(renderer);

    // Bandeau kente en haut
    const ImU32 kCols[3] = {
        IM_COL32(180, 70, 15, 255),
        IM_COL32(210, 160, 30, 255),
        IM_COL32(30, 100, 40, 255)
    };

    float totalScroll = m_creditsScrollY;

    // Données du crédit défilant
    struct CredLine { const char* text; float size; bool isTitle; Uint8 r,g,b; };
    static const CredLine lines[] = {
        { "SONGO'O", 3.0f, true, 255, 200, 40 },
        { "Jeu Traditionnel Camerounais", 1.2f, false, 210, 160, 60 },
        { "", 0.8f, false, 0, 0, 0 },
        { "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", 0.8f, false, 140, 80, 20 },
        { "", 0.8f, false, 0, 0, 0 },
        { "AUTEUR & DÉVELOPPEUR", 1.1f, true, 255, 160, 30 },
        { "NGIATE KAMNANG INGRID", 1.3f, false, 255, 230, 150 },
        { "", 0.7f, false, 0, 0, 0 },
        { "ÉTUDIANT INGÉNIEUR", 1.0f, false, 200, 160, 80 },
        { "École Nationale Supérieure Polytechnique de Yaoundé", 0.9f, false, 200, 160, 80 },
        { "ENSPY", 1.1f, false, 255, 185, 40 },
        { "Filière : ARTS NUMÉRIQUE INGÉNIEUR", 1.0f, false, 200, 160, 80 },
        { "", 0.8f, false, 0, 0, 0 },
        { "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", 0.8f, false, 140, 80, 20 },
        { "", 0.8f, false, 0, 0, 0 },
        { "REMERCIEMENTS", 1.2f, true, 255, 160, 30 },
        { "Ma chère famille", 1.0f, false, 230, 200, 130 },
        { "qui m'a toujours soutenue et encouragée", 0.9f, false, 180, 150, 100 },
        { "", 0.7f, false, 0, 0, 0 },
        { "Mes encadreurs a RIHEN", 1.0f, false, 230, 200, 130 },
        { "pour leur soutient permanant et leur soutient imfaillible", 0.9f, false, 180, 150, 100 },
        { "", 0.7f, false, 0, 0, 0 },
        { "Ma petite amie", 1.0f, false, 230, 200, 130 },
        { "pour son soutient et son amour ", 0.9f, false, 180, 150, 100 },
        { "", 0.7f, false, 0, 0, 0 },
        { "Département Génie Informatique – ENSPY", 0.9f, false, 180, 150, 100 },
        { "", 0.8f, false, 0, 0, 0 },
        { "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", 0.8f, false, 140, 80, 20 },
        { "", 0.8f, false, 0, 0, 0 },
        { "DÉDICACE", 1.2f, true, 255, 160, 30 },
        { "À tous les bâtisseurs du numérique camerounais", 1.0f, false, 230, 200, 130 },
        { "et à la jeunesse africaine créatrice", 0.9f, false, 180, 150, 100 },
        { "", 0.8f, false, 0, 0, 0 },
        { "VIVE LA CULTURE CAMEROUNAISE", 1.1f, false, 255, 185, 40 },
        { "", 1.0f, false, 0, 0, 0 },
        { "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", 0.8f, false, 140, 80, 20 },
        { "", 1.0f, false, 0, 0, 0 },
        { "© 2026 RIHEN UNIVERS", 0.85f, false, 140, 110, 60 },
        { "Tous droits réservés", 0.8f, false, 120, 90, 50 },
        { "", 2.0f, false, 0, 0, 0 },
    };
    static const int lineCount = sizeof(lines) / sizeof(lines[0]);

    // Calculer la hauteur totale du contenu
    float totalH = 0.f;
    float baseSize = 22.f;
    for (int i = 0; i < lineCount; i++) {
        totalH += lines[i].size * baseSize * 1.6f;
    }

    // Défilement automatique
    m_creditsScrollY += 35.f * 0.016f;  // ~35 px/s à 60fps
    if (m_creditsScrollY > totalH + WIN_H) m_creditsScrollY = 0.f;

    // Rendu des lignes
    float y = WIN_H - m_creditsScrollY + totalH * 0.f;
    y = (float)WIN_H - m_creditsScrollY;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < lineCount; i++) {
        float lineH = lines[i].size * baseSize * 1.6f;
        if (y + lineH > 0 && y < WIN_H) {
            if (lines[i].text && lines[i].text[0] != '\0') {
                TTF_Font* f = (lines[i].size >= 1.5f) ? m_fonts.huge
                            : (lines[i].size >= 1.1f) ? m_fonts.large
                            : (lines[i].size >= 0.95f) ? m_fonts.small
                            : m_fonts.tiny;
                if (!f) f = m_fonts.small;

                // Lueur derrière les titres
                if (lines[i].isTitle) {
                    SDL_SetRenderDrawColor(renderer, lines[i].r / 2, lines[i].g / 2, 0, 60);
                    SDL_FRect glr = { 0.f, y - 4.f, (float)WIN_W, lineH + 8.f };
                    SDL_RenderFillRect(renderer, &glr);
                }

                SDL_Color col = { lines[i].r, lines[i].g, lines[i].b, 255 };
                SDL_Surface* surf = TTF_RenderText_Blended(f, lines[i].text, 0, col);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                    float tx = (WIN_W - surf->w) * 0.5f;
                    SDL_FRect dst = { tx, y + (lineH - surf->h) * 0.5f, (float)surf->w, (float)surf->h };
                    SDL_RenderTexture(renderer, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                    SDL_DestroySurface(surf);
                }
            }
        }
        y += lineH;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Bandeau kente en bas
    float segW = WIN_W / 14.f;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int k = 0; k < 14; k++) {
        auto c = kCols[k % 3];
        Uint8 r = (c >> 0) & 0xFF, g = (c >> 8) & 0xFF, b = (c >> 16) & 0xFF;
        SDL_SetRenderDrawColor(renderer, r, g, b, 200);
        SDL_FRect seg = { k * segW, (float)WIN_H - 12.f, segW + 1.f, 12.f };
        SDL_RenderFillRect(renderer, &seg);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}


//  RenderOptionsScreen
void Game::RenderOptionsScreen() {
    // Fond
    SDL_SetRenderDrawColor(renderer, 10, 5, 2, 255);
    SDL_RenderClear(renderer);
}


//  RenderImGuiUI — toute l'UI ImGui (menu, jeu, game over, options, crédits)

void Game::RenderImGuiUI() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::GetIO().DisplaySize = ImVec2((float)WIN_W, (float)WIN_H);

    //  Helpers kente 
    auto DrawKenteBand = [](ImDrawList* dl, ImVec2 pos, float w, float y, float h) {
        const ImU32 kc[3] = {
            IM_COL32(180, 70, 15, 220),
            IM_COL32(210, 160, 30, 220),
            IM_COL32(30, 100, 40, 220)
        };
        float seg = w / 14.f;
        for (int k = 0; k < 14; k++) {
            dl->AddRectFilled(
                { pos.x + k * seg,       pos.y + y },
                { pos.x + (k+1) * seg,   pos.y + y + h },
                kc[k % 3]);
        }
    };


    //  MENU

    if (state == GameState::MENU) {
        ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(WIN_W/2.f - 280, WIN_H/2.f - 240), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg,   ImVec4(0.07f, 0.04f, 0.01f, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_Border,     ImVec4(0.82f, 0.50f, 0.14f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   10.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(32.f, 24.f));

        ImGui::Begin("##menu_main", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsize = ImGui::GetWindowSize();

        // Bandeau kente haut & bas
        DrawKenteBand(dl, wpos, wsize.x, 2.f,  12.f);
        DrawKenteBand(dl, wpos, wsize.x, wsize.y - 14.f, 12.f);
        dl->AddLine({ wpos.x + 4.f, wpos.y + 15.f },
                    { wpos.x + wsize.x - 4.f, wpos.y + 15.f },
                    IM_COL32(210, 160, 30, 160), 1.5f);

        ImGui::Dummy(ImVec2(0.f, 16.f));

        // Titre
        ImGui::SetWindowFontScale(3.4f);
        const char* title = "SONGO'O";
        float tw = ImGui::CalcTextSize(title).x;
        // Ombre
        ImGui::SetCursorPos(ImVec2((wsize.x - tw) * 0.5f + 3.f, ImGui::GetCursorPosY() + 3.f));
        ImGui::TextColored(ImVec4(0.10f, 0.04f, 0.01f, 0.85f), "%s", title);
        // Principal or
        ImGui::SetCursorPos(ImVec2((wsize.x - tw) * 0.5f, ImGui::GetCursorPosY() - ImGui::GetTextLineHeightWithSpacing() - 3.f));
        ImGui::TextColored(ImVec4(0.97f, 0.74f, 0.18f, 1.f), "%s", title);
        ImGui::SetWindowFontScale(1.f);

        // Sous-titre
        ImGui::SetWindowFontScale(0.90f);
        const char* sub = "~ Jeu Traditionnel Camerounais ~";
        float sw = ImGui::CalcTextSize(sub).x;
        ImGui::SetCursorPosX((wsize.x - sw) * 0.5f);
        ImGui::TextColored(ImVec4(0.72f, 0.52f, 0.22f, 0.90f), "%s", sub);
        ImGui::SetWindowFontScale(1.f);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.78f, 0.48f, 0.12f, 0.90f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing(); ImGui::Spacing();

        float btnW = 310.f, btnH = 54.f;
        float btnX = (wsize.x - btnW) * 0.5f;

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.52f, 0.20f, 0.04f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.38f, 0.08f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.34f, 0.12f, 0.02f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.f, 11.f));

        ImGui::SetWindowFontScale(1.25f);

        // Nouvelle Partie
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Nouvelle Partie", ImVec2(btnW, btnH))) {
            for (int i = 0; i < MAX_TROU; i++) trou[i]->SetNumberGraine(5);
            joueur1->ResetGain(); joueur2->ResetGain();
            joueur1->SetIsMyRound(true); joueur2->SetIsMyRound(false);
            currentPlayer = 1;
            statusMsg = "Au tour du Joueur 1";
            anim.active = false;
            state = GameState::PLAYING;
            if(options.musicEnabled && bgMusic.getStatus() != sf::Music::Status::Playing) {
                bgMusic.play();
            }
            PlayDrumForPlayer(1);
        }
        ImGui::Spacing();

        // Histoire
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Histoire", ImVec2(btnW, btnH))) {
            state = GameState::STORY;
            storySubtitleFrame = 0;
            storyAnim.SetFinished();
        }
        ImGui::Spacing();

        // Options
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Options", ImVec2(btnW, btnH))) {
            prevState = GameState::MENU;
            state = GameState::OPTIONS_SCREEN;
        }
        ImGui::Spacing();

        // Crédits
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Crédits", ImVec2(btnW, btnH))) {
            prevState = GameState::MENU;
            m_creditsScrollY = 0.f;
            state = GameState::CREDITS_SCREEN;
            bgMusic.pause();
            if(m_creditMusicLoaded) {
                creditMusic.setVolume(options.musicVolume);
                creditMusic.play();
            }
        }
        ImGui::Spacing();

        // Quitter
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.28f, 0.10f, 0.02f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.52f, 0.20f, 0.05f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.16f, 0.06f, 0.01f, 1.f));
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Quitter", ImVec2(btnW, btnH)))
            isRunning = false;
        ImGui::Spacing();
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);
    }


    //  PLAYING

    if (state == GameState::PLAYING || (state == GameState::OPTIONS_SCREEN && prevState == GameState::PLAYING)) {
        
        float pulse = 0.55f + 0.45f * sinf(glowTime * 3.5f);

        //  Panneau Joueur 1 (gauche) 
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(240, 190), ImGuiCond_Always);

        bool p1Active = (currentPlayer == 1);
        ImVec4 bgJ1     = p1Active ? ImVec4(0.28f, 0.10f, 0.02f, 0.97f) : ImVec4(0.08f, 0.04f, 0.01f, 0.90f);
        ImVec4 borderJ1 = p1Active ? ImVec4(0.95f, 0.65f, 0.15f, 1.f)   : ImVec4(0.42f, 0.22f, 0.06f, 0.65f);
        float  bsizeJ1  = p1Active ? 3.f : 1.5f;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgJ1);
        ImGui::PushStyleColor(ImGuiCol_Border,   borderJ1);
        ImGui::PushStyleColor(ImGuiCol_TitleBg,       ImVec4(0.42f, 0.16f, 0.03f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.60f, 0.24f, 0.05f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, bsizeJ1);

        ImGui::Begin("Joueur 1", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImDrawList* dlJ1 = ImGui::GetWindowDrawList();
        ImVec2 wpJ1 = ImGui::GetWindowPos();
        ImVec2 wsJ1 = ImGui::GetWindowSize();

        if (p1Active) {
            // Barre lumineuse gauche pulsante
            Uint8 ba = (Uint8)(180.f * pulse);
            dlJ1->AddRectFilled({ wpJ1.x + 2.f, wpJ1.y + 22.f },
                                { wpJ1.x + 7.f, wpJ1.y + wsJ1.y - 2.f },
                                IM_COL32(255, 185, 30, ba));
            // Halo derrière le panneau
            dlJ1->AddRect({ wpJ1.x - 2.f, wpJ1.y - 2.f },
                          { wpJ1.x + wsJ1.x + 2.f, wpJ1.y + wsJ1.y + 2.f },
                          IM_COL32(255, 185, 30, (Uint8)(80.f * pulse)), 12.f, 0, 2.f);

            ImGui::SetWindowFontScale(1.08f);
            ImGui::TextColored(ImVec4(0.98f, 0.80f, 0.15f, 1.f), "MON TOUR");
        } else {
            ImGui::SetWindowFontScale(1.0f);
            ImGui::TextColored(ImVec4(0.50f, 0.38f, 0.22f, 1.f), "  En attente...");
        }
        ImGui::SetWindowFontScale(1.f);
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.72f, 0.42f, 0.10f, 0.80f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::SetWindowFontScale(1.12f);
        ImGui::TextColored(ImVec4(0.92f, 0.80f, 0.55f, 1.f), "  JOUEUR 1");
        if (options.gameMode == GameMode::IA_VS_JOUEUR)
            ImGui::TextColored(ImVec4(0.65f, 0.50f, 0.25f, 0.80f), "  (Humain)");
        ImGui::SetWindowFontScale(1.f);
        ImGui::Spacing();

        ImGui::SetWindowFontScale(1.8f);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.25f, 1.f), "  %d", joueur1->GetGain());
        ImGui::SetWindowFontScale(0.88f);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.68f, 0.52f, 0.28f, 1.f), " graines");
        ImGui::SetWindowFontScale(1.f);

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        //  Panneau Joueur 2 (droite) 
        ImGui::SetNextWindowPos(ImVec2(WIN_W - 250, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(240, 190), ImGuiCond_Always);

        bool p2Active = (currentPlayer == 2);
        ImVec4 bgJ2     = p2Active ? ImVec4(0.02f, 0.08f, 0.26f, 0.97f) : ImVec4(0.04f, 0.04f, 0.10f, 0.90f);
        ImVec4 borderJ2 = p2Active ? ImVec4(0.35f, 0.65f, 1.f,  1.f)   : ImVec4(0.18f, 0.26f, 0.48f, 0.65f);
        float  bsizeJ2  = p2Active ? 3.f : 1.5f;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgJ2);
        ImGui::PushStyleColor(ImGuiCol_Border,   borderJ2);
        ImGui::PushStyleColor(ImGuiCol_TitleBg,       ImVec4(0.08f, 0.14f, 0.40f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.12f, 0.22f, 0.55f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, bsizeJ2);

        ImGui::Begin("Joueur 2", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImDrawList* dlJ2 = ImGui::GetWindowDrawList();
        ImVec2 wpJ2 = ImGui::GetWindowPos();
        ImVec2 wsJ2 = ImGui::GetWindowSize();

        if (p2Active) {
            Uint8 ba = (Uint8)(180.f * pulse);
            dlJ2->AddRectFilled({ wpJ2.x + wsJ2.x - 7.f, wpJ2.y + 22.f },
                                { wpJ2.x + wsJ2.x - 2.f, wpJ2.y + wsJ2.y - 2.f },
                                IM_COL32(80, 160, 235, ba));
            dlJ2->AddRect({ wpJ2.x - 2.f, wpJ2.y - 2.f },
                          { wpJ2.x + wsJ2.x + 2.f, wpJ2.y + wsJ2.y + 2.f },
                          IM_COL32(80, 160, 235, (Uint8)(80.f * pulse)), 12.f, 0, 2.f);

            ImGui::SetWindowFontScale(1.08f);
            ImGui::TextColored(ImVec4(0.55f, 0.84f, 1.f, 1.f), "MON TOUR");
        } else {
            ImGui::SetWindowFontScale(1.0f);
            ImGui::TextColored(ImVec4(0.38f, 0.44f, 0.62f, 1.f), "  En attente...");
        }
        ImGui::SetWindowFontScale(1.f);
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.30f, 0.52f, 0.82f, 0.80f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::SetWindowFontScale(1.12f);
        ImGui::TextColored(ImVec4(0.72f, 0.84f, 0.97f, 1.f), "  JOUEUR 2");
        if (options.gameMode == GameMode::IA_VS_JOUEUR)
            ImGui::TextColored(ImVec4(0.45f, 0.60f, 0.80f, 0.80f), "  (IA)");
        ImGui::SetWindowFontScale(1.f);
        ImGui::Spacing();

        ImGui::SetWindowFontScale(1.8f);
        ImGui::TextColored(ImVec4(0.55f, 0.88f, 1.f, 1.f), "  %d", joueur2->GetGain());
        ImGui::SetWindowFontScale(0.88f);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.45f, 0.62f, 0.80f, 1.f), " graines");
        ImGui::SetWindowFontScale(1.f);

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        //  Barre de statut (bas) 
        ImGui::SetNextWindowPos(ImVec2(WIN_W/2.f - 340, WIN_H - 72), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(680, 62), ImGuiCond_Always);

        ImVec4 statusBg     = p1Active ? ImVec4(0.20f, 0.08f, 0.02f, 0.93f) : ImVec4(0.04f, 0.06f, 0.22f, 0.93f);
        ImVec4 statusBorder = p1Active ? ImVec4(0.82f, 0.48f, 0.10f, 0.92f) : ImVec4(0.32f, 0.58f, 0.90f, 0.92f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, statusBg);
        ImGui::PushStyleColor(ImGuiCol_Border,   statusBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);

        ImGui::Begin("##status", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar);

        // Kente dans la barre statut
        ImDrawList* dlSt = ImGui::GetWindowDrawList();
        ImVec2 wpSt = ImGui::GetWindowPos();
        float segSt = 680.f / 14.f;
        const ImU32 kenteSt[3] = {
            IM_COL32(180, 70, 15, 200),
            IM_COL32(210, 160, 30, 200),
            IM_COL32(30, 100, 40, 200)
        };
        for (int k = 0; k < 14; k++) {
            dlSt->AddRectFilled({ wpSt.x + k * segSt, wpSt.y + 2.f },
                                { wpSt.x + (k+1) * segSt, wpSt.y + 7.f },
                                kenteSt[k % 3]);
        }

        ImGui::Dummy(ImVec2(0.f, 5.f));
        ImGui::SetWindowFontScale(1.4f);
        float msw = ImGui::CalcTextSize(statusMsg.c_str()).x;
        ImGui::SetCursorPosX((680.f - msw) * 0.5f);
        ImVec4 stColor = p1Active ? ImVec4(0.98f, 0.84f, 0.30f, 1.f) : ImVec4(0.60f, 0.86f, 1.f, 1.f);
        ImGui::TextColored(stColor, "%s", statusMsg.c_str());
        ImGui::SetWindowFontScale(1.f);

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        //  Boutons Menu / Options / Crédits (centre haut) 
        ImGui::SetNextWindowPos(ImVec2(WIN_W/2.f - 230, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(460, 58), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.04f, 0.01f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.70f, 0.40f, 0.10f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.18f, 0.04f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.30f, 0.08f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f, 0.10f, 0.02f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);

        ImGui::Begin("##topbar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar);
        ImGui::SetWindowFontScale(1.1f);

        if (ImGui::Button("  Menu", ImVec2(130.f, 38.f))) {
            anim.active = false;

            state = GameState::MENU;
        }
        ImGui::SameLine(0.f, 8.f);
        if (ImGui::Button("Options", ImVec2(145.f, 38.f))) {
            prevState = GameState::PLAYING;
            state = GameState::OPTIONS_SCREEN;
        }
        ImGui::SameLine(0.f, 8.f);
        if (ImGui::Button("Crédits", ImVec2(145.f, 38.f))) {
            prevState = GameState::PLAYING;
            m_creditsScrollY = 0.f;
            state = GameState::CREDITS_SCREEN;
            bgMusic.pause();
            if(m_creditMusicLoaded) {
                creditMusic.setVolume(options.musicVolume);
                creditMusic.play();
            }
        }
        ImGui::SetWindowFontScale(1.f);
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
    }


    //  GAME OVER

    if (state == GameState::GAME_OVER) {
        ImGui::SetNextWindowSize(ImVec2(540, 390), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(WIN_W/2.f - 270, WIN_H/2.f - 195), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.03f, 0.01f, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.85f, 0.52f, 0.14f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);

        ImGui::Begin("##gameover", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse);

        ImDrawList* dlGO = ImGui::GetWindowDrawList();
        ImVec2 wpGO = ImGui::GetWindowPos();
        ImVec2 wsGO = ImGui::GetWindowSize();

        // Kente haut/bas
        const ImU32 kenteGO[3] = {
            IM_COL32(180, 70, 15, 240),
            IM_COL32(210, 160, 30, 240),
            IM_COL32(30, 100, 40, 240)
        };
        float segGO = wsGO.x / 14.f;
        for (int k = 0; k < 14; k++) {
            dlGO->AddRectFilled({ wpGO.x + k * segGO, wpGO.y + 2.f },
                                { wpGO.x + (k+1) * segGO, wpGO.y + 12.f },
                                kenteGO[k % 3]);
            dlGO->AddRectFilled({ wpGO.x + k * segGO, wpGO.y + wsGO.y - 12.f },
                                { wpGO.x + (k+1) * segGO, wpGO.y + wsGO.y - 2.f },
                                kenteGO[(k+1) % 3]);
        }

        ImGui::Dummy(ImVec2(0.f, 12.f));
        ImGui::SetWindowFontScale(2.2f);
        const char* goTitle = "FIN DE PARTIE";
        float goTW = ImGui::CalcTextSize(goTitle).x;
        ImGui::SetCursorPos(ImVec2((wsGO.x - goTW) * 0.5f + 2.f, ImGui::GetCursorPosY() + 2.f));
        ImGui::TextColored(ImVec4(0.10f, 0.04f, 0.01f, 0.80f), "%s", goTitle);
        ImGui::SetCursorPos(ImVec2((wsGO.x - goTW) * 0.5f, ImGui::GetCursorPosY() - ImGui::GetTextLineHeightWithSpacing() - 2.f));
        ImGui::TextColored(ImVec4(0.97f, 0.72f, 0.18f, 1.f), "%s", goTitle);
        ImGui::SetWindowFontScale(1.f);

        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.78f, 0.46f, 0.10f, 0.88f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        int g1 = joueur1->GetGain(), g2 = joueur2->GetGain();

        ImGui::SetWindowFontScale(1.22f);
        ImGui::TextColored(ImVec4(0.92f, 0.80f, 0.52f, 1.f), "  Joueur 1");
        ImGui::SameLine(210.f);
        ImGui::TextColored(ImVec4(0.97f, 0.82f, 0.25f, 1.f), "%d graines", g1);
        ImGui::TextColored(ImVec4(0.70f, 0.84f, 0.97f, 1.f), "  Joueur 2");
        ImGui::SameLine(210.f);
        ImGui::TextColored(ImVec4(0.60f, 0.86f, 1.f, 1.f), "%d graines", g2);
        ImGui::SetWindowFontScale(1.f);

        // Barres visuelles
        float maxG = (float)((g1 > g2) ? g1 : g2);
        if (maxG < 1.f) maxG = 1.f;
        float barMaxW = wsGO.x - 90.f;
        ImVec2 cur = ImGui::GetCursorScreenPos();
        dlGO->AddRectFilled({ cur.x + 14.f, cur.y + 4.f },
                            { cur.x + 14.f + barMaxW * (g1/maxG), cur.y + 15.f },
                            IM_COL32(215, 158, 30, 230));
        dlGO->AddRectFilled({ cur.x + 14.f, cur.y + 21.f },
                            { cur.x + 14.f + barMaxW * (g2/maxG), cur.y + 32.f },
                            IM_COL32(75, 155, 225, 230));
        ImGui::Dummy(ImVec2(0.f, 42.f));

        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.72f, 0.42f, 0.10f, 0.72f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::SetWindowFontScale(1.55f);
        if (g1 > g2) {
            const char* vic = "VICTOIRE JOUEUR 1 !";
            float vw = ImGui::CalcTextSize(vic).x;
            ImGui::SetCursorPosX((wsGO.x - vw) * 0.5f);
            ImGui::TextColored(ImVec4(0.97f, 0.80f, 0.20f, 1.f), "%s", vic);
        } else if (g2 > g1) {
            const char* vic = "VICTOIRE JOUEUR 2 !";
            float vw = ImGui::CalcTextSize(vic).x;
            ImGui::SetCursorPosX((wsGO.x - vw) * 0.5f);
            ImGui::TextColored(ImVec4(0.55f, 0.84f, 1.f, 1.f), "%s", vic);
        } else {
            const char* eg = "ÉGALITÉ !";
            float ew = ImGui::CalcTextSize(eg).x;
            ImGui::SetCursorPosX((wsGO.x - ew) * 0.5f);
            ImGui::TextColored(ImVec4(0.97f, 0.88f, 0.36f, 1.f), "%s", eg);
        }
        ImGui::SetWindowFontScale(1.f);
        ImGui::Spacing(); ImGui::Spacing();

        float btnW = 220.f;
        float btnX = (wsGO.x - btnW) * 0.5f;
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.20f, 0.04f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.76f, 0.36f, 0.08f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.32f, 0.12f, 0.02f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
        ImGui::SetWindowFontScale(1.22f);

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Rejouer", ImVec2(btnW, 48))) {
            for (int i = 0; i < MAX_TROU; i++) trou[i]->SetNumberGraine(5);
            joueur1->ResetGain(); joueur2->ResetGain();
            joueur1->SetIsMyRound(true); joueur2->SetIsMyRound(false);
            currentPlayer = 1;
            statusMsg = "Au tour du Joueur 1";
            anim.active = false;
            state = GameState::PLAYING;
            PlayDrumForPlayer(1);
        }
        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("   Menu principal", ImVec2(btnW, 48))) {
            anim.active = false;
            state = GameState::MENU;
        }
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }


    //  CREDITS SCREEN — bouton retour seulement

    if (state == GameState::CREDITS_SCREEN) {
        ImGui::SetNextWindowPos(ImVec2(WIN_W - 160, WIN_H - 60), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(150, 50), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.05f, 0.01f, 0.88f));
        ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.70f, 0.40f, 0.10f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.18f, 0.04f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.30f, 0.08f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.f);
        ImGui::Begin("##cr_back", nullptr, ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
        ImGui::SetWindowFontScale(1.1f);
        if (ImGui::Button("Retour", ImVec2(130.f, 36.f))) {
           
           creditMusic.stop();
           if(prevState == GameState::PLAYING && options.musicEnabled)
           bgMusic.play(); 
           state = prevState; 
        }
           
        ImGui::SetWindowFontScale(1.f);
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }


    //  OPTIONS SCREEN

    if (state == GameState::OPTIONS_SCREEN) {
        ImGui::SetNextWindowSize(ImVec2(560, 540), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(WIN_W/2.f - 280, WIN_H/2.f - 260), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg,   ImVec4(0.07f, 0.04f, 0.01f, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_Border,     ImVec4(0.80f, 0.50f, 0.14f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.f, 22.f));

        ImGui::Begin("##options", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dlO = ImGui::GetWindowDrawList();
        ImVec2 wpO = ImGui::GetWindowPos();
        ImVec2 wsO = ImGui::GetWindowSize();

        // Kente haut/bas
        float segO = wsO.x / 14.f;
        const ImU32 kenteO[3] = { IM_COL32(180,70,15,220), IM_COL32(210,160,30,220), IM_COL32(30,100,40,220) };
        for (int k = 0; k < 14; k++) {
            dlO->AddRectFilled({ wpO.x + k * segO, wpO.y + 2.f }, { wpO.x + (k+1) * segO, wpO.y + 11.f }, kenteO[k%3]);
            dlO->AddRectFilled({ wpO.x + k * segO, wpO.y + wsO.y - 11.f }, { wpO.x + (k+1) * segO, wpO.y + wsO.y - 2.f }, kenteO[(k+1)%3]);
        }

        ImGui::Dummy(ImVec2(0.f, 14.f));

        // Titre options
        ImGui::SetWindowFontScale(2.0f);
        const char* optTitle = "OPTIONS";
        float otw = ImGui::CalcTextSize(optTitle).x;
        ImGui::SetCursorPosX((wsO.x - otw) * 0.5f);
        ImGui::TextColored(ImVec4(0.95f, 0.72f, 0.18f, 1.f), "%s", optTitle);
        ImGui::SetWindowFontScale(1.f);

        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.78f, 0.46f, 0.10f, 0.88f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing(); ImGui::Spacing();

        //  Son & Musique 
        ImGui::SetWindowFontScale(1.1f);
        ImGui::TextColored(ImVec4(0.95f, 0.72f, 0.18f, 1.f), "Audio");
        ImGui::SetWindowFontScale(1.f);
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.90f, 0.78f, 0.55f, 1.f), "  Musique de fond :");
        ImGui::SameLine(220.f);
        if (ImGui::Checkbox("##mus", &options.musicEnabled)) {
            if (options.musicEnabled) {
                bgMusic.setVolume(options.musicVolume);
                if (bgMusic.getStatus() != sf::Music::Status::Playing) bgMusic.play();
            } else {
                bgMusic.pause();
            }
        }

        ImGui::TextColored(ImVec4(0.90f, 0.78f, 0.55f, 1.f), "  Effets sonores :");
        ImGui::SameLine(220.f);
        ImGui::Checkbox("##sfx", &options.soundEnabled);

        ImGui::TextColored(ImVec4(0.90f, 0.78f, 0.55f, 1.f), "  Tambours de tour :");
        ImGui::SameLine(220.f);
        ImGui::Checkbox("##drum", &options.drumEnabled);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.90f, 0.78f, 0.55f, 1.f), "  Volume musique  :");
        ImGui::SameLine(220.f);
        ImGui::SetNextItemWidth(200.f);
        if (ImGui::SliderFloat("##mvol", &options.musicVolume, 0.f, 100.f, "%.0f%%")) {
            bgMusic.setVolume(options.musicEnabled ? options.musicVolume : 0.f);
        }

        ImGui::TextColored(ImVec4(0.90f, 0.78f, 0.55f, 1.f), "  Volume effets   :");
        ImGui::SameLine(220.f);
        ImGui::SetNextItemWidth(200.f);
        if (ImGui::SliderFloat("##svol", &options.sfxVolume, 0.f, 100.f, "%.0f%%")) {
            if (depositSound.has_value()) depositSound->setVolume(options.sfxVolume);
            if (pickupSound.has_value())  pickupSound->setVolume(options.sfxVolume);
            if (drumSound.has_value())    drumSound->setVolume(options.sfxVolume);
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.78f, 0.46f, 0.10f, 0.65f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        //  Mode de jeu 
        ImGui::SetWindowFontScale(1.1f);
        ImGui::TextColored(ImVec4(0.95f, 0.72f, 0.18f, 1.f), "Mode de Jeu");
        ImGui::SetWindowFontScale(1.f);
        ImGui::Spacing();

        float btnModeW = 230.f;
        float btnModeX = (wsO.x - btnModeW * 2.f - 12.f) * 0.5f;

        bool isJ1J2 = (options.gameMode == GameMode::JOUEUR_VS_JOUEUR);
        bool isIAJ1 = (options.gameMode == GameMode::IA_VS_JOUEUR);

        ImGui::PushStyleColor(ImGuiCol_Button,
            isJ1J2 ? ImVec4(0.30f, 0.60f, 0.18f, 1.f) : ImVec4(0.40f, 0.18f, 0.04f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.80f, 0.28f, 1.f));
        ImGui::SetCursorPosX(btnModeX);
        if (ImGui::Button("J1 vs J2", ImVec2(btnModeW, 46))) {
            options.gameMode = GameMode::JOUEUR_VS_JOUEUR;
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0.f, 12.f);

        ImGui::PushStyleColor(ImGuiCol_Button,
            isIAJ1 ? ImVec4(0.10f, 0.28f, 0.58f, 1.f) : ImVec4(0.40f, 0.18f, 0.04f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.48f, 0.82f, 1.f));
        if (ImGui::Button("J1 vs IA", ImVec2(btnModeW, 46))) {
            options.gameMode = GameMode::IA_VS_JOUEUR;
        }
        ImGui::PopStyleColor(2);

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.78f, 0.46f, 0.10f, 0.65f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // Bouton Retour
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.20f, 0.04f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.76f, 0.36f, 0.08f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.32f, 0.12f, 0.02f, 1.f));
        ImGui::SetWindowFontScale(1.2f);
        float bkW = 200.f;
        ImGui::SetCursorPosX((wsO.x - bkW) * 0.5f);
        if (ImGui::Button("Retour", ImVec2(bkW, 46)))
            state = prevState;
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}


//  RenderNumberOfGraines

void Game::RenderNumberOfGraines(float x, float y) {
    int id = GetClickedID((int)x, (int)y);
    if (id >= 1 && id <= 14) {
        SDL_Color blanc = { 255, 235, 180, 255 };
        TTF_Font* f = TTF_OpenFont("C:/Windows/Fonts/timesbd.ttf", 50);
        if (!f) f = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 50);
        if (!f) return;
        SDL_Surface* surface = TTF_RenderText_Blended(f, std::to_string(trou[id - 1]->GetNumberGraine()).c_str(), 0, blanc);
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FRect dst = { x, y, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
        SDL_DestroySurface(surface);
        TTF_CloseFont(f);
    }
}

bool Fonts::load(const std::string& path) {
    huge   = TTF_OpenFont(path.c_str(), 90);
    large  = TTF_OpenFont(path.c_str(), 40);
    medium = TTF_OpenFont(path.c_str(), 54);
    small  = TTF_OpenFont(path.c_str(), 22);
    tiny   = TTF_OpenFont(path.c_str(), 18);
    if (!huge || !large || !medium || !small || !tiny) {
        std::cerr << "Erreur chargement police '" << path << "': " << SDL_GetError() << "\n";
        return false;
    }
    return true;
}
void Fonts::free() {
    if (huge)   TTF_CloseFont(huge);
    if (large)  TTF_CloseFont(large);
    if (medium) TTF_CloseFont(medium);
    if (small)  TTF_CloseFont(small);
    if (tiny)   TTF_CloseFont(tiny);
    huge = large = medium = small = tiny = nullptr;
}

//  Run() — boucle principale

void Game::Run() {
    InitSDL();
    InitImGui();
    InitEntities();
    InitAudio();
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    
    if (storyVoiceBuffer.loadFromFile("audio/songo2.mp3")) {
        storyVoice.emplace(storyVoiceBuffer);
        storyVoice->setLooping(false);
        storyVoice->setVolume(100.f);
    }

    isRunning = true;
    bool isClick = false;
    SDL_Event event;
    Vector2D position;

    // Tracker le frame story pour les sous-titres
    float storyFrameAccum = 0.f;
    static const float STORY_DURATIONS[6] = { 11.f, 8.f, 11.f, 9.f, 9.f, 9.f };

    isRunningLogo = true;  // utilise le membre de classe, pas une variable locale

    Uint64 lastTime = SDL_GetTicksNS();

    while (isRunningLogo) {
        // Permettre de fermer la fenêtre pendant le logo
        SDL_Event logoEvent;
        while (SDL_PollEvent(&logoEvent)) {
            if (logoEvent.type == SDL_EVENT_QUIT) {
                isRunningLogo = false;
                isRunning = false;
            }
        }

        Uint64 currentTime = SDL_GetTicksNS();
        double deltaTime = (currentTime - lastTime) / 1e9;
        lastTime = currentTime;

        update(deltaTime);
        render();
    }

    //liberer la memoire
    for (auto tex : frames) {
        SDL_DestroyTexture(tex);
    }
    frames.clear();

    while (isRunning) {

        //  Events 
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) isRunning = false;

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    if (event.button.button == SDL_BUTTON_LEFT)
                        HandleClick((int)event.button.x, (int)event.button.y, joueur1, joueur2);
                    if (event.button.button == SDL_BUTTON_RIGHT) {
                        float lx, ly;
                        SDL_RenderCoordinatesFromWindow(renderer, event.button.x, event.button.y, &lx, &ly);
                        position.x = lx;
                        position.y = ly;
                        isClick = true;
                    }
                }
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
                if (event.button.button == SDL_BUTTON_RIGHT) isClick = false;
                
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    if (state == GameState::CREDITS_SCREEN || state == GameState::OPTIONS_SCREEN) {
                        state = prevState;
                    } else {
                        state = GameState::MENU;
                        if (storyVoice.has_value() && storyVoice->getStatus() == sf::Sound::Status::Playing) {
                            storyAnim.Reset();
                            storyVoice->stop();
                        }
                    }
                }
                // Plein écran toggle avec F11
                if (event.key.key == SDLK_F11) {
                    bool isFS = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
                    SDL_SetWindowFullscreen(window, !isFS);
                }
            }
        }

        //  Mise à jour de l'animation 
        UpdateAnimation();
        glowTime += 0.016f;

        //  Rendu 
        
        RenderBackground();

        if (state == GameState::PLAYING || state == GameState::GAME_OVER) {
            if (creditMusic.getStatus() == sf::Music::Status::Playing) creditMusic.stop();
            if(options.musicEnabled && bgMusic.getStatus() != sf::Music::Status::Playing) {
                bgMusic.play();
            }
            //pendant l'option du jeu
            RenderPlayerGlow();
            RenderMap(renderer, trou);
            RenderHand();
            if (isClick) RenderNumberOfGraines(position.x, position.y);
        } else {
            bgMusic.pause();
        }

        if (state == GameState::STORY) {
            if (storyVoice.has_value() && storyVoice->getStatus() != sf::Sound::Status::Playing && !storyAnim.IsFinished()) {
                storyVoice->play();
                storyFrameAccum = 0.f;
                storySubtitleFrame = 0;
            }

            // Avancer le frame subtitle
            storyFrameAccum += 0.016f;
            if (storySubtitleFrame < 5) {
                float dur = STORY_DURATIONS[storySubtitleFrame];
                if (storyFrameAccum >= dur) {
                    storyFrameAccum -= dur;
                    storySubtitleFrame++;
                }
            }

            storyAnim.Update();
            storyAnim.Render(renderer);
            RenderStorySubtitle();

            if (storyAnim.IsFinished()) {
                if (storyVoice.has_value()) storyVoice->stop();
                storyAnim.Reset();
                storySubtitleFrame = 0;
                state = GameState::MENU;
            }
        }

        if (state == GameState::CREDITS_SCREEN) {
            RenderCreditsScreen();
        }

        if ((state == GameState::OPTIONS_SCREEN) && (prevState == GameState::PLAYING)) {
            if(options.musicEnabled && bgMusic.getStatus() != sf::Music::Status::Playing) {
                bgMusic.play();
            }
            RenderOptionsScreen();
            RenderPlayerGlow();
            RenderMap(renderer, trou);
            RenderNumberOfGraines(position.x, position.y);
        }

        RenderImGuiUI();
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (storyVoice.has_value()) storyVoice->stop();
    bgMusic.stop();
    creditMusic.stop();
    Cleanup();
}

void Game::Cleanup() {
    for (int i = 0; i < MAX_TROU; i++) {
        if (trou[i]) { trou[i]->Destroy(); delete trou[i]; trou[i] = nullptr; }
    }
    if (joueur1) { joueur1->Destroy(); delete joueur1; joueur1 = nullptr; }
    if (joueur2) { joueur2->Destroy(); delete joueur2; joueur2 = nullptr; }
    if (handTexture) { SDL_DestroyTexture(handTexture); handTexture = nullptr; }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    storyAnim.Unload();

    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window)   { SDL_DestroyWindow(window);     window   = nullptr; }

    if (font) { TTF_CloseFont(font); font = nullptr; }
    TTF_Quit();
    SDL_Quit();
}