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

#pragma once
#include "Mapper.h"

class Mapper_002 : public Mapper {
public:
    Mapper_002(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper_002();

    bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) override;

    void reset() override;

private:
    uint8_t nPRGBankSelect_Lo = 0; // Номер банку для вікна $8000-$BFFF
    uint8_t nPRGBankSelect_Hi = 0; // Жорстко зафіксований останній банк для $C000-$FFFF
};