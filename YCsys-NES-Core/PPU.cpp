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
// ГОЛОВНИЙ ЦИКЛ PPU (Генератор таймінгів екрану та рендеринг)
// =========================================================================
void PPU::clock() {

	// потрібно додати скидання Sprite Zero Hit на початку кадру, щоб уникнути помилкових спрацьовувань
    if (scanline == -1 && cycle == 1) {
        bSpriteZeroHitPossible = false; // Скидаємо прапорець можливості Sprite Zero Hit на початку кадру
        status &= ~0xE0; //Очищуємо одним махом (це скине біти 7, 6 та 5 одночасно) VBlank (0x80), Sprite Zero Hit (0x40) та Sprite Overflow (0x20)
        nmi_occurred = false; // ФІКС: Надійно знімаємо сигнал NMI після закінчення VBlank
	}

	// =====================================================================
	// 1. ПАЙПЛАЙН ЧИТАННЯ (Fetch Pipeline) та 2. СИНХРОНІЗАЦІЯ РЯДКІВ (Скролінг)
	// =====================================================================
    if (scanline >= -1 && scanline < 240) {

        // --- 1. ПАЙПЛАЙН ЧИТАННЯ (Fetch Pipeline) ---
        if ((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336)) {
            UpdateShifters();

            switch ((cycle - 1) % 8) {
            case 0:
                LoadBackgroundShifters();
                bg_next_tile_id = ppuRead(0x2000 | (v.reg & 0x0FFF));
                break;
            case 2:
                // Читання атрибутів палітри (з безпечним кастуванням типів)
                bg_next_tile_attrib = ppuRead(0x23C0
                    | (static_cast<uint16_t>(v.nametable_y) << 11)
                    | (static_cast<uint16_t>(v.nametable_x) << 10)
                    | ((static_cast<uint16_t>(v.coarse_y) >> 2) << 3)
                    | (static_cast<uint16_t>(v.coarse_x) >> 2));

                if (v.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
                if (v.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
                bg_next_tile_attrib &= 0x03;
                break;
            case 4:
                bg_next_tile_lsb = ppuRead(((control & 0x10) ? 0x1000 : 0x0000)
                    + (static_cast<uint16_t>(bg_next_tile_id) << 4)
                    + v.fine_y + 0);
                break;
            case 6:
                bg_next_tile_msb = ppuRead(((control & 0x10) ? 0x1000 : 0x0000)
                    + (static_cast<uint16_t>(bg_next_tile_id) << 4)
                    + v.fine_y + 8);
                break;
            case 7:
                IncrementScrollX(); // Зсув на наступний тайл
                break;
            }
        }

        // --- 2. СИНХРОНІЗАЦІЯ РЯДКІВ (Скролінг) ---
        if (cycle == 256) {
            IncrementScrollY(); // Кінець видимого рядка - спускаємось нижче
        }

        if (cycle == 257) {
            LoadBackgroundShifters();
            TransferAddressX(); // Повертаємо промінь у лівий край
        }

        // ФІКС ДЛЯ MMC3: Саме тут PPU офіційно закінчив видимий рядок. 
            // Якщо у нас є картридж (і це MMC3), він повинен перерахувати свій таймер.
            // Але сигнал передається ТІЛЬКИ якщо включений рендер фону або спрайтів!
        if (cart && (mask & 0x18) && cycle == 260) {
            cart->scanline();
        }

        if (scanline == -1 && cycle >= 280 && cycle < 305) {
            TransferAddressY(); // Підготовка до малювання нового кадру
        }
    }

    // =====================================================================
    // 3. ОЦІНКА СПРАЙТІВ (Виконується в кінці видимого рядка)
    // =====================================================================
    if (cycle == 257 && scanline >= 0 && scanline < 240) {
        // Очищаємо дані з попереднього рядка
        sprite_count = 0;
        for (int i = 0; i < 8; i++) {
            sprite_shifter_pattern_lo[i] = 0;
            sprite_shifter_pattern_hi[i] = 0;
        }

        uint8_t nOAMEntry = 0;
        bSpriteZeroHitPossible = false;

        while (nOAMEntry < 64 && sprite_count < 9) {
            // Визначаємо поточний розмір спрайтів: 8x8 або 8x16 (біт 5 регістра PPUCTRL)
            int spriteSize = (control & 0x20) ? 16 : 8;

            int16_t diff = static_cast<int16_t>(scanline) - static_cast<int16_t>(OAM[nOAMEntry].y);

            // Якщо спрайт перетинає наш рядок (перевіряємо динамічну висоту!)
            if (diff >= 0 && diff < spriteSize) {
                if (sprite_count < 8) {
                    if (nOAMEntry == 0) bSpriteZeroHitPossible = true; // Це Спрайт №0

                    spriteScanline[sprite_count] = OAM[nOAMEntry];

                    uint16_t sprite_pattern_addr_lo = 0;

                    if (!(control & 0x20)) {
                        // ==========================================
                        // РЕЖИМ 8x8 (Стандартний)
                        // ==========================================
                        if (!(OAM[nOAMEntry].attribute & 0x80)) {
                            // Звичайний (не перевернутий)
                            sprite_pattern_addr_lo = ((control & 0x08) ? 0x1000 : 0x0000)
                                | (static_cast<uint16_t>(OAM[nOAMEntry].id) << 4)
                                | static_cast<uint16_t>(diff);
                        }
                        else {
                            // Перевернутий по вертикалі
                            sprite_pattern_addr_lo = ((control & 0x08) ? 0x1000 : 0x0000)
                                | (static_cast<uint16_t>(OAM[nOAMEntry].id) << 4)
                                | static_cast<uint16_t>(7 - diff);
                        }
                    }
                    else {
                        // ==========================================
                        // РЕЖИМ 8x16 (ФІКС ДЛЯ ZELDA II ТА ВИСОКИХ ПЕРСОНАЖІВ)
                        // ==========================================
                        if (!(OAM[nOAMEntry].attribute & 0x80)) {
                            // Звичайний
                            if (diff < 8) {
                                // Малюємо верхню половину спрайта
                                sprite_pattern_addr_lo = ((OAM[nOAMEntry].id & 0x01) ? 0x1000 : 0x0000)
                                    | (static_cast<uint16_t>(OAM[nOAMEntry].id & 0xFE) << 4)
                                    | static_cast<uint16_t>(diff);
                            }
                            else {
                                // Малюємо нижню половину спрайта (наступний тайл)
                                sprite_pattern_addr_lo = ((OAM[nOAMEntry].id & 0x01) ? 0x1000 : 0x0000)
                                    | (static_cast<uint16_t>((OAM[nOAMEntry].id & 0xFE) + 1) << 4)
                                    | static_cast<uint16_t>(diff - 8);
                            }
                        }
                        else {
                            // Перевернутий по вертикалі (міняємо половини місцями)
                            if (diff < 8) {
                                // Верхня половина екрана (відображає нижній тайл)
                                sprite_pattern_addr_lo = ((OAM[nOAMEntry].id & 0x01) ? 0x1000 : 0x0000)
                                    | (static_cast<uint16_t>((OAM[nOAMEntry].id & 0xFE) + 1) << 4)
                                    | static_cast<uint16_t>(7 - diff);
                            }
                            else {
                                // Нижня половина екрана (відображає верхній тайл)
                                sprite_pattern_addr_lo = ((OAM[nOAMEntry].id & 0x01) ? 0x1000 : 0x0000)
                                    | (static_cast<uint16_t>(OAM[nOAMEntry].id & 0xFE) << 4)
                                    | static_cast<uint16_t>(15 - diff);
                            }
                        }
                    }

                    uint16_t sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;
                    sprite_shifter_pattern_lo[sprite_count] = ppuRead(sprite_pattern_addr_lo);
                    sprite_shifter_pattern_hi[sprite_count] = ppuRead(sprite_pattern_addr_hi);

                    // Якщо спрайт перевернутий по горизонталі (біт 6) - дзеркалимо байти
                    if (OAM[nOAMEntry].attribute & 0x40) {
                        auto flipbyte = [](uint8_t b) {
                            b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                            b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                            b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                            return b;
                            };
                        sprite_shifter_pattern_lo[sprite_count] = flipbyte(sprite_shifter_pattern_lo[sprite_count]);
                        sprite_shifter_pattern_hi[sprite_count] = flipbyte(sprite_shifter_pattern_hi[sprite_count]);
                    }
                }
                sprite_count++;
            }
            nOAMEntry++;
        }
        
        // Якщо більше 8 спрайтів на рядку - встановлюємо біт переповнення спрайтів
        if (sprite_count > 8) {
            sprite_count = 8; // Обмежуємо до 8 спрайтів, які можна відобразити
            status |= 0x20; // Встановлюємо біт Sprite Overflow у статусі
		}
    }

    // =====================================================================
    // 4. РЕНДЕРИНГ ПІКСЕЛЯ 
    // =====================================================================
    if (scanline >= 0 && scanline < 240 && cycle >= 1 && cycle <= 256) {
        uint8_t bg_pixel = 0x00;
        uint8_t bg_palette = 0x00;
        uint8_t fg_pixel = 0x00;
        uint8_t fg_palette = 0x00;
        uint8_t fg_priority = 0x00;

        if (mask & 0x08) {
            uint16_t bit_mux = 0x8000 >> fine_x;
            uint8_t p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
            uint8_t p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;
            bg_pixel = (p1_pixel << 1) | p0_pixel;

            uint8_t bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
            uint8_t bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
            bg_palette = (bg_pal1 << 1) | bg_pal0;
        }

        // --- Отримуємо піксель СПРАЙТУ ---
        bool bSpriteZeroBeingRendered = false;
        if (mask & 0x10) {
            for (uint8_t i = 0; i < sprite_count && i < 8; i++) {
                if (spriteScanline[i].x == 0) {
                    uint8_t pixel_lo = (sprite_shifter_pattern_lo[i] & 0x80) > 0;
                    uint8_t pixel_hi = (sprite_shifter_pattern_hi[i] & 0x80) > 0;
                    fg_pixel = (pixel_hi << 1) | pixel_lo;

                    fg_palette = (spriteScanline[i].attribute & 0x03) + 0x04; // Спрайти використовують палітри 4-7
                    fg_priority = (spriteScanline[i].attribute & 0x20) == 0;  // 0 = поверх фону

                    if (fg_pixel != 0) {
                        if (i == 0) bSpriteZeroBeingRendered = true;
                        break; // Малюємо лише верхній спрайт, інші перекриваються
                    }
                }
            }
        }

        // =================================================================
        // ФІКС ZELDA II: МАСКУВАННЯ КРАЇВ (Відсікаємо ліві 8 пікселів)
        // =================================================================
        if (cycle >= 1 && cycle < 9) {
            if (!(mask & 0x02)) {
                bg_pixel = 0x00; // Повністю гасимо фон
            }
            if (!(mask & 0x04)) {
                fg_pixel = 0x00; // Повністю гасимо спрайти
                bSpriteZeroBeingRendered = false; // Скасовуємо колізію Sprite 0!
            }
        }

        // --- МУЛЬТИПЛЕКСОР (Хто перемагає на екрані?) ---
        uint8_t final_pixel = 0x00;
        uint8_t final_palette = 0x00;

        if (bg_pixel == 0 && fg_pixel > 0) {
            final_pixel = fg_pixel;
            final_palette = fg_palette;
        }
        else if (bg_pixel > 0 && fg_pixel == 0) {
            final_pixel = bg_pixel;
            final_palette = bg_palette;
        }
        else if (bg_pixel > 0 && fg_pixel > 0) {
            if (fg_priority) {
                final_pixel = fg_pixel;
                final_palette = fg_palette;
            }
            else {
                final_pixel = bg_pixel;
                final_palette = bg_palette;
            }

            // Детекція Sprite 0 Hit (Тепер вона працює бездоганно!)
            if (bSpriteZeroHitPossible && bSpriteZeroBeingRendered) {
                if ((mask & 0x08) && (mask & 0x10)) {
                    // Перевірку cycle < 9 ми вже зробили вище! 
                    // Тому якщо ми дійшли сюди — це 100% легальна колізія
                    status |= 0x40; // Встановлюємо 6-й біт у $2002
                }
            }
        }

        // --- Вивід фінального кольору ---
        uint8_t color_address = 0x00;
        if (final_pixel != 0x00) {
            color_address = (final_palette << 2) | final_pixel;
        }
        uint8_t color_index = ppuRead(0x3F00 + color_address) & 0x3F;

        // БЕЗПЕЧНИЙ ЗАПИС У МАСИВ!
        size_t pixel_index = (static_cast<size_t>(scanline) * 256) + static_cast<size_t>(cycle - 1);
        sprScreen[pixel_index] = palScreen[color_index];

        // --- Зсув таймерів Х для спрайтів ---
        if (mask & 0x10) {
            for (uint8_t i = 0; i < sprite_count && i < 8; i++) {
                if (spriteScanline[i].x > 0) {
                    spriteScanline[i].x--;
                }
                else {
                    sprite_shifter_pattern_lo[i] <<= 1;
                    sprite_shifter_pattern_hi[i] <<= 1;
                }
            }
        }
    }

    // --- ГЕНЕРАЦІЯ СИГНАЛУ NMI НА ПОЧАТКУ VBLANK ---
    if (scanline == 241 && cycle == 1) {
        status |= 0x80; // Встановлюємо прапорець VBlank (біт 7 у $2002)

        if (control & 0x80) { // Якщо NMI дозволено в PPUCTRL
            nmi_occurred = true; // Викидаємо прапорець переривання на шину
			nmiFireCount++; // Збільшуємо лічильник викликів NMI, для трейсування та налагодження
        }
    }

    // Емуляція ходу променя екрану
    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 261) {
            scanline = -1;
            frame_complete = true;
        }
    }

} // Кінець функції clock()

