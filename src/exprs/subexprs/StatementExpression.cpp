#include "StatementExpression.hpp"

#include "ReturnExpression.hpp"
#include "AssignExpression.hpp"
#include "InitVarExpression.hpp"
#include "VarExpression.hpp"
#include "CallExpression.hpp"

unique_ptr<Expression> StatementExpression::Parse(token_stream& str,
						  ParseInfo info)
{
   //This function is the main one that checks SEMICOLONs.

   if (str.cur_tok().GetKind() == token_kind::KEY_RETURN)
   {
      return ReturnExpression::Parse(str, info);
   }

   unique_ptr<Expression> stmt = nullptr;
   
   /*
     If not control flow, must be an assign, init, or call
     (Does not guarantee it is a type token; just checks validity)
     TODO best replace this actually - is_type_token should eventually
     check for naming rules, which might be different from variable
     name rules.

     Following is single-exit for clarity; semicolons checked at the end.
     (Return, on the other hand, is helped by its own semicolon
     checking.)

     TODO: temporary and misleading. is_type_token only
     coincidentally checks assigns without init-vars, because
     it also checks for NAMEs that could be (custom) type names
   */
   if (info.is_type_token(str.cur_tok().GetKind()))
   {
      //Check if call (here because call-parsing needs name)
      if ((str.cur_tok().GetKind() == token_kind::NAME) and
	  (str.peek().GetKind() == token_kind::PAREN_OPEN))
      {
	 stmt = CallExpression::Parse(str, info);
      }

      else //Not a call; init or assign
      {
	 token nmTok = str.cur_tok();
	 const string nm = str.cur_tok().GetValue();

	 //Eat type/variable name
	 str.get();

	 //If only one name, assignment (or call); if more, init
	 if (str.cur_tok().GetKind() != token_kind::NAME)
	 {
	    //Assignment
	    //(nm must be a name, not a type)
	    if (str.cur_tok().GetKind() == token_kind::OP_ASSIGN_VAL)
	    {
	       //Is the lhs a var or a ref?
	       //(This will make more sense when refs are implemented)

	       //Don't eat =, AssignExpression does that.

	       if ((nm.size() == 1) or
		   (nm[nm.size() - 1] != '\''))
	       {
		  //It's a var
		  //TODO: see below, else clause
		  stmt = AssignExpression::Parse(str, info,
						 make_unique<VarExpression>(nm,
									    string()));
	       }

	       //TODO else value assignment to ref alternative
	       //(Better doing a separate ref type check?)

	       //Temporary, just in case (TODO replace)
	       else
	       {
		  Log::log_error(Error(0, 0, string("Invalid variable name in assignment.")));

		  return nullptr;
	       }

	       //Else implicitly not a valid name?
	    }
      
	    //TODO else ref_assign
      
	    else
	    {
	       Log::log_error(Error(0, 0,
				    string("Expected assignment.")));
	       return nullptr;
	    }

	    //Stmt returned at the end
	 }

	 //2 names in a row; must be init; nm must be type
	 else
	 {
	    //Decide whether nm is Ref or Var type

	    switch (nmTok.GetKind())
	    {
	       case token_kind::TYPE_INT:
	       case token_kind::TYPE_FLOAT:
	       case token_kind::TYPE_STRING:
	       {
		  //It's a var
		  const string varNm = str.cur_tok().GetValue();

		  //Eat var name
		  str.get();
	       
		  stmt = make_unique<InitVarExpression>(varNm, nm);

		  break;
	       }
	    
	       case token_kind::NAME:
	       {
		  if (nm.size() < 1)
		  {
		     //TODO log. This would really be an error on the part of the parser
		     Log::log_error(Error(0, 0,
					  string("Name parsed with no characters.")));
	 
		     return nullptr;
		  }

		  if (nm[nm.size() - 1] != '\'')
		  {
		     //It's a var	 
		     const string varNm = str.cur_tok().GetValue();

		     //Eat var name
		     str.get();
	 
		     //Don't really need the Parse tbh
		     stmt = make_unique<InitVarExpression>(varNm, nm);
		  }

		  //TODO else InitRefExpression...

		  break;
	       }

	       default:
	       {
		  Log::log_error(Error(0, 0,
				       string("Expected a name to be initialised.")));
		  return nullptr;
	       }
	    }

	    //Either way, that Init might be part of an assign (an actual
	    //initialisation rather than just a declaration)

	    if (str.cur_tok().GetKind() == token_kind::OP_ASSIGN_VAL)
	    {
	       //Extremely TODO: this must be scoped above, because of
	       //VarRef, RefVar etc. ambiguities. This only works now
	       //because of no refs
	       stmt = AssignExpression::Parse(str, info,
					      move(stmt));
	    }

	    //TODO else ref_assign

	    //Else not an error - just return the init alone, as a
	    //statement.
	 }
      }
   }

   else
   {
      Log::log_error(Error(0, 0,
			   string("Expected statement within function body.")));
      return nullptr;
   }

   //Check semicolon at end
   if (str.cur_tok().GetKind() != token_kind::SEMICOLON)
   {
      Log::log_error(Error(0, 0,
			   string("Expected ; closing statement.")));

      //TODO remove
      cout << str.cur_tok();//
      //
      
      return nullptr;
   }

   //Eat ;
   str.get();
	    
   return stmt;
}
