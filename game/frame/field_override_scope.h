#pragma once

#include "image_identity.h"
#include "native_dispatch.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

class Core;

namespace ctr {

// One guest address owned for a field; the name shows up in override lookup diagnostics. A binding that names an
// image is owned only while that image is resident at its address when the field begins.
struct FieldOverrideBinding {
  uint32_t address = 0;
  const char *name = nullptr;
  psx::cpu::NativeFunction function = nullptr;
  std::string_view image;
};

// A failed install or remove aborts: overrides must not outlive the field. Each is removed by the key it was
// installed under, which stays valid when a load replaces its image during the field.
class FieldOverrideScope final {
public:
  FieldOverrideScope(Core &core, std::span<const FieldOverrideBinding> bindings);
  ~FieldOverrideScope();

  FieldOverrideScope(const FieldOverrideScope &) = delete;
  FieldOverrideScope &operator=(const FieldOverrideScope &) = delete;

  [[nodiscard]] std::size_t installedCount() const;

private:
  struct Installed {
    psx::cpu::NativeKey key;
    const char *name = nullptr;
  };

  // Whether `binding`'s image, if it names one, owns its address now.
  [[nodiscard]] bool resident(const FieldOverrideBinding &binding) const;

  Core &core_;
  std::vector<Installed> installed_;
};

} // namespace ctr
