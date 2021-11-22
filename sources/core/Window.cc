#include <v2d/core/Window.hh>

#include <v2d/support/Array.hh>
#include <v2d/support/Assert.hh>

#include <vulkan/vulkan_xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

#include <cstdlib>

namespace v2d {

Span<const char *const> Window::required_instance_extensions() {
    static const Array s_instance_extensions{
        "VK_KHR_surface",
        "VK_KHR_xcb_surface",
    };
    return s_instance_extensions.span();
}

Window::Window(std::uint32_t width, std::uint32_t height) : m_width(width), m_height(height) {
    // Open X connection.
    m_connection = xcb_connect(nullptr, nullptr);
    V2D_ENSURE(xcb_connection_has_error(m_connection) == 0, "Failed to create X connection");

    // Create window on the first screen and set the title.
    m_id = xcb_generate_id(m_connection);
    std::uint32_t event_mask = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION;
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;
    xcb_create_window(m_connection, screen->root_depth, m_id, screen->root, 0, 0, width, height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, XCB_CW_EVENT_MASK, &event_mask);
    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_id, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, 3, "v2d");

    // Setup the delete window protocol via though WM_PROTOCOLS property.
    auto protocols_atom_request = xcb_intern_atom(m_connection, 1, 12, "WM_PROTOCOLS");
    auto *protocols_atom = xcb_intern_atom_reply(m_connection, protocols_atom_request, nullptr);
    auto delete_window_atom_request = xcb_intern_atom(m_connection, 0, 16, "WM_DELETE_WINDOW");
    m_delete_window_atom = xcb_intern_atom_reply(m_connection, delete_window_atom_request, nullptr);
    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_id, protocols_atom->atom, XCB_ATOM_ATOM, 32, 1,
                        &m_delete_window_atom->atom);
    std::free(protocols_atom);

    // Make the window visible and wait for the server to have processed the requests.
    xcb_map_window(m_connection, m_id);
    xcb_aux_sync(m_connection);
}

Window::~Window() {
    std::free(m_delete_window_atom);
    xcb_destroy_window(m_connection, m_id);
    xcb_disconnect(m_connection);
}

VkSurfaceKHR Window::create_surface(VkInstance instance) const {
    VkXcbSurfaceCreateInfoKHR surface_ci{
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = m_connection,
        .window = m_id,
    };
    VkSurfaceKHR surface = nullptr;
    vkCreateXcbSurfaceKHR(instance, &surface_ci, nullptr, &surface);
    return surface;
}

void Window::close() {
    m_should_close = true;
}

void Window::poll_events() {
    xcb_generic_event_t *event;
    while ((event = xcb_poll_for_event(m_connection)) != nullptr) {
        switch (event->response_type & ~0x80u) {
        case XCB_KEY_PRESS: {
            auto *key_press_event = reinterpret_cast<xcb_key_press_event_t *>(event);
            m_keys[key_press_event->detail] = true;
            break;
        }
        case XCB_KEY_RELEASE: {
            auto *key_release_event = reinterpret_cast<xcb_key_release_event_t *>(event);
            m_keys[key_release_event->detail] = false;
            break;
        }
        case XCB_MOTION_NOTIFY: {
            auto *motion_event = reinterpret_cast<xcb_motion_notify_event_t *>(event);
            m_mouse_x = motion_event->event_x;
            m_mouse_y = motion_event->event_y;
            break;
        }
        case XCB_CLIENT_MESSAGE: {
            const auto &data = reinterpret_cast<xcb_client_message_event_t *>(event)->data;
            if (data.data32[0] == m_delete_window_atom->atom) {
                m_should_close = true;
            }
            break;
        }
        }
        std::free(event);
    }
    xcb_flush(m_connection);
}

} // namespace v2d
