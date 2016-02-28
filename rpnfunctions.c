#include <stdio.h>      // fgets()
#include <stdlib.h>     // strtold()
#include <string.h>     // memmove() memcpy() for rotate()
#include <math.h>       // for ^ powl(). link with -lm
// #include <float.h>   // no. limits just for msg, works for this machine
// #include <errno.h>   // in stack.c too. inf is better than error msgs
#include <fenv.h>       // man fenv; man math_error
#include "rpnstack.h"
#include "rpnfunctions.h"

// rpnfunctions.c
// a reverse polish notation calculator

/* --- comments ----------------------------------------------------------------

sizeof(double): 8
[ ± DBL_MIN:  ± 2.22507e-308 ]
[ ± DBL_MAX:  ± 1.79769e+308 ]

sizeof(long double): 16
[ ± LDBL_MIN: ± 3.3621e-4932  ]
[ ± LDBL_MAX: ± 1.18973e+4932 ]

LDBL_MIN can be divided by 1e19 but not 1e20

number of digits, a very sloppy, nonmath investigation:
#> 1.234567890
displays as
   0: 1.23457   # note the 67 rounded up
when 1.23457 is subtracted:
   0: -2.11e-06

enabling strtold error could be a compile -Doption
#> 1e4933
with perror():
    strtold: Numerical result out of range
without:
   0: inf

handles some hex
#> 0xff
   0: 255
#> 0x.b
   0: 0.6875

printf("HUGE_VALL: %Lg\n", HUGE_VALL); prints inf. needs math.h
memset(inputbuf, '\0', BUFSIZ); // unnecessary
bzero() is deprecated, was legacy already in 2001
fgets() reads in at most one less than size characters from stream
    and places '\0' last
if (errno) {perror("fgets() error"); exit(EXIT_FAILURE);}
strtold(): 0 can be returned on both success and failure
    not checking errno
if (errno) {perror("strtold() error"); exit(EXIT_FAILURE);}

using strcspn(tokenchars, &tok0) is buggy. a while loop works
strtok_r() is overkill. strtok() is enough

printmsg()
there's a wrapper around it and a global to avoid repetition
want math errors repeated, because the causing funs also change the stack

1 e l gives 0.9999999998, must use H_NUMS

--- TODO / IDEAS --------------------------------------

root v is a visual pun on ^

does it matter that not all branches in the program are reachable?

sin cos tan? constant pi p

a command to reset the stacks? keep elemsz, free data, 0u other members

undo could print what cmd is undone

printmsg() can be spammy, have a toggle for it?

is_zero(). what can a long double zero look like? would be obviated by flex
(not very important. false positives don't matter much here)
0 preceded by [+-] fails to insert, interpreted as the operations [+-]

flex/bison
    would be a rewrite in stages
        1 just tokenize()
        2 the loop around tokenize()
        3 handle_input() and inputbuf
        4 replace the flow of the program

readline? C-w and C-u work

one arg or file input batch mode

--- example input --------------------------------------------------------------

toggle hist. undo / stack too small for all cmds whose minsize > 0
math errors, should repeat
a few numbers
print multiline msgs, should be no msg repeats on consecutive cmds
functions
nonsense chars
all branches undo, getting all the opposites on nonhists.
toggle hist
end

t     _     * + ^ / -  v    e l     ~ i c d s r u
0 i 0 i 1.18973e+4932 i _ c ^ - -9 2 v
0x0 -1.1e1 2.2e-2 3.3 4e400 5 6 7 8 9
h h h n n n n
* + ^ / - v e l ~ i c d s r u
a b f g j k m o p x y z
10 _ * _ ^ _ c _ d _ s _ r _ u _
t
q

*/
// --- display -----------------------------------------------------------------

