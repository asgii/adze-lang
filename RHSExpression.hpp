#pragma once

#include "Expression.hpp"

/*
  Helper class: anything on the right-hand of an Assign, for
  disambiguating later into component parts. Effectively a wrapper for
  binary operations and parentheses.

  Still probably better than just a function (even if it only has one
  member function and no state), since it keeps structure visible.

  Also used generally for things that should be reducible to a value,
  eg within ParenExpression, and in call params delimited by commas.

  TODO rename, eg, to ValReducibleExpression?
*/

class RHSExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
};
