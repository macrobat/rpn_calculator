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
// more description in rpn.c

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

--- TODO / IDEAS --------------------------------------

typedef enum funtype_e {BINOP, NONHIST, OTHER_BLA};
decouple the array indices and their elements
no more tokenchars[] minsizes[] and friends
many-membered structs with all aspects of a function
replace vertical arrangement of enum-indexed arrays
with more horizontal groupings of
struct funrow {
    token_t id; char tok; void *fun; void *anti; size_t minsz; int has_msg;
} funs[] = {};


exp x, ln l opposites. what chars for sin cos tan? constant pi p
long double expl(long double x); // x = -inf works?
long double logl(long double x);


printmsg() can be spammy, have a toggle for it?
there's a wrapper around it and a global to avoid repetition
want math errors repeated, because the causing funs also change the stack

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

t     _     * + ^ / -  v    ~ i c d s r u
0 i 0 i 1.18973e+4932 i _ c ^ - -9 2 v
0x0 -1.1e1 2.2e-2 3.3 4e400 5 6 7 8 9
h h h n n n n
* + ^ / - v ~ i c d s r u
w a f f l e z
10 _ * _ ^ _ c _ s _ r _ u _
t
q


not all branches in the program are reachable

*/
// --- display -----------------------------------------------------------------

// with switch cases, cmds don't have to be ordered
// easy to return 0, comment out, or add entries
int has_msg(token_t tok) {
    switch (tok) { // returns. no fallthroughs
        case ROOT_E: return 1;
        case  NEG_E: return 1;
        case INVE_E: return 1;
        case COPY_E: return 1;
        case DISC_E: return 1;
        case SWAP_E: return 1;
        case ROLD_E: return 1;
        case ROLU_E: return 1;
        case UNDO_E: return 1;
        case HTOG_E: return 1;
        case QUIT_E: return 1;
        case HELP_E: return 1;
        case RANG_E: return 1;
        // case JUNK_E: return 0; // like so
        case DBYZ_E: return 1;
        case OFLW_E: return 1;
        case UFLW_E: return 1;
        case  NAN_E: return 1;
        case SMAL_E: return 1;
        case SMLU_E: return 1;
        default:
            return 0;
    }
}

// checking has_msg twice, could check for not NIL_E (from math_err)
void printmsg(token_t msgcode) {
    if (!has_msg(msgcode)) { return; }
    if (msgcode < JUNK_E) { // < strlen(tokenchars)
        printf("%c ", tokenchars[msgcode]);
    }
    printf("%s\n", messages[msgcode - ROOT_E]);
}

// wrapper guarantees freshness. return if seal is broken
void printmsg_fresh(token_t msgcode) {
    if (!has_msg(msgcode)) { return; }
    if (msgcode == last_msg) {
        return;
    } else {
        token_t *fresh_msg = &last_msg;
        *fresh_msg = msgcode;
        printmsg(msgcode);
    }
}


void toggle_hist_flag(void) {
    int *flagp = &hist_flag;
    *flagp = !(*flagp);
}

// format string RPN_FMT for long double is "%.10Lg"
void print_num(void *itemp) {
    printf(RPN_FMT, *(RPN_T*)itemp);
}

