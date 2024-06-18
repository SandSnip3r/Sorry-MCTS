#ifndef PLAYER_COLOR_HPP_
#define PLAYER_COLOR_HPP_

#include <cstdint>
#include <string_view>

namespace sorry {

enum class PlayerColor : uint8_t {
  kGreen,
  kRed,
  kBlue,
  kYellow
};

std::string_view toString(PlayerColor playerColor);

} // namespace sorry

#endif // PLAYER_COLOR_HPP_