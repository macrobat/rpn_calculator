rpn_calculator
==============
![calculator image](./rpn_calculator.svg "rpn calculator")  
reverse polish notation calculator  
It uses one stack to push numbers on, two stacks for history.  
You can pop and manipulate the stack with operators and commands, then undo.  
You can input many numbers and commands in one line.  
You can toggle history display.  
It almost tokenizes input, then often calls a corresponding function.  
It uses the long double type, so it handles hex input, inf and nan.  

no exponential, no natural logarithm. no pi or trig.  

run make.
the Makefile uses clang.  

