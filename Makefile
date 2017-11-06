files = parser.cpp generator.cpp lexer.cpp \
	ParseBuild.cpp ParseInfo.cpp ParseScope.cpp \
	Expression.cpp \
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
	VarExpression.cpp

llvm = `llvm-config --cxxflags --ldflags --system-libs --libs core native`
flags = -std=c++14 -O2 -o adze

clang:
	clang++ $(files) $(llvm) $(flags)

gcc:
	g++ $(files) $(llvm) $(flags)

clean:
	rm -f adze
