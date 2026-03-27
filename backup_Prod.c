#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 30
#define BOARD_WIDTH (SCREEN_WIDTH / BLOCK_SIZE)
#define BOARD_HEIGHT (SCREEN_HEIGHT / BLOCK_SIZE)
#define FRAME_COUNT 5 // Image de l'animation de fond

bool isMusicPlaying = true;
bool isMenuOpen = false;
bool isPaused = false; // New flag for pause state

typedef enum
{
    GAME_RUNNING,
    GAME_OVER
} GameState;

typedef struct
{
    int r, g, b, a;
} Color;

Color colors[7] = {
    {255, 0, 0, 255},   // Rouge
    {0, 255, 0, 255},   // Vert
    {0, 0, 255, 255},   // Bleu
    {255, 255, 0, 255}, // Jaune
    {255, 165, 0, 255}, // Orange
    {128, 0, 128, 255}, // Violet
    {0, 255, 255, 255}  // Cyan
};

int shapes[7][4][2] = {
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, // Cube
    {{0, 0}, {1, 0}, {2, 0}, {1, 1}}, // T
    {{0, 0}, {1, 0}, {2, 0}, {2, 1}}, // L
    {{0, 0}, {1, 0}, {2, 0}, {0, 1}}, // J
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, // Z
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, // S
    {{0, 0}, {1, 0}, {2, 0}, {3, 0}}  // I
};

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
Color boardColors[BOARD_HEIGHT][BOARD_WIDTH]; // Stocke les couleurs du tableau
int score = 0;                                // Variable du score

typedef struct
{
    int x, y;
    int shape[4][2];
    Color color; // Couleur du bloc
    float currentRotationAngle;
    float targetRotationAngle;
} tetoris;

// Declaration des fonctions
void inittetoris(tetoris *tetoris);
void drawtetoris(SDL_Renderer *renderer, tetoris *tetoris);
bool checkCollision(tetoris *tetoris);
void mergetetoris(tetoris *tetoris);
void clearLines(SDL_Renderer *renderer);
void drawScore(SDL_Renderer *renderer, TTF_Font *font);
void showTitleScreen(SDL_Renderer *renderer, TTF_Font *font);
void rotatetetoris(tetoris *tetoris);
void drawBoard(SDL_Renderer *renderer);
void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y);
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *file);
void loadBackgroundFrames(SDL_Renderer *renderer);
void renderBackground(SDL_Renderer *renderer);
void cleanupBackgroundFrames();
void loadBlockTextures(SDL_Renderer *renderer);
void cleanupBlockTextures();
void showMenu(SDL_Renderer *renderer, TTF_Font *font);

// Variables globales pour l'animation de fond
SDL_Texture *backgroundFrames[FRAME_COUNT];
SDL_Texture *blockTextures[7]; // Tableau de textures pour les blocs
int currentFrame = 0;
int frameDelay = 100; // Delai en millisecondes entre les frames
int frameTime = 0;    // Temps ecoule depuis la derniere frame

void inittetoris(tetoris *tetoris)
{
    int shapeIndex = rand() % 7;
    for (int i = 0; i < 4; i++)
    {
        tetoris->shape[i][0] = shapes[shapeIndex][i][0];
        tetoris->shape[i][1] = shapes[shapeIndex][i][1];
    }
    tetoris->x = BOARD_WIDTH / 2 - 1;
    tetoris->y = 0;
    tetoris->color = colors[shapeIndex]; // Assignation de la couleur du bloc
}

void drawtetoris(SDL_Renderer *renderer, tetoris *tetoris)
{
    int shapeIndex = -1;
    for (int i = 0; i < 7; i++)
    {
        if (tetoris->color.r == colors[i].r && tetoris->color.g == colors[i].g && tetoris->color.b == colors[i].b)
        {
            shapeIndex = i;
            break;
        }
    }
    if (shapeIndex == -1)
        return;

    for (int i = 0; i < 4; i++)
    {
        SDL_Rect rect;
        rect.x = (tetoris->x + tetoris->shape[i][0]) * BLOCK_SIZE;
        rect.y = (tetoris->y + tetoris->shape[i][1]) * BLOCK_SIZE;
        rect.w = BLOCK_SIZE;
        rect.h = BLOCK_SIZE;

        // calcule du centre du bloc
        SDL_Point center = {BLOCK_SIZE / 2, BLOCK_SIZE / 2};

        // Dessine le bloc avec la rotation
        SDL_RenderCopyEx(renderer, blockTextures[shapeIndex], NULL, &rect, tetoris->currentRotationAngle, &center, SDL_FLIP_NONE);
    }
}

