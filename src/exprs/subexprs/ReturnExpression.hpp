#pragma once

#include "../Expression.hpp"

/*
  A return statement. (Whether or not any returned values?)
*/

class ReturnExpression : public Expression
{
private:
   vector<unique_ptr<Expression>> rets;

public:
   ReturnExpression(vector<unique_ptr<Expression>> rs);
   
   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
   
   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;

   ostream& print (ostream& stream) override;
};
