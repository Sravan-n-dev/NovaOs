#ifndef SHELL_H
#define SHELL_H

#include "types.h"
#include "console.h"
#include "keyboard.h"
#include "timer.h"
#include "multitasking.h"

#define CMD_BUF_SIZE 64

class Shell {
    char cmdBuffer[CMD_BUF_SIZE];
    int cmdLength;
    TaskManager* taskManager;

    void prompt();
    void execute();

    // Command handlers
    void cmdHelp();
    void cmdInfo();
    void cmdTasks();
    void cmdStart();
    void cmdUptime();
    void cmdClear();

    // String comparison helper
    bool strEqual(const char* a, const char* b);

public:
    Shell(TaskManager* tm);

    // Called from the main kernel loop — processes one character at a time
    void update();
};

#endif
