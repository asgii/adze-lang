#include "LitIntExpression.hpp"

LitIntExpression::LitIntExpression(int val)
   : value (val)
{
}

ostream&
LitIntExpression::print (ostream& stream)
{
   return stream << "LitIntExpression: " << value << endl;
}

unique_ptr<Expression>
LitIntExpression::Parse(token_stream& str,
			ParseInfo info)
{
   int result;

   if (info.get_literal_int(str.cur_tok().GetValue(), result))
   {
      //Eat literal
      str.get();
      
      return std::make_unique<LitIntExpression>(result);
   }

   else
   {
      Log::log_error(Error(0, 0,
			   string("Invalid int literal.")));
      
      return nullptr;
   }
}
