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
 * |      [001] YCsys NES CORE - MAPPER 001 (MMC1) IMPLEMENTATION [001]      |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [MMC1]        [SRG ]        [VBNK]        [CTRL]        [MMC1]
 */

#pragma once
#include "Mapper.h"
#include <vector>

class Mapper_001 : public Mapper {
public:
    Mapper_001(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper_001();

    bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) override;

    bool mirrorMode(uint8_t& mode) override;

    void reset() override;

private:
    // Внутрішні регістри MMC1
    uint8_t nControlRegister = 0x1C;
    uint8_t nLoadRegister = 0x00;
    uint8_t nLoadRegisterCount = 0x00;

    // Регістри банків
    uint8_t nCHRBankSelect4Lo = 0;
    uint8_t nCHRBankSelect4Hi = 0;
    uint8_t nCHRBankSelect8 = 0;

    uint8_t nPRGBankSelect16Lo = 0;
    uint8_t nPRGBankSelect16Hi = 0;
    uint8_t nPRGBankSelect32 = 0;
};
