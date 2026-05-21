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
 * |      [X]  YCsys NES CORE - VIRTUAL MAPPER INTERFACE (BASE)  [X]         |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [ADDR]        [BUS ]        [MAP ]        [PPU ]        [CPU ]
 */

#pragma once
#include <cstdint>

class Mapper {
public:
    Mapper(uint8_t prgBanks, uint8_t chrBanks) {
        nPRGBanks = prgBanks;
        nCHRBanks = chrBanks;
    }
    virtual ~Mapper() = default;

    // Віртуальні функції, які обов'язково мають бути в усіх маперах
    virtual bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr) = 0;
    // ФІКС: Додав uint8_t data, щоб мапери могли читати команди процесора
    virtual bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data = 0) = 0;
    virtual bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) = 0;
    virtual bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) = 0;
    // ФІКС: Віртуальний метод reset(), щоб можна було скидати стан будь-якого мапера
    virtual void reset() = 0;

protected:
    uint8_t nPRGBanks = 0;
    uint8_t nCHRBanks = 0;
};