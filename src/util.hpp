#pragma once

#include <string>

/**
 * @brief Search where a file is located
 * @param filename The filename to search. This is a basename (e.g., "bomb.png")
 * @return The absolute filename if the file is found. Throws std::runtime_error otherwise.
 */
std::string searchImageAbsoluteFilename(const std::string & filename);

/**
 * @brief Search where a file is located
 * @param filename The filename to search. This is a basename (e.g., "DejaVuSansMono.ttf")
 * @return The absolute filename if the file is found. Throws std::runtime_error otherwise.
 */
std::string searchFontAbsoluteFilename(const std::string & filename);

std::string programAbsoluteFilename();
