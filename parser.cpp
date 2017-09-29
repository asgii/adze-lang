#include "parser.hpp"
#include "log.hpp"

using namespace std;

void Parser::print()
{
   for (unsigned int i = 0; i < parsed.size(); ++i)
   {
      cout << *parsed[i].get();
   }
}

bool Parser::get_literal_int(const string& str,
			     int& result)
{
   //These could be replaced by checking exceptions on stoi().  But
   //as it is, might be that there are more stringent requirements
   //for ints than in C++.

   //TODO: Check over size? Note that stoi might throw out_of_range exception.

   //First char can be -
   if (str[0] == '-')
   {
      for (unsigned int i = 1; i < str.size(); ++i)
      {
	 if (!isdigit(str[i]))
	    return false;
      }
   }

   else
   {
      for (unsigned int i = 0; i < str.size(); ++i)
      {
	 if (!isdigit(str[i]))
	    return false;
      }
   }

   result = stoi(str);

   return false;
}

//TODO: get_literal_float, get_literal_string (not sure how latter works)

int Parser::get_binary_precedence(const token& tok)
{
   //This is somewhat redundant with the OP_* token_kinds...

   //It's this way because of the semantics of operator[] in std: need
   //to isolate uses so they don't create an empty member of the
   //map. Thus, no separate function for is_binary etc., to guarantee
   //get_ is only called after is_.
   
   //TODO Cleanup: use this approach in general, unfriend Expressions from
   //Parser, etc.

   if (precedences.count(tok.GetKind()))
      return precedences[tok.GetKind()];

   else return 0;
}

//

unique_ptr<Expression> LitIntExpression::Parse(Parser& prs)
{
   int result;

   if (prs.get_literal_int(prs.cur_tok.GetValue(), result))
   {
      //Eat literal
      prs.get();
      
      return std::make_unique<LitIntExpression>(result);
   }

   else
   {
      Log::log_error(Error(0, 0,
			   string("Invalid int literal.")));
      
      return nullptr;
   }
}

unique_ptr<Expression> VarExpression::Parse(Parser& prs)
{
   //ie, the name of a variable

   const string& name = prs.cur_tok.GetValue();

   //Eat NAME
   prs.get();

   //TODO: Check validity of name before committing to construction?
   //or in the lexer?

   return make_unique<VarExpression>(name);
}

