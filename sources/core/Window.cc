#include <v2d/core/Window.hh>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

namespace v2d {

void Window::poll_events() {
    glfwPollEvents();
}

std::span<const char *> Window::required_instance_extensions() {
    std::uint32_t count = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&count);
    return {extensions, count};
}

Window::Window(std::uint32_t width, std::uint32_t height, bool fullscreen) : m_width(width), m_height(height) {
    if (glfwInit() != GLFW_TRUE) {
        std::fputs("Failed to initialise GLFW", stderr);
        std::exit(1);
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), "v2d",
                                fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    if (m_window == nullptr) {
        std::fputs("Failed to create window", stderr);
        std::exit(1);
    }
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::close() const {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

bool Window::should_close() const {
    return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

} // namespace v2d
