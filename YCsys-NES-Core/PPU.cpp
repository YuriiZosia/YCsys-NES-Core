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
 * |      [P]  YCsys NES CORE - PICTURE PROCESSING UNIT (PPU)   [P]          |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [TV ]        [VRAM]        [PAL]        [SPR]        [TV ]
 */

#include "PPU.h"

PPU::PPU() : tblName{}, tblPalette{} {}
PPU::~PPU() {}

// =========================================================================
// ГОЛОВНИЙ ЦИКЛ PPU (System Clock)
// =========================================================================
void PPU::clock() {
    // Малюємо піксель ТІЛЬКИ якщо промінь знаходиться у видимій зоні (256x240)
    if (scanline >= 0 && scanline < 240 && cycle >= 0 && cycle < 256) {
        // Явно вказуємо, що математику треба робити у великому типі (size_t)
        sprScreen[static_cast<size_t>(scanline) * 256 + cycle] = (rand() % 2 == 0) ? 0xFFFFFFFF : 0xFF000000;
    }

    cycle++;
    if (cycle >= 341) { // Клікнули за межі правого краю екрану
        cycle = 0;
        scanline++;

        if (scanline >= 261) { // Дійшли до кінця кадру
            scanline = -1;     // Скидаємо на перед-рендеринговий рядок
            frame_complete = true; // Сигнал для SDL2, що можна виводити картинку
        }
    }
}

// =========================================================================
// ЧИТАННЯ РЕГІСТРІВ З БОКУ CPU (Головна шина передає адресу як 0x0000 - 0x0007)
// =========================================================================
uint8_t PPU::cpuRead(uint16_t addr, bool rdonly) {
    uint8_t data = 0x00;

    switch (addr) {
    case 0x0000: // $2000 - PPUCTRL (Тільки для запису)
        break;
    case 0x0001: // $2001 - PPUMASK (Тільки для запису)
        break;
    case 0x0002: // $2002 - PPUSTATUS
        // Читаємо 3 старші біти реального статусу. Молодші 5 бітів фізично не підключені 
        // на платі NES до цього регістра, тому там залишається "сміття" з буфера даних.
        data = (status & 0xE0) | (ppu_data_buffer & 0x1F);

        if (!rdonly) {
            status &= ~0x80;   // Апаратна особливість: читання статусу скидає прапорець VBlank (7-й біт)
            address_latch = 0; // Чищення статусу також повністю скидає адресний тригер $2006
        }
        break;
    case 0x0003: // $2003 - OAMADDR
        break;
    case 0x0004: // $2004 - OAMDATA
        break;
    case 0x0005: // $2005 - PPUSCROLL (Тільки для запису)
        break;
    case 0x0006: // $2006 - PPUADDR (Тільки для запису)
        break;
    case 0x0007: // $2007 - PPUDATA
        // Апаратна затримка
        data = ppu_data_buffer;
        ppu_data_buffer = ppuRead(v.reg);

        // ВИНЯТОК: Палітри кольорів ($3F00 - $3FFF) читаються без затримки
        if (v.reg >= 0x3F00) {
            data = ppu_data_buffer;
        }

        // Інкремент адреси залежить від біта 2 регістра PPUCTRL
        v.reg += (control & 0x04) ? 32 : 1;
        break;
    }
    return data;
}

