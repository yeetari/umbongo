#pragma once

namespace kernel {

enum class InterruptPolarity {
    ActiveHigh,
    ActiveLow,
};

enum class InterruptTriggerMode {
    EdgeTriggered,
    LevelTriggered,
};

} // namespace kernel
