#ifndef RPNFUNCTIONS_H
#define RPNFUNCTIONS_H
#include "rpnstack.h"

/*
rpnfunctions.h
a reverse polish notation calculator

example build:
gcc rpnstack.c rpnfunctions.c rpn.c -lm -o rpn

*/

#define RPN_T long double
#define RPN_FMT "%.10Lg"
#define RPN_ZERO 0.0L // easier to change types
#define RPN_ONE  1.0L


// subsets of these enums have different roles
// aspects: tokens, messages, functions and their attributes
typedef enum token_e {
           //   char   n   minsize   comment
     NUM,  //    0     0     0       number
//                                   binary, they use H_NUMS
     MUL,  //    *     1     2
     ADD,  //    +     2     2
    POWE,  //    ^     3     2
    DIVI,  //    /     4     2       msg DBYZ
     SUB,  //    -     5     2
    ROOT,  //    v     6     2       a^(1/b) radical. msg

    LOGN,  //    l     7     1       unary
    EXPE,  //    e     8     1

     NEG,  //    ~     9     1       msgs, no H_NUMS
    INVE,  //    i    10     1       msg DBYZ
    COPY,  //    c    11     1
    SWAP,  //    s    12     2
    ROLD,  //    r    13     2
    ROLU,  //    u    14     2
    DISC,  //    d    15     1       uses H_NUMS
//                                   not in history
    UNDO,  //    _    16     0       undo_score. C-_ is emacs undo

    HTOG,  //    t    17     0       toggle history
    QUIT,  //    q    18     0
    HELP,  //    h    19     0       msg is longer
    RANG,  //    n    20     0       numberrange, not r
//
    JUNK,  //         21             token limit, possible defaultval, ignore
    DBYZ,  //         22             msg Division by zero
    OFLW,  //         23             msg Overflow
    UFLW,  //         24             msg Underflow
    INAN,  //         25             msg Invalid
    SMAL,  //         26             msg Stack too small
    SMLU,  //         27             msg No history to undo. stack too small
} token_t;


static RPN_T (*binaryp)(RPN_T x, RPN_T y);
extern RPN_T  mul(RPN_T x, RPN_T y);
extern RPN_T  add(RPN_T x, RPN_T y);
extern RPN_T powe(RPN_T x, RPN_T y); // pow was taken
extern RPN_T divi(RPN_T x, RPN_T y); // div was taken
extern RPN_T  sub(RPN_T x, RPN_T y);
extern RPN_T root(RPN_T x, RPN_T y);

static RPN_T (*unaryp)(RPN_T x);
extern RPN_T logn(RPN_T x);
extern RPN_T expe(RPN_T x);

static void (*nonhistp)(stack_t *stk);
extern void  neg(stack_t *stk);
extern void inve(stack_t *stk);
extern void copy(stack_t *stk);
extern void swap(stack_t *stk);
extern void rold(stack_t *stk);
extern void rolu(stack_t *stk);

extern void noop(void);

// harmonize these "types" with cases in vet_do() and undo()
enum {BINARY, UNARY, NONHIST, NONOP, MSG, OTHER};