// =========================================================================
// ДОПОМІЖНІ ФУНКЦІЇ ПАЙПЛАЙНУ ТА СКРОЛІНГУ
// =========================================================================
void PPU::LoadBackgroundShifters() {
    bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
    bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;
    bg_shifter_attrib_lo = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
    bg_shifter_attrib_hi = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
}

void PPU::UpdateShifters() {
    if (mask & 0x08) { // Якщо рендеринг фону увімкнено
        bg_shifter_pattern_lo <<= 1;
        bg_shifter_pattern_hi <<= 1;
        bg_shifter_attrib_lo <<= 1;
        bg_shifter_attrib_hi <<= 1;
    }
}

void PPU::IncrementScrollX() {
    if (mask & 0x18) { // Якщо рендеринг фону або спрайтів увімкнено
        if (v.coarse_x == 31) {
            v.coarse_x = 0;
            v.nametable_x = ~v.nametable_x;
        }
        else {
            v.coarse_x++;
        }
    }
}

void PPU::IncrementScrollY() {
    if (mask & 0x18) {
        if (v.fine_y < 7) {
            v.fine_y++;
        }
        else {
            v.fine_y = 0;
            if (v.coarse_y == 29) {
                v.coarse_y = 0;
                v.nametable_y = ~v.nametable_y;
            }
            else if (v.coarse_y == 31) {
                v.coarse_y = 0;
            }
            else {
                v.coarse_y++;
            }
        }
    }
}

