files = parser.cpp generator.cpp lexer.cpp
llvm = `llvm-config --cxxflags --ldflags --system-libs --libs core native`
flags = -std=c++14 -O2 -o adze

clang:
	clang++ $(files) $(llvm) $(flags)

gcc:
	g++ $(files) $(llvm) $(flags)

clean:
	rm -f adze
