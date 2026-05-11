#include <iostream>
#include "Bus.h"
#include "CPU.h"

// Це виправить твою помилку SDL_main
#define SDL_MAIN_HANDLED 

int main(int argc, char* argv[]) {
    // Створюємо екземпляри твоїх систем
    Bus nesBus;
    CPU6502 nesCpu;

    // З'єднуємо їх за твоїм планом
    nesCpu.ConnectBus(&nesBus);

    // Скидаємо процесор (тут спрацює твій код із Reset Vector)
    nesCpu.reset();

    std::cout << "YCsys NES Core: System Initialized." << std::endl;
    std::cout << "Code that works, not just exists." << std::endl;

    return 0;
}