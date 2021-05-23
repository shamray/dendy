#include <SDL2/SDL.h>

SDL_Texture* screen;
SDL_Window* window;
SDL_Renderer* renderer;

void main_loop() {
    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type)
            {
                case SDL_QUIT: return;
                case SDL_KEYDOWN: break;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("hello, world!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 2, 240 * 2, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    auto surface = SDL_LoadBMP("link.bmp");
    screen = SDL_CreateTextureFromSurface(renderer, surface);
    main_loop();

    return 0;
}