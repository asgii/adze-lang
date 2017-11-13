#pragma once

#include "../Expression.hpp"

#include <iostream>

class CallExpression : public Expression
{
private:
   string name;
   vector<unique_ptr<Expression>> args;
      
public:
   CallExpression(const string& funcName,
		  vector<unique_ptr<Expression>> argsGiven);

   ostream& print (ostream& stream) override;

   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);

   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;
};
