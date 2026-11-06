#pragma once

#include <functional>
#include <mutex>
#include <vector>

// Thread-safe Observer. Emit copies the slot list, then runs callbacks without
// holding the internal mutex so a slot may take other locks (e.g. the ready-queue).
template<typename... Args>
class Signal
{
public:
    using SlotType = std::function<void(Args...)>;

    void Connect(SlotType Slot)
    {
        std::lock_guard<std::mutex> Lock(m_Mutex);
        m_Slots.push_back(std::move(Slot));
    }

    void DisconnectAll()
    {
        std::lock_guard<std::mutex> Lock(m_Mutex);
        m_Slots.clear();
    }

    void Emit(Args... args)
    {
        std::vector<SlotType> Slots;
        {
            std::lock_guard<std::mutex> Lock(m_Mutex);
            Slots = m_Slots;
        }
        for (auto& Slot : Slots)
        {
            Slot(args...);
        }
    }

private:
    std::vector<SlotType> m_Slots;
    std::mutex m_Mutex;
};
