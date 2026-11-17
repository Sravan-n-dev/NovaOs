#ifndef TIMER_H
#define TIMER_H

#include "types.h"
#include "port.h"
#include "interrupts.h"

// Forward declaration
class TaskManager;

class Timer : public InterruptHandler {
    Port8Bit channel0;
    Port8Bit commandPort;

    static volatile uint32_t ticks;
    TaskManager* taskManager;    // set after scheduler is initialized

public:
    Timer(InterruptManager* manager);
    ~Timer();

    virtual uint32_t HandleInterrupt(uint32_t esp);

    void SetTaskManager(TaskManager* tm);

    static uint32_t GetTicks();
    static uint32_t GetUptimeSeconds();
};

#endif
