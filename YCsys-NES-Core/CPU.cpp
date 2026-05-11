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

uint8_t CPU6502::GetFlag(FLAGS6502 f) {
    // Якщо біт за маскою 'f' встановлений, повертаємо 1, інакше 0
    return ((status & f) > 0) ? 1 : 0;
}

void CPU6502::SetFlag(FLAGS6502 f, bool v) {
    if (v) {
        status |= f;    // Встановлюємо біт (OR)
    }
    else {
        status &= ~f;   // Скидаємо біт (AND з інверсією)
    }
}

// Створюємо заглужки- заготовки для режимів адресації, щоб код компілювався
uint8_t CPU6502::IMP() {
    return 0;
}
uint8_t CPU6502::IMM() {
    return 0;
}
uint8_t CPU6502::ZP0() {
    return 0;
}
uint8_t CPU6502::ZPX() {
    return 0;
}
uint8_t CPU6502::ZPY() {
    return 0;
}
uint8_t CPU6502::REL() {
    return 0;
}
uint8_t CPU6502::ABS() {
    return 0;
}
uint8_t CPU6502::ABX() {
    return 0;
}
uint8_t CPU6502::ABY() {
    return 0;
}
uint8_t CPU6502::IND() {
    return 0;
}
uint8_t CPU6502::IZX() {
    return 0;
}
uint8_t CPU6502::IZY() {
    return 0;
}
