#pragma once

class Core;

namespace ctr {

// DrawLevelOvr1P (overlay 226) drawing the plan's columns. The guest's call is retail's, at retail width, time, pool
// and links. The same call then runs again with guest time held, every store put back, and the view's rect spanning the
// drawn columns, so its OFX and packed window keep margin polygons; those retail's window rejects are drawn as host
// packets, moved back by the left margin, in their buckets after the call's own. At 4:3 the call is retail's alone.
class LevelDrawMargins final {
public:
  void draw(Core &core) const;
};

} // namespace ctr
