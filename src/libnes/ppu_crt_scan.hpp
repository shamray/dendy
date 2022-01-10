#pragma once

namespace nes {

class crt_scan
{
public:
    constexpr crt_scan(int dots, int visible_scanlines, int postrender_scanlines, int vblank_scanlines) noexcept
        : dots_{dots}
        , visible_scanlines_{visible_scanlines}
        , postrender_scanlines_{postrender_scanlines}
        , vblank_scanlines_{vblank_scanlines}
    {}

    [[nodiscard]] constexpr auto line() const noexcept { return line_; }
    [[nodiscard]] constexpr auto cycle() const noexcept { return cycle_; }
    [[nodiscard]] constexpr auto is_odd_frame() const noexcept { return frame_is_odd_; }
    [[nodiscard]] constexpr auto is_frame_finished() const noexcept { return line_ == -1 and cycle_ == 0; }

    [[nodiscard]] constexpr auto is_prerender() const noexcept {
        return line_ == -1;
    }

    [[nodiscard]] constexpr auto is_visible() const noexcept {
        return line_ >= 0 and line_ < visible_scanlines_;
    }

    [[nodiscard]] constexpr auto is_postrender() const noexcept {
        return line_ >= visible_scanlines_ and line_ < visible_scanlines_ + postrender_scanlines_;
    }

    [[nodiscard]] constexpr auto is_vblank() const noexcept {
        return line_ >= visible_scanlines_ + postrender_scanlines_ and
               line_ < visible_scanlines_ + postrender_scanlines_ + vblank_scanlines_;
    }

    constexpr void advance() noexcept {
        if (++cycle_ >= dots_) {
            cycle_ = 0;

            if (++line_ >= visible_scanlines_ + postrender_scanlines_ + vblank_scanlines_) {
                line_ = -1;
                frame_is_odd_ = not frame_is_odd_;
            }
        }
    }

private:
    const int dots_;
    const int visible_scanlines_;
    const int postrender_scanlines_;
    const int vblank_scanlines_;

    short line_{-1};
    short cycle_{0};
    bool frame_is_odd_{false};
};

}
