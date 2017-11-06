#include "ParenExpression.hpp"

#include "RHSExpression.hpp"

unique_ptr<Expression>
ParenExpression::Parse(token_stream& str,
		       ParseInfo info)
{
   //TODO: might have to add case of tuples, I mean '(int, int) = ...'

   //(assuming already checked PAREN_OPEN)

   //Eat (
   str.get();

   unique_ptr<Expression> enclosed = RHSExpression::Parse(str,
							  info);

   //Eat )
   str.get();
   
   if (str.cur_tok().GetKind() != token_kind::PAREN_CLOSE)
   {
      Log::log_error(Error(0, 0,
			   string("Expected a ) to close a parenthesised expression.")));

      return nullptr;
   }

   if (!enclosed)
   {
      Log::log_error(Error(0, 0,
			   string("Failure parsing a variable-reducible expression within parentheses.")));
   }

   return enclosed;
}
