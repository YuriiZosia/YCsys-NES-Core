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
 * |      [001] YCsys NES CORE - MAPPER 001 (MMC1) IMPLEMENTATION [001]      |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [MMC1]        [SRG ]        [VBNK]        [CTRL]        [MMC1]
 */

#include "Mapper_001.h"

Mapper_001::Mapper_001(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
    reset();
}

Mapper_001::~Mapper_001() {}

void Mapper_001::reset() {
    nControlRegister = 0x1C;
    nLoadRegister = 0x00;
    nLoadRegisterCount = 0x00;

    nCHRBankSelect4Lo = 0;
    nCHRBankSelect4Hi = 0;
    nCHRBankSelect8 = 0;

    nPRGBankSelect16Lo = 0;
    nPRGBankSelect16Hi = nPRGBanks - 1;
    nPRGBankSelect32 = 0;
}

bool Mapper_001::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        if (nControlRegister & 0x08) {
            // 16KB PRG Mode
            if (addr >= 0x8000 && addr <= 0xBFFF) {
                mapped_addr = (nPRGBankSelect16Lo * 0x4000) + (addr & 0x3FFF);
            }
            if (addr >= 0xC000 && addr <= 0xFFFF) {
                mapped_addr = (nPRGBankSelect16Hi * 0x4000) + (addr & 0x3FFF);
            }
        }
        else {
            // 32KB PRG Mode
            mapped_addr = (nPRGBankSelect32 * 0x8000) + (addr & 0x7FFF);
        }
        return true;
    }
    return false;
}

bool Mapper_001::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Якщо встановлено старший біт - скидання регістра
        if (data & 0x80) {
            nLoadRegister = 0x00;
            nLoadRegisterCount = 0;
            nControlRegister = nControlRegister | 0x0C;
        }
        else {
            // Заштовхуємо біт у зсувний регістр
            nLoadRegister >>= 1;
            nLoadRegister |= (data & 0x01) << 4;
            nLoadRegisterCount++;

            // Коли накопичили 5 біт - виконуємо команду
            if (nLoadRegisterCount == 5) {
                uint8_t targetRegister = (addr >> 13) & 0x03;

                if (targetRegister == 0) { // $8000 - $9FFF (Control)
                    nControlRegister = nLoadRegister & 0x1F;
                }
                else if (targetRegister == 1) { // $A000 - $BFFF (CHR Bank 0)
                    if (nControlRegister & 0x10) {
                        nCHRBankSelect4Lo = nLoadRegister & 0x1F;
                    }
                    else {
                        nCHRBankSelect8 = (nLoadRegister & 0x1E) >> 1;
                    }
                }
                else if (targetRegister == 2) { // $C000 - $DFFF (CHR Bank 1)
                    if (nControlRegister & 0x10) {
                        nCHRBankSelect4Hi = nLoadRegister & 0x1F;
                    }
                }
                else if (targetRegister == 3) { // $E000 - $FFFF (PRG Bank)
                    uint8_t prgMode = (nControlRegister >> 2) & 0x03;
                    if (prgMode == 0 || prgMode == 1) {
                        nPRGBankSelect32 = (nLoadRegister & 0x0E) >> 1;
                    }
                    else if (prgMode == 2) {
                        nPRGBankSelect16Lo = 0;
                        nPRGBankSelect16Hi = nLoadRegister & 0x0F;
                    }
                    else if (prgMode == 3) {
                        nPRGBankSelect16Lo = nLoadRegister & 0x0F;
                        nPRGBankSelect16Hi = nPRGBanks - 1;
                    }
                }

                // Очищуємо регістр для наступної команди
                nLoadRegister = 0x00;
                nLoadRegisterCount = 0;
            }
        }
    }
    return false; // Повертаємо false, щоб картридж сам не писав у ROM
}

bool Mapper_001::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr < 0x2000) {
        if (nCHRBanks == 0) { // CHR RAM (наприклад, Zelda)
            mapped_addr = addr;
            return true;
        }
        else {
            if (nControlRegister & 0x10) {
                // 4KB CHR Mode
                if (addr >= 0x0000 && addr <= 0x0FFF) {
                    mapped_addr = (nCHRBankSelect4Lo * 0x1000) + (addr & 0x0FFF);
                }
                if (addr >= 0x1000 && addr <= 0x1FFF) {
                    mapped_addr = (nCHRBankSelect4Hi * 0x1000) + (addr & 0x0FFF);
                }
            }
            else {
                // 8KB CHR Mode
                mapped_addr = (nCHRBankSelect8 * 0x2000) + (addr & 0x1FFF);
            }
            return true;
        }
    }
    return false;
}

bool Mapper_001::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    if (addr < 0x2000) {
        if (nCHRBanks == 0) { // Дозволяємо запис тільки якщо це CHR RAM
            mapped_addr = addr;
            return true;
        }
    }
    return false;
}