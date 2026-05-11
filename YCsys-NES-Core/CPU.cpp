// YCsys NES Core - 6502 CPU implementation
// Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.
#include "CPU.h"
#include "Bus.h" // Обов'язково додаємо, щоб бачити методи класу Bus

// Реалізація приватного методу читання
uint8_t CPU6502::read(uint16_t addr) {
    // Якщо процесор підключений до шини, читаємо дані
    if (bus != nullptr) {
        return bus->read(addr);
    }
    return 0x00; // Повертаємо 0, якщо шина не підключена
}

// Реалізація приватного методу запису
void CPU6502::write(uint16_t addr, uint8_t data) {
    // Якщо процесор підключений до шини, записуємо дані
    if (bus != nullptr) {
        bus->write(addr, data);
    }
}