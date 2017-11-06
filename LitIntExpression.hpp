#pragma once

#include "Expression.hpp"

#include <iostream>

class LitIntExpression : public Expression
{
private:
   int value;
   
public:
   LitIntExpression(int val);

   ostream& print (ostream& stream) override;
   
   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
   
   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;
};
