#pragma once

#include "Expression.hpp"

/*
  An assignment, i.e., _ = ___;

  Could have a common, empty Expression with InitVarExpression.
*/

class AssignExpression : public Expression
{
private:
   unique_ptr<Expression> lhs;
   unique_ptr<Expression> rhs;
   
public:
   AssignExpression(unique_ptr<Expression> left,
		    unique_ptr<Expression> right);

   ostream& print (ostream& stream) override;

   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info,
				       unique_ptr<Expression> left);
   
   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;

   string GetSubject() override;
};