// =========================================================================
// ЗАПИС В РЕГІСТРИ З БОКУ CPU (Головна шина передає адресу як 0x0000 - 0x0007)
// =========================================================================
void PPU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
    case 0x0000: // $2000 - PPUCTRL
        control = data;
        // Нижні 2 біти PPUCTRL вказують на стартову таблицю імен (екран)
        t.nametable_x = control & 0x01;
        t.nametable_y = (control & 0x02) >> 1;
        break;
    case 0x0001: // $2001 - PPUMASK
        mask = data;
        break;
    case 0x0002: // $2002 - PPUSTATUS (Тільки для читання)
        break;
    case 0x0003: // $2003 - OAMADDR
        break;
    case 0x0004: // $2004 - OAMDATA
        break;
    case 0x0005: // $2005 - PPUSCROLL
        if (address_latch == 0) {
            // Перший запис: X зміщення
            fine_x = data & 0x07;       // Молодші 3 біти — це точний зсув (fine_x)
            t.coarse_x = data >> 3;     // Старші 5 бітів — це грубий зсув (coarse_x)
            address_latch = 1;
        }
        else {
            // Другий запис: Y зміщення
            t.fine_y = data & 0x07;     // Молодші 3 біти — точний зсув (fine_y)
            t.coarse_y = data >> 3;     // Старші 5 бітів — грубий зсув (coarse_y)
            address_latch = 0;
        }
        break;
    case 0x0006: // $2006 - PPUADDR
        if (address_latch == 0) {
            // Перший запис: старший байт адреси VRAM (обмежений до 14 біт маскою 0x3F)
            t.reg = (t.reg & 0x00FF) | (static_cast<uint16_t>(data & 0x3F) << 8);
            address_latch = 1;
        }
        else {
            // Другий запис: молодший байт адреси VRAM
            t.reg = (t.reg & 0xFF00) | data;
            v = t; // При другому записі адреса ОФІЦІЙНО передається відеочіпу
            address_latch = 0;
        }
        break;
    case 0x0007: // $2007 - PPUDATA
        ppuWrite(v.reg, data);
        // Інкремент адреси після запису залежить від біта 2 регістра PPUCTRL (+1 або +32)
        v.reg += (control & 0x04) ? 32 : 1;
        break;
    }
}

// =========================================================================
// ВЛАСНА ШИНА ВІДЕОПРОЦЕСОРА (PPU Bus - Читання VRAM / CHR)
// =========================================================================
uint8_t PPU::ppuRead(uint16_t addr, bool rdonly) {
    uint8_t data = 0x00;
    addr &= 0x3FFF; // Обмежуємо адресу діапазоном PPU (16 КБ)

    // 1. Читання графіки (CHR ROM/RAM) з картриджа
    if (cart && cart->ppuRead(addr, data)) {
        // Картридж успішно обробив адресу (0x0000 - 0x1FFF)
    }
    // 2. Читання VRAM (Nametables / Таблиці імен)
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF; // Залишаємо адресу в межах 4 КБ

        // Тимчасова реалізація вертикального віддзеркалення (Vertical Mirroring)
        if (addr >= 0x0000 && addr <= 0x03FF)
            data = tblName[0][addr & 0x03FF];
        else if (addr >= 0x0400 && addr <= 0x07FF)
            data = tblName[1][addr & 0x03FF];
        else if (addr >= 0x0800 && addr <= 0x0BFF)
            data = tblName[0][addr & 0x03FF];
        else if (addr >= 0x0C00 && addr <= 0x0FFF)
            data = tblName[1][addr & 0x03FF];
    }
    // 3. Читання Палітр (Кольори)
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F; // Палітра займає лише 32 байти

        // Відтворюємо апаратне віддзеркалення прозорих кольорів спрайтів на фон
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;

        data = tblPalette[addr];
    }

    return data;
}

// =========================================================================
// ВЛАСНА ШИНА ВІДЕОПРОЦЕСОРА (PPU Bus - Запис у VRAM / CHR)
// =========================================================================
void PPU::ppuWrite(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;

    // 1. Запис графіки (CHR RAM) на картридж
    if (cart && cart->ppuWrite(addr, data)) {
        // Картридж обробив адресу
    }
    // 2. Запис у VRAM (Nametables / Таблиці імен)
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;

        // Тимчасова реалізація вертикального віддзеркалення (Vertical Mirroring)
        if (addr >= 0x0000 && addr <= 0x03FF)
            tblName[0][addr & 0x03FF] = data;
        else if (addr >= 0x0400 && addr <= 0x07FF)
            tblName[1][addr & 0x03FF] = data;
        else if (addr >= 0x0800 && addr <= 0x0BFF)
            tblName[0][addr & 0x03FF] = data;
        else if (addr >= 0x0C00 && addr <= 0x0FFF)
            tblName[1][addr & 0x03FF] = data;
    }
    // 3. Запис Палітр (Кольори)
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;

        // Апаратне віддзеркалення прозорих кольорів
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;

        tblPalette[addr] = data;
    }
}