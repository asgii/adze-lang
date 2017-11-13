#include "NameExpression.hpp"

#include "VarExpression.hpp"
#include "CallExpression.hpp"

unique_ptr<Expression>
NameExpression::Parse(token_stream& str,
		      ParseInfo info)
{
   //either a VarExpression or a function call. Should only be on the
   //rhs of an assign (not an assign ref).
   
   //If a variable, should be within some structure.
   //For a genuine variable (eg rhs of a '= or whatever) use VarExpression.

   //Can't eat here - if there aren't parentheses, must be Parsed as variable.

   if (str.peek().GetKind() == token_kind::PAREN_OPEN)
   {
      return CallExpression::Parse(str, info);
   }   

   else return VarExpression::Parse(str, info);
}
