# USES llvm-3.8.0

CXXFLAGS = -I/usr/lib/llvm-3.8/include -fPIC -fvisibility-inlines-hidden -pedantic -std=c++11 -ffunction-sections -fdata-sections -O2 -g -DNDEBUG  -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS

cc: cc.cpp c.tab.cpp c.lex.cpp semanticAnalysis.cpp localOpt.cpp optPasses.cpp codeGeneration.cpp 
	@clang++ -g c.tab.cpp c.lex.cpp semanticAnalysis.cpp localOpt.cpp optPasses.cpp codeGeneration.cpp cc.cpp -lm -ll -lfl $(CXXFLAGS) `llvm-config --ldflags --system-libs --libs all` -o $@

c.tab.cpp c.tab.hpp: c.y
	@bison -o c.tab.cpp -d c.y

c.lex.cpp: c.l c.tab.hpp
	@flex -o c.lex.cpp -l c.l

tests: cc
	@echo "TEST CASE 1:"
	./cc tests/sel_sort.c
	@echo "\n"
	@echo "TEST CASE 1: GENERATED CODE SIZE:"
	@wc generated_code.txt
	@echo "\n"
	@echo "TEST CASE 1: GENERATED CODE RESULTS:"
	@lli generated_code.txt
	@rm generated_code.txt
	@echo "\n"
	@echo "TEST CASE 1: OPTIMIZED CODE SIZE:"
	@wc optimised_code.txt
	@echo "\n"
	@echo "TEST CASE 1: OPTIMIZED CODE RESULTS:"
	@lli optimised_code.txt
	@rm optimised_code.txt
	@echo "\n"

	@echo "TEST CASE 2:"
	./cc tests/fib_recursive.c
	@echo "\n"
	@echo "TEST CASE 2: GENERATED CODE SIZE:"
	@wc generated_code.txt
	@echo "\n"
	@echo "TEST CASE 2: GENERATED CODE RESULTS:"
	@lli generated_code.txt
	@rm generated_code.txt
	@echo "\n"
	@echo "TEST CASE 2: OPTIMIZED CODE SIZE:"
	@wc optimised_code.txt
	@echo "\n"
	@echo "TEST CASE 2: OPTIMIZED CODE RESULTS:"
	@lli optimised_code.txt
	@rm optimised_code.txt
	@echo "\n"

	@echo "TEST CASE 3:"
	./cc tests/array_try.c
	@echo "\n"
	@echo "TEST CASE 3: GENERATED CODE SIZE:"
	@wc generated_code.txt
	@echo "\n"
	@echo "TEST CASE 3: GENERATED CODE RESULTS:"
	@lli generated_code.txt
	@rm generated_code.txt
	@echo "\n"
	@echo "TEST CASE 3: OPTIMIZED CODE SIZE:"
	@wc optimised_code.txt
	@echo "\n"
	@echo "TEST CASE 3: OPTIMIZED CODE RESULTS:"
	@lli optimised_code.txt
	@rm optimised_code.txt
	@echo "\n"

clean::
	rm -f c.tab.cpp c.tab.hpp c.lex.cpp cc c.output code.txt generated_code.txt ssa.txt optimised_code.txt
