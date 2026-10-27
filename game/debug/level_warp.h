#pragma once

#include <cstdint>
#include <optional>
#include <string>

class Core;

namespace ctr {

// The debug menu's `warp <level-hex>` request: armed between fields while a race runs, applied at the next field start
// through the guest's own level-load request, which main-loop state 3 consumes through its loader.
class LevelWarp {
public:
  // Arms a level the debug menu checked against legal(); the returned text answers the client.
  std::string arm(uint32_t level);
  // Raises the armed request once the race still allows one; returns the level requested.
  std::optional<uint32_t> apply(Core &core);

  // A race track is running and the guest has no load of its own pending.
  static bool legal(Core &core);

private:
  std::optional<uint32_t> armed_;
};

} // namespace ctr
