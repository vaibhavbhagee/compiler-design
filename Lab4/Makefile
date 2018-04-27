# USES llvm-3.8.0

CXXFLAGS = -I/usr/lib/llvm-3.8/include -fPIC -fvisibility-inlines-hidden -pedantic -std=c++11 -ffunction-sections -fdata-sections -O2 -g -DNDEBUG  -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS

cc: cc.cpp c.tab.cpp c.lex.cpp semanticAnalysis.cpp codeGeneration.cpp localOpt.cpp
	clang++ -g c.tab.cpp c.lex.cpp semanticAnalysis.cpp codeGeneration.cpp localOpt.cpp cc.cpp -lm -ll -lfl $(CXXFLAGS) `llvm-config --ldflags --system-libs --libs core` -o $@

c.tab.cpp c.tab.hpp: c.y
	bison -o c.tab.cpp -d c.y

c.lex.cpp: c.l c.tab.hpp
	flex -o c.lex.cpp -l c.l

tests: cc
	./cc tests/sel_sort.c
	lli generated_code.txt
	rm generated_code.txt

	./cc tests/sel_sort_monolithic.c
	lli generated_code.txt
	rm generated_code.txt

	./cc tests/fib_recursive.c
	lli generated_code.txt
	rm generated_code.txt

	./cc tests/array_try.c
	lli generated_code.txt
	rm generated_code.txt

clean::
	rm -f c.tab.cpp c.tab.hpp c.lex.cpp cc c.output code.txt
