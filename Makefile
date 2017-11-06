subexprs = $(addprefix subexprs/, \
	AssignExpression.cpp \
	BinaryExpression.cpp \
	CallExpression.cpp \
	FunctionExpression.cpp \
	InitVarExpression.cpp \
	LitIntExpression.cpp \
	NameExpression.cpp \
	ParenExpression.cpp \
	ReturnExpression.cpp \
	RHSExpression.cpp \
	SignatureExpression.cpp \
	StatementExpression.cpp \
	VarExpression.cpp)

exprs = $(addprefix exprs/, Expression.cpp $(subexprs))

parse = Parser.cpp ParseBuild.cpp ParseInfo.cpp ParseScope.cpp

others = generator.cpp lexer.cpp

files = $(addprefix src/, $(parse) $(exprs) $(others))

llvm = `llvm-config --cxxflags --ldflags --system-libs --libs core native`
flags = -std=c++14 -O2 -o adze

clang:
	clang++ $(files) $(llvm) $(flags)

gcc:
	g++ $(files) $(llvm) $(flags)

clean:
	rm -f adze
