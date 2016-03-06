# for pasting into a terminal:
# gcc -g3 -Wall -Wextra -lm rpnstack.c rpnfunctions.c rpn.c -o rpn

# # some profiling:
#
# gcc -fprofile-arcs -ftest-coverage -lm \
#   rpnstack.c rpnfunctions.c rpn.c -o rpn
#
# ./rpn # run it with input. file generated
# gcov -b rpnfunctions.c # or
# gcov -bf rpnfunctions.c # generates rpnfunctions.c.gcov
# grep "^function" rpnfunctions.c.gcov| cut -d' ' -f2,4| sort -g -k2| column -t
# # to see sorted number of calls per function
#
# # gprof gives a call graph table
# gcc -pg -o rpn -lm rpnstack.c rpnfunctions.c rpn.c \
#
# ./rpn # run it with input. file generated.
# gprof rpn gmon.out > gprof_analysis.txt
# make profiling_clean # to remove generated files


# rpn_test not that useful atm. keep tests in their own folder ./tests_rpn/
#rpn_test: rpnstack.o rpnfunctions.c rpnfunctions.h.gch rpn.c
	#$(CC) -o $@ -lm rpnstack.o rpnfunctions.c rpn.c -DRPN_TEST

# precompiled headers (*.h.gch) don't save time in this build, just fun to have
# clang precompiled headers are ~ an order of magnitude smaller than gcc ones


CC = clang -g

.PHONY: exec all clean distclean objclean headerclean profiling_clean

exec: rpn
	./rpn

all: rpn

rpn: rpn.o rpnstack.o rpnfunctions.o
	$(CC) -o $@ -lm rpnfunctions.o rpnstack.o rpn.o

rpn.o: rpnstack.h rpnfunctions.h rpn.c
	$(CC) -c rpn.c

rpnfunctions.o: rpnstack.h rpnfunctions.h rpnfunctions.c
	$(CC) -c rpnfunctions.c

rpnstack.o: rpnstack.c rpnstack.h
	$(CC) -c rpnstack.c

clean: objclean headerclean profiling_clean
	@- $(RM) rpn rpn_test

distclean: clean

headerclean:
	@- $(RM) *.h.gch

objclean:
	@- $(RM) *.o

profiling_clean:
	@- $(RM) *.out *.gcda *.gcno *.gcov

