rpn_calculator
==============  
![calculator image](./rpn_calculator.svg "rpn calculator")  
reverse polish notation calculator  
It uses one stack to push numbers on.  
It uses the long double type, so it handles hex input, inf and nan.  
There are two stacks for history (to restore the interactive stack).  
Pop and manipulate the stack with operators and commands, then undo.  
Input many numbers and commands in one line.  
Toggle the history display.  

There is no trig.  

To compile and launch: run make in the rpn_calculator folder.  
The Makefile uses clang.  Compile in your favourite manner.  

