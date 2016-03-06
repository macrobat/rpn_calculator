![calculator image](./rpn_calculator.png "rpn calculator")  

rpn_calculator  
--------------  
reverse polish notation calculator, a stack to push numbers on.  
There are two stacks for history (to restore the interactive stack).  
Toggle the history display.  
Pop and manipulate the stack with operators and commands, then undo.  
Input many numbers and commands in one line.  
It uses the long double type, so it handles hex input, inf and nan.  

To compile and launch: run make in the rpn_calculator folder.  
There's an rpn target. The Makefile uses clang.  
You could compile it like:  
gcc rpnstack.c rpnfunctions.c rpn.c -lm -o rpn  
Run the program interactively like so: ./rpn  

Operators: + * - / ^ power, v root, e exp, l log  
 Commands: ~ negate, i invert, c copy, d discard, s swap,  
           r rolldown, u rollup, w dump stack, t toggle history,  
           _ undo, h this help, n number range, q quit  

There's a batch mode if you give it commandline arguments:  
    
    ./rpn "0xf 0x7f 0xff 0x3ff"  
    15 127 255 1023  
    
    ./rpn "0 i c" "/ ~" "d 1 2 3" "+ +"  
    inf inf  
    nan  
    1 2 3  
    6  
    
    ./rpn "1 1" "c r +" "c r +" "c r +" "c r +" "c r +" "c r +" "c r +"  
    1 1  
    1 2  
    2 3  
    3 5  
    5 8  
    8 13  
    13 21  
    21 34  
    
    ./rpn "-40 0 37.8 100" "1.8 * 32 + r 1.8 * 32 + r 1.8 * 32 + r 1.8 * 32 + r" \  
    | column -t  
    -40  0   37.8    100  
    -40  32  100.04  212  
    
