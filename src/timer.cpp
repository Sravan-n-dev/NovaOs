#include "timer.h"
#include "multitasking.h"

volatile uint32_t Timer::ticks = 0;

// PIT frequency: 1193182 Hz base oscillator clock
// We want ~100 Hz periodic timer ticks (10 ms time quantum)
// Divisor = 1193182 / 100 = 11932 (0x2E9C)
#define PIT_FREQUENCY 100
#define PIT_DIVISOR   11932

Timer::Timer(InterruptManager* manager)
    : InterruptHandler(manager, 0x20),  // IRQ0 mapped to interrupt vector 0x20
      channel0(0x40),
      commandPort(0x43),
      taskManager(0)
{
    // Configure PIT Channel 0 in Mode 2 (Rate Generator) with lobyte/hibyte access
    // Control Byte breakdown:
    //   Bits 7-6: Select Channel 0 (00b)
    //   Bits 5-4: Access mode lobyte/hibyte (11b)
    //   Bits 3-1: Operating Mode 2 - Rate Generator (010b)
    //   Bit 0:    16-bit binary counter (0b)
    //   Value:    0b00110100 = 0x34
    commandPort.Write(0x34);

    // Send 16-bit divisor to Channel 0 data port (low byte first, then high byte)
    channel0.Write(PIT_DIVISOR & 0xFF);        // Low byte (0x9C)
    channel0.Write((PIT_DIVISOR >> 8) & 0xFF); // High byte (0x2E)
}

Timer::~Timer() {}

uint32_t Timer::HandleInterrupt(uint32_t esp) {
    ticks++;

    // If a task manager is registered, perform preemptive round-robin scheduling
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
