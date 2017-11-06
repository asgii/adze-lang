#include "ReturnExpression.hpp"

#include "RHSExpression.hpp"

ReturnExpression::ReturnExpression(vector<unique_ptr<Expression>> rs)
   : rets (move(rs))
{
}

ostream&
ReturnExpression::print (ostream& stream)
{
   stream << "ReturnExpression: " << endl;

   for (unsigned int i = 0; i < rets.size(); ++i)
   {
      stream << "[Return " << i << ":]" << endl << *rets[i];
   }

   return stream << "ReturnExpression end" << endl;
}

unique_ptr<Expression> ReturnExpression::Parse(token_stream& str,
					       ParseInfo info)
{
   //Eat 'return'
   str.get();

   vector<unique_ptr<Expression>> rs;
   
   while (str.cur_tok().GetKind() != token_kind::SEMICOLON)
   {
      if (str.cur_tok().GetKind() == token_kind::COMMA)
      {
	 //Eat comma
	 str.get();
	 
	 continue;
      }
      
      unique_ptr<Expression> cur = RHSExpression::Parse(str,
							info);

      if (cur)
      {
	 rs.push_back(move(cur));
      }

      else
      {
	 Log::log_error(Error(0, 0,
			      string("Expected a value-reducible expression as part of a return statement.")));
	 return nullptr;
      }
   }

   //Eat semicolon
   str.get();

   return make_unique<ReturnExpression>(move(rs));
}