// checking has_msg twice, could check for not JUNK (from math_err)
void printmsg(token_t msgcode) {
    if (!funrows[msgcode].has_msg) { return; }
    if (funrows[msgcode].tok) { // don't want a superfluous space
        printf("%c ", funrows[msgcode].tok);
    }
    printf("%s\n", funrows[msgcode].name);
    if (msgcode == HELP || msgcode == RANG) {
        printf("%s\n", multiline_messages[msgcode - HELP]);
    }
}

// wrapper guarantees freshness. return if seal is broken
void printmsg_fresh(token_t msgcode) {
    if (!funrows[msgcode].has_msg) { return; }
    if (msgcode == last_msg) {
        return;
    } else {
        token_t *fresh_msg = &last_msg;
        *fresh_msg = msgcode;
        printmsg(msgcode);
    }
}


// format string RPN_FMT for long double is "%.10Lg"
void print_num(void *itemp) {
    printf(RPN_FMT, *(RPN_T*)itemp);
}

void print_cmdname(void *itemp) {
    printf("%s", funrows[*(token_t*)itemp].name);
}


// prints the stack from the bottom to the top. display_len set in rpn.c
void display_stack(void (*print_item)(void*),
                   void *itemp,
                   size_t display_len,
                   stack_t *stk)
{
    size_t stksize = stack_size(stk);
    size_t z = 0u;
    if (stksize > display_len) {
        z = stksize - display_len;
    }
    while (z < stksize) { // z < display_len
        printf("%4zu: ", stksize - z - 1u); // pad the index
        stack_peek(itemp, z, stk);
        print_item(itemp);
        printf("\n");
        z++;
    }
}


void display_history(size_t items_display_limit, stack_t *stks[]) {
    if (!stack_empty(stks[H_NUMS])) {
        puts("history numbers:");
        RPN_T num;
        display_stack(print_num, &num, items_display_limit, stks[H_NUMS]);
        puts("");
    }
    if (!stack_empty(stks[H_CMDS])) {
        puts("history cmds:");
        token_t cmd;
        display_stack(print_cmdname, &cmd, items_display_limit, stks[H_CMDS]);
        puts(hist_sep);
        puts("");
    }
}


// print: separator, optional history stacks, interactive stack, prompt
void display(size_t display_len, stack_t *stks[]) {
    puts(display_sep);
    if (hist_flag) {
        display_history(display_len, stks);
    }
    if (!stack_empty(stks[I_STK ])) {
        RPN_T num;
        display_stack(print_num, &num, display_len, stks[I_STK ]);
    }
    printf("%s", rpn_prompt); // "#> "
}


// --- operations * + / - ^ v e l ----------------------------------------------
// with long doubles: can return and use inf and nan

// binary operations
RPN_T mul(RPN_T x, RPN_T y) {
    return x * y;
}

RPN_T add(RPN_T x, RPN_T y) {
    return x + y;
}

// can handle floating point division with 0.0 or 0.0L. returns inf
RPN_T divi(RPN_T x, RPN_T y) {
    return x / y;
}

// sub is the binary operation subtract, not neg() ~
RPN_T sub(RPN_T x, RPN_T y) {
    return x - y;
}

RPN_T powe(RPN_T x, RPN_T y) {
    return powl(x, y);
}

// root, radical anti-power x^(1/y)
// a "2 v" input means square root
RPN_T root(RPN_T x, RPN_T y) {
    return powl(x, RPN_ONE / y); // 1.0L
}


// unary operations EXPE x, LOGN l
RPN_T expe(RPN_T x) {
    return expl(x);
}

RPN_T logn(RPN_T x) {
    return logl(x);
}

// --- commands ----------------------------------------------------------------

// negate, unary minus
void neg(stack_t *stk) {
    RPN_T tmp;
    stack_pop(&tmp, stk);
    tmp = - tmp;
    stack_push(&tmp, stk);
}

// invert
void inve(stack_t *stk) {
    RPN_T tmp;
    stack_pop(&tmp, stk);
    tmp = RPN_ONE / tmp;
    stack_push(&tmp, stk);
}

void copy(stack_t *stk) {
    RPN_T tmp;
    stack_top(&tmp, stk);
    stack_push(&tmp, stk);
}

