#include <libnes/ppu.hpp>
#include <libnes/cpu.hpp>
#include <libnes/clock.hpp>
#include <libnes/literals.hpp>

#include <SDL2/SDL.h>

#include <stdexcept>
#include <array>
#include <random>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <tuple>
#include <string>
#include <deque>
#include <numeric>
#include "icon16.hpp"

using namespace nes::literals;
using namespace std::string_literals;

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

        uint32_t rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        int shift = (my_icon.bytes_per_pixel == 3) ? 8 : 0;
        rmask = 0xff000000 >> shift;
        gmask = 0x00ff0000 >> shift;
        bmask = 0x0000ff00 >> shift;
        amask = 0x000000ff >> shift;
#else // little endian, like x86
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = (icon16::gimp_image.bytes_per_pixel == 3) ? 0 : 0xff000000;
#endif
        SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(
            (void*)icon16::gimp_image.pixel_data,
            icon16::gimp_image.width,
            icon16::gimp_image.height,
            icon16::gimp_image.bytes_per_pixel*8,
            icon16::gimp_image.bytes_per_pixel*icon16::gimp_image.width,
            rmask, gmask, bmask, amask);

        SDL_SetWindowIcon(window_, icon);
        SDL_FreeSurface(icon);
    }

    void display_fps(double fps) {
        fps_.push_back(fps);
        if (fps_.size() > 100) {
            fps_.pop_front();
        }
        auto average_fps = std::accumulate(fps_.begin(), fps_.end(), 0.0) / fps_.size();
        auto title = "NES Emulator | "s + std::to_string(average_fps) + " fps"s;
        SDL_SetWindowTitle(window_, title.c_str());
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

    std::deque<float> fps_;
};

class chr_window: public window
{
public:
    chr_window(const char* title) {
        window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 256, SDL_WINDOW_UTILITY);
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
    struct controller_hack
    {
        uint8_t keys{0};
        uint8_t snapshot{0};
    } j1;

    dummy_bus(std::tuple<std::array<uint8_t, 64_Kb>, std::array<uint8_t, 8_Kb>>&& rom, nes::ppu& ppu)
        : mem{std::move(std::get<0>(rom))}
        , chr{std::move(std::get<1>(rom))}
        , ppu{ppu}
    {
        ppu.connect_pattern_table(&chr);
    }

    auto nmi() {
        if (not ppu.nmi)
            return false;

        ppu.nmi = false;
        return true;
    }

    void write(uint16_t addr, uint8_t value) {
        if (ppu.write(addr, value))
            return;

        if (addr == 0x4014) {
            ppu.dma_write(&mem[value << 8]);
            return;
        }

        if (addr == 0x4016) {
            j1.snapshot = j1.keys;
        }

        mem[addr] = value;
    }

    uint8_t read(uint16_t addr) {
        if (auto r = ppu.read(addr); r.has_value())
            return r.value();

        if (addr == 0x4016) {
            auto r = (j1.snapshot & 0x80) ? 1 : 0;
            j1.snapshot <<= 1;
            return r;
        }

        return mem[addr];
    }

    std::array<uint8_t, 64_Kb>  mem;
    std::array<uint8_t, 8_Kb>   chr;

    nes::ppu& ppu;
};

auto load_rom(auto filename) {
    auto memory = std::array<uint8_t, 64_Kb>{};
    auto romfile = std::ifstream{filename, std::ifstream::binary};
    assert(romfile.is_open());

    struct {
        char name[4];
        uint8_t prg_rom_chunks;
        uint8_t chr_rom_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        char unused[5];
    } header;
    static_assert(sizeof(header) == 16);
    romfile.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.prg_rom_chunks > 2)
        throw std::runtime_error("unsupported mapper, too many PRG sections");

    if (header.chr_rom_chunks > 1)
        throw std::runtime_error("unsupported mapper, too many CHR sections");

    auto prg = std::array<uint8_t, 16_Kb>{};
    romfile.read(reinterpret_cast<char*>(prg.data()), prg.size());

    std::ranges::copy(prg, memory.begin() + 0x8000);

    if (header.prg_rom_chunks > 1) {
        romfile.read(reinterpret_cast<char*>(prg.data()), prg.size());
    }
    std::ranges::copy(prg, memory.begin() + 0xC000);

    auto chr = std::array<uint8_t, 8_Kb>{};
    romfile.read(reinterpret_cast<char*>(chr.data()), chr.size());

    return std::tuple{memory, chr};
}

static std::random_device rd;
static std::mt19937 gen(rd());

int main(int argc, char *argv[]) {
    auto frontend = sdl::frontend::create();
    auto window = sdl::main_window("Dendy");

    auto ppu = nes::ppu{nes::DEFAULT_COLORS};
    auto bus = dummy_bus{load_rom("rom/nestest.nes"), ppu};
    auto cpu = nes::cpu{bus};

    const auto FPS   = 60;
    const auto DELAY = static_cast<int>(1000.0f / FPS);
    uint32_t frameStart, frameTime;

    frontend.add_window(&window);

    for (;;) {
        frameStart = SDL_GetTicks();

        auto stop = frontend.process_events();
        if (stop)
            break;

        {
            auto kb_state = SDL_GetKeyboardState(nullptr);
            bus.j1.keys = 0;
            if (kb_state[SDL_SCANCODE_SPACE])   bus.j1.keys |= 0x80;
            if (kb_state[SDL_SCANCODE_LSHIFT])  bus.j1.keys |= 0x40;
            if (kb_state[SDL_SCANCODE_C])       bus.j1.keys |= 0x20;
            if (kb_state[SDL_SCANCODE_V])       bus.j1.keys |= 0x10;
            if (kb_state[SDL_SCANCODE_UP])      bus.j1.keys |= 0x08;
            if (kb_state[SDL_SCANCODE_DOWN])    bus.j1.keys |= 0x04;
            if (kb_state[SDL_SCANCODE_LEFT])    bus.j1.keys |= 0x02;
            if (kb_state[SDL_SCANCODE_RIGHT])   bus.j1.keys |= 0x01;
        }

        auto count = 0;
        for (;;++count) {
            cpu.tick();

            ppu.tick(); if (ppu.is_frame_ready()) break;
            ppu.tick(); if (ppu.is_frame_ready()) break;
            ppu.tick(); if (ppu.is_frame_ready()) break;
        }
        assert(count == 29780 || count == 29781);

        window.render(ppu.frame_buffer);

        auto distrib = std::uniform_int_distribution<short>(0, 7);

        frameTime = SDL_GetTicks() - frameStart;

        window.display_fps(1.0 / frameTime * 1000);
        if (frameTime < DELAY) {
            SDL_Delay((int)(DELAY - frameTime));
        }
    }

    return 0;
}