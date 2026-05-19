#pragma once

#include "Signal.hpp"

namespace AppEvents {
    // Общий сигнал для всех потоков.
    // inline — один экземпляр на всю программу, даже если .hpp подключён везде.
    inline Signal<int> OnChunkCompleted;
}
