#include <SDL2/SDL.h>
#include <stdexcept>

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

    bool process_events()
    {
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
    window(const char* title)
    {
        window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 2, 240 * 2, 0);
        if (window_ == nullptr)
            throw std::runtime_error("Cannot create window");

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (renderer_ == nullptr)
            throw std::runtime_error("Cannot create renderer");
    }

    auto renderer() const { return renderer_; }

    void render(SDL_Texture* screen)
    {
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, screen, NULL, NULL);
        SDL_RenderPresent(renderer_);
    }

private:
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
};

}

auto load_dummy_texture(auto renderer)
{
    auto surface = SDL_LoadBMP("link.bmp");
    return SDL_CreateTextureFromSurface(renderer, surface);
}

int main(int argc, char *argv[])
{
    auto frontend = sdl::frontend::create();
    auto window = sdl::window("Dendy");
    auto texture = load_dummy_texture(window.renderer());

    for (;;) {
        auto stop = frontend.process_events();
        if (stop)
            break;

        window.render(texture);
    }

    return 0;
}