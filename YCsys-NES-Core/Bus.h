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
 * |      [#]  YCsys NES CORE - SYSTEM BUS & RAM IMPLEMENTATION  [#]         |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [|||]        [|||]        [BUS]        [|||]        [|||]
 */

#pragma once
#include <cstdint>
#include <array>
#include <memory>
#include "CPU.h"
#include "Cartridge.h"
#include "PPU.h"
#include "APU.h"

class Bus {
public:
    Bus();
    ~Bus();

    void clock(); // Головний системний такт

public: // Пристрої на шині
    CPU6502 cpu;
    APU apu;
    std::array<uint8_t, 2048> cpuRam;
    PPU ppu;

    // Стан контролерів (Joypad 1 та Joypad 2)
    std::array<uint8_t, 2> controller{};       // Поточний фізичний стан кнопок
    std::array<uint8_t, 2> controller_state{}; // Зсувний регістр (заморожений стан)

    // Вказівник на підключений картридж
    std::shared_ptr<Cartridge> cart;

public: // Управління системою
    // Метод для вставлення картриджа
    void insertCartridge(const std::shared_ptr<Cartridge>& cartridge);
    
	// Фікс: Потрібен прапорець для стробування геймпада, щоб контролювати, коли ми зчитуємо стан кнопок. Це дозволяє нам правильно обробляти натискання та відпускання кнопок, а також забезпечує сумісність з іграми, які використовують стробування для читання контролерів.
    bool bStrobe = false;         // Прапорець для стробування геймпада
    uint16_t dma_cycles = 0;      // Лічильник заморозки процесора для OAM DMA

public: // Читання та запис
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr, bool bReadOnly = false);

private:
    uint32_t nSystemClockCounter = 0; // Лічильник тактів
};