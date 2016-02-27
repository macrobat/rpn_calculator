#ifndef RPNFUNCTIONS_H
#define RPNFUNCTIONS_H
#include "rpnstack.h"

/*
rpnfunctions.h
build like this:
gcc rpnstack.c rpn_array.c rpnfunctions_array.c -lm -o rpn_array

*/

#define RPN_T long double
#define RPN_FMT "%.10Lg"
#define RPN_ZERO 0.0L // easier to change types
#define RPN_ONE  1.0L


// subsets of these enums have different roles
// aspects: tokens, messages, functions and their attributes
typedef enum token_e {
             //   char   n    minsize    comment
     NUM,  //    0     0     0       number, could signify normal, default
//  ------------------------------     binops, they use H_NUMS
     MUL,  //    *     1     2
     ADD,  //    +     2     2
    POWE,  //    ^     3     2
    DIVI,  //    /     4     2       msg DBYZ
     SUB,  //    -     5     2
    ROOT,  //    v     6     2       a^(1/b) radical. msg

     NEG,  //    ~     7     1       msgs, no H_NUMS
    INVE,  //    i     8     1       msg DBYZ
    COPY,  //    c     9     1
    DISC,  //    d    10     1       uses H_NUMS
    SWAP,  //    s    11     2
    ROLD,  //    r    12     2
    ROLU,  //    u    13     2
//  ------------------------------     not in history
    UNDO,  //    _    14     0       undo_score. C-_ is emacs undo

    HTOG,  //    t    15     0       toggle history
    QUIT,  //    q    16     0
    HELP,  //    h    17     0
    RANG,  //    n    18     0       numberrange, not r
//  ------------------------------
    JUNK,  //         19             token limit, possible defaultval, ignore
    DBYZ,  //         20             msg Division by zero
    OFLW,  //         21             msg Overflow
    UFLW,  //         22             msg Underflow
    INAN,  //         23             msg Invalid
    SMAL,  //         24             msg Stack too small
    SMLU,  //         25             msg No history to undo. stack too small
     NIL,  //         26             ok, no msg, no error. limit
} token_t;


extern RPN_T  mul(RPN_T x, RPN_T y);
extern RPN_T  add(RPN_T x, RPN_T y);
extern RPN_T powe(RPN_T x, RPN_T y); // pow was taken
extern RPN_T divi(RPN_T x, RPN_T y); // div was taken
extern RPN_T  sub(RPN_T x, RPN_T y);
extern RPN_T root(RPN_T x, RPN_T y);

extern void  neg(stack_t *stk);
extern void inve(stack_t *stk);
extern void copy(stack_t *stk);
extern void swap(stack_t *stk);
extern void rold(stack_t *stk);
extern void rolu(stack_t *stk);


extern void noop(void);

enum {BINOP, NONHIST, NONOP, MSG, OTHER};

