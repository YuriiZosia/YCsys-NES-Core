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

#pragma once
#include <cstdint>
#include <array>
#include <memory>
#include "Cartridge.h"

class PPU {
public:
    PPU();
    ~PPU();

    // Підключення картриджа (використовуємо такий самий shared_ptr, як у Bus)
    void ConnectCartridge(const std::shared_ptr<Cartridge>& cartridge) { cart = cartridge; }

    // --- Зв'язок з головною шиною (CPU Bus) ---
    uint8_t cpuRead(uint16_t addr, bool rdonly = false);
    void cpuWrite(uint16_t addr, uint8_t data);

    // --- Власна шина PPU (PPU Bus) ---
    uint8_t ppuRead(uint16_t addr, bool rdonly = false);
    void ppuWrite(uint16_t addr, uint8_t data);

private:
    std::shared_ptr<Cartridge> cart; // Розумний вказівник на картридж

    // VRAM (Nametables) - 2 таблиці по 1024 байти (для фону)
    std::array<std::array<uint8_t, 1024>, 2> tblName;

    // Палітри (Palettes) - 32 байти
    std::array<uint8_t, 32> tblPalette;
};
