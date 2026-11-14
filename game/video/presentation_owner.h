#pragma once

#include <cstdint>

class Core;

namespace ctr {

class SceneCut;

// Owns the framework presentation fence at each frame-driver step; the record path seals and presents each.
class PresentationOwner final {
public:
  explicit PresentationOwner(SceneCut &sceneCut);

  void finishField(Core &core, uint32_t fields);

  [[nodiscard]] uint64_t completedFences() const;

private:
  SceneCut &sceneCut_;
  uint64_t completedFences_ = 0;
};

} // namespace ctr
