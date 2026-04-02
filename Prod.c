#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <emscripten.h>

#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 30
#define BOARD_WIDTH (SCREEN_WIDTH / BLOCK_SIZE)
#define BOARD_HEIGHT (SCREEN_HEIGHT / BLOCK_SIZE)
#define FRAME_COUNT 5

bool isMusicPlaying = true;
bool isMenuOpen = false;
bool isPaused = false;

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
    {255, 0, 0, 255},
    {0, 255, 0, 255},
    {0, 0, 255, 255},
    {255, 255, 0, 255},
    {255, 165, 0, 255},
    {128, 0, 128, 255},
    {0, 255, 255, 255}
};

int shapes[7][4][2] = {
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
    {{0, 0}, {1, 0}, {2, 0}, {1, 1}},
    {{0, 0}, {1, 0}, {2, 0}, {2, 1}},
    {{0, 0}, {1, 0}, {2, 0}, {0, 1}},
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
    {{0, 0}, {1, 0}, {2, 0}, {3, 0}}
};

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
Color boardColors[BOARD_HEIGHT][BOARD_WIDTH];
int score = 0;

typedef struct
{
    int x, y;
    int shape[4][2];
    Color color;
    float currentRotationAngle;
    float targetRotationAngle;
} tetoris;

// Global state for the main loop
SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;
Mix_Music *music;
tetoris currenttetoris;
GameState gameState;
SDL_Event event;
int dropTime = 0;
int dropInterval = 500;
Uint32 lastFrameTime;

// Declarations
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

SDL_Texture *backgroundFrames[FRAME_COUNT];
SDL_Texture *blockTextures[7];
int currentFrame = 0;
int frameDelay = 75;
int frameTime = 0;

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
    tetoris->color = colors[shapeIndex];
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

        SDL_Point center = {BLOCK_SIZE / 2, BLOCK_SIZE / 2};
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
            for (int yy = y; yy > 0; yy--)
            {
                for (int xx = 0; xx < BOARD_WIDTH; xx++)
                {
                    board[yy][xx] = board[yy - 1][xx];
                    boardColors[yy][xx] = boardColors[yy - 1][xx];
                }
            }
            for (int xx = 0; xx < BOARD_WIDTH; xx++)
            {
                board[0][xx] = 0;
            }
            y++;
            score += 1000;
        }
    }
}

void rotatetetoris(tetoris *tetoris)
{
    tetoris->targetRotationAngle += 90.0f;
    if (tetoris->targetRotationAngle >= 360.0f)
        tetoris->targetRotationAngle = 0.0f;

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

    if (checkCollision(tetoris))
    {
        for (int i = 0; i < 4; i++)
        {
            tetoris->shape[i][0] = tempShape[i][0];
            tetoris->shape[i][1] = tempShape[i][1];
        }
        tetoris->targetRotationAngle -= 90.0f;
        if (tetoris->targetRotationAngle < 0.0f)
            tetoris->targetRotationAngle = 270.0f;
    }
}

void updateRotation(tetoris *tetoris, float deltaTime)
{
    float rotationSpeed = 360.0f;
    if (tetoris->currentRotationAngle != tetoris->targetRotationAngle)
    {
        float angleDifference = tetoris->targetRotationAngle - tetoris->currentRotationAngle;
        if (angleDifference > 180.0f)
            angleDifference -= 360.0f;
        if (angleDifference < -180.0f)
            angleDifference += 360.0f;

        float rotationStep = rotationSpeed * deltaTime;
        if (fabs(angleDifference) < rotationStep)
            tetoris->currentRotationAngle = tetoris->targetRotationAngle;
        else
            tetoris->currentRotationAngle += (angleDifference > 0 ? rotationStep : -rotationStep);
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
            return true;
    }
    return false;
}

void mergetetoris(tetoris *tetoris)
{
    for (int i = 0; i < 4; i++)
    {
        int x = tetoris->x + tetoris->shape[i][0];
        int y = tetoris->y + tetoris->shape[i][1];
        board[y][x] = 1;
        boardColors[y][x] = tetoris->color;
    }
    score += 100;
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
        sprintf(filename, "/assets/frame%d.png", i + 1);
        backgroundFrames[i] = loadTexture(renderer, filename);
    }
}

