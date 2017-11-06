#include "FunctionExpression.hpp"

#include "SignatureExpression.hpp"
#include "StatementExpression.hpp"

FunctionExpression::FunctionExpression(unique_ptr<Expression> sig,
				       vector<unique_ptr<Expression>>& stmts)
   : signature (move(sig))
   , statements (move(stmts))
{
}

ostream&
FunctionExpression::print (ostream& stream)
{
   stream << "FunctionExpression: " << endl;

   stream << "[Function signature:]" << endl << *signature;

   for (unsigned int i = 0; i < statements.size(); ++i)
   {
      stream << "[Function statement " << i << ":]" << *statements[i];
   }

   return stream << endl << "FunctionExpression end" << endl;
}

unique_ptr<Expression>
FunctionExpression::Parse(token_stream& str,
			  ParseInfo info)
{
   unique_ptr<Expression> sig = SignatureExpression::Parse(str, info);

   if (!sig)
   {
      Log::log_error(Error(0, 0, string("Failed to parse SignatureExpression.")));
      
      return nullptr;
   }

   //Check for {; if none, it's just a signature
   if (str.cur_tok().GetKind() != token_kind::BRACE_OPEN)
      return sig;

   //Eat {
   str.get();

   //Parse body
   vector<unique_ptr<Expression>> stmts;

   set<string> tempVars;
   
   while (true)
   {
      if (str.cur_tok().GetKind() == token_kind::BRACE_CLOSE)
      {
	 //Eat }
	 str.get();

	 break;
      }

      //You can remove this later - see below
      //(Currently just to rule out hanging)
      if (str.cur_tok().GetKind() == token_kind::END)
      {
	 Log::log_error(Error(0, 0,
			      string("Expected a }; got end of file.")));

	 return nullptr;
      }

      //TODO: some way of checking that these are the right
      //statements. That way you can tell if there's a missing }
      
      //Actually that could be a fallback after an invalid token -
      //check if it's a base-level expression (declaration of
      //function, struct, etc.)

      unique_ptr<Expression> stmt = StatementExpression::Parse(str,
							       info);

      if (!stmt)
      {
	 Log::log_error(Error(0, 0,
			      string("Failure parsing statement.")));
	 return nullptr;
      }

      /*
	Check there is such a variable name if it's either a declaration of a
	variable or a statement regarding it

	TODO: Could check type constraints right about now, too

	! This won't work: scope! It assumes the whole body is just
          one block.

	Maybe there should be a block-specific Expression which
	checks.
	It could recurse.

	Obvious way generally is to have a stack of names, just as scoping
	blocks imply. (Especially if you have an anonymous {}
	expression like C.)

	So in fact you could do it at the Parse level, by keeping that
	stack outside the stmt loop. But that would imply yet more
	expansive reflection methods for Expressions because you'd
	have to check whether they were scoping expressions, then
	check the blocks within those control expressions...
	Maybe you'll have to do that anyway, even if somewhere else.

	TODO, one way or the other
      */
      string lhs = stmt->GetSubject();

      //Scope checking currently done at Generate() level
      
      stmts.push_back(move(stmt));
   }

   return make_unique<FunctionExpression>(move(sig),
					  stmts);
}
