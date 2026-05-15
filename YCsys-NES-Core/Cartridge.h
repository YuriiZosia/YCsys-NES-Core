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
 * |      [M]  YCsys NES CORE - CARTRIDGE & MAPPER IMPLEMENTATION [M]        |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [ROM]        [PRG]        [CHR]        [MAP]        [ROM]
 */

#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

class Cartridge {
public:
    Cartridge(const std::string& sFileName);
    ~Cartridge();

    // Правильна структура 16-байтного заголовка iNES
    struct sHeader {
        char name[4];           // 'N', 'E', 'S', '\x1A'
        uint8_t prg_rom_chunks;
        uint8_t chr_rom_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        char unused[5];
    } header;

    bool bImageValid = false; // Прапорець, щоб знати, чи успішно завантажено гру

    // Вектори для зберігання пам'яті
    std::vector<uint8_t> vPRGMemory;
    std::vector<uint8_t> vCHRMemory;

    uint8_t nPRGBanks = 0;
    uint8_t nCHRBanks = 0;
    uint8_t nMapperID = 0;
};