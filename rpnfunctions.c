#include <stdio.h>      // fgets()
#include <stdlib.h>     // strtold()
#include <string.h>     // memmove() memcpy() for rotate()
#include <math.h>       // for ^ powl(). link with -lm
#include <fenv.h>       // man fenv; man math_error
// #include <float.h>   // no. limits just for msg, works for this machine
// #include <errno.h>   // in stack.c too. inf is better than error msgs
#include "rpnstack.h"
#include "rpnfunctions.h"

// rpnfunctions.c
// a reverse polish notation calculator

/* ___ comments ________________________________________________________________

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

not enabling strtold error
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

___ TODO / IDEAS ______________________________________

root v is a visual pun on ^

does it matter that not all branches in the program are reachable?

sin cos tan? constant pi p

undo could print what cmd is undone

printmsg() can be spammy, have a toggle for it? run in batchmode

is_zero(). what can a long double zero look like? would be obviated by flex
(not very important. false positives don't matter much here)
0 preceded by [+-] fails to insert, interpreted as the operations [+-]


___ example input _____________________________________

at the interactive prompt:
toggle hist. undo / stack too small for all cmds whose minsize > 0
math errors, should repeat
a few numbers
print multiline msgs, should be no msg repeats on consecutive cmds
functions
nonsense chars
all branches undo, getting all the opposites on nonhists, stack is the same?
toggle hist
end

t     _     * + ^ / -  v    e l     ~ i c d s r u
0 i 0 i 1.18973e+4932 i _ c ^ - -9 2 v
0x0 -1.1e1 2.2e-2 3.3 4e10 5 6 7 8 9 8 7 6 5 4 3 2
h h h n n n n
* + ^ / - v e l ~ i c d s r u w
a b f g j k m o p x y z
10 _ * _ ^ _ c _ d _ s _ r _ u _
t
q

*/
// ___ convenience for RPN_T stacks ____________________________________________

// not a full abstraction layer
RPN_T pop(stack_t *stk) {
    RPN_T tmp;
    stack_pop(&tmp, stk);
    return tmp;
}

RPN_T top(stack_t *stk) {
    RPN_T tmp;
    stack_top(&tmp, stk);
    return tmp;
}

void push(RPN_T item, stack_t *stk) {
    RPN_T tmp = item;
    stack_push(&tmp, stk);
}

// moves _and_ returns the item. opposite arg order from memmove
RPN_T transfer(stack_t *src_stk, stack_t *dest_stk) {
    RPN_T item = pop(src_stk);
    push(item, dest_stk);
    return item;
}

// ___ display, print __________________________________________________________

// for interactive mode, use the vanilla print functions that do stuff
void (*p_printmsg)(token_t msgcode) = printmsg;
void (*p_printmsg_fresh)(token_t msgcode, token_t *last_msgp) = printmsg_fresh;

// checking has_msg a 2nd time, so it is not JUNK from math_err()
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
void printmsg_fresh(token_t msgcode, token_t *last_msgp) {
    if (!funrows[msgcode].has_msg) { return; }
    if (msgcode == *last_msgp) {
        return;
    } else {
        *last_msgp = msgcode;
        printmsg(msgcode);
    }
}

// batch mode in main points p_printmsg and p_printmsg_fresh to these
void donot_printmsg(token_t msgcode) { return; }
void donot_printmsg_fresh(token_t msgcode, token_t *last_msgp) { return; }

// format string RPN_FMT for long double is "%.10Lg"
void print_num(void *itemp) {
    printf(RPN_FMT, *(RPN_T*)itemp);
}

void print_cmdname(void *itemp) {
    printf("%s", funrows[*(token_t*)itemp].name);
}


// prints the stack from the bottom to the top
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


void display_history(stack_t *stks[]) {
    size_t hist_display_limit = 30u;
    if (!stack_empty(stks[H_NUMS])) {
        puts("history numbers:");
        RPN_T num;
        display_stack(print_num, &num, hist_display_limit, stks[H_NUMS]);
        puts("");
    }
    if (!stack_empty(stks[H_CMDS])) {
        puts("history cmds:");
        token_t cmd;
        display_stack(print_cmdname, &cmd, hist_display_limit, stks[H_CMDS]);
        puts(hist_sep);
        puts("");
    }
}


// print: separator, optional history stacks, interactive stack, prompt
void display(int *hist_flagp, stack_t *stks[]) {
    puts(display_sep);
    if (*hist_flagp) {
        display_history(stks);
    }
    if (!stack_empty(stks[I_STK ])) {
        size_t items_display_limit = 55u;
        RPN_T num;
        display_stack(print_num, &num, items_display_limit, stks[I_STK ]);
    }
    printf("%s", rpn_prompt); // "#> "
}


// ___ operations * + / - ^ v e l ______________________________________________
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

// ___ commands ________________________________________________________________

// negate, unary minus
void neg(stack_t *stk) {
    push(-pop(stk), stk);
}

// invert
void inve(stack_t *stk) {
    push(RPN_ONE / pop(stk), stk);
}

void copy(stack_t *stk) {
    push(top(stk), stk);
}

// DISC d discard would have typesig like call_binary

void swap(stack_t *stk) {
    RPN_T topnum = pop(stk);
    RPN_T nextnum = pop(stk);
    push(topnum, stk);
    push(nextnum, stk);
}

// HTOG t
void toggle(int *flag) {
    *flag = !(*flag);
}

// DUMP w, print the contents of the stack in one line
void dump_stack(stack_t *stk) {
    size_t lim = stack_size(stk);
    size_t z;
    RPN_T item;
    for (z = 0u; z < lim; z++) {
        stack_peek(&item, z, stk);
        print_num(&item);
        printf(" ");
    }
    puts("");
}

