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

	void clock(); // Головний такт PPU (генерація пікселів, скролінг, VBlank)

    // Підключення картриджа (використовуємо такий самий shared_ptr, як у Bus)
    void ConnectCartridge(const std::shared_ptr<Cartridge>& cartridge) { cart = cartridge; }

    // --- Зв'язок з головною шиною (CPU Bus) ---
    uint8_t cpuRead(uint16_t addr, bool rdonly = false);
    void cpuWrite(uint16_t addr, uint8_t data);

    // --- Власна шина PPU (PPU Bus) ---
    uint8_t ppuRead(uint16_t addr, bool rdonly = false);
    void ppuWrite(uint16_t addr, uint8_t data);

public: // --- Графічний вивід та синхронізація кадрів ---
    // Буфер екрану (256x240). Використовуємо безпечний std::array із зануленням
    std::array<uint32_t, 256 * 240> sprScreen{};
    bool frame_complete = false; // Сигнал для шини, що кадр повністю відмальовано

private:
    std::shared_ptr<Cartridge> cart; // Розумний вказівник на картридж

    // VRAM (Nametables) - 2 таблиці по 1024 байти (для фону)
    std::array<std::array<uint8_t, 1024>, 2> tblName{};

    // Палітри (Palettes) - 32 байти
    std::array<uint8_t, 32> tblPalette{};

private: 
    // --- Регістри стану PPU (Нові змінні) ---
    uint8_t control = 0x00;         // $2000 - PPUCTRL (Налаштування генерації NMI, розміру спрайтів тощо)
    uint8_t mask = 0x00;            // $2001 - PPUMASK (Керування рендером фону, спрайтів, кольоровими ефектами)
    uint8_t status = 0x00;          // $2002 - PPUSTATUS (Прапорці VBlank, Спрайт 0, Переповнення спрайтів)
    uint8_t address_latch = 0;      // Адресний тригер: 0 = чекаємо старший байт, 1 = чекаємо молодший байт
    uint8_t ppu_data_buffer = 0x00; // Внутрішній буфер затримки читання для регістра PPUDATA ($2007)

    // Структура регістра Loopy (описує внутрішню 15-бітну адресу VRAM)
    union loopy_register {
        struct {
            uint16_t coarse_x : 5;      // Грубе зміщення по X (по тайлах, 32 колонки)
            uint16_t coarse_y : 5;      // Грубе зміщення по Y (по тайлах, 30 рядків)
            uint16_t nametable_x : 1;   // Вибір горизонтальної таблиці імен
            uint16_t nametable_y : 1;   // Вибір вертикальної таблиці імен
            uint16_t fine_y : 3;        // Точне зміщення по Y (всередині тайла, 0-7 пікселів)
            uint16_t unused : 1;
        };
        uint16_t reg = 0x0000;          // Вся 15-бітна адреса цілком
    };

    loopy_register v{};      // Active VRAM Address (поточна адреса розгортки)
    loopy_register t{};      // Temporary VRAM Address (адреса, яку готує CPU)
    uint8_t fine_x = 0x00;   // Точне зміщення по горизонталі (від 0 до 7 пікселів)

private: // --- Координати віртуального променя розгортки екрану ---
    int16_t scanline = 0; // Поточний рядок (від -1 до 260)
    int16_t cycle = 0;    // Поточний такт/піксель у рядку (від 0 до 340)
};