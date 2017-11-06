#pragma once

#include "Expression.hpp"

class BinaryExpression : public Expression
{
private:
   token_kind op;
      
   unique_ptr<Expression> lhs;
   unique_ptr<Expression> rhs;

public:
   BinaryExpression(token_kind opKind,
		    unique_ptr<Expression> left,
		    unique_ptr<Expression> right);

   ostream& print (ostream& stream) override;
   
   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info,
				       unique_ptr<Expression> left);
   
   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;
};
