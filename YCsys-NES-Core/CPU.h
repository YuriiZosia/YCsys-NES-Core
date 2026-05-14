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

    uint8_t  fetched = 0x00;   // Змінна для зберігання прочитаних даних
    uint16_t addr_abs = 0x0000; // Змінна для зберігання обчисленої адреси (куди звертатися)
    uint8_t  opcode = 0x00;   // Поточний опкод

    bool is_accumulator = false; // Показує, чи працює поточна команда з акумулятором

	void reset(){              // Скидання процесора до початкового стану
	    a = x = y = 0;
        stkp = 0xFD;
        status = 0x00 | FLAGS6502::U; // Встановлюємо прапорець U (завжди 1)
		pc = read(0xFFFC) | (read(0xFFFD) << 8); // Читаємо адресу початку виконання з 0xFFFC/0xFFFD
    }

    // Прапорці статусу (Status Flags) - C Z I D B U V N
    // Ми використовуємо enum для визначення позиції кожного прапорця у 8-бітному регістрі статусу.
    enum FLAGS6502 {
		C = (1 << 0),   // Carry Bit (Перенесення). 0000 0001, hex = 0x01, dec = 1
        Z = (1 << 1),   // Zero (Нуль). 0000 0010, hex = 0x02, dec = 2
        I = (1 << 2),   // Disable Interrupts (Вимкнення переривань). 0000 0100, hex = 0x04, dec = 4
        D = (1 << 3),   // Decimal Mode (Десятковий режим - в NES не використовується, але є в процесорі). 0000 1000, hex = 0x08, dec = 8
		B = (1 << 4),   // Break (Програмне переривання). 0001 0000, hex = 0x10, dec = 16
		U = (1 << 5),   // Unused (Не використовується, завжди 1). 0010 0000, hex = 0x20, dec = 32
		V = (1 << 6),   // Overflow (Переповнення). 0100 0000, hex = 0x40, dec = 64
        N = (1 << 7)    // Negative (Від'ємне число). 1000 0000, hex = 0x80, dec = 128
    };

    // Отримання значення конкретного прапорця (повертає 1 або 0)
    uint8_t GetFlag(FLAGS6502 f) const;

    // Встановлення (true = 1) або скидання (false = 0) конкретного прапорця
    void SetFlag(FLAGS6502 f, bool v);

    // --- Режими адресації ---
    uint8_t IMP();  uint8_t IMM();
    uint8_t ZP0();  uint8_t ZPX();
    uint8_t ZPY();  uint8_t REL();
    uint8_t ABS();  uint8_t ABX();
    uint8_t ABY();  uint8_t IND();
    uint8_t IZX();  uint8_t IZY();

	// --- Інструкції процесора Завантаження/Збереження ---
	uint8_t LDA();  uint8_t LDX();  uint8_t LDY();
	uint8_t STA();  uint8_t STX();  uint8_t STY();

    // --- Інструкції процесора Арифметичні ---
	uint8_t ADC();  uint8_t SBC();

	// --- Інструкції процесора Інкремент/Декремент ---
    uint8_t INC();  uint8_t INX();  uint8_t INY();
    uint8_t DEC();  uint8_t DEX();  uint8_t DEY();

    // --- Інструкції процесора  Логічні операції ---
    uint8_t AND();  uint8_t ORA();  uint8_t EOR(); uint8_t BIT();

	// --- Інструкції процесора Зсуви та обертання ---
    uint8_t ASL(); uint8_t LSR(); uint8_t ROL(); uint8_t ROR();

	// --- Інструкції процесора Управління потоком ---
    uint8_t JMP();  uint8_t JSR();  uint8_t RTS();  uint8_t RTI();

    // --- Інструкції процесора Умовні переходи ---
    uint8_t BCC();  uint8_t BCS();  uint8_t BEQ();  uint8_t BMI();
    uint8_t BNE();  uint8_t BPL();  uint8_t BVC();  uint8_t BVS();

private:
    // Вказівник на головну шину
    Bus* bus = nullptr;

    // Внутрішні функції CPU для зручного спілкування з шиною
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    uint8_t fetch();

	void clock();
};