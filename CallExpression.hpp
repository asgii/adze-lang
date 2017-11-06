#pragma once

#include "Expression.hpp"

#include <iostream>

/*
  This is a stub: simple enough that it can just be parsed by callers,
  through its constructor.

  TODO still might want to give it a Parse for standardisation's sake.
*/

class CallExpression : public Expression
{
private:
   string name;
   vector<unique_ptr<Expression>> args;
      
public:
   CallExpression(const string& funcName,
		  vector<unique_ptr<Expression>> argsGiven);

   ostream& print (ostream& stream) override;

   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;
};
