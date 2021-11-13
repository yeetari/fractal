#pragma once

#include <cstddef>

#define V2D_DECLARE_COMPONENT(name) static constexpr std::size_t component_id = static_cast<std::size_t>(name)
