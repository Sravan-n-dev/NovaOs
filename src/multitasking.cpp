#include "multitasking.h"

TaskManager::TaskManager()
    : numTasksCreated(0), currentTask(0), contextSwitches(0), schedulingActive(false)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].id = 0;
        tasks[i].cpuState = 0;
    }

    // Task slot 0 is reserved for the boot/shell context.
    // Its cpuState will be filled in by the first call to Schedule().
    // This task represents the kernelMain execution context.
    tasks[0].id = 0;  // ID 0 = boot/shell
    tasks[0].state = TASK_RUNNING;
    numTasksCreated = 0;  // user-visible tasks start from 1
}

TaskManager::~TaskManager() {}

int TaskManager::CreateTask(TaskFunction entryPoint) {
    // Find a free slot (skip slot 0 which is the boot task)
    int slot = -1;
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return -1;  // no free slots

    // Assign a unique task ID
    numTasksCreated++;
    tasks[slot].id = numTasksCreated;
    tasks[slot].state = TASK_READY;

    // Set up the initial CPU context on this task's stack.
    // When the scheduler switches to this task, int_bottom will
    // pop the CPUState from the stack and iret will jump to entryPoint.
    uint8_t* stackTop = tasks[slot].stack + TASK_STACK_SIZE;
    CPUState* state = (CPUState*)(stackTop - sizeof(CPUState));

    // Zero all general-purpose registers
    state->eax = 0;
    state->ebx = 0;
    state->ecx = 0;
    state->edx = 0;
    state->esi = 0;
    state->edi = 0;
    state->ebp = 0;
    state->pusha_esp = 0;  // ignored by popa
    state->error = 0;      // dummy error code

    // The iret instruction will pop eip, cs, eflags:
    state->eip = (uint32_t)entryPoint;
    state->cs = 0x10;       // kernel code segment selector (GDT offset)
    state->eflags = 0x202;  // IF=1 (interrupts enabled), reserved bit 1 set

    // Data segment registers: kernel data segment
    state->ds = 0x18;
    state->es = 0x18;
    state->fs = 0x18;
    state->gs = 0x18;

    tasks[slot].cpuState = state;
    return tasks[slot].id;
}

uint32_t TaskManager::Schedule(uint32_t esp) {
    // Save the current task's stack pointer
    if (currentTask >= 0 && currentTask < MAX_TASKS) {
        tasks[currentTask].cpuState = (CPUState*)esp;
        if (tasks[currentTask].state == TASK_RUNNING) {
            tasks[currentTask].state = TASK_READY;
        }
    }

    // Round-robin: find the next READY task
    int start = currentTask;
    int next = currentTask;
    for (int i = 0; i < MAX_TASKS; i++) {
        next = (next + 1) % MAX_TASKS;
        if (tasks[next].state == TASK_READY) {
            break;
        }
    }

    // If no READY task found, stay on current
    if (tasks[next].state != TASK_READY) {
        // Restore current task as running
        if (currentTask >= 0 && currentTask < MAX_TASKS) {
            tasks[currentTask].state = TASK_RUNNING;
        }
        return esp;
    }

    // Perform context switch
    if (next != start) {
        contextSwitches++;
    }
    currentTask = next;
    tasks[currentTask].state = TASK_RUNNING;

    // Return the new task's saved stack pointer.
    // int_bottom in interruptstubs.s will do: movl %eax, %esp
    // then pop gs, fs, es, ds, popa, skip error, iret
    // This restores the new task's full CPU context!
    return (uint32_t)tasks[currentTask].cpuState;
}

int TaskManager::GetTaskCount() {
    int count = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_UNUSED)
            count++;
    }
    return count;
}

int TaskManager::GetCurrentTaskId() {
    if (currentTask >= 0 && currentTask < MAX_TASKS)
        return tasks[currentTask].id;
    return -1;
}

int TaskManager::GetContextSwitches() {
    return contextSwitches;
}

TaskState TaskManager::GetTaskState(int index) {
    if (index >= 0 && index < MAX_TASKS)
        return tasks[index].state;
    return TASK_UNUSED;
}

int TaskManager::GetTaskId(int index) {
    if (index >= 0 && index < MAX_TASKS)
        return tasks[index].id;
    return -1;
}

int TaskManager::GetMaxTasks() {
    return MAX_TASKS;
}
