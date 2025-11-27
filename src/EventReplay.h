#pragma once
#include <cstdint>

class GloomMap;
class GameLogic;

namespace EventReplay {

/// Record a one-shot event that was actually executed during play (e.g., wall button / zone trigger).
/// Only records events with id < 19 (one-shot semantics in GameLogic).
void Record(uint32_t ev);

/// Re-run recorded events after a savegame is applied, so movers/doors/rotators resume.
/// Also marks events as "hit" in GameLogic to avoid re-triggering.
void Replay(GloomMap& gmap, GameLogic& logic);

/// Clear the persistent event list for a fresh level start.
void Clear();

} // namespace EventReplay
