# be sure there are no matching source files in the clean section
# @- $(RM) will remove them

# rule is a criterion for being out of date
# target: prerequisite.c prerequisite.h		# target: rules
# <tab><compiler><flags><files><-o><name>	# recipe for updating target

# -Wpedantic is just annoying

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
#
#
# # gprof gives a call graph table
# gcc -pg -o rpn -lm rpnstack.c rpnfunctions.c rpn.c \
#
# ./rpn # run it with input. file generated.
# gprof rpn gmon.out > gprof_analysis.txt
# make profiling_clean # to remove generated files


# rpn_test not that useful atm. keep tests in their own folder ./tests_rpn/
# don'tlink -c and link-with-math -lm options don't go together
# rpnstack.o recipe makes a rpnstack.o and a rpnstack.h.gch precompiled header
# @- $(RM) stack.o stack.h.gch rpnstack.o rpnstack.h.gch rpnfunctions.o rpn.o
# to be safe: -DSTACK_ELEM_T="long double" -DSTACK_ELEM_FMT='"%.10Lg"'

# lexing and yaccing apparently has a higher prio than the first recipe?
# so if there's a .y file, a "make" or "make rpn" means "make yacc"
# even if there are recipes with .y
# since there is a rpn.c file, make overwrites it. implicit rule .l --> .c
# there's no foo.o target, so make will guess
# avoid implicit .c to exec targets.

# let the targets be the generated filenames
# generated files should be gitignored and cleaned out
#flex_rpn: rpnl.y rpn.l
#	echo flex dummy
#
#bison_rpn: rpny.y
#	echo bison dummy


CC = clang -g

.PHONY: exec all clean distclean oclean

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
	$(CC) -DSTACK_ELEM_T="long double" -DSTACK_ELEM_FMT='"%.10Lg"' \
		-c rpnstack.c rpnstack.h

dontuse_oldrecipe_rpn: rpnstack.c rpnstack.h rpnfunctions.c rpnfunctions.h rpn.c
	$(CC) -o $@ -lm -DSTACK_ELEM_T="long double" -DSTACK_ELEM_FMT='"%.10Lg"' \
		rpnstack.c rpnfunctions.c rpn.c

clean: oclean execlean profiling_clean

distclean: clean

execlean:
	@- $(RM) rpn rpn_test

oclean:
	@- $(RM) *.o *.h.gch

profiling_clean:
	@- $(RM) *.out *.gcda *.gcno *.gcov

