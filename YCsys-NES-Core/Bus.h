// YCsys NES Core - Bus implementation
// Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.

#pragma once
#include <cstdint>
#include <array>
#include "CPU.h"

class Bus {
public:
    Bus();
    ~Bus();

public: // Пристрої на шині
    // Створюємо об'єкт CPU прямо тут. Він автоматично ініціалізується конструктором за замовчуванням.
    CPU6502 cpu;

    // Оперативна пам'ять NES (2KB)
    std::array<uint8_t, 2048> cpuRam = { 0 };

public: // Читання та запис
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr, bool bReadOnly = false);
};