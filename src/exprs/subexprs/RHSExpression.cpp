#include "RHSExpression.hpp"

#include "ParenExpression.hpp"
#include "LitIntExpression.hpp"
#include "NameExpression.hpp"
#include "BinaryExpression.hpp"

unique_ptr<Expression>
RHSExpression::Parse(token_stream& str,
		     ParseInfo info)
{
   //NB: don't assume it's ended by a semicolon; these expressions are
   //just meant to be value-reducible and might, e.g., be delimited by commas.

   //Wrapper for (potential) binary op
   unique_ptr<Expression> cur = nullptr;
   
   //First symbol
   switch (str.cur_tok().GetKind())
   {
      case token_kind::LIT_INT:
      {
	 cur = LitIntExpression::Parse(str, info);
      }
      break;

   //TODO: other literals

      case token_kind::PAREN_OPEN:
      {
	 cur = ParenExpression::Parse(str, info);
      }
      break;

      case token_kind::NAME:
      {
	 cur = NameExpression::Parse(str, info); //Includes function calls
      }
      break;

      default: //to silence Wswitch
	 break;
   }

   if (!cur)
   {
      Log::log_error(Error(0, 0,
			   string("Failure parsing value-reducible expression.")));
      return nullptr;
   }

   //Check if op next (no get() because of above Parse()s)
   
   if (info.get_binary_precedence(str.cur_tok()))
   {
      //Binary ops are already recursive; it should go to the end of
      //the RHS
      return BinaryExpression::Parse(str, info,
				     move(cur));
   }

   else return cur;
}
