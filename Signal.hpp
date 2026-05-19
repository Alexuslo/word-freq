#pragma once

#include <functional>
#include <vector>
#include <mutex>

// Шаблонный класс Signal реализует потокобезопасный паттерн Observer (Наблюдатель).
// Он позволяет ослабить связность (Loose Coupling) между отправителем события (Worker)
// и получателем события (Main/App).
// Worker'у не нужно знать, кто и как обрабатывает его завершение. Он просто вызывает Emit().
template<typename... Args>
class Signal
{
public:
    using SlotType = std::function<void(Args...)>;

    // Метод для подписки на сигнал.
    // Принимает лямбду или функцию и сохраняет её в список.
    // Вызывается в контексте главного потока (при инициализации).
    void Connect(SlotType Slot) 
    {
        std::lock_guard<std::mutex> Lock(m_Mutex);
        m_Slots.push_back(std::move(Slot));
    }

    // Метод для рассылки сигнала.
    // Вызывается в контексте рабочего потока (Worker'а).
    // Поочередно выполняет все сохраненные лямбды-подписчики НА СТОРОНЕ (и ресурсах)
    // того потока, который вызвал Emit().
    void Emit(Args... args)
    {
        std::lock_guard<std::mutex> Lock(m_Mutex);
        for (auto& Slot : m_Slots)
        {
            Slot(args...);
        }
    }

private:
    std::vector<SlotType> m_Slots;
    std::mutex m_Mutex;
};
