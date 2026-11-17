#include "shell.h"

// Task entry functions for dynamically created tasks
// Each newly created task runs this loop on its own stack
static void dynamicTaskEntry() {
    while (1) {
        // Busy-wait loop — the timer interrupt will preempt this
        // The task exists and executes, demonstrating real scheduling
        for (volatile int d = 0; d < 100000; d++);
    }
}

Shell::Shell(TaskManager* tm)
    : cmdLength(0), taskManager(tm)
{
    for (int i = 0; i < CMD_BUF_SIZE; i++) cmdBuffer[i] = 0;
}

bool Shell::strEqual(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

void Shell::prompt() {
    console.setColor(COLOR_LGREEN);
    console.print("novaos> ");
    console.setColor(COLOR_WHITE);
}

void Shell::update() {
    if (!KeyboardDriver::hasChar()) return;

    char c = KeyboardDriver::getChar();

    if (c == '\n') {
        console.putChar('\n');
        cmdBuffer[cmdLength] = '\0';
        if (cmdLength > 0) {
            execute();
        }
        cmdLength = 0;
        prompt();
    } else if (c == '\b') {
        if (cmdLength > 0) {
            cmdLength--;
            console.putChar('\b');
        }
    } else if (cmdLength < CMD_BUF_SIZE - 1) {
        cmdBuffer[cmdLength++] = c;
        console.putChar(c);
    }
}

void Shell::execute() {
    if (strEqual(cmdBuffer, "help"))       cmdHelp();
    else if (strEqual(cmdBuffer, "info"))  cmdInfo();
    else if (strEqual(cmdBuffer, "tasks")) cmdTasks();
    else if (strEqual(cmdBuffer, "start")) cmdStart();
    else if (strEqual(cmdBuffer, "uptime")) cmdUptime();
    else if (strEqual(cmdBuffer, "clear")) cmdClear();
    else {
        console.setColor(COLOR_LRED);
        console.print("Unknown command: ");
        console.print(cmdBuffer);
        console.printLine("");
        console.printLine("Type 'help' for available commands.");
        console.setColor(COLOR_WHITE);
    }
}

void Shell::cmdHelp() {
    console.setColor(COLOR_YELLOW);
    console.printLine("Available commands:");
    console.setColor(COLOR_LCYAN);
    console.printLine("  help   - Show this help message");
    console.printLine("  info   - System information");
    console.printLine("  tasks  - List all tasks and states");
    console.printLine("  start  - Create a new kernel task");
    console.printLine("  uptime - Show system uptime");
    console.printLine("  clear  - Clear the screen");
    console.setColor(COLOR_WHITE);
}

void Shell::cmdInfo() {
    console.setColor(COLOR_YELLOW);
    console.printLine("=== NovaOS System Info ===");
    console.setColor(COLOR_WHITE);
    console.print("  Version:      ");
    console.setColor(COLOR_LCYAN); console.printLine("NovaOS 0.1");
    console.setColor(COLOR_WHITE);
    console.print("  Architecture: ");
    console.setColor(COLOR_LCYAN); console.printLine("x86 (i386)");
    console.setColor(COLOR_WHITE);
    console.print("  Mode:         ");
    console.setColor(COLOR_LCYAN); console.printLine("32-bit Protected Mode");
    console.setColor(COLOR_WHITE);
    console.print("  Scheduler:    ");
    console.setColor(COLOR_LGREEN); console.printLine("Round Robin (Active)");
    console.setColor(COLOR_WHITE);
    console.print("  Timer:        ");
    console.setColor(COLOR_LGREEN); console.printLine("PIT @ 100 Hz (Active)");
    console.setColor(COLOR_WHITE);
    console.print("  Keyboard:     ");
    console.setColor(COLOR_LGREEN); console.printLine("PS/2 (Active)");
    console.setColor(COLOR_WHITE);
}

void Shell::cmdTasks() {
    console.setColor(COLOR_YELLOW);
    console.printLine("=== Task List ===");
    console.setColor(COLOR_WHITE);

    console.printLine("  ID    STATE");
    console.printLine("  ----  ----------");

    int maxT = taskManager->GetMaxTasks();
    for (int i = 0; i < maxT; i++) {
        TaskState st = taskManager->GetTaskState(i);
        if (st == TASK_UNUSED) continue;

        int id = taskManager->GetTaskId(i);
        console.print("  ");

        // Slot 0 is the shell/boot context
        if (i == 0) {
            console.print("S");     // 'S' for shell
        } else {
            console.printInt(id);
        }

        // Pad
        if (id < 10 || i == 0) console.print("     ");
        else                    console.print("    ");

        switch (st) {
            case TASK_RUNNING:
                console.setColor(COLOR_LGREEN);
                console.print("RUNNING  <--");
                break;
            case TASK_READY:
                console.setColor(COLOR_LCYAN);
                console.print("READY");
                break;
            case TASK_TERMINATED:
                console.setColor(COLOR_DGRAY);
                console.print("TERMINATED");
                break;
            default:
                console.print("???");
        }
        console.setColor(COLOR_WHITE);
        console.printLine("");
    }

    console.print("\n  Total tasks: ");
    console.printInt(taskManager->GetTaskCount());
    console.print("  |  Context switches: ");
    console.printInt(taskManager->GetContextSwitches());
    console.printLine("");
}

void Shell::cmdStart() {
    int id = taskManager->CreateTask(dynamicTaskEntry);
    if (id < 0) {
        console.setColor(COLOR_LRED);
        console.printLine("Error: No free task slots available.");
        console.setColor(COLOR_WHITE);
    } else {
        console.setColor(COLOR_LGREEN);
        console.print("Task ");
        console.printInt(id);
        console.printLine(" created successfully.");
        console.setColor(COLOR_WHITE);
    }
}

void Shell::cmdUptime() {
    uint32_t secs = Timer::GetUptimeSeconds();
    uint32_t ticks = Timer::GetTicks();

    console.print("  Uptime: ");
    console.printInt(secs / 60);
    console.print("m ");
    console.printInt(secs % 60);
    console.print("s  (");
    console.printInt(ticks);
    console.printLine(" ticks)");
}

void Shell::cmdClear() {
    console.clear();
    prompt();
}
