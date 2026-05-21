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
 * |      [004] YCsys NES CORE - MAPPER 004 (MMC3) IMPLEMENTATION [004]      |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [MMC3]        [IRQ ]        [SCAN]        [BANK]        [MMC3]
 */

#pragma once
#include "Mapper.h"

class Mapper_004 : public Mapper {
public:
    Mapper_004(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper_004();

    bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) override;

    void reset() override;

    // Взаємодія з перериваннями (IRQ)
    bool irqState() override;
    void irqClear() override;
    void scanline() override;

	// Динамічне дзеркало для MMC3
    bool mirrorMode(uint8_t& mode) override;

private:
    uint8_t nTargetRegister = 0x00;
    bool bPRGBankMode = false;
    bool bCHRInversion = false;

    uint32_t pRegister[8] = { 0 };
    uint32_t pCHRBank[8] = { 0 };
    uint32_t pPRGBank[4] = { 0 };

    bool bIRQActive = false;
    bool bIRQEnable = false;
    bool bIRQUpdate = false;
    uint16_t nIRQCounter = 0x0000;
    uint16_t nIRQLatch = 0x0000;

    uint8_t nMirrorMode = 0; // 0 = Vertical, 1 = Horizontal
};