#ifndef RPNFUNCTIONS_H
# define RPNFUNCTIONS_H
# include "rpnstack.h"

// rpnfunctions.h
// a reverse polish notation calculator

# ifndef RPN_T
#  define RPN_T long double
#  define RPN_FMT "%.10Lg"
#  define RPN_ZERO 0.0L
#  define RPN_ONE  1.0L
# endif

// subsets of these enums have different roles
// aspects: tokens, messages, functions and their attributes
typedef enum token_e {
           //  char   n   minsize   comment
     NUM,  //   0     0     0       number
//                                  binary, they use H_NUMS
     MUL,  //   *     1     2
     ADD,  //   +     2     2
    POWE,  //   ^     3     2
    DIVI,  //   /     4     2       msg DBYZ
     SUB,  //   -     5     2
    ROOT,  //   v     6     2       a^(1/b) radical. msg

    LOGN,  //   l     7     1       unary
    EXPE,  //   e     8     1

     NEG,  //   ~     9     1       nonhists. have msgs, no H_NUMS
    INVE,  //   i    10     1       also msg DBYZ
    COPY,  //   c    11     1
    SWAP,  //   s    12     2
    ROLD,  //   r    13     2
    ROLU,  //   u    14     2
    DISC,  //   d    15     1       uses H_NUMS
//                                  not in history:
    UNDO,  //   _    16     0       undo_score. C-_ is emacs undo
    DUMP,  //   w    17     0       print stack. reset stacks
    HTOG,  //   t    18     0       toggle history
    QUIT,  //   q    19     0
    HELP,  //   h    20     0       msg is multiline
    RANG,  //   n    21     0       numberrange, not r
//
    JUNK,  //        22             token limit, possible defaultval, ignore
    DBYZ,  //        23             msg math_error() Division by zero
    OFLW,  //        24             msg math_error() Overflow
    UFLW,  //        25             msg math_error() Underflow
    INAN,  //        26             msg math_error() Invalid
    SMAL,  //        27             msg Stack too small
    SMLU,  //        28             msg No history to undo. stack too small
} token_t;


static RPN_T (*binaryp)(RPN_T x, RPN_T y);
RPN_T  mul(RPN_T x, RPN_T y);
RPN_T  add(RPN_T x, RPN_T y);
RPN_T powe(RPN_T x, RPN_T y); // the name pow() is taken
RPN_T divi(RPN_T x, RPN_T y); // the name div() is taken
RPN_T  sub(RPN_T x, RPN_T y);
RPN_T root(RPN_T x, RPN_T y);

static RPN_T (*unaryp)(RPN_T x);
RPN_T logn(RPN_T x);
RPN_T expe(RPN_T x);

static void (*nonhistp)(stack_t *stk);
// can undo neg and inve easily without H_NUMS, unlike logn, expe
void  neg(stack_t *stk);
void inve(stack_t *stk);
void copy(stack_t *stk);
void swap(stack_t *stk);
void rold(stack_t *stk);
void rolu(stack_t *stk);

void noop(void);


// harmonizing these "categories" with conditionals in vet_do()
void binary(token_t cmd, stack_t *stks[]);
void unary(token_t cmd, stack_t *stks[]);
void nonhist(token_t cmd, stack_t *stks[]);
// more padding noops to complicate the program and make it longer
void nonop(token_t cmd, stack_t *stks[]);
void msg(token_t cmd, stack_t *stks[]);
void other(token_t cmd, stack_t *stks[]);

// nonhist and nonop need better names. DISCARD would be its own type_t
typedef enum {BINARY, UNARY, NONHIST, NONOP, OTHER, MSG} type_t;
static void (*callfun[])(token_t cmd, stack_t *stks[]) = {
              binary, unary, nonhist, nonop, other, msg};