void PPU::TransferAddressX() {
    if (mask & 0x18) {
        v.nametable_x = t.nametable_x;
        v.coarse_x = t.coarse_x;
    }
}

void PPU::TransferAddressY() {
    if (mask & 0x18) {
        v.fine_y = t.fine_y;
        v.nametable_y = t.nametable_y;
        v.coarse_y = t.coarse_y;
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
            nmi_occurred = false; // ФІКС: Миттєво скасовуємо будь-яке заплановане NMI!
        }
        break;
    case 0x0003: // $2003 - OAMADDR
        break;
    case 0x0004: // $2004 - OAMDATA
        data = reinterpret_cast<uint8_t*>(OAM.data())[oam_addr];
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
    {
        bool nmi_was_enabled = (control & 0x80) != 0;

        control = data;
        t.nametable_x = control & 0x01;
        t.nametable_y = (control & 0x02) >> 1;

        bool nmi_now_enabled = (control & 0x80) != 0;

        // ФІКС: додатковий NMI генеруємо ТІЛЬКИ на фронті вмикання (0 -> 1),
        // а не щоразу, коли NMI і так вже був увімкнений і vblank ще триває.
        // Інакше гра, що щокадру перезаписує PPUCTRL з уже активним NMI,
        // зациклить переривання нескінченно і CPU ніколи не поверне керування.
        if (!nmi_was_enabled && nmi_now_enabled && (status & 0x80)) {
            nmi_occurred = true;
        }
        else if (!nmi_now_enabled) {
            nmi_occurred = false; // ФІКС: Миттєво скасовуємо NMI, якщо гра його вимкнула!
        }
        break;
    }
    case 0x0001: // $2001 - PPUMASK
        mask = data;
        break;
    case 0x0002: // $2002 - PPUSTATUS (Тільки для читання)
        break;
    case 0x0003: // $2003 - OAMADDR
        oam_addr = data;
        break;
    case 0x0004: // $2004 - OAMDATA
        reinterpret_cast<uint8_t*>(OAM.data())[oam_addr] = data;
        oam_addr++;
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

        // Захист: якщо картридж не підключено, використовуємо Вертикальне за замовчуванням
        Cartridge::MIRROR current_mirror = cart ? cart->mirror : Cartridge::MIRROR::VERTICAL;

        if (current_mirror == Cartridge::MIRROR::VERTICAL) {
            // Вертикальне віддзеркалення (A B A B)
            if (addr >= 0x0000 && addr <= 0x03FF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF)
                data = tblName[1][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                data = tblName[1][addr & 0x03FF];
        }
        else if (current_mirror == Cartridge::MIRROR::HORIZONTAL) {
            // Горизонтальне віддзеркалення (A A B B)
            if (addr >= 0x0000 && addr <= 0x03FF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                data = tblName[1][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                data = tblName[1][addr & 0x03FF];
        }
        else if (current_mirror == Cartridge::MIRROR::ONESCREEN_LO) {
            data = tblName[0][addr & 0x03FF];   // або = data у ppuWrite
        }
        else if (current_mirror == Cartridge::MIRROR::ONESCREEN_HI) {
            data = tblName[1][addr & 0x03FF];
        }
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

        Cartridge::MIRROR current_mirror = cart ? cart->mirror : Cartridge::MIRROR::VERTICAL;

        if (current_mirror == Cartridge::MIRROR::VERTICAL) {
            if (addr >= 0x0000 && addr <= 0x03FF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF)
                tblName[1][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                tblName[1][addr & 0x03FF] = data;
        }
        else if (current_mirror == Cartridge::MIRROR::HORIZONTAL) {
            if (addr >= 0x0000 && addr <= 0x03FF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                tblName[1][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                tblName[1][addr & 0x03FF] = data;
        }
        else if (current_mirror == Cartridge::MIRROR::ONESCREEN_LO) {
            tblName[0][addr & 0x03FF] = data;
        }
        else if (current_mirror == Cartridge::MIRROR::ONESCREEN_HI) {
            tblName[1][addr & 0x03FF] = data;
        }

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