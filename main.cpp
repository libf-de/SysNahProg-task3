#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <cstring>
#include "parse.tab.h"
#include "lex.yy.h"
#include "main.h"
#include <unistd.h>
#include <climits>
#include <sys/wait.h>
#include <algorithm>
#include <pwd.h>
#include <map>

extern FILE *yyin;  // Externe Variable, die vom yacc-Parser verwendet wird

std::vector<Command> pipe_commands;
std::map<std::string, std::vector<const char *>> aliases;

std::string getCurrentWorkingDir() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string workingDir(cwd);
        std::string homeDir(getenv("HOME"));

        // Ersetzen des Home-Verzeichnisses durch '~', falls vorhanden
        size_t homeDirPos = workingDir.find(homeDir);
        if (homeDirPos == 0) {
            workingDir.replace(0, homeDir.length(), "~");
        }

        return workingDir;
    } else {
        perror("getcwd() error");
        return "";
    }
}

void show_prompt() {
    char hostname[1024];
    gethostname(hostname, 1024);

    printf("\033[1;31m%s@%s\033[0m:\033[1;36m%s\033[0m\033[1m → \033[0m", getenv("USER"), hostname, getCurrentWorkingDir().c_str());

    // read string from stdin
    char *str = nullptr;
    size_t size = 0;
    getline(&str, &size, stdin);

    yyin = fmemopen(str, size, "r");
    yyrestart(yyin);

    // run parser on string
    yyparse();

    /*fclose(yyin);*/
    free(str);
}

int main(int argc, char *argv[])
{
    aliases["ll"] = std::vector<const char *>{ "ls", "-l" };


    while(true) {
        show_prompt();
    }
    return 0;
}

void sh_add_argument(char *arg) {
    pipe_commands.back().cmdline.emplace_back(arg);
}

void sh_add_command(char *cmd) {
    pipe_commands.emplace_back(std::vector<const char*>{ cmd }, 0, 1);
}

bool is_number(const char * input) {
    for (int i = 0; i < strlen(input); i++) {
        if(!isdigit(input[i])) {
            return false;
        }
    }
    return true;
}

int sigstr_to_int(const char* sigstr) {
    if(is_number(sigstr)) {
        return std::stoi(sigstr);
    } else {
        // convert sigstr to lowercase std::string
        std::string sigstr_lower(sigstr);
        std::transform(sigstr_lower.begin(), sigstr_lower.end(), sigstr_lower.begin(), ::tolower);

        if(sigstr_lower == "hup") return SIGHUP;
        else if(sigstr_lower == "int") return SIGINT;
        else if(sigstr_lower == "quit") return SIGQUIT;
        else if(sigstr_lower == "ill") return SIGILL;
        else if(sigstr_lower == "trap") return SIGTRAP;
        else if(sigstr_lower == "abrt") return SIGABRT;
        else if(sigstr_lower == "iot") return SIGIOT;
        else if(sigstr_lower == "bus") return SIGBUS;
        else if(sigstr_lower == "fpe") return SIGFPE;
        else if(sigstr_lower == "kill") return SIGKILL;
        else if(sigstr_lower == "usr1") return SIGUSR1;
        else if(sigstr_lower == "segv") return SIGSEGV;
        else if(sigstr_lower == "usr2") return SIGUSR2;
        else if(sigstr_lower == "pipe") return SIGPIPE;
        else if(sigstr_lower == "alrm") return SIGALRM;
        else if(sigstr_lower == "term") return SIGTERM;
        else if(sigstr_lower == "stkflt") return SIGSTKFLT;
        else if(sigstr_lower == "chld") return SIGCHLD;
        else if(sigstr_lower == "cont") return SIGCONT;
        else if(sigstr_lower == "stop") return SIGSTOP;
        else if(sigstr_lower == "tstp") return SIGTSTP;
        else if(sigstr_lower == "ttin") return SIGTTIN;
        else if(sigstr_lower == "ttou") return SIGTTOU;
        else if(sigstr_lower == "urg") return SIGURG;
        else if(sigstr_lower == "xcpu") return SIGXCPU;
        else if(sigstr_lower == "xfsz") return SIGXFSZ;
        else if(sigstr_lower == "vtalrm") return SIGVTALRM;
        else if(sigstr_lower == "prof") return SIGPROF;
        else if(sigstr_lower == "winch") return SIGWINCH;
        else if(sigstr_lower == "io") return SIGIO;
        else if(sigstr_lower == "pwr") return SIGPWR;
        else if(sigstr_lower == "sys") return SIGSYS;
        else return -1;
    }
}

int builtin_alias(const Command& cmd) {
    if (cmd.cmdline.size() == 1) { //print all aliases
        for (auto &alias: aliases) {
            std::cout << alias.first << " = ";
            for (auto &a_cmd: alias.second) {
                std::cout << a_cmd << " ";
            }
            std::cout << std::endl;
        }
        return EXIT_SUCCESS;
    } else if (cmd.cmdline.size() == 2) { //print an alias
        std::string aliascmd(cmd.cmdline[1]);
        if (aliases.find(aliascmd) != aliases.end()) {
            std::cout << aliascmd << " = ";
            for (auto &a_cmd: aliases[aliascmd]) {
                std::cout << a_cmd << " ";
            }
            std::cout << std::endl;
            return EXIT_SUCCESS;
        } else {
            return EXIT_FAILURE;
        }
    } else if (cmd.cmdline.size() >= 3) { //set an alias
        std::string alias(cmd.cmdline[1]);

        aliases[alias] = std::vector<const char *>();
        aliases[alias].insert(aliases[alias].end(), cmd.cmdline.begin() + 2, cmd.cmdline.end());

        //print aliases
        for (auto &aliase: aliases) {
            std::cout << aliase.first << " = ";
            for (auto &a_cmd: aliase.second) {
                std::cout << a_cmd << " ";
            }
            std::cout << std::endl;
        }
        return EXIT_SUCCESS;
    }
}

