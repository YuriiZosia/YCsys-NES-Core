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
 * |      [?]  YCsys NES CORE - 6502 CPU IMPLEMENTATION  [?]                 |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 *            [?]         [CPU]         [?]         [CPU]         [?]
 */
#include "CPU.h"
#include "Bus.h" // Обов'язково додаємо, щоб бачити методи класу Bus

// Конструктор: виконується при створенні об'єкта
CPU6502::CPU6502() {
    a = 0x00;
    x = 0x00;
    y = 0x00;
    stkp = 0x00;
    pc = 0x0000;
    status = 0x00;
}
// Деструктор: виконується при видаленні об'єкта
CPU6502::~CPU6502() {
    
}

// Реалізація приватного методу читання
uint8_t CPU6502::read(uint16_t addr) {
    // Якщо процесор підключений до шини, читаємо дані
    if (bus != nullptr) {
        return bus->read(addr);
    }
    return 0x00; // Повертаємо 0, якщо шина не підключена
}

// Реалізація приватного методу запису
void CPU6502::write(uint16_t addr, uint8_t data) {
    // Якщо процесор підключений до шини, записуємо дані
    if (bus != nullptr) {
        bus->write(addr, data);
    }
}

// Допоміжна функція fetch() для читання даних за адресою addr_abs
uint8_t CPU6502::fetch() {
    if (!is_accumulator) {
        fetched = read(addr_abs);
    }
    return fetched;
}

uint8_t CPU6502::GetFlag(FLAGS6502 f) const{
    // Якщо біт за маскою 'f' встановлений, повертаємо 1, інакше 0
    return ((status & f) > 0) ? 1 : 0;
}

void CPU6502::SetFlag(FLAGS6502 f, bool v) {
    if (v) {
        status |= f;    // Встановлюємо біт (OR)
    }
    else {
        status &= ~f;   // Скидаємо біт (AND з інверсією)
    }
}

//  _______________________________________________________
// |  ___________________________________________________  |
// | |                                                   | |
// | |   _____  _      _____  _____  _   __              | |
// | |  /  __ \| |    |  _  |/  __ \| | / /              | |
// | |  | /  \/| |    | | | || /  \/| |/ /               | |
// | |  | |    | |    | | | || |    |    \               | |
// | |  | \__/\| |____\ \_/ /| \__/\| |\  \              | |
// | |   \____/\_____/ \___/  \____/\_| \_/              | |
// | |                                                   | |
// | |____________________[ СЕРЦЕБИТТЯ ПРОЦЕСОРА ]_______| |
// |_______________________________________________________|
//        ||           ||           ||           ||

