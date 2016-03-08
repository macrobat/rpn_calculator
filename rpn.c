#include <stdio.h>
#include <string.h>         // strncpy()
#include "rpnstack.h"
#include "rpnfunctions.h"

// rpn.c
// a reverse polish notation calculator
// gcc rpnstack.c rpnfunctions.c rpn.c -lm -o rpn

int main(int argc, char* argv[]) {
    stack_t *rpn_stacks[3];
    rpn_stacks[I_STK ] = stack_create(sizeof(RPN_T));   // 0 interactive stack
    rpn_stacks[H_NUMS] = stack_create(sizeof(RPN_T));   // 1 history nums
    rpn_stacks[H_CMDS] = stack_create(sizeof(token_t)); // 2 history cmds

    char *inputbuf = malloc(BUFSIZ); // [8192] here

    if (argc == 1) {
        // interactive mode
        printmsg(HELP); // not printmsg_fresh(), let user repeat first help cmd
        int quit = 0;
        while (!quit) {
            // separator, optional history stacks, interactive stack, prompt
            display(&hist_flag, rpn_stacks);
            fgets(inputbuf, BUFSIZ, stdin); // get a line of input
            // prompt, read input line, operations, print messages
            quit = handle_input(&hist_flag, inputbuf, rpn_stacks);
        }
    } else {
        // batch mode
        // strncpy() instead of fgets() the inputbuf
        // no display(). don't print anything but the stack contents
        p_printmsg_fresh = donot_printmsg;
        p_printmsg = donot_printmsg;

        int i;
        for (i = 1; i < argc; i++) {
            strncpy(inputbuf, argv[i], BUFSIZ - 1);
            inputbuf[BUFSIZ - 1] = '\0';
            if (handle_input(&hist_flag, inputbuf, rpn_stacks)) {
                printmsg(QUIT);
                break;
            }
            dump_stack(rpn_stacks[I_STK]);
        }
    }

    free(inputbuf);
    stack_destroy(rpn_stacks[I_STK ]);
    stack_destroy(rpn_stacks[H_NUMS]);
    stack_destroy(rpn_stacks[H_CMDS]);

    return 0;
}