unique_ptr<Expression> NameExpression::Parse(Parser& prs)
{
   //either a VarExpression or a function call. Should only be on the
   //rhs of an assign (not an assign ref).
   
   //If a variable, should be within some structure.
   //For a genuine variable (eg rhs of a '= or whatever) use VarExpression.

   const string& curName = prs.cur_tok.GetValue();

   //Can't eat here - if there aren't parentheses, must be Parsed as variable.

   if (prs.peek().GetKind() == token_kind::PAREN_OPEN)
   {
      //Eat function name
      prs.get();
      
      //Eat (
      prs.get();
      
      //Function call (not definition).      
      //I'm just going to do calls inline here.

      vector<unique_ptr<Expression>> args;

      while (true)
      {
	 switch (prs.cur_tok.GetKind())
	 {
	    case token_kind::PAREN_CLOSE:
	    {
	       //Eat )
	       prs.get();

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
	       if (prs.get_binary_precedence(prs.peek()))
	       {
		  //It's part of a binary op - can't be a ref
		  arg = BinaryExpression::Parse(prs,
						VarExpression::Parse(prs));
	       }

	       else
	       {
		  //Could still be a ref...
		  //TODO: figure out how to disambiguate this when you
		  //want to add refs. A good start would be having one
		  //Expression for both vars and refs, which,
		  //depending on whether it found the name in var or
		  //ref scope, Generated differently.
		  
		  arg = VarExpression::Parse(prs);
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

   else return VarExpression::Parse(prs);
}

unique_ptr<Expression> BinaryExpression::Parse(Parser& prs,
					       unique_ptr<Expression> left)
{
   /*
     For clarity: this function starts with the lhs already parsed
     (left), with cur_tok on a binary expression. get() gets the rhs
     of the original binary expression (right). After that, we check
     for -another- binary expression occurring without parentheses;
     that's a situation which needs disambiguating.
   */

   token_kind kind = prs.cur_tok.GetKind();   
   int precedence = prs.get_binary_precedence(kind);

   //Eat binary op   
   prs.get();
   
   unique_ptr<Expression> right = RHSExpression::Parse(prs);

   if (!right)
   {
      Log::log_error(Error(0, 0,
			   string("Failure parsing right-hand side of a binary operation.")));
      
      return nullptr;
   }
   
   //Parse should eat the last relevant token; no get().

   token_kind furtherKind = prs.cur_tok.GetKind();
   int furtherPrecedence = prs.get_binary_precedence(furtherKind);

   //Don't get() here either; it's to be parsed by another call
   //whether or not it's another binary op.

   //ie if it's a binary op
   if (furtherPrecedence)
   {
      if (precedence >= furtherPrecedence)
      {
	 //Eat up this binary op and feed into a new BinaryExpression, as
	 //new left
	 return BinaryExpression::Parse(prs,
					make_unique<BinaryExpression>(kind,
								      move(left),
								      move(right)));
								      
      }

      else
      {
	 /*
	   Eat up the next one, by making a new parse of it, and feed
	   it into this one.

	   Whichever 'goes first', ie has the highest precedence, is
	   not the one that's returned directly, since height in the
	   tree represents backward-order of operations; the leaves
	   are evaluated first.
	  */
	 return make_unique<BinaryExpression>(furtherKind,
					      BinaryExpression::Parse(prs, move(right)),
					      move(left));
      }
   }

   //i.e., if the now current token isn't another binary op.
   else return make_unique<BinaryExpression>(kind, move(left), move(right));
}

unique_ptr<Expression> ParenExpression::Parse(Parser& prs)
{
   //TODO: might have to add case of tuples, I mean '(int, int) = ...'

   //(assuming already checked PAREN_OPEN)

   //Eat (
   prs.get();

   unique_ptr<Expression> enclosed = RHSExpression::Parse(prs);

   //Eat )
   prs.get();
   
   if (prs.cur_tok.GetKind() != token_kind::PAREN_CLOSE)
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

unique_ptr<Expression> FunctionExpression::Parse(Parser& prs)
{
   //Either a full function or just a signature

   unique_ptr<Expression> sig = SignatureExpression::Parse(prs);

   if (!sig)
   {
      Log::log_error(Error(0, 0, string("Failed to parse SignatureExpression.")));
      
      return nullptr;
   }

   //Check for {; if none, it's just a signature
   if (prs.cur_tok.GetKind() != token_kind::BRACE_OPEN)
      return sig;

   //Eat {
   prs.get();

   //Parse body
   vector<unique_ptr<Expression>> stmts;

   set<string> tempVars;
   
   while (true)
   {
      if (prs.cur_tok.GetKind() == token_kind::BRACE_CLOSE)
      {
	 //Eat }
	 prs.get();

	 break;
      }

      //You can remove this later - see below
      //(Currently just to rule out hanging)
      if (prs.cur_tok.GetKind() == token_kind::END)
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

      unique_ptr<Expression> stmt = StatementExpression::Parse(prs);

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

unique_ptr<Expression> SignatureExpression::Parse(Parser& prs)
{
   vector<string> rs;
   vector<tuple<string, string>> args;

   string name;

   //Return values, and function name

   /*
     For ease of parsing, multiple return types must be enclosed in ().
    */

   if (prs.cur_tok.GetKind() == token_kind::PAREN_OPEN)
   {
      //Eat (
      prs.get();

      while (true)
      {
	 if (prs.cur_tok.GetKind() == token_kind::NAME)
	 {
	    if (!prs.is_valid_type_name(prs.cur_tok.GetValue()))
	    {
	       Log::log_error(Error(0, 0,
				    string("Invalid type name for return type list of function.")));

	       return nullptr;
	    }

	    rs.push_back(prs.cur_tok.GetValue());

	    //Eat type
	    prs.get();
	 }

	 else if (prs.cur_tok.GetKind() == token_kind::TYPE_VOID)
	 {
	    Log::log_error(Error(0, 0,
				 string("'void' included in function return type list.")));
	    return nullptr;
	 }

	 else if (prs.cur_tok.GetKind() == token_kind::TYPE_INT)
	 {
	    rs.emplace_back("int");

	    prs.get();
	 }

	 else if (prs.cur_tok.GetKind() == token_kind::TYPE_FLOAT)
	 {
	    rs.emplace_back("float");

	    prs.get();
	 }

	 else if (prs.cur_tok.GetKind() == token_kind::TYPE_STRING)
	 {
	    rs.emplace_back("string");

	    prs.get();
	 }

	 else if (prs.cur_tok.GetKind() == token_kind::PAREN_CLOSE)
	 {
	    if (!rs.size())
	    {
	       //Might get rid of this, it's a bit OTT
	       Log::log_error(Error(0, 0,
				    string("No return types mentioned in a function return type list. Perhaps you meant 'void'?")));
	       return nullptr;
	    }

	    //Eat )
	    prs.get();

	    break;
	 }

	 else
	 {
	    Log::log_error(Error(0, 0,
				 string("Expected end or continuation of a function return list.")));
	 
	    return nullptr;
	 }

	 if (prs.cur_tok.GetKind() != token_kind::COMMA)
	 {
	    Log::log_error(Error(0, 0,
				 string("Expected comma delimiting function returns.")));
	    return nullptr;
	 }
      }
   }

   else
   {
      //Get just one return
      switch (prs.cur_tok.GetKind())
      {
	 case token_kind::TYPE_VOID:
	 {
	    break;
	 }
	 
	 case token_kind::TYPE_INT:
	 {
	    rs.emplace_back("int");
	    break;
	 }

	 case token_kind::TYPE_FLOAT:
	 {
	    rs.emplace_back("float");
	    break;
	 }

	 case token_kind::TYPE_STRING:
	 {
	    rs.emplace_back("string");
	    break;
	 }

	 case token_kind::NAME:
	 {
	    if (!prs.is_valid_type_name(prs.cur_tok.GetValue()))
	    {
	       Log::log_error(Error(0, 0,
				    string("Invalid return type name.")));
	       return nullptr;
	    }

	    rs.push_back(prs.cur_tok.GetValue());
	    break;
	 }

	 default:
	 {
	    Log::log_error(Error(0, 0,
				 string("Expected return type name.")));

	    return nullptr;
	 }
      }

      //Eat return type name
      prs.get();
   }

   //Either way, function name is next
   if (prs.cur_tok.GetKind() != token_kind::NAME)
   {
      Log::log_error(Error(0, 0,
			   string("Expected function name after return list.")));
      return nullptr;
   }

   if (!prs.is_valid_func_name(prs.cur_tok.GetValue()))
   {
      Log::log_error(Error(0, 0,
			   string("Invalid function name.")));
      return nullptr;
   }

   //Save name for later
   name = prs.cur_tok.GetValue();

   //Eat function name
   prs.get();

   //Eat (
   if (prs.cur_tok.GetKind() != token_kind::PAREN_OPEN)
   {
      Log::log_error(Error(0, 0,
			   string("Expected ( opening function parameter list.")));
      return nullptr;
   }

   prs.get();

   //Params
   while (true)
   {
      if (prs.cur_tok.GetKind() == token_kind::PAREN_CLOSE)
      {
	 //Eat )
	 prs.get();

	 break;
      }

      else if (!prs.is_type_token(prs.cur_tok))
      {
	 Log::log_error(Error(0, 0,
			      string("Expected type name of a function signature parameter.")));
	 return nullptr;
      }

      //Eat a type name
      const string& typeName = prs.cur_tok.GetValue();

      prs.get();

      //Check param name
      if (prs.cur_tok.GetKind() != token_kind::NAME)
      {
	 Log::log_error(Error(0, 0,
			      string("Expected name of a function signature parameter.")));
	 return nullptr;
      }

      args.push_back(tuple<string, string>(typeName,
					   prs.cur_tok.GetValue()));

      //Eat name
      prs.get();

      if (prs.cur_tok.GetKind() == token_kind::COMMA)
      {
	 //Eat ,
	 prs.get();

	 continue;
      }

      else if (prs.cur_tok.GetKind() == token_kind::PAREN_CLOSE)
      {
	 //Eat )
	 prs.get();

	 break;
      }

      else
      {
	 Log::log_error(Error(0, 0,
			      string("Expected ) to close function parameter list, or , to continue it.")));
      
	 return nullptr;
      }
   }

   return make_unique<SignatureExpression>(name, rs, args);
}

unique_ptr<Expression> AssignExpression::Parse(Parser& prs,
					       unique_ptr<Expression> left)
{
   //Eat =
   prs.get();

   //TODO: should these checks even be done, if they've been passed
   //here as if they are valid already? Caller should really be the
   //one checking, since implicitly it checks for correctness as part
   //of an AssignExpression
   if (!left)
   {
      Log::log_error(Error(0, 0,
			   string("Failure parsing left-hand side of assignment.")));
      
      return nullptr;
   }
   
   unique_ptr<Expression> right = RHSExpression::Parse(prs);

   if (right == nullptr)
   {
      //TODO: log specifics somehow
      Log::log_error(Error(0, 0,
			   string("Failure parsing right-hand side of assignment.")));
      
      return nullptr;
   }

   //Don't check for SEMICOLON here; do it in StatementExpression,
   //which is what calls this.
   
   return make_unique<AssignExpression>(move(left), move(right));
}

unique_ptr<Expression> RHSExpression::Parse(Parser& prs)
{
   //Something that must reduce to a -value-. Effectively just a
   //wrapper for binary operations and parentheses
   //NB: don't assume it's ended by a semicolon; these expressions are
   //just meant to be value-reducible and might, e.g., be delimited by commas.

   //Wrapper for (potential) binary op
   unique_ptr<Expression> cur = nullptr;
   
   //First symbol
   if (prs.cur_tok.GetKind() == token_kind::LIT_INT)
   {
      cur = LitIntExpression::Parse(prs);
   }

   //TODO: other literals

   else if (prs.cur_tok.GetKind() == token_kind::PAREN_OPEN)
   {
      cur = ParenExpression::Parse(prs);
   }

   else if (prs.cur_tok.GetKind() == token_kind::NAME)
   {
      cur = NameExpression::Parse(prs); //Includes function calls
   }

   if (!cur)
   {
      //
      cout << prs.cur_tok; //
      //
      
      Log::log_error(Error(0, 0,
			   string("Failure parsing value-reducible expression.")));
      return nullptr;
   }

   //Check if op next (no get() because of above Parse()s)
   
   if (prs.get_binary_precedence(prs.cur_tok))
   {
      //Binary ops are already recursive; it should go to the end of
      //the RHS
      return BinaryExpression::Parse(prs,
				     move(cur));
   }

   else return cur;
}

unique_ptr<Expression> ReturnExpression::Parse(Parser& prs)
{
   //Eat 'return'
   prs.get();

   vector<unique_ptr<Expression>> rs;
   
   while (prs.cur_tok.GetKind() != token_kind::SEMICOLON)
   {
      if (prs.cur_tok.GetKind() == token_kind::COMMA)
      {
	 //Eat comma
	 prs.get();
	 
	 continue;
      }
      
      unique_ptr<Expression> cur = RHSExpression::Parse(prs);

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
   prs.get();

   return make_unique<ReturnExpression>(move(rs));
}

unique_ptr<Expression> StatementExpression::Parse(Parser& prs)
{
   //ie a control statement (including if etc) or an assign of some
   //kind or an init
   //This function is the main one that checks SEMICOLONs.

   if (prs.cur_tok.GetKind() == token_kind::KEY_RETURN)
   {
      return ReturnExpression::Parse(prs);
   }

   //If not control flow, must be an assign or an init
   //(Does not guarantee it is a type token; just checks validity)
   //TODO best replace this actually - is_type_token should eventually
   //check for naming rules, which might be different from variable
   //name rules.

   //Following is single-exit for clarity; semicolons checked at the end.
   //(Return, on the other hand, is helped by its own semicolon
   //checking.)

   unique_ptr<Expression> stmt = nullptr;
   
   if (prs.is_type_token(prs.cur_tok.GetKind()))
   {
      token nmTok = prs.cur_tok;
      const string nm = prs.cur_tok.GetValue();

      //Eat type/variable name
      prs.get();

      //If 2 names in a row, must be init. Else assignment
      if (prs.cur_tok.GetKind() != token_kind::NAME)
      {
	 //Assignment	 
	 //(nm must be a name, not a type)
	 if (prs.cur_tok.GetKind() == token_kind::OP_ASSIGN_VAL)
	 {
	    //Is the lhs a var or a ref?
	    //(This will make more sense when refs are implemented)

	    //Don't eat =, AssignExpression does that.

	    if ((nm.size() == 1) ||
		(nm[nm.size() - 1] != '\''))
	    {
	       //It's a var
	       //TODO: see below, else clause
	       stmt = AssignExpression::Parse(prs,
					      make_unique<VarExpression>(nm,
									 string()));
	    }

	    //TODO else value assignment to ref alternative
	    //(Better doing a separate ref type check?)

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
	       const string varNm = prs.cur_tok.GetValue();

	       //Eat var name
	       prs.get();
	       
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
		  const string varNm = prs.cur_tok.GetValue();

		  //Eat var name
		  prs.get();
	 
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

	 if (prs.cur_tok.GetKind() == token_kind::OP_ASSIGN_VAL)
	 {
	    //Extremely TODO: this must be scoped above, because of
	    //VarRef, RefVar etc. ambiguities. This only works now
	    //because of no refs
	    stmt = AssignExpression::Parse(prs, move(stmt));
	 }

	 //TODO else ref_assign

	 //Else not an error - just return the init alone, as a
	 //statement.
      }
   }

   else
   {
      Log::log_error(Error(0, 0,
			   string("Expected statement within function body.")));
      return nullptr;
   }

   //Check semicolon at end
   if (prs.cur_tok.GetKind() != token_kind::SEMICOLON)
   {
      Log::log_error(Error(0, 0,
			   string("Expected ; closing statement.")));
      return nullptr;
   }

   //Eat ;
   prs.get();
	    
   return stmt;
}

void Parser::Parse(token_string toks)
{   
   //TODO Might isolate this
   str = token_stream(toks);
   cur_tok = str.get();

   while (cur_tok.GetKind() != token_kind::END)
   {
      unique_ptr<Expression> func = FunctionExpression::Parse(*this);

      if (!func)
      {
	 Log::log_error(Error(0, 0, "Failed to parse FunctionExpression."));
	 
	 break;
      }

      else parsed.push_back(move(func));
   }
}

int main(int argc, char** argv)
{
   lexer lexer;

   if (argc <= 1)
   {
      return 1;
   }
   
   token_string toks = lexer.lex(argv[1]);
   
   //cout << toks;
   
   Parser prs;

   prs.Parse(toks);

   prs.print();

   //Log::print();

   //prs.print();

   prs.Generate();

   Log::print();

   return 0;
}