void print_cmdname(void *itemp) {
    printf("%s", token_names[*(token_t*)itemp]);
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
    if (! stack_empty(stks[H_NUMS])) {
        puts("history numbers:");
        RPN_T num;
        display_stack(print_num, &num, items_display_limit, stks[H_NUMS]);
        puts("");
    }
    if (! stack_empty(stks[H_CMDS])) {
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
    if (! stack_empty(stks[I_STK ])) {
        RPN_T num;
        display_stack(print_num, &num, display_len, stks[I_STK ]);
    }
    printf("%s", rpn_prompt); // "#> "
}


// --- operations * + / - ^ v --------------------------------------------------

// binops: * + ^ / - v
// have the typesig RPN_T fn(RPN_T, RPN_T)
int is_binop(token_t cmd) {
    switch (cmd) {
        case  MUL_E: return 1;
        case  ADD_E: return 1;
        case POWE_E: return 1;
        case DIVI_E: return 1;
        case  SUB_E: return 1;
        case ROOT_E: return 1;
        default:
            return 0;
    }
}


// binary operations
// with long doubles: can return and use inf and nan
RPN_T bop_mul(RPN_T x, RPN_T y) {
    return x * y;
}
RPN_T bop_add(RPN_T x, RPN_T y) {
    return x + y;
}
// can handle floating point division with 0.0 or 0.0L. returns inf
RPN_T bop_div(RPN_T x, RPN_T y) {
    return x / y;
}
// sub is the binary operation subtract, not neg() ~
RPN_T bop_sub(RPN_T x, RPN_T y) {
    return x - y;
}

RPN_T bop_pow(RPN_T x, RPN_T y) {
    return powl(x, y);
}

// root, radical anti-power x^(1/y)
// a "2 v" input means square root
RPN_T bop_root(RPN_T x, RPN_T y) {
    return powl(x, RPN_ONE / y); // 1.0L
}


// --- commands ----------------------------------------------------------------

// ~ i c s r u, no d discard
int is_nonhist(token_t cmd) {
    switch (cmd) {
        case  NEG_E: return 1;
        case INVE_E: return 1;
        case COPY_E: return 1;
        case SWAP_E: return 1;
        case ROLD_E: return 1;
        case ROLU_E: return 1;
        default:
            return 0;
    }
}

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

// no discard() for DISC_E

void swap(stack_t *stk) {
    RPN_T topnum;
    RPN_T nextnum;
    stack_pop(&topnum, stk);
    stack_pop(&nextnum, stk);
    stack_push(&topnum, stk);
    stack_push(&nextnum, stk);
}

// --- commands to roll stack up or down ---------------------------------------

// helper for rolldown and rollup
// there are atleast 2 elements when called
// warning: pointer of type ‘void *’ used in arithmetic [-Wpointer-arith]
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

// ~ i c s r u  don't use H_NUMS
void call_nonhist(token_t cmd, stack_t *stk) {
    int n = (int)NELEMS(nonhists);
    int i = 0;
    while (i < n) {
        if (nonhists[i].id == cmd) {
            (nonhists[i].nonhist)(stk);
            return;
        }
        i++;
    }
}


//  ~ i c s r u ---> ~ i d s u r
//  not using it for c --> d
token_t opposite(token_t cmd) {
    int n = (int)NELEMS(opposites);
    int i = 0;
    while (i < n) {
        if (opposites[i].fwd == cmd) {
           return opposites[i].rev;
        }
        i++;
    }
    return JUNK_E; // return to reassure the compiler
}


// can't undo an undo
// could print what cmd is undone
void undo(stack_t *stks[]) {
    if (stack_empty(stks[H_CMDS])) {
        printmsg_fresh(SMLU_E);
        return;
    } // don't msg until we actually undo
    printmsg_fresh(UNDO_E);
    token_t cmd;
    RPN_T tmp;
    stack_pop(&cmd, stks[H_CMDS]);
    if (cmd == NUM_E || cmd == COPY_E) { // test copy() before other nonhists
        stack_pop(&tmp, stks[I_STK]);
    } else if (is_binop(cmd)) { //  * + ^ / - v
        RPN_T topnum;
        RPN_T nextnum;
        stack_pop(&tmp,      stks[I_STK ]);
        stack_pop(&topnum,   stks[H_NUMS]);
        stack_pop(&nextnum,  stks[H_NUMS]);
        stack_push(&topnum,  stks[I_STK ]);
        stack_push(&nextnum, stks[I_STK ]);
    } else if (cmd == DISC_E) {
        stack_pop(&tmp, stks[H_NUMS]);
        stack_push(&tmp, stks[I_STK]);
    } else if (is_nonhist(cmd) && cmd != COPY_E) {
            call_nonhist(opposite(cmd), stks[I_STK]);
    }
}

// H_NUMS will have pairwise reverse nums compared to I_STK
// cmd to [index] 1..6 ---> 0..5 just subtracting MUL_E which is 1
// NUM_E is 0 and before the binops in token enums
void call_binop(token_t cmd, stack_t *stks[]) {
    RPN_T topnum;
    RPN_T nextnum;
    stack_pop(&topnum,   stks[I_STK ]);
    stack_pop(&nextnum,  stks[I_STK ]);
    stack_push(&topnum,  stks[H_NUMS]);
    stack_push(&nextnum, stks[H_NUMS]);
    stack_push(&cmd,     stks[H_CMDS]);
    RPN_T resnum = binops[cmd - MUL_E](nextnum, topnum); // arg order
    stack_push(&resnum,  stks[I_STK ]);
}


token_t math_error(void) {
    if (fetestexcept(FE_DIVBYZERO)) {
       return DBYZ_E;  // 0 i
    } else if (fetestexcept(FE_OVERFLOW)) {
        return OFLW_E; // 1e4932 2 ^
    } else if (fetestexcept(FE_UNDERFLOW)) {
        return UFLW_E; // 1e4932 i
    } else if (fetestexcept(FE_INVALID)) {
        return NAN_E;  // inf inf -     or  -4 2 v
    } else {
        return NIL_E;
    }
}


// vet cmds against stack size. print msgs, smallstack and math errors
void vet_do(RPN_T inputnum, token_t cmd, stack_t *stks[]) {
    if (stack_size(stks[I_STK]) < minsizes[cmd]) {
        printmsg_fresh(SMAL_E);
        return;
    }
    feclearexcept(FE_ALL_EXCEPT); //    not using errno
    if (cmd == NUM_E) {
        stack_push(&inputnum, stks[I_STK]);
        stack_push(&cmd, stks[H_CMDS]);
    } else if (is_binop(cmd)) {   //    * + ^ / - v
        call_binop(cmd, stks);
    } else if (is_nonhist(cmd)) { //    ~ i c s r u
        stack_push(&cmd, stks[H_CMDS]);
        call_nonhist(cmd, stks[I_STK ]);
    } else if (cmd == DISC_E) {   //    not using a discard()
        RPN_T tmp;
        stack_pop(&tmp, stks[I_STK ]);
        stack_push(&tmp, stks[H_NUMS]);
        stack_push(&cmd, stks[H_CMDS]);
    } else if (cmd == HTOG_E) {
        toggle_hist_flag();
    }
    printmsg_fresh(cmd);
    printmsg(math_error()); // print even if it's an old msg
}


// "0*+^/-v~icdsru_qhn" also used by printmsg()
//  012345678901234567
// looks only for numbers and single chars
// naively checks tok0 == '0'. not using an is_zero()
token_t tokenize(char *inputbuf, RPN_T *inputnum) {
    *inputnum = strtold(inputbuf, NULL); // no error check
    char tok0 = *inputbuf;
    if (*inputnum || (tok0 == '0')) {
        // not 0.0L, so it's a number || the input 0.0L comes from a '0'
        return NUM_E;
    } else {
        int i = 1; // 0 is the padding NUM_E
        while (i < JUNK_E) {
            if (tok0 == tokenchars[i]) {
                return (token_t)i;
            }
            i++;
        }
    }
    return JUNK_E;
}


int handle_input(char *inputbuf, stack_t *stks[]) {
    RPN_T inputnum = RPN_ZERO;
    fgets(inputbuf, BUFSIZ, stdin); // no error check
    char *chp, *str;
    token_t tok = NUM_E;
    for (chp = inputbuf; tok != QUIT_E; chp = NULL) {
        str = strtok(chp, " \t\n");
        if (str == NULL) {
            break; // reached end of input
        }
        tok = tokenize(str, &inputnum);
        if (tok == UNDO_E) {
            undo(stks);
        } else if (tok < JUNK_E) {
            vet_do(inputnum, tok, stks);
        }
    }
    return (tok == QUIT_E);
}

