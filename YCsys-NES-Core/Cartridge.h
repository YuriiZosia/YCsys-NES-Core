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
#include <memory>
#include "Mapper_000.h"
#include "Mapper_001.h"
#include "Mapper_002.h"
#include "Mapper_003.h"
#include "Mapper_004.h"

class Cartridge {
public:
    Cartridge(const std::string& sFileName);
    ~Cartridge();

	void reset(); // Скидання стану картриджа (наприклад, при натисканні Reset на консолі)

    // Зв'язок з головною шиною (Bus)
    bool cpuRead(uint16_t addr, uint8_t& data);
    bool cpuWrite(uint16_t addr, uint8_t data);

    // Зв'язок з відеошиною (PPU)
    bool ppuRead(uint16_t addr, uint8_t& data);
    bool ppuWrite(uint16_t addr, uint8_t data);

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

    bool bImageValid = false;

    // Режими віддзеркалення (використовуємо безпечний enum class)
    enum class MIRROR {
        HORIZONTAL,
        VERTICAL,
        ONESCREEN_LO,
        ONESCREEN_HI
    };

    MIRROR mirror = MIRROR::HORIZONTAL;

private:
    std::vector<uint8_t> vPRGMemory;
    std::vector<uint8_t> vCHRMemory;

    uint8_t nPRGBanks = 0;
    uint8_t nCHRBanks = 0;
    uint8_t nMapperID = 0;

    // Вказівник на наш мапер
    std::shared_ptr<Mapper> pMapper;
};