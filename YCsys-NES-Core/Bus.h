// YCsys NES Core - 6502 CPU implementation
// Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.
#pragma once
#include <cstdint>
#include <array>

// Попереднє оголошення класів, щоб шина про них знала
class CPU6502;

class Bus {
public:
    Bus();
    ~Bus();

public: // Пристрої на шині
    // Наша RAM для NES (2KB)
    std::array<uint8_t, 2048> cpuRam;

public: // Читання та запис
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr, bool bReadOnly = false);
};