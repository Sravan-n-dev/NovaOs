#ifndef MULTITASKING_H
#define MULTITASKING_H

#include "types.h"

// Maximum number of tasks (statically allocated)
#define MAX_TASKS 16
// Stack size per task: 4 KiB
#define TASK_STACK_SIZE 4096

// Task states
enum TaskState {
    TASK_READY      = 0,
    TASK_RUNNING    = 1,
    TASK_TERMINATED = 2,
    TASK_UNUSED     = 3
};

// CPU state saved/restored by interruptstubs.s
// This struct MUST match the stack layout created by the interrupt stub:
//   int_bottom pushes: pusha, ds, es, fs, gs
//   Then the stub also pushed: error_code (or dummy $0)
//   The CPU pushed: eip, cs, eflags
//
// Stack layout (top = low address, what %esp points to after int_bottom saves):
//   gs, fs, es, ds,
//   edi, esi, ebp, pusha_esp, ebx, edx, ecx, eax,   (pusha order)
//   error,
//   eip, cs, eflags                                   (CPU pushed)
struct CPUState {
    // Segment registers (pushed by int_bottom)
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    // General-purpose registers (pushed by pusha, in pusha order)
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t pusha_esp;  // ignored by popa
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    // Error code / dummy (pushed by stub macro)
    uint32_t error;

    // Pushed by CPU on interrupt entry
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));

// Task entry point function type
typedef void (*TaskFunction)();

// Task Control Block
struct Task {
    uint8_t    stack[TASK_STACK_SIZE];  // independent kernel stack
    CPUState*  cpuState;               // pointer to saved CPU state on the stack
    int        id;
    TaskState  state;
};

class TaskManager {
    Task       tasks[MAX_TASKS];
    int        numTasksCreated;        // total ever created (for unique IDs)
    int        currentTask;            // index of currently running task
    uint32_t   contextSwitches;        // total context switch count
    bool       schedulingActive;       // set to true once Schedule is first called

public:
    TaskManager();
    ~TaskManager();

    // Create a new task with the given entry function
    // Returns the task ID, or -1 on failure
    int CreateTask(TaskFunction entryPoint);

    // Called from timer interrupt to schedule next task
    // Takes current esp, returns new esp (possibly different task's stack)
    uint32_t Schedule(uint32_t esp);

    // Accessors for shell display
    int GetTaskCount();
    int GetCurrentTaskId();
    int GetContextSwitches();
    TaskState GetTaskState(int index);
    int GetTaskId(int index);
    int GetMaxTasks();
};

#endif
