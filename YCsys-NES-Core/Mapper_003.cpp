/*
 *  _________________________________________________________________________
 * |     __   __  ______   ______              __    _  _______  _______     |
 * |     \ \ / / |  ____| |  ____|     _      |  \  | ||  _____||  _____|    |
 * |      \   /  | |      | |____     (_)     |   \ | || |_____ | |_____     |
 * |       | |   | |      |____  |    _       | |\   ||  _____| \____  |     |
 * |       | |   | |____   ____| |   (_)      | | \  || |_____  _____| |     |
 * |       |_|   |______| |______|            |_|  \_||_______||_______|     |
 * |        Y C s y s                          N E S   C O R E               |
 * |_________________________________________________________________________|
 * |                                                                         |
 * |      [003] YCsys NES CORE - MAPPER 003 (CNROM) IMPLEMENTATION [003]     |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [CNROM]       [8KB ]        [CVSW]        [CHR ]        [CNROM]
 */

#include "Mapper_003.h"

Mapper_003::Mapper_003(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
    reset();
}

Mapper_003::~Mapper_003() {}

void Mapper_003::reset() {
    nCHRBankSelect = 0;
}

bool Mapper_003::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Якщо гра має лише 1 банк PRG (16 КБ), він дублюється (віддзеркалюється). 
        // Якщо 2 банки (32 КБ) - вони заповнюють весь простір.
        if (nPRGBanks == 1) {
            mapped_addr = addr & 0x3FFF;
        }
        else if (nPRGBanks == 2) {
            mapped_addr = addr & 0x7FFF;
        }
        return true;
    }
    return false;
}

bool Mapper_003::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Будь-який запис у пам'ять CPU перемикає банк графіки.
        // Зазвичай використовуються лише молодші 2 біти (0-3), 
        // але іноді ігри використовують більше. Беремо 4 біти для сумісності.
        nCHRBankSelect = data & 0x0F;
    }
    return false; // Картриджу не потрібно писати це в ROM
}

bool Mapper_003::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr < 0x2000) {
        // Відеочіп отримує графіку з вибраного банку
        mapped_addr = (nCHRBankSelect * 0x2000) + addr;
        return true;
    }
    return false;
}

bool Mapper_003::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    // Ігри на CNROM зазвичай використовують ROM для графіки (Read Only).
    // Тому запис від відеочіпа не підтримується.
    return false;
}