void clearLines(SDL_Renderer *renderer)
{
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--)
    {
        bool isLineFull = true;
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            if (board[y][x] == 0)
            {
                isLineFull = false;
                break;
            }
        }
        if (isLineFull)
        {
            // Décale toutes les lignes au-dessus de la ligne actuelle vers le bas
            for (int yy = y; yy > 0; yy--)
            {
                for (int xx = 0; xx < BOARD_WIDTH; xx++)
                {
                    board[yy][xx] = board[yy - 1][xx];
                    boardColors[yy][xx] = boardColors[yy - 1][xx];
                }
            }
            // efface la ligne du haut
            for (int xx = 0; xx < BOARD_WIDTH; xx++)
            {
                board[0][xx] = 0;
            }
            y++;           // On doit vérifier la ligne actuelle à nouveau
            score += 1000; // Incrémentation du score pour chaque ligne effacee
        }
    }
}

void rotatetetoris(tetoris *tetoris)
{
    // Met à jour instantanément l'angle de rotation cible
    tetoris->targetRotationAngle += 90.0f;
    if (tetoris->targetRotationAngle >= 360.0f)
    {
        tetoris->targetRotationAngle = 0.0f;
    }

    // applique la rotation
    int tempShape[4][2];
    for (int i = 0; i < 4; i++)
    {
        tempShape[i][0] = tetoris->shape[i][0];
        tempShape[i][1] = tetoris->shape[i][1];
    }
    for (int i = 0; i < 4; i++)
    {
        tetoris->shape[i][0] = tempShape[i][1];
        tetoris->shape[i][1] = -tempShape[i][0];
    }

    // Vérifie si la rotation est valide
    if (checkCollision(tetoris))
    {
        // Si la rotation est invalide, annule la rotation
        for (int i = 0; i < 4; i++)
        {
            tetoris->shape[i][0] = tempShape[i][0];
            tetoris->shape[i][1] = tempShape[i][1];
        }
        // Ajuste l'angle de rotation cible
        tetoris->targetRotationAngle -= 90.0f;
        if (tetoris->targetRotationAngle < 0.0f)
        {
            tetoris->targetRotationAngle = 270.0f;
        }
    }
}

void updateRotation(tetoris *tetoris, float deltaTime)
{
    float rotationSpeed = 360.0f; // Degrés par seconde
    if (tetoris->currentRotationAngle != tetoris->targetRotationAngle)
    {
        float angleDifference = tetoris->targetRotationAngle - tetoris->currentRotationAngle;
        if (angleDifference > 180.0f)
            angleDifference -= 360.0f;
        if (angleDifference < -180.0f)
            angleDifference += 360.0f;

        float rotationStep = rotationSpeed * deltaTime;
        if (fabs(angleDifference) < rotationStep)
        {
            tetoris->currentRotationAngle = tetoris->targetRotationAngle;
        }
        else
        {
            tetoris->currentRotationAngle += (angleDifference > 0 ? rotationStep : -rotationStep);
        }
    }
}

void drawScore(SDL_Renderer *renderer, TTF_Font *font)
{
    char scoreText[20];
    sprintf(scoreText, "Score: %d", score);
    renderText(renderer, font, scoreText, 10, 10);
}

bool checkCollision(tetoris *tetoris)
{
    for (int i = 0; i < 4; i++)
    {
        int x = tetoris->x + tetoris->shape[i][0];
        int y = tetoris->y + tetoris->shape[i][1];
        
        if (x < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT || board[y][x])
        {
            return true;
        }
    }
    return false;
}

void mergetetoris(tetoris *tetoris)
{
    for (int i = 0; i < 4; i++)
    
    {
        int x = tetoris->x + tetoris->shape[i][0];
        int y = tetoris->y + tetoris->shape[i][1];
        board[y][x] = 1;                    // Marque le bloc comme etant occupe
        boardColors[y][x] = tetoris->color; // Stocke la couleur du bloc
    }
    score += 100; // Incrémentation du score pour chaque bloc
}

SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *file)
{
    SDL_Surface *surface = IMG_Load(file);
    if (!surface)
    
    {
        fprintf(stderr, "Erreur de chargement de l'image: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void loadBackgroundFrames(SDL_Renderer *renderer)
{
    for (int i = 1; i < FRAME_COUNT; i++)
    
    {
        char filename[30];
        sprintf(filename, "assets/frame%d.png", i + 1); // Charge les images de l'animation de fond
        backgroundFrames[i] = loadTexture(renderer, filename);
    }
}

void renderBackground(SDL_Renderer *renderer)
{
    // Active le mode RGBA pour la transparence
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Rend le fond transparent
    Uint8 alpha = 20; // 20% de 255
    SDL_SetTextureAlphaMod(backgroundFrames[currentFrame], alpha);

    // Rend la frame actuel
    SDL_RenderCopy(renderer, backgroundFrames[currentFrame], NULL, NULL);

    // Rend la frame suivante pour une transition fluide
    int nextFrame = (currentFrame + 1) % FRAME_COUNT;
    SDL_SetTextureAlphaMod(backgroundFrames[nextFrame], alpha);
    SDL_RenderCopy(renderer, backgroundFrames[nextFrame], NULL, NULL);
}

void cleanupBackgroundFrames()
{
    for (int i = 0; i < FRAME_COUNT; i++)
    
    {
        SDL_DestroyTexture(backgroundFrames[i]);
    }
}

void loadBlockTextures(SDL_Renderer *renderer)
{
    const char *textureFiles[7] = {
        "assets/rouge.png",
        "assets/vert.png",
        "assets/bleu.png",
        "assets/jaune.png",
        "assets/orange.png",
        "assets/violet.png",
        "assets/cyan.png"};
    for (int i = 0; i < 7; i++)
    
    {
        blockTextures[i] = loadTexture(renderer, textureFiles[i]);
        if (!blockTextures[i])
        
        {
            fprintf(stderr, "Texture Introuvable: %s\n", textureFiles[i]);
        }
    }
}

void cleanupBlockTextures()
{
    for (int i = 0; i < 7; i++)
    
    {
        SDL_DestroyTexture(blockTextures[i]);
    }
}

void drawBoard(SDL_Renderer *renderer)
{
    for (int y = 0; y < BOARD_HEIGHT; y++)
    
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        
        {
            if (board[y][x])
            
            {
                int shapeIndex = -1;
                for (int i = 0; i < 7; i++)
                
                {
                    if (boardColors[y][x].r == colors[i].r && boardColors[y][x].g == colors[i].g && boardColors[y][x].b == colors[i].b)
                    
                    {
                        shapeIndex = i;
                        break;
                    }
                }
                
                if (shapeIndex == -1)
                    continue;

                SDL_Rect rect;
                rect.x = x * BLOCK_SIZE;
                rect.y = y * BLOCK_SIZE;
                rect.w = BLOCK_SIZE;
                rect.h = BLOCK_SIZE;
                SDL_RenderCopy(renderer, blockTextures[shapeIndex], NULL, &rect);
            }
        }
    }
}

void showTitleScreen(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderText(renderer, font, "TETORIS", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 20); // Centrage du titre

    SDL_RenderPresent(renderer);
    SDL_Delay(1500); // Titre pendant 3 secondes
}

void showMenu(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderText(renderer, font, "Menu", SCREEN_WIDTH / 2 - 30, SCREEN_HEIGHT / 2 - 40);
    renderText(renderer, font, "q - quit", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2);
    renderText(renderer, font, "r - restart", SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 + 30);

    SDL_RenderPresent(renderer);
}

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y)
{
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    Uint32 lastFrameTime = SDL_GetTicks();
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    
    { // Initialisation de SDL avec la video et l'audio
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1)
    
    { // Initialisation de SDL_ttf (pour les polices)
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    
    { // Initialisation de SDL_mixer (pour la musique)
        fprintf(stderr, "Mix_OpenAudio Error: %s\n", Mix_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_Music *music = Mix_LoadMUS("assets/music.mp3"); // chargement de la musique
    if (!music)
    
    {
        fprintf(stderr, "Erreur d'audio: %s\n", Mix_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    Mix_PlayMusic(music, -1); // jouer la musique en boucle

    TTF_Font *font = TTF_OpenFont("assets/DejaVuSans.ttf", 24); // chargement de la police
    if (!font)
    
    {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("TETORIS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window)
    
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    
    {
        SDL_DestroyWindow(window);
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // chargement de l'animation de fond
    loadBackgroundFrames(renderer);
    loadBlockTextures(renderer);

    showTitleScreen(renderer, font); // montre le titre lors du démarrage du jeu

    tetoris currenttetoris;
    inittetoris(&currenttetoris);

    GameState gameState = GAME_RUNNING;
    SDL_Event event;
    int dropTime = 0;
    int dropInterval = 1500; // Temps en millisecondes entre les chutes de blocs (vitesses du jeu)

    while (gameState == GAME_RUNNING)
    {
        // Gestion du temps
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastFrameTime) / 1000.0f; // Convertit en secondes
        lastFrameTime = currentTime;                               // Mise à jour du temps de la dernière frame

        // Gestion des événements
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            
            {
                gameState = GAME_OVER;
            }
            
            if (event.type == SDL_KEYDOWN)
            {
                
                switch (event.key.keysym.sym)
                {
                
                case SDLK_LEFT:
                    currenttetoris.x--;
                    
                    if (checkCollision(&currenttetoris))
                        currenttetoris.x++;
                    
                    break;
                
                case SDLK_RIGHT:
                    
                    currenttetoris.x++;
                    
                    if (checkCollision(&currenttetoris))
                        currenttetoris.x--;
                    break;
                
                case SDLK_DOWN:
                    currenttetoris.y++;
                    
                    if (checkCollision(&currenttetoris))
                    {
                        currenttetoris.y--;
                        mergetetoris(&currenttetoris);
                        clearLines(renderer);
                        inittetoris(&currenttetoris);
                        
                        if (checkCollision(&currenttetoris))
                        {
                            gameState = GAME_OVER; // Game over
                        }
                    
                    }
                    break;
                
                case SDLK_UP:
                    
                    rotatetetoris(&currenttetoris); // Rotation du bloc
                    break;
                
                case SDLK_ESCAPE:
                    
                    isMenuOpen = !isMenuOpen; // Ouvre le menu
                    isPaused = !isPaused;     // Met en pause
                    break;
                
                case SDLK_q:
                    
                    if (isMenuOpen)
                        gameState = GAME_OVER;
                    break;
                
                case SDLK_r:
                    
                    if (isMenuOpen)
                    {
                        // Reset le jeu
                        
                        for (int y = 0; y < BOARD_HEIGHT; y++)
                        {
                            for (int x = 0; x < BOARD_WIDTH; x++)
                            {
                                board[y][x] = 0;
                            }
                        }
                        
                        score = 0; // Reset le score
                        inittetoris(&currenttetoris);
                        isMenuOpen = false; // Ferme le menu à chaque action
                        isPaused = false;   // Reprend le jeu
                    }
                    
                    break;
                
                case SDLK_m:
                    
                    // Met en pause ou reprend la musique
                    
                    if (isMusicPlaying)
                    {
                        Mix_PauseMusic();
                    }
                    
                    else
                    
                    {
                        Mix_ResumeMusic();
                    }
                    
                    isMusicPlaying = !isMusicPlaying;
                    break;
                }
            }
        }

        if (!isPaused)
        
        { // mets à jour le jeu si il n'est pas en pause
            
            // Mettre à jour la durée des frames pour l'animation en arrière-plan
            frameTime += SDL_GetTicks();
            
            if (frameTime > frameDelay)
            {
                currentFrame = (currentFrame + 1) % FRAME_COUNT; // Change de frame
                frameTime = 0;                                   // Reset les frames
            }

            // Mets à jour la rotation du bloc
            updateRotation(&currenttetoris, deltaTime);

            // Gestion de la chute des blocs
            dropTime += SDL_GetTicks();
            if (dropTime > dropInterval)
            
            {
                currenttetoris.y++;
                if (checkCollision(&currenttetoris))
                {
                    currenttetoris.y--;
                    mergetetoris(&currenttetoris);
                    clearLines(renderer);
                    inittetoris(&currenttetoris);
                    if (checkCollision(&currenttetoris))
                    {
                        gameState = GAME_OVER; // Game over
                    }
                }
                dropTime = 0;
            }
        }

        // Rendu du jeu
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Rendu de l'animation de fond
        renderBackground(renderer);

        drawBoard(renderer);
        drawtetoris(renderer, &currenttetoris);
        drawScore(renderer, font); // écrit le score

        if (isMenuOpen)
        
        {
            showMenu(renderer, font); // montre le menu si il est ouvert
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    // Nettoyage
    cleanupBlockTextures();    // rend libre les textures des blocs
    Mix_FreeMusic(music);      // rend libre la musique
    cleanupBackgroundFrames(); // rend libre les frames de l'animation de fond
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font); // rend libre la police
    Mix_CloseAudio();    // ferme SDL_mixer
    TTF_Quit();          // Quitte SDL_ttf
    SDL_Quit();          // Quitte SDL
    return 0;            // fin du programme
}