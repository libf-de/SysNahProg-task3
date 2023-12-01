#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
//#include <bits/fcntl.h>
#include <fcntl.h>
#include <cstring>
#include "parse.tab.h"
#include "main.h"
#include <unistd.h>
#include <climits>
#include <sys/wait.h>

extern FILE *yyin;  // Externe Variable, die vom yacc-Parser verwendet wird

std::vector<Command> pipe_commands;

std::vector<const char*> current_command_buf;
int current_command_input = 0;
int current_command_output = 1;

void show_prompt() {
    char *username = getenv("USER");
    char *hostname = getenv("HOSTNAME");
    printf("%s@%s → ", username, hostname);

    // read string from stdin
    char *str = nullptr;
    size_t size = 0;
    getline(&str, &size, stdin);

    yyin = fmemopen(str, size, "r");

    // run parser on string
    int result = yyparse();

    if(result != 0) {
        printf("error parsing input: %d\n", result);
    }
}

int main(int argc, char *argv[])
{
    while(true) {
        show_prompt();
    }

    return 0;
}

void test() {
    printf("hallo test");
}

void sh_add_argument(char *arg) {
    /*current_command_buf.emplace_back(arg);*/
    pipe_commands.back().cmdline.emplace_back(arg);
    //current_command_buf += std::string(" ") + std::string(arg);
    printf("add_arg: %s\n", arg);
}

void sh_add_command(char *cmd) {
    pipe_commands.emplace_back(std::vector<const char*>{ cmd }, 0, 1);
    /*//current_command_buf = std::string(cmd);
    current_command_buf.clear();
    current_command_buf.emplace_back(cmd);
    current_command_input = 0;
    current_command_output = 1;*/
    printf("add_cmd: %s\n", cmd);
}

void push_command() {
    //insert the command into pipe_commands
    //pipe_commands.push_back(Command{current_command_buf, current_command_input, current_command_output});

    printf("pushed command to set: ");

    for(auto &arg : pipe_commands.back().cmdline) {
        printf("%s ", arg);
    }
    printf("\n");
}

void sh_set_output(char *file) {
    if(file == nullptr) return;
    current_command_output = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(current_command_output < 0) {
        printf("error opening file: %s\n", strerror(errno));
    } else {
        printf("set_output: %s\n", file);
    }
}

void sh_set_input(char *file) {
    if(file == nullptr) return;
    current_command_input = open(file, O_RDONLY);
    if(current_command_input < 0) {
        printf("error opening file: %s\n", strerror(errno));
    } else {
        printf("set_input: %s\n", file);
    }
}

void sh_exit() {
    printf("exit\n");
    exit(0);
}

// Gets called on the trailing arguments if pipes are used
void sh_signal_pipe() {
    printf("signal_pipe\n");
    push_command();
}

// Always gets called after the first command
void sh_signal_simple() {
    printf("signal_simple\n");
    push_command();

}

void print_to_terminal(const char* message) {
    int fd = open("/dev/tty", O_WRONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    write(fd, message, strlen(message));
    close(fd);
}

void execute_command_in_pipe(const Command& cmd, int in, int out) {
    if(fork() == 0) {
        if(cmd.input == 0 && in != 0) {
            // From Pipe
            dup2(in, 0);
        }

        if(cmd.output == 1 && out != 1) {
            // To Pipe
            dup2(out, 1);
        }

        /*print_to_terminal("EXEC: ");

        for (auto &arg : cmd.cmdline) {
            print_to_terminal(arg);
            print_to_terminal(" ");
        }

        print_to_terminal("\n");*'*/

        execvp(cmd.cmdline[0], const_cast<char *const *>(cmd.cmdline.data()));
        exit(EXIT_FAILURE);
    }
}


void execute_command_pipe() {

}

void execute_commands() {
    int cmd_index = 0;
    int in = 0;
    int fd[2];

    for(auto &command : pipe_commands) {
        if (cmd_index++ < pipe_commands.size() - 1) {
            // Not last command
            pipe(fd);

            execute_command_in_pipe(command, in, fd[1]);

            close(fd[1]);
            in = fd[0];
        } else {
            // Last command
            execute_command_in_pipe(command, in, 1);
        }
    }

    // Schließt die letzte offene Lese-Seite der Pipe, falls vorhanden
    if (in != STDIN_FILENO) {
        close(in);
    }

    // Warten auf die Beendigung aller Kindprozesse
    while (wait(NULL) > 0);

    pipe_commands.clear();
}

void _execute_commands() {
    int cmd_index = 0;

    printf("\n--------------------------\n");

    for(auto &command : pipe_commands) {
        if(command.input != 0) {
            //get filename from filedescriptor
            char path[PATH_MAX];
            ssize_t len;

            len = readlink(
                    (std::string("/proc/self/fd/") + std::to_string(command.input)).c_str(),
                    path,
                    sizeof(path) - 1);

            if (len == -1) {
                perror("readlink");
                exit(EXIT_FAILURE);
            }
            path[len] = '\0';

            printf("INPUT = %s\n", path);
        } else {
            if(cmd_index++ == 0) {
                printf("INPUT = stdin\n");
            } else {
                printf("INPUT = ↑\n");
            }
        }
        //printf("COMMAND = %s\n", command.cmdline.c_str());

        if(command.output != 1) {
            //get filename from filedescriptor
            char path[PATH_MAX];
            ssize_t len;

            len = readlink(
                    (std::string("/proc/self/fd/") + std::to_string(command.output)).c_str(),
                    path,
                    sizeof(path) - 1);

            if (len == -1) {
                perror("readlink");
                exit(EXIT_FAILURE);
            }
            path[len] = '\0';

            printf("OUTPUT = %s\n", path);
        } else {
            int last_index = pipe_commands.size();
            if(cmd_index == last_index) {
                printf("OUTPUT = stdout\n");
            } else {
                printf("OUTPUT = ↓\n");
            }
        }

        printf("--------------------------\n");
    }

    pipe_commands.clear();
}

void sh_signal_foreground() {
    printf("signal_foreground\n");
    execute_commands();
}

void sh_signal_background() {
    printf("signal_background\n");
    execute_commands();
}