static struct funrow {
    char tok;
    void *fun;
    size_t minsz;
    type_t type;
    int has_msg;
    token_t anti;
    char *name;
} funrows[] = {
//    tok   fun  sz  type   msg? anti  name / msg
    {'\0', noop, 0u, OTHER  , 0, JUNK, "number"         }, //  NUM

    { '*',  mul, 2u, BINARY , 0, JUNK, "multiply"       }, //  MUL
    { '+',  add, 2u, BINARY , 0, JUNK, "add"            }, //  ADD
    { '^', powe, 2u, BINARY , 0, JUNK, "power"          }, // POWE
    { '/', divi, 2u, BINARY , 0, JUNK, "divide"         }, // DIVI
    { '-',  sub, 2u, BINARY , 0, JUNK, "subtract"       }, //  SUB
    { 'v', root, 2u, BINARY , 1, JUNK, "root"           }, // ROOT

    { 'l', logn, 1u, UNARY  , 1, JUNK, "log"            }, // LOGN
    { 'e', expe, 1u, UNARY  , 1, JUNK, "exp"            }, // EXPE

    { '~', neg , 1u, NONHIST, 1,  NEG, "negate"         }, //  NEG
    { 'i', inve, 1u, NONHIST, 1, INVE, "invert"         }, // INVE
    { 'c', copy, 1u, NONHIST, 1, JUNK, "copy"           }, // COPY
    { 's', swap, 2u, NONHIST, 1, SWAP, "swap"           }, // SWAP
    { 'r', rold, 2u, NONHIST, 1, ROLU, "rolldown"       }, // ROLD
    { 'u', rolu, 2u, NONHIST, 1, ROLD, "rollup"         }, // ROLU

    { 'd', noop, 1u, OTHER  , 1, JUNK, "discard"        }, // DISC

    { '_', noop, 0u, NONOP  , 1, JUNK, "undo"           }, // UNDO
    { 'w', noop, 1u, NONOP  , 1, JUNK, "dumpstack"      }, // DUMP
    { 't', noop, 0u, NONOP  , 1, JUNK, "togglehist"     }, // HTOG
    { 'q', noop, 0u, NONOP  , 1, JUNK, "quit"           }, // QUIT
    { 'h', noop, 0u, NONOP  , 1, JUNK, "help"           }, // HELP
    { 'n', noop, 0u, NONOP  , 1, JUNK, "numberrange"    }, // RANG

    {'\0', noop, 0u, OTHER  , 0, JUNK, "Junk"           }, // JUNK
    {'\0', noop, 0u, MSG    , 1, JUNK, "Divide by zero" }, // DBYZ
    {'\0', noop, 0u, MSG    , 1, JUNK, "Overflow"       }, // OFLW
    {'\0', noop, 0u, MSG    , 1, JUNK, "Underflow"      }, // UFLW
    {'\0', noop, 0u, MSG    , 1, JUNK, "Invalid num"    }, // INAN
    {'\0', noop, 0u, MSG    , 1, JUNK, "Stack too small"}, // SMAL
    {'\0', noop, 0u, MSG    , 1, JUNK, "No undo history"}, // SMLU
}; // wall-to-wall padding


// no commas after these h, n msg strings. index stays the same
// strings get concatenated when printed, need newlines
static const char *multiline_messages[] = {
    "rpn, Reverse Polish Notation floating point calculator\n"
    "Input number to push to the stack. hex format, inf and nan work too\n"
    "Operators: + * - /,    ^ power, v root, e exp, l log\n"
    " Commands: ~ negate, i invert, c copy, d discard, s swap,\n"
    "           r rolldown, u rollup, w dump stack, t toggle history,\n"
    "           _ undo, h this help, n number range, q quit",

    // not #include'ing <float.h> for these limits
    // redo the numbers for other types
    "IEEE 754 says long doubles have 30 digit precision\n"
    "[ ± LDBL_MIN: ± 3.3621e-4932  ]\n"
    "[ ± LDBL_MAX: ± 1.18973e+4932 ]", // no newline in the last strings
};


static char *hist_sep =
    "-------------------------------------------------------------------------";
static char *display_sep =
    "=========================================================================";
static char *rpn_prompt = "#> ";

void printmsg(token_t msgcode);
void printmsg_fresh(token_t msgcode, token_t *last_msgp);
// supress printing in batch mode
void donot_printmsg(token_t msgcode);
void donot_printmsg_fresh(token_t msgcode, token_t *last_msgp);
void (*p_printmsg)(token_t msgcode);
void (*p_printmsg_fresh)(token_t msgcode, token_t *last_msgp);

void dump_stack(stack_t *stack);

void display(int *hist_flagp, stack_t *stks[]);

// indices for the rpn_stacks array
// 0: interactive stack
// 1: history nums and 2: history cmds are for restoring the interactive stack
enum {I_STK, H_NUMS, H_CMDS};

int handle_input(int *hist_flagp,
                 token_t *last_msgp,
                 char *inputbuf,
                 stack_t *stks[]);


// ___ prototypes for when you write tests. not used in main ___________________

# ifdef RPN_TEST

// convenience, not for H_CMDS
RPN_T pop(stack_t *stk);
RPN_T top(stack_t *stk);
void push(RPN_T item, stack_t *stk);
RPN_T transfer(stack_t *src_stk, stack_t *dest_stk);

void display_stack(void (*print_item)(void*),
                   void *itemp,
                   size_t display_len,
                   stack_t *stack);

void display_history(stack_t *stks[]);

void print_num(void *itemp);
void print_cmdname(void *itemp);

void toggle(int *flag);

void undo(token_t *last_msgp, stack_t *stks[]);
token_t math_error(void);
void vet_do(int *hist_flagp,
            token_t *last_msgp,
            RPN_T inputnum,
            token_t cmd,
            stack_t *stks[]);

token_t tokenize(char *inputbuf, RPN_T *inputnum);

# endif // RPN_TEST
#endif // RPNFUNCTIONS_H

