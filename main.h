#include <vector>
//
// Created by fsch on 15.11.23.
//

#ifndef MYSHELL_MAIN_H
#define MYSHELL_MAIN_H

#endif //MYSHELL_MAIN_H

struct Command {
    std::vector<const char *> cmdline;
    int input{};
    int output{};

    Command(std::vector<const char*> cmdline, int input, int output) : cmdline(std::move(cmdline)), input(input), output(output) {}
};

int main(int argc, char *argv[]);

void execute_command_in_pipe(const Command& cmd, int in, int out);

void sh_exit();
void sh_add_argument(char *arg);
void sh_add_command(char *cmd);
bool sh_set_output(char *file);
bool sh_set_input(char *file);
void sh_signal_pipe();
void sh_signal_simple();
void sh_signal_foreground();
void sh_signal_background();

void test();