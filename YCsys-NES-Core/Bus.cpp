// YCsys NES Core - 6502 CPU implementation
// Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.
// додам заглушки для класу Bus, щоб код компілювався і ми могли тестувати CPU
#include "Bus.h"
uint8_t Bus::read(uint16_t addr, bool bReadOnly) {
	// Для тестування просто повертаємо 0x00 для всіх адрес
	return 0x00;
}
void Bus::write(uint16_t addr, uint8_t data) {
	// Для тестування просто нічого не робимо
}
Bus::Bus() {}
Bus::~Bus() {}