// roll stack up or down
void rold(stack_t *stk) {
    stack_roll(1, stk);
}

void rolu(stack_t *stk) {
    stack_roll(-1, stk);
}

void noop(void) { return; }

// ___ handle input, use stacks, print msgs ____________________________________

// undo is for restoring I_STK to a previous state
// only for the functions < UNDO
void undo(token_t *last_msgp, stack_t *stks[]) {
    if (stack_empty(stks[H_CMDS])) {
        p_printmsg_fresh(SMLU, last_msgp);
        return;
    }
    p_printmsg_fresh(UNDO, last_msgp);
    token_t cmd;
    stack_pop(&cmd, stks[H_CMDS]);
    if (cmd == NUM || cmd == COPY) { // testing COPY before other nonhists
        pop(stks[I_STK]);
    } else if (funrows[cmd].type == BINARY) { //  * + ^ / - v
        pop(stks[I_STK ]);
        transfer(stks[H_NUMS], stks[I_STK ]);
        transfer(stks[H_NUMS], stks[I_STK ]);
    } else if (funrows[cmd].type == UNARY) {  // e l
        pop(stks[I_STK ]);
        transfer(stks[H_NUMS], stks[I_STK ]);
    } else if (funrows[cmd].type == NONHIST && cmd != COPY) {
        // ~ i s r u. disc() as anti for copy() wouldn't be useful
        nonhist(funrows[cmd].anti, stks);
    } else if (cmd == DISC) {
        transfer(stks[H_NUMS], stks[I_STK ]);
    }
}


// H_NUMS will have numbers in pairwise reverse order of I_STK
void binary(token_t cmd, stack_t *stks[]) {
    RPN_T topnum = transfer(stks[I_STK ], stks[H_NUMS]);
    RPN_T nextnum = transfer(stks[I_STK ], stks[H_NUMS]);
    binaryp = funrows[cmd].fun;
    push(binaryp(nextnum, topnum), stks[I_STK ]); // arg order is important
}

void unary(token_t cmd, stack_t *stks[]) {
    RPN_T operand = transfer(stks[I_STK ], stks[H_NUMS]);
    unaryp = funrows[cmd].fun;
    push(unaryp(operand), stks[I_STK ]);
}

// just need I_STK, but using *stks[] to harmonize with other functions
void nonhist(token_t cmd, stack_t *stks[]) {
    nonhistp = funrows[cmd].fun;
    nonhistp(stks[I_STK]);
}

// filler, one would be enough
void nonop (token_t cmd, stack_t *stks[]) { return; }
void other (token_t cmd, stack_t *stks[]) { return; }
void msg   (token_t cmd, stack_t *stks[]) { return; }


// feclearexcept(FE_ALL_EXCEPT) previously in vet_do()
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
void vet_do(int *hist_flagp,
            token_t *last_msgp,
            RPN_T inputnum,
            token_t cmd,
            stack_t *stks[])
{
    if (stack_size(stks[I_STK]) < funrows[cmd].minsz) {
        p_printmsg_fresh(SMAL, last_msgp);
        return;
    }
    if (funrows[cmd].type != NONOP) { // is not  _ w t q h n   (< UNDO)
        stack_push(&cmd, stks[H_CMDS]);
    }
    feclearexcept(FE_ALL_EXCEPT);
    if (cmd == NUM) {
        stack_push(&inputnum, stks[I_STK]);
    } else if (funrows[cmd].type < NONOP) { // BINARY, UNARY, NONHIST
        callfun[funrows[cmd].type](cmd, stks);
    } else if (cmd == DISC) {     // d  not using a discard()
        transfer(stks[I_STK ], stks[H_NUMS]);
    } else if (cmd == HTOG) {
        toggle(hist_flagp);
    } else if (cmd == DUMP) {
        // w msg _before_ printing stack. below, msg is unfresh and supressed
        p_printmsg_fresh(cmd, last_msgp);
        dump_stack(stks[I_STK ]);
    }
    p_printmsg_fresh(cmd, last_msgp);
    p_printmsg(math_error()); // print even if it's an old msg
}


// 0*+^/-vel~icsrud_wtqhn       tok chars also used in printmsg()
// 0123456789012345678901
// looks only for numbers and single chars
// naively checks tok0 == '0'. not using an is_zero()
token_t tokenize(char *inputbuf, RPN_T *inputnum) {
    *inputnum = strtold(inputbuf, NULL); // no error check
    char tok0 = *inputbuf;
    if (*inputnum || (tok0 == '0')) {
        // not 0.0L, so it's a number || the input 0.0L comes from a '0'
        return NUM;
    } else {
        int i = 1; // assuming 0 is NUM
        while (i < JUNK) {
            if (tok0 == funrows[i].tok) {
                return i;
            }
            i++;
        }
    }
    return JUNK;
}


int handle_input(int *hist_flagp,
                 token_t *last_msgp,
                 char *inputbuf,
                 stack_t *stks[])
{
    RPN_T inputnum = RPN_ZERO;
    char *chp, *str;
    token_t tok = NUM;
    for (chp = inputbuf; tok != QUIT; chp = NULL) {
        str = strtok(chp, " \t\n");
        if (str == NULL) {
            break; // reached end of input
        }
        tok = tokenize(str, &inputnum);
        if (tok == UNDO) {
            undo(last_msgp, stks);
        } else if (tok < JUNK) {
            vet_do(hist_flagp, last_msgp, inputnum, tok, stks);
        }
    }
    return (tok == QUIT);
}


