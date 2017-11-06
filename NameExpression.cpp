#include "NameExpression.hpp"

#include "VarExpression.hpp"
#include "CallExpression.hpp"
#include "BinaryExpression.hpp"

unique_ptr<Expression>
NameExpression::Parse(token_stream& str,
		      ParseInfo info)
{
   //either a VarExpression or a function call. Should only be on the
   //rhs of an assign (not an assign ref).
   
   //If a variable, should be within some structure.
   //For a genuine variable (eg rhs of a '= or whatever) use VarExpression.

   const string& curName = str.cur_tok().GetValue();

   //Can't eat here - if there aren't parentheses, must be Parsed as variable.

   if (str.peek().GetKind() == token_kind::PAREN_OPEN)
   {
      //Eat function name
      str.get();
      
      //Eat (
      str.get();
      
      //Function call (not definition).      
      //I'm just going to do calls inline here.

      vector<unique_ptr<Expression>> args;

      while (true)
      {
	 switch (str.cur_tok().GetKind())
	 {
	    case token_kind::PAREN_CLOSE:
	    {
	       //Eat )
	       str.get();

	       return make_unique<CallExpression>(curName,
						  move(args));
	       //Does this need 'move'-ing?
	    }

	    //TODO: Would be useful to have a token_kind::COMMA here

	    //You want a comma-delimited sequence of value-reducible
	    //expressions, i.e., RHSExpressions.
	    //TODO actually not just that. A function could take a ref
	    //instead. So need to check for that, then whether it
	    //continues (in a binary op - in that case it'd be an RHS)
	    
	    default:
	    {
	       unique_ptr<Expression> arg = nullptr;

	       //Need to disambiguate ref arguments from
	       //value-reducible ref arguments. But can only really do
	       //that at Generation, assuming there's no '
	       //requirement. So check if it's alone - refs should
	       //only be alone (since you can't operate on them)
	       if (info.get_binary_precedence(str.peek()))
	       {
		  //It's part of a binary op - can't be a ref
		  arg = BinaryExpression::Parse(str,
						info,
						VarExpression::Parse(str,
								     info));
	       }

	       else
	       {
		  //Could still be a ref...
		  //TODO: figure out how to disambiguate this when you
		  //want to add refs. A good start would be having one
		  //Expression for both vars and refs, which,
		  //depending on whether it found the name in var or
		  //ref scope, Generated differently.
		  
		  arg = VarExpression::Parse(str,
					     info);
	       }

	       if (!arg)
	       {
		  Log::log_error(Error(0, 0,
				       string("Expected a variable or variable-reducible expression as an argument to a call.")));
		  return nullptr;
	       }

	       args.push_back(move(arg));

	       //Don't need to eat; will have been done by
	       //Expression::Parse().

	       break; //the switch, not the while
	    }
	 }
      }
   }

   else return VarExpression::Parse(str,
				    info);
}