void CPU6502::clock() {
    // Якщо cycles дорівнює 0, це означає, що процесор завершив 
    // виконання попередньої інструкції і готовий читати нову.
    if (cycles == 0) {
        is_accumulator = false; // Скидаємо перед кожною інструкцією

        // 1. Читаємо опкод
        opcode = read(pc);
        pc++;

        // 2. Отримуємо базову кількість тактів З ТАБЛИЦІ!
        cycles = cycle_table[opcode];

        // 3. Декодуємо та виконуємо через switch
        switch (opcode) {

            // =================================================================
            // --- ГРУПА ЗАВАНТАЖЕННЯ / ЗБЕРЕЖЕННЯ (Load / Store) ---
            // =================================================================

            // LDA (Load Accumulator)
        case 0xA9: IMM(); LDA(); break;
        case 0xA5: ZP0(); LDA(); break;
        case 0xB5: ZPX(); LDA(); break;
        case 0xAD: ABS(); LDA(); break;
        case 0xBD: ABX(); LDA(); break;
        case 0xB9: ABY(); LDA(); break;
        case 0xA1: IZX(); LDA(); break;
        case 0xB1: IZY(); LDA(); break;

            // LDX (Load X Register)
        case 0xA2: IMM(); LDX(); break;
        case 0xA6: ZP0(); LDX(); break;
        case 0xB6: ZPY(); LDX(); break;
        case 0xAE: ABS(); LDX(); break;
        case 0xBE: ABY(); LDX(); break;

            // LDY (Load Y Register)
        case 0xA0: IMM(); LDY(); break;
        case 0xA4: ZP0(); LDY(); break;
        case 0xB4: ZPX(); LDY(); break;
        case 0xAC: ABS(); LDY(); break;
        case 0xBC: ABX(); LDY(); break;

            // STA (Store Accumulator)
        case 0x85: ZP0(); STA(); break;
        case 0x95: ZPX(); STA(); break;
        case 0x8D: ABS(); STA(); break;
        case 0x9D: ABX(); STA(); break;
        case 0x99: ABY(); STA(); break;
        case 0x81: IZX(); STA(); break;
        case 0x91: IZY(); STA(); break;

            // STX (Store X Register)
        case 0x86: ZP0(); STX(); break;
        case 0x96: ZPY(); STX(); break;
        case 0x8E: ABS(); STX(); break;

            // STY (Store Y Register)
        case 0x84: ZP0(); STY(); break;
        case 0x94: ZPX(); STY(); break;
        case 0x8C: ABS(); STY(); break;

            // =================================================================
            // --- ГРУПА АРИФМЕТИКИ (Arithmetic) ---
            // =================================================================

            // ADC (Add with Carry)
        case 0x69: IMM(); ADC(); break;
        case 0x65: ZP0(); ADC(); break;
        case 0x75: ZPX(); ADC(); break;
        case 0x6D: ABS(); ADC(); break;
        case 0x7D: ABX(); ADC(); break;
        case 0x79: ABY(); ADC(); break;
        case 0x61: IZX(); ADC(); break;
        case 0x71: IZY(); ADC(); break;

            // SBC (Subtract with Carry)
        case 0xE9: IMM(); SBC(); break;
        case 0xE5: ZP0(); SBC(); break;
        case 0xF5: ZPX(); SBC(); break;
        case 0xED: ABS(); SBC(); break;
        case 0xFD: ABX(); SBC(); break;
        case 0xF9: ABY(); SBC(); break;
        case 0xE1: IZX(); SBC(); break;
        case 0xF1: IZY(); SBC(); break;

            // =================================================================
            // --- ГРУПА ІНКРЕМЕНТУ / ДЕКРЕМЕНТУ (Inc / Dec) ---
            // =================================================================

            // INC (Increment Memory)
        case 0xE6: ZP0(); INC(); break;
        case 0xF6: ZPX(); INC(); break;
        case 0xEE: ABS(); INC(); break;
        case 0xFE: ABX(); INC(); break;

            // INX / INY (Increment Registers)
        case 0xE8: IMP(); INX(); break;
        case 0xC8: IMP(); INY(); break;

            // DEC (Decrement Memory)
        case 0xC6: ZP0(); DEC(); break;
        case 0xD6: ZPX(); DEC(); break;
        case 0xCE: ABS(); DEC(); break;
        case 0xDE: ABX(); DEC(); break;

            // DEX / DEY (Decrement Registers)
        case 0xCA: IMP(); DEX(); break;
        case 0x88: IMP(); DEY(); break;

            // =================================================================
            // --- ЛОГІЧНІ ОПЕРАЦІЇ (Logical) ---
            // =================================================================

            // AND (Logical AND)
        case 0x29: IMM(); AND(); break;
        case 0x25: ZP0(); AND(); break;
        case 0x35: ZPX(); AND(); break;
        case 0x2D: ABS(); AND(); break;
        case 0x3D: ABX(); AND(); break;
        case 0x39: ABY(); AND(); break;
        case 0x21: IZX(); AND(); break;
        case 0x31: IZY(); AND(); break;

            // ORA (Logical Inclusive OR)
        case 0x09: IMM(); ORA(); break;
        case 0x05: ZP0(); ORA(); break;
        case 0x15: ZPX(); ORA(); break;
        case 0x0D: ABS(); ORA(); break;
        case 0x1D: ABX(); ORA(); break;
        case 0x19: ABY(); ORA(); break;
        case 0x01: IZX(); ORA(); break;
        case 0x11: IZY(); ORA(); break;

            // EOR (Exclusive OR / XOR)
        case 0x49: IMM(); EOR(); break;
        case 0x45: ZP0(); EOR(); break;
        case 0x55: ZPX(); EOR(); break;
        case 0x4D: ABS(); EOR(); break;
        case 0x5D: ABX(); EOR(); break;
        case 0x59: ABY(); EOR(); break;
        case 0x41: IZX(); EOR(); break;
        case 0x51: IZY(); EOR(); break;

            // BIT (Bit Test)
        case 0x24: ZP0(); BIT(); break;
        case 0x2C: ABS(); BIT(); break;

            // =================================================================
            // --- ЗСУВИ ТА ОБЕРТАННЯ (Shifts & Rotates) ---
            // =================================================================

            // ASL (Arithmetic Shift Left)
        case 0x0A: IMP(); ASL(); break; // Accumulator
        case 0x06: ZP0(); ASL(); break;
        case 0x16: ZPX(); ASL(); break;
        case 0x0E: ABS(); ASL(); break;
        case 0x1E: ABX(); ASL(); break;

            // LSR (Logical Shift Right)
        case 0x4A: IMP(); LSR(); break; // Accumulator
        case 0x46: ZP0(); LSR(); break;
        case 0x56: ZPX(); LSR(); break;
        case 0x4E: ABS(); LSR(); break;
        case 0x5E: ABX(); LSR(); break;

            // ROL (Rotate Left)
        case 0x2A: IMP(); ROL(); break; // Accumulator
        case 0x26: ZP0(); ROL(); break;
        case 0x36: ZPX(); ROL(); break;
        case 0x2E: ABS(); ROL(); break;
        case 0x3E: ABX(); ROL(); break;

            // ROR (Rotate Right)
        case 0x6A: IMP(); ROR(); break; // Accumulator
        case 0x66: ZP0(); ROR(); break;
        case 0x76: ZPX(); ROR(); break;
        case 0x6E: ABS(); ROR(); break;
        case 0x7E: ABX(); ROR(); break;

            // =================================================================
            // --- УПРАВЛІННЯ ПОТОКОМ (Flow Control) ---
            // =================================================================

            // JMP (Jump)
        case 0x4C: ABS(); JMP(); break;
        case 0x6C: IND(); JMP(); break;

            // JSR (Jump to Subroutine)
        case 0x20: ABS(); JSR(); break;

            // RTS / RTI (Returns)
        case 0x60: IMP(); RTS(); break;
        case 0x40: IMP(); RTI(); break;

            // УМОВНІ ПЕРЕХОДИ (Branches)
        case 0x90: REL(); BCC(); break;
        case 0xB0: REL(); BCS(); break;
        case 0xF0: REL(); BEQ(); break;
        case 0xD0: REL(); BNE(); break;
        case 0x30: REL(); BMI(); break;
        case 0x10: REL(); BPL(); break;
        case 0x50: REL(); BVC(); break;
        case 0x70: REL(); BVS(); break;

            // =================================================================
            // --- СКИДАННЯ ТА ВСТАНОВЛЕННЯ ПРАПОРЦІВ (Flags) ---
            // =================================================================

            // Очищення (Clear)
        case 0x18: IMP(); CLC(); break;
        case 0xD8: IMP(); CLD(); break;
        case 0x58: IMP(); CLI(); break;
        case 0xB8: IMP(); CLV(); break;

            // Встановлення (Set)
        case 0x38: IMP(); SEC(); break;
        case 0xF8: IMP(); SED(); break;
        case 0x78: IMP(); SEI(); break;

            // =================================================================
            // --- РОБОТА З РЕГІСТРАМИ ТА СТЕКОМ (System & Stack) ---
            // =================================================================

            // Системні
        case 0x00: IMP(); BRK(); break;
        case 0xEA: IMP(); NOP(); break;

            // Передача регістрів
        case 0xAA: IMP(); TAX(); break;
        case 0xA8: IMP(); TAY(); break;
        case 0x8A: IMP(); TXA(); break;
        case 0x98: IMP(); TYA(); break;
        case 0xBA: IMP(); TSX(); break;
        case 0x9A: IMP(); TXS(); break;

            // Робота зі стеком
        case 0x48: IMP(); PHA(); break;
        case 0x08: IMP(); PHP(); break;
        case 0x68: IMP(); PLA(); break;
        case 0x28: IMP(); PLP(); break;

            // =================================================================
            // --- ГРУПА ПОРІВНЯННЯ (Compare) ---
            // =================================================================

            // CMP
        case 0xC9: IMM(); CMP(); break;
        case 0xC5: ZP0(); CMP(); break;
        case 0xD5: ZPX(); CMP(); break;
        case 0xCD: ABS(); CMP(); break;
        case 0xDD: ABX(); CMP(); break;
        case 0xD9: ABY(); CMP(); break;
        case 0xC1: IZX(); CMP(); break;
        case 0xD1: IZY(); CMP(); break;

            // CPX
        case 0xE0: IMM(); CPX(); break;
        case 0xE4: ZP0(); CPX(); break;
        case 0xEC: ABS(); CPX(); break;

            // CPY
        case 0xC0: IMM(); CPY(); break;
        case 0xC4: ZP0(); CPY(); break;
        case 0xCC: ABS(); CPY(); break;

            // =================================================================
            // Якщо опкод ще не реалізований або невідомий (нелегальний)
            // =================================================================
        default:
            printf("Unknown opcode: %02X\n", opcode);
            break;
        }
    }

    // Кожен виклик clock() імітує 1 такт генератора
    cycles--;
}

