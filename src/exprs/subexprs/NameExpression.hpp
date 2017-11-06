#pragma once

#include "../Expression.hpp"

/*
  A name, either of a variable or a function.
*/

class NameExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
};
