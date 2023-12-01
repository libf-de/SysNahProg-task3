//
// Created by fsch on 15.11.23.
//

#ifndef MYSHELL_MAIN_H
#define MYSHELL_MAIN_H

#endif //MYSHELL_MAIN_H

int main(int argc, char *argv[]);

void sh_exit();
void sh_add_argument(char *arg);
void sh_add_command(char *cmd);
void sh_set_output(char *file);
void sh_set_input(char *file);
void sh_signal_pipe();
void sh_signal_simple();
void sh_signal_foreground();
void sh_signal_background();

void test();