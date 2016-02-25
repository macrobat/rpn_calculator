# for pasting into a terminal:
# gcc -g3 -Wall -Wextra -lm \
#	rpnstack.c rpnfunctions.c rpn.c -o rpn

# # try this with gcov.
# gcc -fprofile-arcs -ftest-coverage -lm \
#   rpnstack.c rpnfunctions.c rpn.c -o rpn
#
# ./rpn # run it with input. file generated
# gcov -b rpnfunctions.c # or
# gcov -bf rpnfunctions.c # generates rpnfunctions.c.gcov
# grep "^function" rpnfunctions.c.gcov| cut -d' ' -f2,4| sort -g -k2| column -t
# # to see sorted number of calls per function


# # gprof gives a call graph table
# gcc -pg -o rpn -lm rpnstack.c rpnfunctions.c rpn.c \
#
# ./rpn # run it with input. file generated.
# gprof rpn gmon.out > gprof_analysis.txt
# make profiling_clean # to remove generated files


# rpn_test not that useful atm. keep tests in their own folder ./tests_rpn/

CC = clang -g

.PHONY: exec all clean distclean oclean execlean profiling_clean

exec: rpn
	./rpn

all: rpn

rpn: rpn.o rpnstack.o rpnfunctions.o
	$(CC) -o $@ -lm rpnfunctions.o rpnstack.o rpn.o

rpn.o: rpnstack.o rpnfunctions.h rpn.c
	$(CC) -c rpn.c

rpnfunctions.o: rpnstack.o rpnfunctions.h rpnfunctions.c
	$(CC) -c rpnfunctions.c

rpn_test: rpnstack.o rpnfunctions.c rpnfunctions.h rpn.c
	$(CC) -o $@ -lm rpnstack.o rpnfunctions.c rpn.c -DRPN_TEST

rpnstack.o: rpnstack.c rpnstack.h
	$(CC) -c rpnstack.c rpnstack.h

clean: oclean execlean profiling_clean

distclean: clean

execlean:
	@- $(RM) rpn rpn_test

oclean:
	@- $(RM) *.o *.h.gch

profiling_clean:
	@- $(RM) *.out *.gcda *.gcno *.gcov

