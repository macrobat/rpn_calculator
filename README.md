rpn_calculator  
--------------  
![calculator image](./rpn_calculator.png "rpn calculator")  
reverse polish notation calculator, a stack to push numbers on.  
There are two stacks for history (to restore the interactive stack).  
Toggle the history display.  
Pop and manipulate the stack with operators and commands, then undo.  
Input many numbers and commands in one line.  
It uses the long double type, so it handles hex input, inf and nan.  

To compile and launch: run make in the rpn_calculator folder.  
There's a rpn target. The Makefile uses clang.  
You could compile it like:  
gcc rpnstack.c rpnfunctions.c rpn.c -lm -o rpn  
Run the program interactively like so: ./rpn  
There's a batch mode if you give it commandline arguments:  

    user@host$ ./rpn "0 i c" "/ ~" "d 1 2 3" "+ +"  
    inf inf  
    nan  
    1 2 3  
    6  
     
    user@host$ ./rpn "1 1" "c r +" "c r +" "c r +" "c r +" "c r +" "c r +" "c r +"  
    1 1  
    1 2  
    2 3  
    3 5  
    5 8  
    8 13  
    13 21  
    21 34  

