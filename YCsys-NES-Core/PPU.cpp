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
        // Апаратна затримка: CPU отримує дані, які PPU прочитав ПІД ЧАС МИНУЛОГО ВИКЛИКУ.
        data = ppu_data_buffer;

        // PPU робить упереджувальне читання з власної відеошини для наступного кроку
        ppu_data_buffer = ppuRead(vram_addr);

        // ВИНЯТОК: Палітри кольорів ($3F00 - $3FFF) підключені до PPU напряму в обхід системного
        // буфера затримки. Якщо адреса вказує на палітру, дані повертаються негайно.
        if (vram_addr >= 0x3F00) {
            data = ppu_data_buffer;
        }

        // Автоматичний інкремент адреси після кожного звернення до пам'яті
        vram_addr++;
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
        break;
    case 0x0006: // $2006 - PPUADDR (Запис 16-бітної адреси у два етапи)
        if (address_latch == 0) {
            // Етап 1: Записуємо старший байт адреси у верхню половину vram_addr
            vram_addr = (vram_addr & 0x00FF) | (static_cast<uint16_t>(data) << 8);
            address_latch = 1; // Перемикаємо тригер — наступний байт буде молодшим
        }
        else {
            // Етап 2: Записуємо молодший байт адреси у нижню половину vram_addr
            vram_addr = (vram_addr & 0xFF00) | static_cast<uint16_t>(data);
            vram_addr &= 0x3FFF; // Відеопам'ять обмежена 14 бітами, маскуємо все що вище
            address_latch = 0;   // Скидаємо тригер у вихідний стан
        }
        break;
    case 0x0007: // $2007 - PPUDATA
        ppuWrite(vram_addr, data); // Пишемо байт безпосередньо на відеошину PPU
        vram_addr++;               // Автоматично зміщуємо адресу вперед
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