void renderBackground(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    Uint8 alpha = 20;
    SDL_SetTextureAlphaMod(backgroundFrames[currentFrame], alpha);
    SDL_RenderCopy(renderer, backgroundFrames[currentFrame], NULL, NULL);

    int nextFrame = (currentFrame + 1) % FRAME_COUNT;
    SDL_SetTextureAlphaMod(backgroundFrames[nextFrame], alpha);
    SDL_RenderCopy(renderer, backgroundFrames[nextFrame], NULL, NULL);
}

void cleanupBackgroundFrames()
{
    for (int i = 0; i < FRAME_COUNT; i++)
        SDL_DestroyTexture(backgroundFrames[i]);
}

void loadBlockTextures(SDL_Renderer *renderer)
{
    const char *textureFiles[7] = {
        "/assets/rouge.png",
        "/assets/vert.png",
        "/assets/bleu.png",
        "/assets/jaune.png",
        "/assets/orange.png",
        "/assets/violet.png",
        "/assets/cyan.png"
    };
    for (int i = 0; i < 7; i++)
    {
        blockTextures[i] = loadTexture(renderer, textureFiles[i]);
        if (!blockTextures[i])
            fprintf(stderr, "Texture Introuvable: %s\n", textureFiles[i]);
    }
}

void cleanupBlockTextures()
{
    for (int i = 0; i < 7; i++)
        SDL_DestroyTexture(blockTextures[i]);
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
    renderText(renderer, font, "TETORIS", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 20);
    SDL_RenderPresent(renderer);
    SDL_Delay(1500);
}

void showMenu(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    renderText(renderer, font, "Menu", SCREEN_WIDTH / 2 - 30, SCREEN_HEIGHT / 2 - 40);
    renderText(renderer, font, "r - restart", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2);
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

void cleanup()
{
    cleanupBlockTextures();
    Mix_FreeMusic(music);
    cleanupBackgroundFrames();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
}

void main_loop()
{
    if (gameState != GAME_RUNNING)
    {
        emscripten_cancel_main_loop();
        cleanup();
        return;
    }

    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsed = currentTime - lastFrameTime;
    float deltaTime = elapsed / 1000.0f;
    lastFrameTime = currentTime;

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
                        gameState = GAME_OVER;
                }
                break;

            case SDLK_UP:
                rotatetetoris(&currenttetoris);
                break;

            case SDLK_ESCAPE:
                isMenuOpen = !isMenuOpen;
                isPaused = !isPaused;
                break;

            case SDLK_q:
                if (isMenuOpen)
                    gameState = GAME_OVER;
                break;

            case SDLK_r:
                if (isMenuOpen)
                {
                    for (int y = 0; y < BOARD_HEIGHT; y++)
                        for (int x = 0; x < BOARD_WIDTH; x++)
                            board[y][x] = 0;
                    score = 0;
                    inittetoris(&currenttetoris);
                    isMenuOpen = false;
                    isPaused = false;
                }
                break;

            case SDLK_m:
                if (isMusicPlaying)
                    Mix_PauseMusic();
                else
                    Mix_ResumeMusic();
                isMusicPlaying = !isMusicPlaying;
                break;
            }
        }
    }

    if (!isPaused)
    {
        // Animation de fond
        frameTime += elapsed;
        if (frameTime > frameDelay)
        {
            currentFrame = (currentFrame + 1) % FRAME_COUNT;
            frameTime = 0;
        }

        updateRotation(&currenttetoris, deltaTime);

        // Chute des blocs
        dropTime += elapsed;
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
                    gameState = GAME_OVER;
            }
            dropTime = 0;
        }
    }

    // Rendu
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    renderBackground(renderer);
    drawBoard(renderer);
    drawtetoris(renderer, &currenttetoris);
    drawScore(renderer, font);

    if (isMenuOpen)
        showMenu(renderer, font);

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1)
    {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        fprintf(stderr, "Mix_OpenAudio Error: %s\n", Mix_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    music = Mix_LoadMUS("/assets/music.ogg");
    if (!music)
    {
        fprintf(stderr, "Erreur d'audio: %s\n", Mix_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    Mix_PlayMusic(music, -1);

    font = TTF_OpenFont("/assets/DejaVuSans.ttf", 24);
    if (!font)
    {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("TETORIS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window)
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    loadBackgroundFrames(renderer);
    loadBlockTextures(renderer);
    showTitleScreen(renderer, font);

    inittetoris(&currenttetoris);
    gameState = GAME_RUNNING;
    lastFrameTime = SDL_GetTicks();
    dropTime = 0;

    emscripten_set_main_loop(main_loop, 0, 1);

    return 0;
}