static struct funrow {
    char tok;
    void *fun;
    size_t minsz;
    int type;
    int has_msg;
    void *anti;
    char *name;
} funrows[] = {
//    tok   fun  sz  type   msg? anti  name / msg
    {'\0', noop, 0u, OTHER  , 0, noop, "number"         }, //  NUM

    { '*',  mul, 2u, BINARY , 0, noop, "multiply"       }, //  MUL
    { '+',  add, 2u, BINARY , 0, noop, "add"            }, //  ADD
    { '^', powe, 2u, BINARY , 0, noop, "power"          }, // POWE
    { '/', divi, 2u, BINARY , 0, noop, "divide"         }, // DIVI
    { '-',  sub, 2u, BINARY , 0, noop, "subtract"       }, //  SUB
    { 'v', root, 2u, BINARY , 1, noop, "root"           }, // ROOT

    { 'l', logn, 1u, UNARY  , 1, noop, "log"            }, // LOGN
    { 'e', expe, 1u, UNARY  , 1, noop, "exp"            }, // EXPE

    { '~', neg , 1u, NONHIST, 1, neg , "negate"         }, //  NEG
    { 'i', inve, 1u, NONHIST, 1, inve, "invert"         }, // INVE
    { 'c', copy, 1u, NONHIST, 1, noop, "copy"           }, // COPY
    { 's', swap, 2u, NONHIST, 1, swap, "swap"           }, // SWAP
    { 'r', rold, 2u, NONHIST, 1, rolu, "rolldown"       }, // ROLD
    { 'u', rolu, 2u, NONHIST, 1, rold, "rollup"         }, // ROLU
    { 'd', noop, 1u, OTHER  , 1, noop, "discard"        }, // DISC

    { '_', noop, 0u, NONOP  , 1, noop, "undo"           }, // UNDO
    { 't', noop, 0u, NONOP  , 1, noop, "togglehist"     }, // HTOG
    { 'q', noop, 0u, NONOP  , 1, noop, "quit"           }, // QUIT
    { 'h', noop, 0u, NONOP  , 1, noop, "help"           }, // HELP
    { 'n', noop, 0u, NONOP  , 1, noop, "numberrange"    }, // RANG

    {'\0', noop, 0u, OTHER  , 0, noop, "Junk"           }, // JUNK
    {'\0', noop, 0u, MSG    , 1, noop, "Divide by zero" }, // DBYZ
    {'\0', noop, 0u, MSG    , 1, noop, "Overflow"       }, // OFLW
    {'\0', noop, 0u, MSG    , 1, noop, "Underflow"      }, // UFLW
    {'\0', noop, 0u, MSG    , 1, noop, "Invalid num"    }, // INAN
    {'\0', noop, 0u, MSG    , 1, noop, "Stack too small"}, // SMAL
    {'\0', noop, 0u, MSG    , 1, noop, "No undo history"}, // SMLU
};

// no commas after these h, n msg strings. using the same index
// strings get concatenated when printed, need newlines
static const char *multiline_messages[] = {
    "rpn, Reverse Polish Notation floating point calculator\n"
    "Input number to push to the stack. hex format, inf and nan work too\n"
    "Operators: + * - / ^ v  e l to pop number(s) and push result\n"
    " Commands: ~ negate, i invert, c copy, d discard, s swap\n"
    "           r rolldown, u rollup, _ undo, t toggle history\n"
    "           h this help, n numrange, q quit",

    // not #include'ing <float.h>
    // redo the numbers for other types or architectures
    "IEEE 754 says long doubles have 30 digit precision\n"
    "[ ± LDBL_MIN: ± 3.3621e-4932  ]\n"
    "[ ± LDBL_MAX: ± 1.18973e+4932 ]", // no newline in the last string
};


static token_t last_msg = JUNK; // not math errors
static int hist_flag = 0;

static char *hist_sep =
"----------------------------------------------------------------------";
static char *display_sep =
"======================================================================";
static char *rpn_prompt = "#> ";

extern void printmsg(token_t msgcode);
extern void display(size_t display_len, stack_t *stks[]);

// indices for the rpn_stacks array
// interactive stack
// history nums, history cmds are for restoring the interactive stack
enum {I_STK, H_NUMS, H_CMDS};

extern int handle_input(char *inputbuf, stack_t *stks[]);


// --- prototypes for when you write tests -------------------------------------

#ifdef RPN_TEST

extern void toggle_hist_flag(void);

// printmsg() declared above
extern void printmsg_fresh(token_t msgcode);

extern void display_stack(void (*print_item)(void*),
                          void *itemp,
                          size_t display_len,
                          stack_t *stack);

extern void display_history(size_t items_display_limit, stack_t *stks[]);

extern void print_num(void *itemp);
extern void print_cmdname(void *itemp);

// display() declared
// binops and nonhists funs declared

extern void call_binary(token_t cmd, stack_t *stks[]);
extern void call_unary(token_t cmd, stack_t *stks[]);

extern void roll_stack(int direction, stack_t *stk);
extern void rold(stack_t *stk);
extern void rolu(stack_t *stk);

extern void undo(stack_t *stks[]);
extern token_t math_error(void);
extern void vet_do(RPN_T inputnum, token_t cmd, stack_t *stks[]);

extern token_t tokenize(char *inputbuf, RPN_T *inputnum);
// handle_input() declared

#endif // RPN_TEST
#endif // RPNFUNCTIONS_H

