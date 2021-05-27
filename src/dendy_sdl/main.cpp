#include <SDL2/SDL.h>
#include <stdexcept>

#include "libnes/ppu.hpp"

namespace sdl
{
class frontend
{
    frontend()  { SDL_Init(SDL_INIT_VIDEO); }

public:
    ~frontend() { SDL_Quit(); }
    frontend(frontend&&) = default;
    frontend& operator=(frontend&&) = default;

    auto static create() { return frontend{}; }

    bool process_events() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type)
            {
                case SDL_QUIT: return true;
                case SDL_KEYDOWN: break;
            }
        }
        return false;
    }
};

class window
{
public:
    window(const char* title) {
        window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600 * 2, 480 * 2, 0);
        if (window_ == nullptr)
            throw std::runtime_error("Cannot create window");

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (renderer_ == nullptr)
            throw std::runtime_error("Cannot create renderer");
    }

    auto renderer() const { return renderer_; }

    void render(SDL_Texture* screen) {
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, screen, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }

private:
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
};

}

auto load_dummy_texture(auto renderer) {
    auto surface = SDL_LoadBMP("link.bmp");
    return SDL_CreateTextureFromSurface(renderer, surface);
}

auto load_texture(const auto& frame_bufer) {

}

int main(int argc, char *argv[]) {
    auto frontend = sdl::frontend::create();
    auto window = sdl::window("Dendy");

//    auto texture = load_dummy_texture(window.renderer());
    auto texture = SDL_CreateTexture (window.renderer(),
                                     SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                     256, 240);

    auto ppu = nes::ppu{};

    const auto FPS   = 60;
    const auto DELAY = static_cast<int>(1000.0f / FPS);
    uint32_t frameStart, frameTime;

    for (;;) {
        frameStart = SDL_GetTicks();

        auto stop = frontend.process_events();
        if (stop)
            break;

        do {
            ppu.tick();
        } while(!ppu.is_frame_ready());
        SDL_UpdateTexture(texture, nullptr, ppu.frame_buffer.data(), 256 * sizeof(uint32_t));

        window.render(texture);

        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < DELAY)
            SDL_Delay((int)(DELAY - frameTime));
    }

    return 0;
}