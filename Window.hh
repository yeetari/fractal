#pragma once

#include <cstdint>
#include <span>

struct GLFWwindow;

class Window {
    const std::uint32_t m_width;
    const std::uint32_t m_height;
    GLFWwindow *m_window;

public:
    static void poll_events();
    static std::span<const char *> required_instance_extensions();

    Window(std::uint32_t width, std::uint32_t height, bool fullscreen);
    Window(const Window &) = delete;
    Window(Window &&) = delete;
    ~Window();

    Window &operator=(const Window &) = delete;
    Window &operator=(Window &&) = delete;

    void close() const;
    bool should_close() const;

    GLFWwindow *operator*() const { return m_window; }
    std::uint32_t width() const { return m_width; }
    std::uint32_t height() const { return m_height; }
};