int builtin_unalias(const Command& cmd) {
    if (cmd.cmdline.size() == 2) {
        if (aliases.find(cmd.cmdline[1]) != aliases.end()) {
            aliases.erase(cmd.cmdline[1]);
            return EXIT_SUCCESS;
        } else {
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "unalias: not enough arguments" << std::endl;
        return EXIT_FAILURE;
    }
}

int builtin_cd(const Command& cmd) {
    if(cmd.cmdline.size() == 2) {
        if(chdir(cmd.cmdline[1]) == -1) {
            std::cerr << "error: " << strerror(errno) << ": " << cmd.cmdline[1] << std::endl;
            return EXIT_FAILURE;
        } else {
            return EXIT_SUCCESS;
        }
    } else if(cmd.cmdline.size() == 1) {
        struct passwd *pw = getpwuid(getuid());
        if(chdir(pw->pw_dir) == -1) {
            std::cerr << "error: " << strerror(errno) << ": " << cmd.cmdline[1] << std::endl;
            return EXIT_FAILURE;
        } else {
            return EXIT_SUCCESS;
        }
    }
}

void run_command_or_builtin(Command cmd) {
    std::string command(cmd.cmdline[0]);
    if(command == "cd") {
        builtin_cd(cmd);
    } else if(command == "pwd") {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            printf("%s\n", cwd);
            exit(EXIT_SUCCESS);
        } else {
            std::cerr << "error: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if(command == "kill") {
        if (cmd.cmdline.size() == 3) {
            if (kill(std::stoi(cmd.cmdline[1]), sigstr_to_int(cmd.cmdline[2])) == -1) {
                std::cerr << "error: " << strerror(errno) << ": " << cmd.cmdline[1] << std::endl;
                exit(EXIT_FAILURE);
            } else {
                exit(EXIT_SUCCESS);
            }
        } else {
            std::cerr << "kill: not enough arguments" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if(command == "alias") {
        builtin_alias(cmd);
    } else if(command == "unalias") {
        builtin_unalias(cmd);
    } else {
        std::string executable(cmd.cmdline[0]);
        // check if aliases contains cmd.cmdline[0]
        if(aliases.find(executable) != aliases.end()) {
            // remove first element from cmd.cmdline
            cmd.cmdline.erase(cmd.cmdline.begin());

            // prepend alias to cmd.cmdline
            cmd.cmdline.insert(cmd.cmdline.begin(), aliases[executable].begin(), aliases[executable].end());
        }

        execvp(cmd.cmdline[0], const_cast<char *const *>(cmd.cmdline.data()));
        exit(EXIT_FAILURE);
    }
}

void push_command() {
    //insert the command into pipe_commands
}

bool sh_set_output(char *file) {
    if(file == nullptr) {
        std::cerr << "error: output file is null" << std::endl;
        return false;
    }

    int out_fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(out_fd < 0) {
        std::cerr << "error opening file: " << strerror(errno) << ": " << file << std::endl;
        return false;
    } else {
        pipe_commands.back().output = out_fd;
        return true;
    }
}

bool sh_set_input(char *file) {
    if(file == nullptr) {
        std::cerr << "error: input file is null" << std::endl;
        return false;
    }

    int in_fd = open(file, O_RDONLY);

    if(in_fd < 0) {
        std::cerr << "error opening file: " << strerror(errno) << ": " << file << std::endl;
        return false;
    } else {
        pipe_commands.back().input = in_fd;
        return true;
    }
}

void sh_exit() {
    printf("exit\n");
    exit(0);
}

// Gets called on the trailing arguments if pipes are used
void sh_signal_pipe() {
    //printf("signal_pipe\n");
    push_command();
}

// Always gets called after the first command
void sh_signal_simple() {
    //printf("signal_simple\n");
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
    std::string command(cmd.cmdline[0]);
    if(command == "alias") {
        builtin_alias(cmd);
        return;
    } else if(command == "unalias") {
        builtin_unalias(cmd);
        return;
    } else if(command == "cd") {
        builtin_cd(cmd);
        return;
    }


    if(fork() == 0) {
        // Set command input and output file descriptors
        if(cmd.input == 0 && in != 0) {
            // From Pipe
            dup2(in, 0);
        } else if(cmd.input != 0) {
            // From File
            dup2(cmd.input, 0);
        } // Else: From stdin

        if(cmd.output == 1 && out != 1) {
            // To Pipe
            dup2(out, 1);
        } else if(cmd.output != 1) {
            // To File
            dup2(cmd.output, 1);
        } // Else: To stdout

        //execvp(cmd.cmdline[0], const_cast<char *const *>(cmd.cmdline.data()));
        run_command_or_builtin(cmd);
        exit(EXIT_FAILURE);
    }
}

void execute_commands(bool wait_for_child = true) {
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
    if(wait_for_child)
        while (wait(nullptr) > 0);

    pipe_commands.clear();
}

void sh_signal_foreground() {
    //printf("signal_foreground\n");
    execute_commands();
}

//TO DEBUG CHILDS: switch to gdb tab, enter: set follow-fork-mode child  |AND|  set detach-on-fork off

void sh_signal_background() {
    printf("Running in Background -> WIP!\n");
    execute_commands(false);
}
