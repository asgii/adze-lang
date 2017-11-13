#include "CallExpression.hpp"
#include "RHSExpression.hpp"

CallExpression::CallExpression(const string& funcName,
	       vector<unique_ptr<Expression>> argsGiven)
   : name (funcName)
   , args (move(argsGiven))
{
}

ostream&
CallExpression::print (ostream& stream)
{
   stream << "CallExpression: " << name << endl;

   for (unsigned int i = 0; i < args.size(); ++i)
   {
      stream << "[Call argument " << i << ":]" << endl << *args[i];
   }

   return stream << "CallExpression end" << endl;
}

unique_ptr<Expression>
CallExpression::Parse(token_stream& str,
		      ParseInfo info)
{
   const string curName = str.cur_tok().GetValue();

   //This will be called when a NAME is found with a PAREN_OPEN after.
   //So you can immediately eat both.
   
   //Eat function name
   str.get();      
   //Eat (
   str.get();

   vector<unique_ptr<Expression>> args;

   //First time, check close paren.
   if (str.cur_tok().GetKind() == token_kind::PAREN_CLOSE)
   {
      //Eat )
      str.get();

      return make_unique<CallExpression>(curName,
					 move(args));
   }

   //You're effectively expecting at least one parenthesised
   //expression by now.
   while (true)
   {
      unique_ptr<Expression> arg = RHSExpression::Parse(str, info);
      
      if (!arg)
      {
	 Log::log_error(Error(0, 0,
			      string("Expected a variable or variable-reducible expression as an argument to a call - or a closing parenthesis.")));
	 return nullptr;
      }

      args.push_back(move(arg));

      //Don't need to eat; will have been done by
      //RHSExpression::Parse().
	 
      //Next must come either a comma or a close paren
      switch (str.cur_tok().GetKind())
      {
	 case token_kind::PAREN_CLOSE:
	 {
	    //Eat )
	    str.get();

	    return make_unique<CallExpression>(curName,
					       move(args));
	 }

	 case token_kind::COMMA:
	 {
	    //Eat ,
	    str.get();
	 }
	 break;

	 default:
	 {
	    Log::log_error(Error(0, 0,
				 string("Expected a comma or closing parenthesis after an argument in a call.")));
	    return nullptr;
	 }
      }
   }
}
