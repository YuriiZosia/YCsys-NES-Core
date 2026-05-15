// YCsys NES Core - 6502 CPU implementation
// Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.
// додам заглушки для класу Bus, щоб код компілювався і ми могли тестувати CPU
#include "Bus.h"

Bus::Bus() : cpuRam{ 0 } { //забиваємо cpuRam нулями при створенні об'єкта Bus
    // Підключаємо CPU до цієї шини при створенні
	cpu.ConnectBus(this);

    // Очищуємо оперативну пам'ять
    for (auto& i : cpuRam) i = 0x00;
}

Bus::~Bus() {}

void Bus::write(uint16_t addr, uint8_t data) {
    // Перевіряємо, чи запис іде в діапазон RAM (0x0000 - 0x1FFF)
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Маска 0x07FF реалізує дзеркалювання 2КБ оперативної пам'яті
        cpuRam[addr & 0x07FF] = data;
    }
}

uint8_t Bus::read(uint16_t addr, bool bReadOnly) {
    uint8_t data = 0x00;

    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Читаємо з RAM з урахуванням дзеркалювання
        data = cpuRam[addr & 0x07FF];
    }

    return data;
}