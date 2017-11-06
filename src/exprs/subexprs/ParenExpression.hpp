#pragma once

#include "../Expression.hpp"

/*
  The insides of a parenthesis - an actual parenthesis, reducible to a
  value, not the parentheses in a function call.
*/

class ParenExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
};
