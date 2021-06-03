#include <libnes/ppu.hpp>
#include <libnes/literals.hpp>

#include <SDL2/SDL.h>

#include <stdexcept>
#include <array>
#include <random>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <fstream>

using namespace nes::literals;

namespace sdl
{

class window {
public:
    virtual ~window() {
        SDL_DestroyWindow(window_);
    }

    void process_event(const SDL_Event e) {
        assert(e.type == SDL_WINDOWEVENT);
        assert(SDL_GetWindowID(window_) == e.window.windowID);

        switch(e.window.event) {
            case SDL_WINDOWEVENT_CLOSE:
                close();
        }
    }

    virtual void close() {
        SDL_HideWindow(window_);
    }

    [[nodiscard]] auto id() const { return SDL_GetWindowID(window_); }
    [[nodiscard]] auto quit() const { return quit_; }

protected:
    bool quit_{false};
    SDL_Window* window_{nullptr};
};

class frontend
{
    frontend()  { SDL_Init(SDL_INIT_VIDEO); }

public:
    ~frontend() { SDL_Quit(); }
    frontend(frontend&&) = default;
    frontend& operator=(frontend&&) = default;

    auto static create() { return frontend{}; }

    auto process_window_event(const SDL_Event& e) {
        auto found = windows_.find(e.window.windowID);
        if (found == std::end(windows_))
            return false;

        auto window = found->second;
        window->process_event(e);
        return window->quit();
    }

    auto process_events() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type)
            {
                case SDL_WINDOWEVENT:
                    if (process_window_event(e))
                        return true;
                    break;
                case SDL_QUIT: return true;
                case SDL_KEYDOWN: break;
            }
        }
        return false;
    }

    void add_window(window* w) {
        windows_.insert(std::pair{w->id(), w});
    }

private:
    std::unordered_map<std::uint32_t, window*> windows_;
};

class main_window: public window
{
public:
    main_window(const char* title) {
        window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 480, 0);
        if (window_ == nullptr)
            throw std::runtime_error("Cannot create window");

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (renderer_ == nullptr)
            throw std::runtime_error("Cannot create renderer");

        screen_ = SDL_CreateTexture (renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    }

    void render(const auto& frame_buffer) {
        SDL_RenderClear(renderer_);
        SDL_UpdateTexture(screen_, nullptr, frame_buffer.data(), 256 * sizeof(uint32_t));
        SDL_RenderCopy(renderer_, screen_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }

    void close() override {
        quit_ = true;
    }

    ~main_window() override {
        SDL_DestroyTexture(screen_);
        SDL_DestroyRenderer(renderer_);
    }

private:
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* screen_{nullptr};
};

class chr_window: public window
{
public:
    chr_window(const char* title) {
        window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 512, SDL_WINDOW_UTILITY);
        if (window_ == nullptr)
            throw std::runtime_error("Cannot create window");

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (renderer_ == nullptr)
            throw std::runtime_error("Cannot create renderer");

        chr_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 128, 128);
    }

    void render(const auto& pattern_table) {
        SDL_RenderClear(renderer_);
        SDL_UpdateTexture(chr_, nullptr, pattern_table.data(), 128 * sizeof(uint32_t));
        SDL_RenderCopy(renderer_, chr_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }

    ~chr_window() override {
        SDL_DestroyTexture(chr_);
        SDL_DestroyRenderer(renderer_);
    }

private:
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* chr_{nullptr};
};

}

auto load_dummy_texture(auto renderer) {
    auto surface = SDL_LoadBMP("link.bmp");
    return SDL_CreateTextureFromSurface(renderer, surface);
}

auto load_texture(const auto& frame_bufer) {

}

struct dummy_bus
{
    void chr_write(uint16_t addr, uint8_t value) { mem[addr] = value; }
    uint8_t chr_read(uint16_t addr) const {
        return mem[addr];
    }

    std::array<uint8_t, 8_Kb> mem;
};

auto load_rom(auto filename) {
    auto memory = std::vector<uint8_t>(64_Kb, 0);
    auto romfile = std::ifstream{filename, std::ifstream::binary};
    assert(romfile.is_open());

    romfile.seekg(16); // header

    auto prg = std::array<uint8_t, 16_Kb>{};
    romfile.read(reinterpret_cast<char*>(prg.data()), prg.size());

    std::ranges::copy(prg, memory.begin() + 0x8000);
    std::ranges::copy(prg, memory.begin() + 0xC000);

    auto chr = std::array<uint8_t, 8_Kb>{};
    romfile.read(reinterpret_cast<char*>(chr.data()), chr.size());

    return chr;
}

int main(int argc, char *argv[]) {
    auto frontend = sdl::frontend::create();
    auto window = sdl::main_window("Dendy");

    auto chr = std::array{sdl::chr_window("CHR 0"), sdl::chr_window("CHR 1")};

    auto bus = dummy_bus{load_rom("rom/tanks.nes")};
    auto ppu = nes::ppu{bus};

    const auto FPS   = 60;
    const auto DELAY = static_cast<int>(1000.0f / FPS);
    uint32_t frameStart, frameTime;

    frontend.add_window(&window);
    frontend.add_window(&chr[0]);
    frontend.add_window(&chr[1]);

    for (;;) {
        frameStart = SDL_GetTicks();

        auto stop = frontend.process_events();
        if (stop)
            break;

        do {
            ppu.tick();
        } while(!ppu.is_frame_ready());

        ppu.render_noise([](){
            static std::random_device rd;
            static std::mt19937 gen(rd());

            auto distrib = std::uniform_int_distribution<short>(0, 255);

            return static_cast<uint8_t>(distrib(gen));
        });
        window.render(ppu.frame_buffer);

        for (auto i = 0; i < chr.size(); ++i) {
            chr[i].render(ppu.display_pattern_table(i));
        }

        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < DELAY)
            SDL_Delay((int)(DELAY - frameTime));
    }

    return 0;
}