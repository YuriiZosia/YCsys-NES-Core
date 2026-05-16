/*
 * _________________________________________________________________________
 * |     __   __  ______   ______              __    _  _______  _______     |
 * |     \ \ / / |  ____| |  ____|     _      |  \  | ||  _____||  _____|    |
 * |      \   /  | |      | |____     (_)     |   \ | || |_____ | |_____     |
 * |       | |   | |      |____  |    _       | |\   ||  _____| \____  |     |
 * |       | |   | |____   ____| |   (_)      | | \  || |_____  _____| |     |
 * |       |_|   |______| |______|            |_|  \_||_______||_______|     |
 * |        Y C s y s                          N E S   C O R E               |
 * |_________________________________________________________________________|
 * |                                                                         |
 * |      [000] YCsys NES CORE - MAPPER 000 (NROM) IMPLEMENTATION [000]      |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [NROM]        [16KB]        [32KB]        [BANK]        [NROM]
 */

#include "Mapper_000.h"

Mapper_000::Mapper_000(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {}

Mapper_000::~Mapper_000() {}

// Маршрутизація для CPU
bool Mapper_000::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
	if (addr >= 0x8000 && addr <= 0xFFFF) { // Адресація для читання з PRG ROM починається з 0x8000 до 0xFFFF
		mapped_addr = addr & (nPRGBanks > 1 ? 0x7FFF : 0x3FFF); // Якщо є 2 банки, маскуємо адресацію на 32 КБ, інакше на 16 КБ
        return true;
    }
    return false;
}

bool Mapper_000::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
		mapped_addr = addr & (nPRGBanks > 1 ? 0x7FFF : 0x3FFF); // Якщо є 2 банки, маскуємо адресацію на 32 КБ, інакше на 16 КБ
        return true;
    }
    return false;
}

// Маршрутизація для PPU
bool Mapper_000::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
	if (addr >= 0x0000 && addr <= 0x1FFF) { // Адресація для читання з CHR ROM починається з 0x0000 до 0x1FFF
        mapped_addr = addr;
        return true;
    }
    return false;
}

bool Mapper_000::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        if (nCHRBanks == 0) {
            mapped_addr = addr;
            return true;
        }
    }
    return false;
}