//  _______________________________________________
// |    __  __  ____  __  __  ____  ____  _  _     |
// |   |  \/  || ___||  \/  ||    ||  _ \( \/ )    |   
// |   | |\/| || ___|| |\/| || || ||   /  \  /     |
// |   |_|  |_||____||_|  |_||____||_|\_\ (__)     |
// |                                               |
// |  ---[ ТРУБИ ДОСТУПУ ДО ПАМ'ЯТІ ]--------------|
// |_______________________________________________|
//   ||  ||       ||  ||       ||  ||       ||  || 
//   ||__||       ||__||       ||__||       ||__|| 
//   |____|       |____|       |____|       |____| 

// Режим адресації IMP (Implied) - операнд не вказується, оскільки він є частиною інструкції (наприклад, INX, CLC, SEC тощо), і зазвичай працює з акумулятором або просто виконує дію без операнда
uint8_t CPU6502::IMP() {
    is_accumulator = true;
    fetched = a; // Одразу завантажуємо значення акумулятора для роботи
    return 0;
}

// Режим адресації IMM (Immediate) - дані знаходяться в наступному байті після опкода
uint8_t CPU6502::IMM() {
    // В безпосередній адресації адреса даних - це поточне значення лічильника команд
    addr_abs = pc;
    pc++; // Одразу зсуваємо лічильник на наступний байт
    return 0; // Додаткові такти не потрібні
}

// Режим адресації ZP0 (Zero Page) - 8-бітна адреса вказується в наступному байті після опкода, і звертається до першої сторінки пам'яті (0x0000-0x00FF)
uint8_t CPU6502::ZP0() {
    // Читаємо 1 байт адреси
	addr_abs = read(pc);
	pc++; // Зсуваємо лічильник команд на наступний байт
	addr_abs &= 0x00FF; // Маскуємо старший байт, щоб залишити лише 8 біт (адреса в нульовій сторінці)
    return 0;
}

// Режим адресації ZPX (Zero Page X) - адреса обчислюється як базова 8-бітна адреса з нульової сторінки плюс значення регістра X. Якщо результат перевищує 255, він "обгортається" (залишається в межах нульової сторінки).
uint8_t CPU6502::ZPX() {
    // Читаємо базову адресу, додаємо X і маскуємо старший байт, щоб залишитись у нульовій сторінці
    addr_abs = (read(pc) + x) & 0x00FF;
    pc++;
    return 0;
}

// Режим адресації ZPY (Zero Page Y) - аналогічно до ZPX, але використовується регістр Y. Застосовується переважно з інструкціями LDX та STX.
uint8_t CPU6502::ZPY() {
    // Читаємо базову адресу, додаємо Y і маскуємо старший байт
    addr_abs = (read(pc) + y) & 0x00FF;
    pc++;
    return 0;
}

// Режим адресації REL (Relative) - використовується для умовних переходів (Branching), де 8-бітне знакове зміщення вказується в наступному байті після опкода, і додається до поточного лічильника команд для отримання цільової адреси переходу
uint8_t CPU6502::REL() {
    // Це автоматично робить "Sign Extension": 
    // якщо 7-й біт == 1, то addr_rel (16-біт) отримає 0xFF в старший байт.
    // (Замінює конструкцію: if (addr_rel & 0x80) addr_rel |= 0xFF00;)
    addr_rel = (uint8_t)read(pc);
    pc++;
    // Якщо 7-й біт встановлений (число від'ємне), ми маємо вручну заповнити 
    // старший байт 16-бітної змінної одиницями (0xFF), щоб зберегти знак.
    if (addr_rel & 0x80) {
        addr_rel |= 0xFF00;
    }
    return 0;
}