// DISC d discard would be just a pop
// and then you'd have to push to H_NUMS anyway

void swap(stack_t *stk) {
    RPN_T topnum;
    RPN_T nextnum;
    stack_pop(&topnum, stk);
    stack_pop(&nextnum, stk);
    stack_push(&topnum, stk);
    stack_push(&nextnum, stk);
}

// HTOG t
void toggle_hist_flag(void) {
    int *flagp = &hist_flag;
    *flagp = !(*flagp);
}

// DUMP w
// print the contents of the stack
void dump_stack(stack_t *stks[]) {
    size_t lim = stack_size(stks[I_STK ]);
    size_t z;
    RPN_T item;
    for (z = 0u; z < lim; z++) {
        stack_peek(&item, z, stks[I_STK ]);
        print_num(&item);
        printf(" ");
    }
    puts("");
}

void noop(void) { return; }


// --- roll stack up or down ---------------------------------------------------

// there are atleast 2 elements when called
void roll_stack(int direction, stack_t *stk) {
    void *tmp = malloc(stk->elemsz);
    if (tmp == NULL) {
        puts("Failed to roll stack");
        exit(EXIT_FAILURE);
    }
    size_t blocksize = (stk->index - 1u) * stk->elemsz;
    if (direction == 1) {           // down
        memcpy(tmp, stk->data + blocksize, stk->elemsz);
        memmove(stk->data + stk->elemsz, stk->data, blocksize);
        memcpy(stk->data, tmp, stk->elemsz);
    } else if (direction == -1) {   // up
        memcpy(tmp, stk->data, stk->elemsz);
        memmove(stk->data, stk->data + stk->elemsz, blocksize);
        memcpy(stk->data + blocksize, tmp, stk->elemsz);
     } // else fail silently
    free(tmp);
}


void rold(stack_t *stk) {
    roll_stack(1, stk);
}

void rolu(stack_t *stk) {
    roll_stack(-1, stk);
}

// --- handle input, use stacks, print msgs ------------------------------------

// undo is for restoring I_STK to a previous state
// only for the functions < UNDO
void undo(stack_t *stks[]) {
    if (stack_empty(stks[H_CMDS])) {
        printmsg_fresh(SMLU);
        return;
    }
    printmsg_fresh(UNDO);
    token_t cmd;
    RPN_T tmp;
    stack_pop(&cmd, stks[H_CMDS]);
    if (cmd == NUM || cmd == COPY) { // testing COPY before other nonhists
        stack_pop(&tmp, stks[I_STK]);
    } else if (funrows[cmd].type == BINARY) { //  * + ^ / - v
        RPN_T topnum;
        RPN_T nextnum;
        stack_pop(&tmp,      stks[I_STK ]);
        stack_pop(&topnum,   stks[H_NUMS]);
        stack_pop(&nextnum,  stks[H_NUMS]);
        stack_push(&topnum,  stks[I_STK ]);
        stack_push(&nextnum, stks[I_STK ]);
    } else if (funrows[cmd].type == UNARY) {  // e l
        stack_pop(&tmp,      stks[I_STK ]);
        stack_pop(&tmp,      stks[H_NUMS]);
        stack_push(&tmp,     stks[I_STK ]);
    } else if (funrows[cmd].type == NONHIST && cmd != COPY) {
        // ~ i s r u. disc() as anti for copy() wouldn't be useful
        nonhistp = funrows[cmd].anti;
        nonhistp(stks[I_STK]);
    } else if (cmd == DISC) {
        stack_pop(&tmp, stks[H_NUMS]);
        stack_push(&tmp, stks[I_STK]);
    }
}


