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

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void clock();
    double GetOutputSample() const;

private:
    uint32_t clock_counter = 0; // Системний лічильник тактів APU

    // ==========================================
    // СЕКВЕНСОР КАДРІВ ТА ТАБЛИЦІ
    // ==========================================
    uint32_t frame_clock_counter = 0;
    uint8_t frame_mode = 0; // Режим секвенсора: 0 = 4-кроковий, 1 = 5-кроковий

    // ФІКС: Правильний розмір апаратної таблиці довжин нот NES (32 значення)
    static constexpr uint8_t length_table[32] = {
        10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
        12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
    };

    // ==========================================
    // ДОПОМІЖНІ АПАРАТНІ БЛОКИ
    // ==========================================

    // Блок Апаратної Огинаючої (Envelope) - Керує плавним загасанням звуку
    struct Envelope {
        bool start = false;
        bool disable = false;
        uint16_t divider_count = 0;
        uint8_t volume = 0;
        uint8_t output = 0;
        uint8_t decay_count = 0;

        void clock(bool loop) {
            if (!start) {
                if (divider_count == 0) {
                    divider_count = volume;
                    if (decay_count == 0) {
                        if (loop) decay_count = 15;
                    }
                    else {
                        decay_count--;
                    }
                }
                else {
                    divider_count--;
                }
            }
            else {
                start = false;
                decay_count = 15;
                divider_count = volume;
            }
            output = disable ? volume : decay_count;
        }
    };

    // Блок зсуву частоти (Sweep) - Створює ефект ковзання (свисту)
    struct Sweep {
        bool enabled = false;
        bool down = false;
        bool reload = false;
        uint8_t shift = 0;
        uint8_t timer = 0;
        uint8_t period = 0;
        uint16_t change = 0;

        // ФІКС: Динамічне глушення. Канал мусить замовкати, якщо частота вийшла за межі!
        bool is_muted(uint16_t target_timer) const {
            if (target_timer < 8) return true;
            uint16_t offset = target_timer >> shift;
            if (!down && (target_timer + offset > 0x07FF)) return true;
            return false;
        }

        void clock(uint16_t& target_timer, bool channel_1) {
            if (timer == 0 && enabled && shift > 0 && !is_muted(target_timer)) {
                uint16_t offset = target_timer >> shift;
                if (down) {
                    if (channel_1) offset++; // Pulse 1 має зміщення на -1
                    target_timer -= offset;
                }
                else {
                    target_timer += offset;
                }
            }
            if (timer == 0 || reload) {
                timer = period;
                reload = false;
            }
            else {
                timer--;
            }
        }
    };

    // ==========================================
    // ЗВУКОВІ КАНАЛИ
    // ==========================================

    struct Pulse {
        uint8_t duty = 0;
        uint16_t timer = 0;
        bool enabled = false;
        uint8_t length_counter = 0;
        bool halt = false; // Використовується також як Loop для Envelope

        uint16_t timer_value = 0;
        uint8_t duty_step = 0;

        Envelope env;
        Sweep sweep;

        uint8_t clock() {
            // Звук глушиться, якщо вимкнено, таймер < 8, довжина вичерпана або Sweep заблокував частоту
            if (!enabled || timer < 8 || length_counter == 0 || sweep.is_muted(timer)) return 0;

            timer_value--;
            if (timer_value == 0xFFFF) {
                timer_value = timer;
                duty_step = (duty_step + 1) & 0x07;
            }

            static const uint8_t duty_table[4][8] = {
                {0, 1, 0, 0, 0, 0, 0, 0},
                {0, 1, 1, 0, 0, 0, 0, 0},
                {0, 1, 1, 1, 1, 0, 0, 0},
                {1, 0, 0, 1, 1, 1, 1, 1}
            };

            return duty_table[duty][duty_step] ? env.output : 0;
        }
    };

    struct Triangle {
        uint16_t timer = 0;
        uint16_t timer_value = 0;
        uint8_t sequence_step = 0;
        bool enabled = false;

        uint8_t length_counter = 0;
        bool halt = false;

        // ФІКС: Лінійний лічильник (без нього трикутник не замовкає правильно)
        uint8_t linear_counter = 0;
        uint8_t linear_reload = 0;
        bool linear_reload_flag = false;

        void clock_linear() {
            if (linear_reload_flag) {
                linear_counter = linear_reload;
            }
            else if (linear_counter > 0) {
                linear_counter--;
            }
            if (!halt) {
                linear_reload_flag = false;
            }
        }

        uint8_t clock() {
            // Замовкає, якщо хоч один з лічильників дійшов до нуля
            if (!enabled || timer < 2 || length_counter == 0 || linear_counter == 0) return 0;

            timer_value--;
            if (timer_value == 0xFFFF) {
                timer_value = timer;
                sequence_step = (sequence_step + 1) & 0x1F;
            }

            static const uint8_t sequence[32] = {
                15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
            };
            return sequence[sequence_step];
        }
    };

    struct Noise {
        uint16_t timer = 0;
        uint16_t timer_value = 0;
        uint16_t lfsr = 1;
        bool enabled = false;
        uint8_t length_counter = 0;
        bool halt = false;
        bool mode = false;

        Envelope env;

        uint8_t clock() {
            if (!enabled || timer == 0 || length_counter == 0) return 0;

            timer_value--;
            if (timer_value == 0xFFFF) {
                timer_value = timer;
                // ФІКС 4A: Враховуємо режим! Mode 1 (Біт 7) - це металевий дзенькіт
                uint16_t feedback = (lfsr & 0x0001) ^ ((mode ? (lfsr & 0x0040) >> 6 : (lfsr & 0x0002) >> 1));
                lfsr >>= 1;
                lfsr |= (feedback << 14);
            }
            return (lfsr & 0x0001) ? 0 : env.output;
        }
    };

    Pulse pulse1;
    Pulse pulse2;
    Triangle triangle;
    Noise noise;

    uint8_t pulse1_sample = 0;
    uint8_t pulse2_sample = 0;
    uint8_t triangle_sample = 0;
    uint8_t noise_sample = 0;

    // Внутрішні методи секвенсора
    void clock_quarter_frame();
    void clock_half_frame();
};