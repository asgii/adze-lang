adze:
	clang++ -g parser.cpp generator.cpp lexer.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core native` -std=c++14 -O3 -o adze

.PHONY:	clean

clean:
	rm -f adze