// Режим адресації ABS (Absolute) - 16-бітна адреса вказується безпосередньо в наступних двох байтах після опкода
uint8_t CPU6502::ABS() {
    // Читаємо спочатку молодший байт (lo), потім старший (hi)
    uint16_t lo = read(pc);
    pc++;
    uint16_t hi = read(pc);
    pc++;

    // Склеюємо їх у 16-бітну адресу: зсуваємо hi на 8 біт вліво і додаємо lo
    addr_abs = (hi << 8) | lo;
    return 0;
}

// Режим адресації ABX (Absolute X) - до повної 16-бітної абсолютної адреси додається значення регістра X.
uint8_t CPU6502::ABX() {
    uint16_t lo = read(pc);
    pc++;
    uint16_t hi = read(pc);
    pc++;

    uint16_t base_addr = (hi << 8) | lo; // Склеюємо базову адресу
    addr_abs = base_addr + x;            // Додаємо зміщення з регістра X

    // Якщо при додаванні змінився старший байт (перетин сторінки пам'яті), 
    // може знадобитися додатковий такт, тому повертаємо 1.
    if ((addr_abs & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

// Режим адресації ABY (Absolute Y) - до повної 16-бітної абсолютної адреси додається значення регістра Y.
uint8_t CPU6502::ABY() {
    uint16_t lo = read(pc);
    pc++;
    uint16_t hi = read(pc);
    pc++;

    uint16_t base_addr = (hi << 8) | lo;
    addr_abs = base_addr + y; // Додаємо зміщення з регістра Y

    // Якщо перетнули сторінку пам'яті - повертаємо 1
    if ((addr_abs & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

// Режим адресації IND (Indirect) - використовується лише для JMP, де 16-бітна адреса вказується в наступних двох байтах після опкода, але фактична адреса для переходу береться з пам'яті за цією адресою
uint8_t CPU6502::IND() {
    uint16_t ptr_lo = read(pc);
    pc++;
    uint16_t ptr_hi = read(pc);
    pc++;

    uint16_t ptr = (ptr_hi << 8) | ptr_lo;

    // Відтворення апаратного багу 6502 з межею сторінки
    if (ptr_lo == 0x00FF) {
        // Симулюємо баг переходу сторінки
        addr_abs = (read(ptr & 0xFF00) << 8) | read(ptr);
    }
    else {
        // Нормальна поведінка
        addr_abs = (read(ptr + 1) << 8) | read(ptr);
    }

    return 0;
}

// Режим адресації IZX (Indexed Indirect / Indirect, X) - спочатку до 8-бітної адреси додається X (з обгортанням в нульовій сторінці). Потім за цією новою адресою (і наступною) читається справжня 16-бітна адреса.
uint8_t CPU6502::IZX() {
    uint16_t t = read(pc); // Читаємо базову адресу (вказівник) з коду
    pc++;

    uint16_t ptr = (t + x) & 0x00FF; // Додаємо X з обгортанням у нульовій сторінці

    // Читаємо молодший і старший байти цільової адреси з нульової сторінки
    uint16_t lo = read(ptr & 0x00FF);
    uint16_t hi = read((ptr + 1) & 0x00FF);

    addr_abs = (hi << 8) | lo; // Формуємо фінальну адресу
    return 0; // Перетин сторінки тут не дає додаткових тактів
}

// Режим адресації IZY (Indirect Indexed / (Indirect), Y) - спочатку читається 16-бітна базова адреса з нульової сторінки. Потім до цієї базової адреси додається Y для отримання фінальної адреси.
uint8_t CPU6502::IZY() {
    uint16_t t = read(pc); // Читаємо адресу вказівника
    pc++;

    // Читаємо базову адресу з нульової сторінки
    uint16_t lo = read(t & 0x00FF);
    uint16_t hi = read((t + 1) & 0x00FF);

    uint16_t base_addr = (hi << 8) | lo;
    addr_abs = base_addr + y; // Додаємо Y до базової адреси

    // Якщо перетнули сторінку пам'яті під час додавання Y - повертаємо 1
    if ((addr_abs & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

//  _______________________________________________________________
//      🚩
//      |
//      |    I N S T R U C T I O N S
//      |   _________________________
//      |  |                         |
//      |  |   [A] [X] [Y] [P] [S]   |
//      |  |_________________________|
//      |
//   ___|___                                           _   _   _
//  |_______|                 [ 8-БІТНІ СУПЕР-СИЛИ ]  [ ] [ ] [ ]
//  |_______|_________________________________________|___|___|____
//     [?]  [?]  [?]  [?]  [?]  [?]  [?]  [?]  [?]  [?]  [?]  [?]

//
// Завантаження та Збереження (Load/Store)
//

// LDA (Load Accumulator)
uint8_t CPU6502::LDA() {
	fetch(); // Отримуємо дані, які потрібно завантажити в акумулятор
    a = fetched; // Завантажуємо дані в акумулятор
	// Встановлюємо прапорці Zero та Negative на основі нового значення акумулятора
	SetFlag(FLAGS6502::Z, a == 0x00); // Якщо a == 0, встановлюємо прапорець Zero
	SetFlag(FLAGS6502::N, (a & 0x80) != 0); // Якщо старший біт a встановлений, встановлюємо прапорець Negative. 0x80 - це 10000000 у двійковій. a & 0x80 це перевірка, чи встановлений старший біт (бит 7) в a.
    return 1; // Ця команда може потребувати додаткового такту, тому повертаємо 1 (на майбутнє)
}

// LDX (Load X Register)
uint8_t CPU6502::LDX() {
    fetch();
    x = fetched;
    SetFlag(FLAGS6502::Z, x == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 1;
}

// LDY (Load Y Register)
uint8_t CPU6502::LDY() {
    fetch();
    y = fetched;
    SetFlag(FLAGS6502::Z, y == 0x00);
    SetFlag(FLAGS6502::N, (y & 0x80) != 0);
    return 1;
}

// STA (Store Accumulator)
uint8_t CPU6502::STA() {
	write(addr_abs, a); // Записуємо значення акумулятора в пам'ять за адресою addr_abs
    return 0;
}

// STX (Store X Register)
uint8_t CPU6502::STX() {
    write(addr_abs, x); // Записуємо значення регістра X в пам'ять за адресою addr_abs
    return 0;
}

// STY (Store Y Register)
uint8_t CPU6502::STY() {
    write(addr_abs, y); // Записуємо значення регістра Y в пам'ять за адресою addr_abs
    return 0;
}

// --------------------------------------------------------------

//
// Арифметичні операції (Arithmetic)
//

// ADC (Add with Carry)
uint8_t CPU6502::ADC() {
	fetch(); // Отримуємо дані, які потрібно додати до акумулятора

    // Додаємо значення акумулятора, отримані дані та прапорець перенесення (Carry)
	uint16_t temp = (uint16_t)a + (uint16_t)fetched + (uint16_t)GetFlag(FLAGS6502::C);

    // Встановлюємо прапорці
	SetFlag(FLAGS6502::C, temp > 0xFF); // Встановлюємо прапорець перенесення, якщо результат перевищує 255
	SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0); // Встановлюємо прапорець Zero, якщо результат дорівнює нулю
	SetFlag(FLAGS6502::N, (temp & 0x80) != 0); // Встановлюємо прапорець Negative, якщо старший біт результату встановлений

    // Магія обчислення переповнення (Overflow) для 6502
	SetFlag(FLAGS6502::V, ((~((uint16_t)a ^ (uint16_t)fetched) & ((uint16_t)a ^ (uint16_t)temp)) & 0x80) != 0);

	a = temp & 0x00FF; // Зберігаємо лише молодший байт результату в акумуляторі (6502 працює з 8-бітними числами)
    return 1; // Ця команда може потребувати додаткового такту в деяких режимах
}

// SBC (Subtract with Carry)
uint8_t CPU6502::SBC() {
    fetch(); // Отримуємо дані, які потрібно додати до акумулятора

    // Робимо інверсію через XOR & 0x00FF, бо віднімання в 6502 - це додавання інвертованого значення
    uint16_t value = (uint16_t)fetched ^ 0x00FF;

    uint16_t temp = (uint16_t)a + value + (uint16_t)GetFlag(FLAGS6502::C);

    // Встановлюємо прапорці
    SetFlag(FLAGS6502::C, temp > 0xFF);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);
    SetFlag(FLAGS6502::V, (~((uint16_t)a ^ value) & ((uint16_t)a ^ (uint16_t)temp)) & 0x0080);

    a = temp & 0x00FF;
    return 1;
}

// --------------------------------------------------------------
// Інкремент та Декремент (Increment/Decrement)
//

// INX (Increment X Register)
uint8_t CPU6502::INX() {
    x++;
    SetFlag(FLAGS6502::Z, (x & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 0;
}

// INY (Increment Y Register)
uint8_t CPU6502::INY() {
    y++;
    SetFlag(FLAGS6502::Z, (y & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (y & 0x80) != 0);
    return 0;
}

// INC (Increment Memory) - інкрементує значення в пам'яті за адресою addr_abs
uint8_t CPU6502::INC() {
    fetch();
    uint16_t temp = (uint16_t)fetched + 1;
    write(addr_abs, temp & 0x00FF);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);
    return 0;
}

// DEX (Decrement X Register)
uint8_t CPU6502::DEX() {
    x--;
    SetFlag(FLAGS6502::Z, (x & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 0;
}

// DEY (Decrement Y Register)
uint8_t CPU6502::DEY() {
    y--;
    SetFlag(FLAGS6502::Z, (y & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (y & 0x80) != 0);
    return 0;
}

// DEC (Decrement Memory) - декрементує значення в пам'яті за адресою addr_abs
uint8_t CPU6502::DEC() {
    fetch();
    uint16_t temp = (uint16_t)fetched - 1;
    write(addr_abs, temp & 0x00FF);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);
    return 0;
}

// --------------------------------------------------------------
//
// Логічні операції (Logical)
//

// Логічне І (AND)
uint8_t CPU6502::AND() {
    fetch();
    a = a & fetched;
    SetFlag(FLAGS6502::Z, (a & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 1;
}

// Логічне АБО (ORA - OR with Accumulator)
uint8_t CPU6502::ORA() {
    fetch();
    a = a | fetched;
    SetFlag(FLAGS6502::Z, (a & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 1;
}

// Виключне АБО (EOR - Exclusive OR / XOR)
uint8_t CPU6502::EOR() {
    fetch();
    a = a ^ fetched;
    SetFlag(FLAGS6502::Z, (a & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 1;
}

// BIT (Bit Test) - тестує біти в акумуляторі та встановлює прапорці на основі даних з пам'яті
uint8_t CPU6502::BIT() {
    fetch();

    // Z-flag: Встановлюється, якщо (A AND M) == 0
    uint8_t temp = a & fetched;
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);

    // N-flag: Набуває значення 7-го біта даних прямо з пам'яті
    SetFlag(FLAGS6502::N, (fetched & (1 << 7)) != 0);

    // V-flag: Набуває значення 6-го біта даних прямо з пам'яті
    SetFlag(FLAGS6502::V, (fetched & (1 << 6)) != 0);

    return 0; // BIT не потребує додаткових тактів
}

// --------------------------------------------------------------

//
// Операції зсуву та обертання (Shifts and Rotates)
//

// ASL (Arithmetic Shift Left) - зсуває біти вліво, нуль потрапляє в правий бік, а лівий біт потрапляє в Carry
uint8_t CPU6502::ASL() {
    fetch(); // Завдяки логіці в IMP() та fetch(), тут уже буде або 'a', або значення з пам'яті
    uint16_t temp = (uint16_t)fetched << 1; // Зсуваємо вліво. Використовуємо 16 біт, щоб "зловити" 7-й біт, який випаде у 8-й

    SetFlag(FLAGS6502::C, (temp & 0xFF00) != 0);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);

    // А тепер вирішуємо, куди повернути результат
    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }

    return 0;
}

// LSR (Logical Shift Right) - зсуває біти вправо, нуль потрапляє в лівий бік, а правий біт потрапляє в Carry
uint8_t CPU6502::LSR() {
    fetch();
    // Нульовий біт потрапляє в Carry
    SetFlag(FLAGS6502::C, (fetched & 0x0001) != 0);

    // Зсуваємо вправо
    uint8_t temp = fetched >> 1;

    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x0080) != 0);

    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

// ROL (Rotate Left) - зсуває біти вліво, нуль потрапляє в правий бік, а лівий біт потрапляє в Carry. При цьому, старий Carry потрапляє в правий біт.
uint8_t CPU6502::ROL() {
    fetch();

    /// Зсуваємо вліво, звільняючи 0-й біт, і записуємо в нього поточний Carry
    uint16_t temp = (uint16_t)(fetched << 1) | GetFlag(FLAGS6502::C);

    SetFlag(FLAGS6502::C, (temp & 0xFF00) != 0);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x0080) != 0);

    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

// ROR (Rotate Right) - зсуває біти вправо, нуль потрапляє в лівий бік, а правий біт потрапляє в Carry. При цьому, старий Carry потрапляє в лівий біт.
uint8_t CPU6502::ROR() {
    fetch();
    // Новий Carry — це 0-й біт ДО зсуву
    bool bNewCarry = (fetched & 0x01) != 0;

    // Зсуваємо вправо, звільняючи 7-й біт, і «десантуємо» в нього Carry (зсунувши його на 7 позицій вліво)
    uint16_t temp = (uint16_t)(fetched >> 1) | ((uint16_t)GetFlag(FLAGS6502::C) << 7);

    SetFlag(FLAGS6502::C, bNewCarry); // Встановлюємо той самий 0-й біт
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);

    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

// --------------------------------------------------------------

//
// Стрибки та виклики (Branching and Jumps)
//

// JMP (Jump) - безумовний стрибок на адресу, обчислену в режимі адресації
uint8_t CPU6502::JMP() {
    pc = addr_abs; // Просто встановлюємо лічильник команд на адресу, обчислену в режимі адресації
    return 0;
}

// JSR (Jump to Subroutine) - виконує стрибок на адресу, обчислену в режимі адресації, але перед цим зберігає адресу повернення (адресу наступної інструкції після JSR) у стек. Важливо: 6502 зберігає адресу повернення як (PC - 1), тобто адресу поточного JSR, а не наступної інструкції. Це пов'язано з тим, як 6502 обробляє стек і виклики підпрограм.
uint8_t CPU6502::JSR() {
    pc--;

    write(0x0100 + stkp, (pc >> 8) & 0x00FF); // 1. Зберігаємо старший (High) байт адреси у стек (сторінка 0x0100)
    stkp--;                                   // 2. Зсуваємо вказівник стека вниз після запису
    write(0x0100 + stkp, pc & 0x00FF);        // 3. Зберігаємо молодший (Low) байт адреси у наступну комірку стека
    stkp--;                                   // 4. Зсуваємо вказівник для підготовки до майбутніх операцій запису

    pc = addr_abs; // Переходимо в підпрограму
    return 0;
}

// RTS дістає адресу повернення зі стека і встановлює лічильник команд на цю адресу + 1 (щоб перейти до наступної реальної інструкції після підпрограми)
uint8_t CPU6502::RTS() {
    stkp++;
    uint16_t lo = read(0x0100 + stkp);
    stkp++;
    uint16_t hi = read(0x0100 + stkp);

    pc = (hi << 8) | lo;
    pc++; // Після RTS ми маємо перейти до наступної реальної інструкції
    return 0;
}

// RTI (Return from Interrupt) - подібно до RTS, але також відновлює статусний регістр (P) зі стека. Після RTI процесор повертається до виконання інструкції, яка була перервана, з повністю відновленим станом.
uint8_t CPU6502::RTI() {
    stkp++;
    status = read(0x0100 + stkp);
	status &= ~FLAGS6502::B;  // Обнуляємо прапорець B, бо він не зберігається в статусі при перериванні
    status |= FLAGS6502::U;   // Прапорець U завжди 1

    // Потім дістаємо адресу повернення (Low-High)
    stkp++;
    uint16_t lo = read(0x0100 + stkp);
    stkp++;
    uint16_t hi = read(0x0100 + stkp);

	pc = (hi << 8) | lo; // Зміщуємо hi вліво на 8 біт і додаємо lo, щоб отримати повну 16-бітну адресу
    return 0;
}

// --------------------------------------------------------------

//
// Умовні переходи (Branching)
//

// BCC (Branch if Carry Clear) Перехід, якщо прапорець Carry дорівнює 0
uint8_t CPU6502::BCC() {
    if (GetFlag(FLAGS6502::C) == 0) {
        cycles++; // +1 такт за сам факт переходу
        addr_abs = pc + addr_rel;

        // Перевірка перетину сторінки
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++; // ще +1 такт за перетин сторінки
        }

        pc = addr_abs;
    }
    return 0;
}

// BCS (Branch if Carry Set) Перехід, якщо прапорець Carry дорівнює 1
uint8_t CPU6502::BCS() {
    if (GetFlag(FLAGS6502::C) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;

        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }

        pc = addr_abs;
    }
    return 0;
}

// BEQ (Branch if Equal) Перехід, якщо прапорець Zero дорівнює 1 (результат дорівнює нулю)
uint8_t CPU6502::BEQ() {
    if (GetFlag(FLAGS6502::Z) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;

        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }

        pc = addr_abs;
    }
    return 0;
}

// BMI (Branch if Minus) Перехід, якщо прапорець Negative дорівнює 1
uint8_t CPU6502::BMI() {
    if (GetFlag(FLAGS6502::N) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;

        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }

        pc = addr_abs;
    }
    return 0;
}

// BNE (Branch if Not Equal) Перехід, якщо прапорець Zero дорівнює 0 (результат не дорівнює нулю)
uint8_t CPU6502::BNE() {
    if (GetFlag(FLAGS6502::Z) == 0) {
        cycles++;
        addr_abs = pc + addr_rel;

        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }

        pc = addr_abs;
    }
    return 0;
}

// BPL (Branch if Plus) Перехід, якщо прапорець Negative дорівнює 0
uint8_t CPU6502::BPL() {
    if (GetFlag(FLAGS6502::N) == 0) {
        cycles++;
        addr_abs = pc + addr_rel;

        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }

        pc = addr_abs;
    }
    return 0;
}

// BVC (Branch if Overflow Clear) Перехід, якщо прапорець Overflow дорівнює 0
uint8_t CPU6502::BVC() {
    if (GetFlag(FLAGS6502::V) == 0) {
        cycles++;
        addr_abs = pc + addr_rel;

        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }

        pc = addr_abs;;
    }
    return 0;
}

// BVS (Branch if Overflow Set) Перехід, якщо прапорець Overflow дорівнює 1
uint8_t CPU6502::BVS() {
    if (GetFlag(FLAGS6502::V) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;

        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }

        pc = addr_abs;
    }
    return 0;
}

// --------------------------------------------------------------

//
// Управління прапорцями (Flag Operations)
//

// Очищення прапорців (Clear - ставимо false/0)
uint8_t CPU6502::CLC() { SetFlag(FLAGS6502::C, false); return 0; } // CLC (Clear Carry) Прапорець Carry = 0 [2]
uint8_t CPU6502::CLD() { SetFlag(FLAGS6502::D, false); return 0; } // CLD (Clear Decimal) Прапорець Decimal = 0 [2]
uint8_t CPU6502::CLI() { SetFlag(FLAGS6502::I, false); return 0; } // CLI (Clear Interrupt) Прапорець Interrupt = 0 [2]
uint8_t CPU6502::CLV() { SetFlag(FLAGS6502::V, false); return 0; } // CLV (Clear Overflow) Прапорець Overflow = 0 [2]

// Встановлення прапорців (Set - ставимо true/1)
uint8_t CPU6502::SEC() { SetFlag(FLAGS6502::C, true); return 0; }  // SEC (Set Carry) Прапорець Carry = 1 [2]
uint8_t CPU6502::SED() { SetFlag(FLAGS6502::D, true); return 0; }  // SED (Set Decimal) Прапорець Decimal = 1 [2]
uint8_t CPU6502::SEI() { SetFlag(FLAGS6502::I, true); return 0; }  // SEI (Set Interrupt) Прапорець Interrupt = 1 [2]

// --------------------------------------------------------------

//
// Робота з регістрами та стеком (Register and Stack Operations) Системні та пересилочні інструкції
//

// NOP (No Operation) - Не робить нічого, лише витрачає 2 такти
uint8_t CPU6502::NOP() {
    return 0;
}

// TAX (Transfer Accumulator to X) Копіювання A в X
uint8_t CPU6502::TAX() {
    x = a;
    SetFlag(FLAGS6502::Z, x == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 0;
}

// TAY (Transfer Accumulator to Y) Копіювання A в Y
uint8_t CPU6502::TAY() {
    y = a;
    SetFlag(FLAGS6502::Z, y == 0x00);
    SetFlag(FLAGS6502::N, (y & 0x80) != 0);
    return 0;
}

// TXA (Transfer X to Accumulator) Копіювання X в A
uint8_t CPU6502::TXA() {
    a = x;
    SetFlag(FLAGS6502::Z, a == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 0;
}

// TYA (Transfer Y to Accumulator) Копіювання Y в A
uint8_t CPU6502::TYA() {
    a = y;
    SetFlag(FLAGS6502::Z, a == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 0;
}

// TSX (Transfer Stack Pointer to X) Копіювання вказівника стека в X
uint8_t CPU6502::TSX() {
    x = stkp;
    SetFlag(FLAGS6502::Z, x == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 0;
}

// TXS (Transfer X to Stack Pointer) Копіювання X у вказівник стека (Прапорці НЕ змінюються)
uint8_t CPU6502::TXS() {
    stkp = x;
    return 0;
}

// PHA (Push Accumulator) Запис акумулятора в стек
uint8_t CPU6502::PHA() {
    write(0x0100 + stkp, a);
    stkp--;
    return 0;
}

// PHP (Push Processor Status) Запис регістра статусу в стек
uint8_t CPU6502::PHP() {
    // На стек статус завжди пишеться з встановленими бітами 4 (B) та 5 (U)
    write(0x0100 + stkp, status | FLAGS6502::B | FLAGS6502::U);
    stkp--;
    return 0;
}

// PLA (Pull Accumulator) Отримання акумулятора зі стека
uint8_t CPU6502::PLA() {
    stkp++;
    a = read(0x0100 + stkp);
    SetFlag(FLAGS6502::Z, a == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 0;
}

// PLP (Pull Processor Status) Отримання регістра статусу зі стека
uint8_t CPU6502::PLP() {
    stkp++;
    status = read(0x0100 + stkp);
    // Встановлюємо біт U в 1, а біт B скидаємо в 0 за допомогою маски
	// це виправлення бо у тесті у мене вийшло P:FF, а має бути P:EF, тобто B=0, U=1
    status = (status | FLAGS6502::U) & ~FLAGS6502::B;
    return 0;
}

// BRK (Break) Програмне переривання
uint8_t CPU6502::BRK() {
    pc++; // Пропускаємо байт після опкоду BRK

    // Зберігаємо адресу повернення на стек (High, потім Low)
    write(0x0100 + stkp, (pc >> 8) & 0x00FF); stkp--;
    write(0x0100 + stkp, pc & 0x00FF); stkp--;

    // Зберігаємо статус із прапорцем Break
    SetFlag(FLAGS6502::B, true);
    write(0x0100 + stkp, status); stkp--;
    SetFlag(FLAGS6502::B, false); // Скидаємо B після запису

    SetFlag(FLAGS6502::I, true); // Блокуємо переривання

    // Завантажуємо адресу обробника з вектору переривань (0xFFFE)
    pc = (uint16_t)read(0xFFFE) | ((uint16_t)read(0xFFFF) << 8);

    return 0;
}

// --------------------------------------------------------------

//
// Групу порівння (Compare)
//

// CMP (Compare Accumulator) Порівняння акумулятора з пам'яттю
uint8_t CPU6502::CMP() {
    fetch(); // Читаємо дані з пам'яті
    uint16_t temp = (uint16_t)a - (uint16_t)fetched;
    SetFlag(FLAGS6502::C, a >= fetched);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x0000);
    SetFlag(FLAGS6502::N, temp & 0x0080);
    return 1; // Може бути +1 такт за перетин сторінки, але поки що 1
}

// CPX (Compare X Register) Порівняння регістра X з пам'яттю
uint8_t CPU6502::CPX() {
    fetch();
    uint16_t temp = (uint16_t)x - (uint16_t)fetched;
    SetFlag(FLAGS6502::C, x >= fetched);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x0000);
    SetFlag(FLAGS6502::N, temp & 0x0080);
    return 0;
}

// CPY (Compare Y Register) Порівняння регістра Y з пам'яттю
uint8_t CPU6502::CPY() {
    fetch();
    uint16_t temp = (uint16_t)y - (uint16_t)fetched;
    SetFlag(FLAGS6502::C, y >= fetched);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x0000);
    SetFlag(FLAGS6502::N, temp & 0x0080);
    return 0;
}

// =========================================================================
// АПАРАТНЕ ПЕРЕРИВАННЯ NMI (Non-Maskable Interrupt)
// =========================================================================
void CPU6502::nmi() {
    // Зберігаємо поточний лічильник команд у стек (High-Byte, потім Low-Byte)
    write(static_cast<uint16_t>(0x0100) + stkp, static_cast<uint8_t>((pc >> 8) & 0x00FF));
    stkp--;
    write(static_cast<uint16_t>(0x0100) + stkp, static_cast<uint8_t>(pc & 0x00FF));
    stkp--;

    // Налаштовуємо прапорці статусу для апаратного переривання
    SetFlag(FLAGS6502::B, false); // Прапорець Break = 0
    SetFlag(FLAGS6502::U, true);  // Unused завжди = 1
    SetFlag(FLAGS6502::I, true);  // Блокуємо звичайні IRQ під час обробки NMI

    write(static_cast<uint16_t>(0x0100) + stkp, status);
    stkp--;

    // Зчитуємо адресу обробника з жорстко зафіксованого вектора NMI ($FFFA-$FFFB)
    addr_abs = 0xFFFA;
    uint16_t lo = read(addr_abs);
    uint16_t hi = read(addr_abs + 1);
    pc = (hi << 8) | lo;

    // Апаратне переривання NMI в архітектурі 6502 займає рівно 7 тактів CPU
    cycles = 7;
}

// =========================================================================
// АПАРАТНЕ ПЕРЕРИВАННЯ IRQ (Interrupt Request)
// =========================================================================
void CPU6502::irq() {
    // IRQ спрацьовує ТІЛЬКИ якщо прапорець переривань (I) дозволяє це (дорівнює 0)
    // Якщо у тебе прапорець називається інакше (наприклад, DISABLE_INTERRUPTS), заміни 'I' на нього.
    if (GetFlag(FLAGS6502::I) == 0) {
        // Зберігаємо поточний лічильник команд (Program Counter) у стек
        write(0x0100 + stkp, (pc >> 8) & 0x00FF);
        stkp--;
        write(0x0100 + stkp, pc & 0x00FF);
        stkp--;

        // Налаштовуємо регістр статусу
        SetFlag(FLAGS6502::B, 0); // Для апаратних переривань прапорець Break = 0
        SetFlag(FLAGS6502::U, 1); // Невикористаний біт завжди 1
        SetFlag(FLAGS6502::I, 1); // Одразу блокуємо нові переривання, поки обробляємо це

        // Зберігаємо статус у стек
        write(0x0100 + stkp, status);
        stkp--;

        // Читаємо новий вектор (адресу, куди стрибнути) з жорсткої пам'яті $FFFE-$FFFF
        addr_abs = 0xFFFE;
        uint16_t lo = read(addr_abs + 0);
        uint16_t hi = read(addr_abs + 1);
        pc = (hi << 8) | lo;

        // Апаратне переривання займає рівно 7 тактів процесора
        cycles = 7;
    }
}