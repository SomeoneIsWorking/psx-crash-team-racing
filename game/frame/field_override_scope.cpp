#include "frame/field_override_scope.h"

#include "core.h"
#include "native_dispatch.h"

#include <cstdlib>
#include <lucent/log.h>

namespace ctr {

FieldOverrideScope::FieldOverrideScope(Core &core, std::span<const FieldOverrideBinding> bindings) : core_(core) {
  installed_.reserve(bindings.size());
  for (const FieldOverrideBinding &binding : bindings) {
    if (!resident(binding)) {
      continue;
    }
    const auto key = psx::cpu::tryInstallNativeOverride(core_, binding.address, binding.name, binding.function);
    if (!key) {
      lucent::error("ctr-frame", "could not install '{}' at 0x{:08X}", binding.name, binding.address);
      std::abort();
    }
    installed_.push_back({.key = *key, .name = binding.name});
  }
}

FieldOverrideScope::~FieldOverrideScope() {
  for (const Installed &override : installed_) {
    if (!core_.nativeDispatcher().remove(override.key)) {
      lucent::error("ctr-frame", "could not remove '{}' at 0x{:08X}", override.name, override.key.address);
      std::abort();
    }
  }
}

std::size_t FieldOverrideScope::installedCount() const {
  return installed_.size();
}

bool FieldOverrideScope::resident(const FieldOverrideBinding &binding) const {
  if (binding.image.empty()) {
    return true;
  }
  const auto image = core_.imageCatalog().resolve(binding.address);
  if (!image) {
    return false;
  }
  const auto description = core_.imageCatalog().describe(*image);
  return description && description->name == binding.image;
}

} // namespace ctr
