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

#pragma once
#include <cstdint>

class APU {
public:
    APU();
    ~APU();

    // Зв'язок з CPU шиною
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);

    // Системний годинник APU
    void clock();

    // Отримання поточного змішаного звукового сигналу (для мікшера)
    double GetOutputSample() const;

private:
    double global_time = 0.0; // Тимчасовий акумулятор часу для хвильових функцій
    uint32_t clock_counter = 0; // Системний лічильник тактів APU

    // Структура генератора прямокутної хвилі (Pulse)
    struct Pulse {
        uint8_t duty = 0;       // Робочий цикл (0-3)
        uint16_t timer = 0;     // 11-бітний таймер (частота)
        uint8_t volume = 0;     // Гучність (0-15)
        bool enabled = false;   // Чи увімкнений канал

        // Внутрішні лічильники
        uint16_t timer_value = 0;
        uint8_t duty_step = 0;

        uint8_t clock() {
            // Звук генерується лише якщо канал увімкнено і частота не надто висока
            if (!enabled || timer < 8) return 0;

            timer_value--;
            if (timer_value == 0xFFFF) { // Переповнення таймера (відлік вниз)
                timer_value = timer;
                duty_step = (duty_step + 1) & 0x07; // Зсуваємо секвенсор (0..7)
            }

            // ФІКС: Правильний розмір масиву 4x8
            static const uint8_t duty_table[4][8] = {
                {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
                {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
                {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
                {1, 0, 0, 1, 1, 1, 1, 1}  // 75%
            };

            // Якщо поточний крок містить 1 — видаємо гучність, інакше 0
            return duty_table[duty][duty_step] ? volume : 0;
        }
    };

    // --- Канал Triangle (Басова лінія) ---
    struct Triangle {
        uint16_t timer = 0;
        uint16_t timer_value = 0;
        uint8_t sequence_step = 0;
        bool enabled = false;

        uint8_t clock() {
            // Генерація відбувається лише якщо канал увімкнено, а таймер не надто малий
            if (!enabled || timer < 2) return 0;

            timer_value--;
            if (timer_value == 0xFFFF) { // Переповнення (відлік вниз завершено)
                timer_value = timer;
                sequence_step = (sequence_step + 1) & 0x1F; // Секвенсор на 32 кроки (0..31)
            }

            // ФІКС: Правильний розмір масиву послідовності трикутної хвилі (32 елементи)
            static const uint8_t sequence[32] = {
                15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
            };

            return sequence[sequence_step];
        }
    };

    // --- Канал Noise (Псевдовипадковий шум) ---
    struct Noise {
        uint16_t timer = 0;
        uint16_t timer_value = 0;
        uint16_t lfsr = 1; // Регістр зсуву на старті обов'язково має бути 1 (не 0!)
        uint8_t volume = 0;
        bool enabled = false;

        uint8_t clock() {
            if (!enabled || timer == 0) return 0;

            timer_value--;
            if (timer_value == 0xFFFF) {
                timer_value = timer;
                // Формула LFSR лінійного зворотного зв'язку (Mode 0)
                uint16_t feedback = (lfsr & 0x0001) ^ ((lfsr & 0x0002) >> 1);
                lfsr >>= 1;
                lfsr |= (feedback << 14);
            }

            // Якщо 0-й біт дорівнює 1 — канал видає тишу, якщо 0 — видає гучність
            return (lfsr & 0x0001) ? 0 : volume;
        }
    };

	// Об'єкти каналів звуку
	Pulse pulse1; // Об'єкт першого каналу прямокутної хвилі
    Pulse pulse2; // Об'єкт другого каналу прямокутної хвилі
    Triangle triangle; // Об'єкт басового каналу
    Noise noise;       // Об'єкт каналу шуму

    uint8_t pulse1_sample = 0;
    uint8_t pulse2_sample = 0;
    uint8_t triangle_sample = 0;
    uint8_t noise_sample = 0;
};