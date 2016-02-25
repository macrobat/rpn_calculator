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

#define NELEMS(arr) (sizeof(arr) / sizeof((arr)[0]))


static char tokenchars[] =
    "0*+^/-v~icdsru_tqhn";
//   0123456789012345678   len == 19 without '\0'. no token for junk chars

// the orders in these arrays are important. enums as array indices
// subsets of these enums have different roles
// aspects: tokens, messages, functions and their attributes
typedef enum token_e {
             //   char   n    minsize    comment
     NUM_E,  //    0     0     0       number, could signify normal, default
//  ------------------------------     binops, they use H_NUMS
     MUL_E,  //    *     1     2
     ADD_E,  //    +     2     2
    POWE_E,  //    ^     3     2
    DIVI_E,  //    /     4     2       msg DBYZ_E
     SUB_E,  //    -     5     2
    ROOT_E,  //    v     6     2       a^(1/b) radical. msg

     NEG_E,  //    ~     7     1       msgs, no H_NUMS
    INVE_E,  //    i     8     1       msg DBYZ_E
    COPY_E,  //    c     9     1
    DISC_E,  //    d    10     1       uses H_NUMS
    SWAP_E,  //    s    11     2
    ROLD_E,  //    r    12     2
    ROLU_E,  //    u    13     2
//  ------------------------------     not in history
    UNDO_E,  //    _    14             undo_score. C-_ is emacs undo

    HTOG_E,  //    t    15             toggle history
    QUIT_E,  //    q    16
    HELP_E,  //    h    17
    RANG_E,  //    n    18             numberrange, not r
//  ------------------------------
    JUNK_E,  //         19             token limit, possible defaultval, ignore
    DBYZ_E,  //         20             msg Division by zero
    OFLW_E,  //         21             msg Overflow
    UFLW_E,  //         22             msg Underflow
     NAN_E,  //         23             msg Invalid
    SMAL_E,  //         24             msg Stack too small
    SMLU_E,  //         25             msg No history to undo. stack too small
     NIL_E,  //         26             ok, no msg, no error. limit
} token_t;

// mainly names of the functions, but non-functions are here too
// use the old 4-letter names for input with flex?
// currently only using those that use H_CMDS
static char *token_names[] = {
    "number",       //     0

    "multiply",     //     1
    "add",          //     2
    "power",        //     3
    "divide",       //     4
    "subtract",     //     5
    "root",         //     6

    "negate",       //     7
    "invert",       //     8
    "copy",         //     9
    "discard",      //    10
    "swap",         //    11
    "rolldown",     //    12
    "rollup",       //    13
//  -------------------------------------- not using these:
    "undo",         //    14
    "togglehist",   //    15

    "quit",         //    16
    "help",         //    17
    "numberrange",  //    18

    "junk",         //    19
    "divbyzero",    //    20
    "overflow",     //    21
    "underflow",    //    22
    "nan",          //    23
    "smallstack",   //    24
    "nohist",       //    25
    "nil",          //    26
};


extern RPN_T bop_mul(RPN_T x, RPN_T y);
extern RPN_T bop_add(RPN_T x, RPN_T y);
extern RPN_T bop_pow(RPN_T x, RPN_T y); // pow taken
extern RPN_T bop_div(RPN_T x, RPN_T y); // div taken
extern RPN_T bop_sub(RPN_T x, RPN_T y);
extern RPN_T bop_root(RPN_T x, RPN_T y);

static RPN_T (*binops[])(RPN_T, RPN_T) = {
    bop_mul,    // 0
    bop_add,    // 1
    bop_pow,    // 2
    bop_div,    // 3
    bop_sub,    // 4
    bop_root,   // 5
};

extern void  neg(stack_t *stk);
extern void inve(stack_t *stk);
extern void copy(stack_t *stk);
extern void swap(stack_t *stk);
extern void rold(stack_t *stk);
extern void rolu(stack_t *stk);

// no discard as it would use H_NUMS, not nonhist.
// anyway, there is no discard() function atm
static struct {
    token_t id;
    void (*nonhist)(stack_t*);
} nonhists[] = {
    { NEG_E,  neg},   //   ~
    {INVE_E, inve},   //   i
    {COPY_E, copy},   //   c
    {SWAP_E, swap},   //   s
    {ROLD_E, rold},   //   r
    {ROLU_E, rolu},   //   u
};

// minimal sizes for the stack for allowing commands
static const size_t minsizes[] = {
    0u,  //    0     0

    2u,  //    *     1
    2u,  //    +     2
    2u,  //    ^     3
    2u,  //    /     4
    2u,  //    -     5
    2u,  //    v     6

    1u,  //    ~     7
    1u,  //    i     8
    1u,  //    c     9
    1u,  //    d    10
    2u,  //    s    11
    2u,  //    r    12
    2u,  //    u    13
// ----------------------   not that meaningful:
    0u,  //    _    14
    0u,  //    t    15
    0u,  //    q    16
    0u,  //    h    17
    0u,  //    n    18
};


// opposite cmds to undo nonhists. no binops here
// {COPY_E, DISC_E} not used atm, handled like NUM_E
static struct cmd_coord {
    token_t fwd;
    token_t rev;
} opposites[] = {
    {NEG_E , NEG_E },
    {INVE_E, INVE_E},
    {COPY_E, DISC_E}, // leads out of the set. not closed
    {SWAP_E, SWAP_E},
    {ROLD_E, ROLU_E},
    {ROLU_E, ROLD_E},
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

static token_t last_msg = NIL_E; // not math errors
static int hist_flag = 0;

static char *hist_sep =
"----------------------------------------------------------------------";
static char *display_sep =
"======================================================================";
static char *rpn_prompt = "#> ";

extern void printmsg(token_t msgcode);
extern void display(size_t display_len, stack_t *stks[]);

// named indices for the stacks array
// could combine, but separate stacks for cmds and nums is clearer
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

extern int has_msg(token_t tok);

extern void print_num(void *itemp);
extern void print_cmdname(void *itemp);

// display() declared
// binops and nonhists funs declared

extern int is_binop(token_t cmd);
extern void call_binop(token_t cmd, stack_t *stks[]);
extern int is_nonhist(token_t cmd);
extern void call_nonhist(token_t cmd, stack_t *stk);

extern void roll_stack(int direction, stack_t *stk);
extern void rold(stack_t *stk);
extern void rolu(stack_t *stk);

extern token_t opposite(token_t cmd);
extern void undo(stack_t *stks[]);
extern token_t math_error(void);
extern void vet_do(RPN_T inputnum, token_t cmd, stack_t *stks[]);

extern token_t tokenize(char *inputbuf, RPN_T *inputnum);
// handle_input() declared

#endif // RPN_TEST
#endif // RPNFUNCTIONS_H

