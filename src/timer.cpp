#include "timer.h"
#include "multitasking.h"

volatile uint32_t Timer::ticks = 0;

// PIT frequency: 1193182 Hz base clock
// We want ~100 Hz timer ticks -> divisor = 1193182 / 100 = 11932
#define PIT_FREQUENCY 100
#define PIT_DIVISOR   11932

Timer::Timer(InterruptManager* manager)
    : InterruptHandler(manager, 0x20),  // IRQ0 = interrupt 0x20
      channel0(0x40),
      commandPort(0x43),
      taskManager(0)
{
    // Configure PIT Channel 0 in mode 2 (rate generator), lobyte/hibyte access
    // Command byte: channel 0 (00), access lobyte/hibyte (11), mode 2 (010), binary (0)
    // = 0b00110100 = 0x34
    commandPort.Write(0x36);
    channel0.Write(PIT_DIVISOR & 0xFF);        // Low byte
    channel0.Write((PIT_DIVISOR >> 8) & 0xFF); // High byte
}

Timer::~Timer() {}

uint32_t Timer::HandleInterrupt(uint32_t esp) {
    ticks++;

    // If a task manager is registered, let it schedule (context switch)
    if (taskManager != 0) {
        esp = taskManager->Schedule(esp);
    }

    return esp;
}

void Timer::SetTaskManager(TaskManager* tm) {
    taskManager = tm;
}

uint32_t Timer::GetTicks() {
    return ticks;
}

uint32_t Timer::GetUptimeSeconds() {
    return ticks / PIT_FREQUENCY;
}