// H_NUMS will have numbers in pairwise reverse order of I_STK
void call_binary(token_t cmd, stack_t *stks[]) {
    RPN_T topnum;
    RPN_T nextnum;
    stack_pop(&topnum,   stks[I_STK ]);
    stack_pop(&nextnum,  stks[I_STK ]);
    stack_push(&topnum,  stks[H_NUMS]);
    stack_push(&nextnum, stks[H_NUMS]);
    binaryp = funrows[cmd].fun;
    RPN_T resnum = binaryp(nextnum, topnum); // arg order is important
    stack_push(&resnum,  stks[I_STK ]);
}


void call_unary(token_t cmd, stack_t *stks[]) {
        RPN_T operand;
        stack_pop(&operand,  stks[I_STK ]);
        stack_push(&operand, stks[H_NUMS]);
        unaryp = funrows[cmd].fun;
        RPN_T result = unaryp(operand);
        stack_push(&result,  stks[I_STK ]);
}


token_t math_error(void) {
    if (fetestexcept(FE_DIVBYZERO)) {
        return DBYZ; // 0 i
    } else if (fetestexcept(FE_OVERFLOW)) {
        return OFLW; // 1e4932 2 ^    or  11357 e
    } else if (fetestexcept(FE_UNDERFLOW)) {
        return UFLW; // 1e4932 i
    } else if (fetestexcept(FE_INVALID)) {
        return INAN; // inf inf -     or  -1 2 v
    } else {
        return JUNK;
    }
}


// vet cmds against stack size. print msgs, smallstack and math errors
void vet_do(RPN_T inputnum, token_t cmd, stack_t *stks[]) {
    if (stack_size(stks[I_STK]) < funrows[cmd].minsz) {
        printmsg_fresh(SMAL);
        return;
    }
    if (cmd < UNDO) {
        stack_push(&cmd, stks[H_CMDS]);
    }
    feclearexcept(FE_ALL_EXCEPT);
    if (cmd == NUM) {
        stack_push(&inputnum, stks[I_STK]);
    } else if (funrows[cmd].type == BINARY) {  //  * + ^ / - v
        call_binary(cmd, stks);
    } else if (funrows[cmd].type == UNARY) {   // e l
        call_unary(cmd, stks);
    } else if (funrows[cmd].type == NONHIST) { //  ~ i c s r u
        nonhistp = funrows[cmd].fun;
        nonhistp(stks[I_STK]);
    } else if (cmd == DISC) {     //    not using a discard()
        RPN_T tmp;
        stack_pop(&tmp,  stks[I_STK ]);
        stack_push(&tmp, stks[H_NUMS]);
    } else if (cmd == HTOG) {
        toggle_hist_flag();
    } else if (cmd == DUMP) {
        dump_stack(stks);
    }
    printmsg_fresh(cmd);
    printmsg(math_error()); // print even if it's an old msg
}


// 0*+^/-vel~icdsru_qhn     also used in printmsg()
// 01234567890123456789
// looks only for numbers and single chars
// naively checks tok0 == '0'. not using an is_zero()
token_t tokenize(char *inputbuf, RPN_T *inputnum) {
    *inputnum = strtold(inputbuf, NULL); // no error check
    char tok0 = *inputbuf;
    if (*inputnum || (tok0 == '0')) {
        // not 0.0L, so it's a number || the input 0.0L comes from a '0'
        return NUM;
    } else {
        int i = 1; // 0 is NUM
        while (i < JUNK) {
            if (tok0 == funrows[i].tok) {
                return (token_t)i;
            }
            i++;
        }
    }
    return JUNK;
}


int handle_input(char *inputbuf, stack_t *stks[]) {
    RPN_T inputnum = RPN_ZERO;
    fgets(inputbuf, BUFSIZ, stdin); // no error check
    char *chp, *str;
    token_t tok = NUM;
    for (chp = inputbuf; tok != QUIT; chp = NULL) {
        str = strtok(chp, " \t\n");
        if (str == NULL) {
            break; // reached end of input
        }
        tok = tokenize(str, &inputnum);
        if (tok == UNDO) {
            undo(stks);
        } else if (tok < JUNK) {
            vet_do(inputnum, tok, stks);
        }
    }
    return (tok == QUIT);
}


