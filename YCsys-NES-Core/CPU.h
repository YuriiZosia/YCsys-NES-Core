// YCsys NES Core - 6502 CPU implementation
// Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.
#pragma once
#include <cstdint>

// Знову попереднє оголошення, щоб CPU знав про існування шини
class Bus;

class CPU6502 {
public:
    CPU6502();
    ~CPU6502();

    // Підключення процесора до шини (щоб він міг читати/писати)
    void ConnectBus(Bus* n) { bus = n; }

public:
	uint8_t  a, x, y;	       // Акумулятор та індексні регістри
	uint8_t stkp;              // Вказівник стека (Stack Pointer)
    uint8_t  status;           // Статус (Status)
    uint16_t pc;               // Лічильник команд (Program Counter)

	void reset(){              // Скидання процесора до початкового стану
	    a = x = y = 0;
        stkp = 0xFD;
        status = 0x00 | FLAGS6502::U; // Встановлюємо прапорець U (завжди 1)
		pc = read(0xFFFC) | (read(0xFFFD) << 8); // Читаємо адресу початку виконання з 0xFFFC/0xFFFD
    }

    // Прапорці статусу (Status Flags) - C Z I D B U V N
    // Ми використовуємо enum для визначення позиції кожного прапорця у 8-бітному регістрі статусу.
    enum FLAGS6502 {
        C = (1 << 0),   // Carry Bit (Перенесення)
        Z = (1 << 1),   // Zero (Нуль)
        I = (1 << 2),   // Disable Interrupts (Вимкнення переривань)
        D = (1 << 3),   // Decimal Mode (Десятковий режим - в NES не використовується, але є в процесорі)
        B = (1 << 4),   // Break (Програмне переривання)
        U = (1 << 5),   // Unused (Не використовується, завжди 1)
        V = (1 << 6),   // Overflow (Переповнення)
        N = (1 << 7)    // Negative (Від'ємне число)
    };

    // Отримання значення конкретного прапорця (повертає 1 або 0)
    uint8_t GetFlag(FLAGS6502 f);

    // Встановлення (true = 1) або скидання (false = 0) конкретного прапорця
    void SetFlag(FLAGS6502 f, bool v);

private:
    // Вказівник на головну шину
    Bus* bus = nullptr;

    // Внутрішні функції CPU для зручного спілкування з шиною
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
};