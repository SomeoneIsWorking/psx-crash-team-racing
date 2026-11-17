#pragma once

class Core;

namespace ctr {

// The Naughty Dog crate's skip: overlay 233's CS_Thread_UseOpcode takes Start only once the level is 5.8 s old. After
// the scene's own call (the one with no instance), a Start or Cross tap with no load pending takes retail's crate skip
// branch: the sequence music stops, the XA clip fades, the flag comes on and the main menu loads. The call has run its
// opcodes for the field, which retail's skip leaves out; the decoded opcode is restored as retail does.
class CrateLogoSkip final {
public:
  static void useOpcode(Core *core);
};

} // namespace ctr
