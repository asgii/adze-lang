#pragma once

#include "Expression.hpp"

/*
  This doesn't generate anything - it's effectively a helper class.
  
  A control statement (including if etc), an assign of some
  kind, or an init
*/

class StatementExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
};