// only using the message names that are shown in history
static struct funrow {
    char tok;
    void *fun;
    size_t minsz;
    int type;
    int has_msg;
    token_t anti;
    char* name;
} funrows[] = {
//    tok   fun  sz  type    msg anti   name
    {'\0', noop, 0u, OTHER  , 0, NIL , "number"     }, //  NUM

    { '*',  mul, 2u, BINOP  , 0, NIL , "multiply"   }, //  MUL
    { '+',  add, 2u, BINOP  , 0, NIL , "add"        }, //  ADD
    { '^', powe, 2u, BINOP  , 0, NIL , "power"      }, // POWE
    { '/', divi, 2u, BINOP  , 0, NIL , "divide"     }, // DIVI
    { '-',  sub, 2u, BINOP  , 0, NIL , "subtract"   }, //  SUB
    { 'v', root, 2u, BINOP  , 1, NIL , "root"       }, // ROOT

    { '~', neg , 1u, NONHIST, 1, NEG , "negate"     }, //  NEG
    { 'i', inve, 1u, NONHIST, 1, INVE, "invert"     }, // INVE
    { 'c', copy, 1u, NONHIST, 1, NIL , "copy"       }, // COPY
    { 'd', noop, 1u, OTHER  , 1, NIL , "discard"    }, // DISC
    { 's', swap, 2u, NONHIST, 1, SWAP, "swap"       }, // SWAP
    { 'r', rold, 2u, NONHIST, 1, ROLU, "rolldown"   }, // ROLD
    { 'u', rolu, 2u, NONHIST, 1, ROLD, "rollup"     }, // ROLU

    { '_', noop, 0u, NONOP  , 1, NIL , "undo"       }, // UNDO
    { 't', noop, 0u, NONOP  , 1, NIL , "togglehist" }, // HTOG
    { 'q', noop, 0u, NONOP  , 1, NIL , "quit"       }, // QUIT
    { 'h', noop, 0u, NONOP  , 1, NIL , "help"       }, // HELP
    { 'n', noop, 0u, NONOP  , 1, NIL , "numberrange"}, // RANG

    {'\0', noop, 0u, OTHER  , 0, NIL , "junk"       }, // JUNK
    {'\0', noop, 0u, MSG    , 1, NIL , "divbyzero"  }, // DBYZ
    {'\0', noop, 0u, MSG    , 1, NIL , "overflow"   }, // OFLW
    {'\0', noop, 0u, MSG    , 1, NIL , "underflow"  }, // UFLW
    {'\0', noop, 0u, MSG    , 1, NIL , "nan"        }, //  NAN
    {'\0', noop, 0u, MSG    , 1, NIL , "smallstack" }, // SMAL
    {'\0', noop, 0u, MSG    , 1, NIL , "nohist"     }, // SMLU
    {'\0', noop, 0u, MSG    , 0, NIL , "nil"        }, //  NIL
};


// no commas for h and n strings, using the same index
// strings get concatenated when printed, need newlines in the strings
// "junk" padding
static const char *messages[] = {
    "root",                     //  0   v pun on ^. non-obvious, so it has msg
    "negate",                   //  1
    "invert",                   //  2
    "copy",                     //  3
    "discard",                  //  4
    "swap",                     //  5
    "rolldown",                 //  6
    "rollup",                   //  7
    "undo",                     //  8
    "togglehist",               //  9
    "quit",                     // 10
    "help\n"                    // 11
    "rpn, Reverse Polish Notation floating point calculator\n"
    "Input number to push to the stack. hex format, inf and nan work too\n"
    "Operators: + * - / ^ v will pop numbers and push the result\n"
    " Commands: ~ negate, i invert, c copy, d discard, s swap\n"
    "           r rolldown, u rollup, _ undo, t toggle history\n"
    "           h this help, n numrange, q quit", // index 11 still

    "numberrange\n"             // 12
    "IEEE 754 says long doubles have 10 digit precision\n"
    "[ ± LDBL_MIN: ± 3.3621e-4932  ]\n"     // not #include'ing <float.h>
    "[ ± LDBL_MAX: ± 1.18973e+4932 ]",      // no extra newline

    "junk",                     // 13   suppressed elsewhere
    "Division by zero",         // 14
    "Overflow",                 // 15
    "Underflow",                // 16
    "Not a number, invalid",    // 17
    "Stack too small",          // 18
    "No history to undo",       // 19
};

static token_t last_msg = NIL; // not math errors
static int hist_flag = 0;

static char *hist_sep =
"----------------------------------------------------------------------";
static char *display_sep =
"======================================================================";
static char *rpn_prompt = "#> ";

extern void printmsg(token_t msgcode);
extern void display(size_t display_len, stack_t *stks[]);

// indices for the rpn_stacks array
// interactive stack, history nums, history cmds
enum {I_STK, H_NUMS, H_CMDS};

extern int handle_input(char *inputbuf, stack_t *stks[]);


// --- prototypes for when you write tests -------------------------------------

#ifdef RPN_TEST
// test programs need to know more stuff
// split to another file?

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

extern void call_binop(token_t cmd, stack_t *stks[]);
extern void call_nonhist(token_t cmd, stack_t *stk);

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

