#pragma once

#include <v2d/maths/Vec.hh>

#include <vulkan/vulkan_core.h>
#include <xcb/xcb.h>

#include <cstdint>
#include <span>

namespace v2d {

class Window {
    std::uint32_t m_width;
    std::uint32_t m_height;
    xcb_connection_t *m_connection{nullptr};
    xcb_intern_atom_reply_t *m_delete_window_atom{nullptr};
    std::uint32_t m_id{0};

    std::uint16_t m_mouse_x{0};
    std::uint16_t m_mouse_y{0};
    bool m_should_close{false};

public:
    static std::span<const char *const> required_instance_extensions();

    Window(std::uint32_t width, std::uint32_t height);
    Window(const Window &) = delete;
    Window(Window &&) = delete;
    ~Window();

    Window &operator=(const Window &) = delete;
    Window &operator=(Window &&) = delete;

    VkSurfaceKHR create_surface(VkInstance instance);

    void close();
    void poll_events();

    std::uint32_t width() const { return m_width; }
    std::uint32_t height() const { return m_height; }
    Vec2f mouse_position() const { return {static_cast<float>(m_mouse_x), static_cast<float>(m_mouse_y)}; }
    Vec2f resolution() const { return {static_cast<float>(m_width), static_cast<float>(m_height)}; }
    bool should_close() const { return m_should_close; }
};

} // namespace v2d
