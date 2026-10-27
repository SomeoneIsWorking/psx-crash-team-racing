// debug_menu.h — CTR's debug menu: the race-track warp through the guest's own level-load request.
#pragma once

#include "title_debug_menu.h"

namespace ctr {

class DebugMenu final : public psx::game::TitleDebugMenu {
public:
  DebugMenu();
};

} // namespace ctr
