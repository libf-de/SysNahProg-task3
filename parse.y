/* -*- indented-text -*- */

%{
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "main.h"

int yylex(void);
void yyerror(char *s) {
    fprintf(stderr, "error: %s\n", s);
}

%}

%union {
       char *string;
}


%start cmd_line
%token <string> EXIT PIPE INPUT_REDIR OUTPUT_REDIR STRING NL BACKGROUND


%%
cmd_line    :
        | EXIT             { sh_exit(); }
        | pipeline back_ground
        ;

back_ground : BACKGROUND        { sh_signal_background(); }
        |                       { sh_signal_foreground(); }
        ;

simple      : command redir
        ;

command     : command STRING
                {
                sh_add_argument($2);
                }
        | STRING
                {
                sh_add_command($1);
                }
        ;

redir       : input_redir output_redir
        ;

output_redir:    OUTPUT_REDIR STRING
                {
                sh_set_output($2);
                }
        |        /* empty */
				{
				sh_set_output(nullptr);
				}
        ;

input_redir:    INPUT_REDIR STRING
                {
                sh_set_input($2);
                }
        |       /* empty */
                {
                sh_set_input(nullptr);
				}
        ;

pipeline    : pipeline PIPE simple
                {
                sh_signal_pipe();
                }
        | simple
                {
                sh_signal_simple();
                }
        ;
%%