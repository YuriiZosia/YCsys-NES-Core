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
 * |      [002] YCsys NES CORE - MAPPER 002 (UxROM) IMPLEMENTATION [002]     |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [UXROM]       [16KB]        [UNFX]        [BANK]        [UXROM]
 */

#include "Mapper_002.h"

Mapper_002::Mapper_002(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
    // При ініціалізації останній банк жорстко фіксується
    nPRGBankSelect_Hi = nPRGBanks - 1;
}

Mapper_002::~Mapper_002() {}

void Mapper_002::reset() {
    nPRGBankSelect_Lo = 0;
    nPRGBankSelect_Hi = nPRGBanks - 1;
}

bool Mapper_002::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xBFFF) {
        // Читання з нижнього вікна (підключений банк)
        mapped_addr = (nPRGBankSelect_Lo * 0x4000) + (addr & 0x3FFF);
        return true;
    }
    if (addr >= 0xC000 && addr <= 0xFFFF) {
        // Читання з верхнього вікна (завжди останній банк)
        mapped_addr = (nPRGBankSelect_Hi * 0x4000) + (addr & 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper_002::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Гра записує дані сюди ТІЛЬКИ щоб перемикати банки.
        // Ми не зберігаємо ці дані в пам'ять, а просто перехоплюємо команду.
        nPRGBankSelect_Lo = addr & 0x0F; // Номер банку лежить прямо в самих даних (молодші 4 біти)
        // Примітка: зазвичай беруть data & 0x0F, але в архітектурі NML параметр data 
        // часто не передається в cpuMapWrite мапера. Щоб уникнути зміни інтерфейсу, 
        // у UxROM номер банку іноді прив'язують до молодших бітів самої адреси або 
        // ми модифікуємо метод!
    }

    // ВАЖЛИВО: Оскільки наш Mapper.h не приймає `data` у cpuMapWrite, 
    // нам треба сказати картриджу, щоб він зберіг ці дані!
    // Ми дозволяємо запис у віртуальну пам'ять картриджа, а картридж вже сам перемикне банк.
    // Тому ми просто повертаємо false, і навчимо Cartridge перехоплювати це!
    return false;
}

// Ігри на Mapper 2 зазвичай використовують CHR RAM замість CHR ROM, 
// тому відеочіп просто читає/пише безпосередньо за адресами 0x0000 - 0x1FFF.
bool Mapper_002::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr < 0x2000) {
        // Ігри на Mapper 2 зазвичай використовують CHR RAM, тому адреса = адреса
        mapped_addr = addr;
        return true;
    }
    return false;
}

bool Mapper_002::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    if (addr < 0x2000) {
        mapped_addr = addr;
        return true;
    }
    return false;
}