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
 * |      [A]  YCsys NES CORE - AUDIO PROCESSING UNIT (APU)     [A]          |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [SND]        [PUL]        [TRI]        [NSI]        [DAC]        [SND]
 */

#include "APU.h"

APU::APU() {}
APU::~APU() {}

void APU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
        // --- Pulse 1 ---
    case 0x4000: pulse1.duty = (data & 0xC0) >> 6; pulse1.volume = data & 0x0F; break;
    case 0x4001: break; // Sweep (ігноруємо)
    case 0x4002: pulse1.timer = (pulse1.timer & 0xFF00) | data; break;
    case 0x4003: pulse1.timer = (pulse1.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8); pulse1.duty_step = 0; break;

        // --- Pulse 2 ---
    case 0x4004: pulse2.duty = (data & 0xC0) >> 6; pulse2.volume = data & 0x0F; break;
    case 0x4005: break; // Sweep (ігноруємо)
    case 0x4006: pulse2.timer = (pulse2.timer & 0xFF00) | data; break;
    case 0x4007: pulse2.timer = (pulse2.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8); pulse2.duty_step = 0; break;

        // --- Triangle ---
    case 0x4008: break; // Лінійний лічильник (ігноруємо для базового звуку)
    case 0x4009: break; // Невживаний порт
    case 0x400A: // Нижні 8 бітів таймера Triangle
        triangle.timer = (triangle.timer & 0xFF00) | data;
        break;
    case 0x400B: // Верхні 3 біти таймера Triangle (безпечний каст)
        triangle.timer = (triangle.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
        break;

        // --- Noise ---
    case 0x400C: // Гучність каналу шуму
        noise.volume = data & 0x0F;
        break;
    case 0x400D: break; // Невживаний порт
    case 0x400E: { // Вибір частоти (періоду) шуму з таблиці NTSC
        // ФІКС: Правильний розмір масиву періодів шуму (16 елементів)
        static const uint16_t noise_periods[16] = {
            4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
        };
        noise.timer = noise_periods[data & 0x0F];
        break;
    }
    case 0x400F: break; // Довжина звуку (ігноруємо)

        // --- Керування статусом APU ---
    case 0x4015:
        pulse1.enabled = (data & 0x01) > 0;
        pulse2.enabled = (data & 0x02) > 0;
        triangle.enabled = (data & 0x04) > 0; // Вмикаємо/вимикаємо басовий канал
        noise.enabled = (data & 0x08) > 0; // Вмикаємо/вимикаємо канал шуму
        break;
    }
}

uint8_t APU::cpuRead(uint16_t addr) {
    (void)addr;
    uint8_t data = 0x00;
    // Логіка читання статус-регістра APU ($4015) буде тут
    return data;
}

void APU::clock() {
    // Канал Triangle працює апаратно вдвічі швидше за Pulse і тактується КОЖЕН крок APU
    triangle_sample = triangle.clock();

    // Генератори Pulse в NES тактуються кожен 2-й системний крок процесора
    if (clock_counter % 2 == 0) {
        pulse1_sample = pulse1.clock();
        pulse2_sample = pulse2.clock();
        noise_sample = noise.clock();
    }
    clock_counter++;
}

double APU::GetOutputSample() const {
    // Змішуємо 4 канали. Максимальне значення: 15 * 4 = 60.
    double mixed = static_cast<double>(pulse1_sample + pulse2_sample + triangle_sample + noise_sample) / 60.0;

    // Якщо всі канали мовчать (тиша), повертаємо ідеальний 0.0, щоб динаміки не гуділи.
    if (mixed == 0.0) {
        return 0.0;
    }

    // Відцентровуємо сигнал: діапазон [0.0 ... 1.0] перетворюється на [-0.5 ... +0.5]
    return mixed - 0.5;
}