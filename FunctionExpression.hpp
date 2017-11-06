#pragma once

#include "Expression.hpp"

/*
  Top-level expression for functions. Can represent either just a
  declaration (though those will probably be phased out of the
  language), or a full definition, including body.
*/

class FunctionExpression : public Expression
{
private:
   unique_ptr<Expression> signature;
   vector<unique_ptr<Expression>> statements;

public:
   FunctionExpression(unique_ptr<Expression> sig,
		      vector<unique_ptr<Expression>>& stmts);

   ostream& print (ostream& stream) override;

   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
   
   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;
};
