#pragma once
#include <string>

namespace bootselect {

// Returns one of: "deluxe", "gloom", "gloom3", "massacre"
// Shows a minimal pre-boot game selection before assets are loaded.
std::string Run(const char* preselectKey = nullptr);

} // namespace bootselect
