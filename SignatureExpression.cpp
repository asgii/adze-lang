#include "SignatureExpression.hpp"



SignatureExpression::SignatureExpression(const string& name,
					 vector<string> rs,
					 vector<tuple<string, string>> args)
   : funcName (name)
   , rets (move(rs))
   , params (move(args))
{
}

ostream&
SignatureExpression::print (ostream& stream)
{
   stream << "SignatureExpression: " << funcName << endl;

   for (unsigned int i = 0; i < rets.size(); ++i)
   {
      stream << "[Signature return " << i << ":]" << endl << rets[i];
   }

   stream << endl;

   for (unsigned int i = 0; i < rets.size(); ++i)
   {
      stream << "[Signature param " << i << ":]" <<
	 get<0>(params[i]) << " " << get<1>(params[i]) << endl;
   }

   stream << endl;

   return stream << "SignatureExpression end" << endl;
}

string
SignatureExpression::GetFuncName() const
{
   return funcName;
}

string
SignatureExpression::GetParamName(size_t index)
{
   return get<1>(params[index]);
}

//TODO: will have to be changed for user-defined types...
token_kind
SignatureExpression::GetParamType(size_t index) const
{
   const string& typeName = get<0>(params[index]);

   if (typeName == "int")
      return token_kind::TYPE_INT;

   if (typeName == "float")
      return token_kind::TYPE_FLOAT;

   if (typeName == "string")
      return token_kind::TYPE_STRING;

   else return token_kind::INVALID;
}

//ie has this name for a param already been used as a name for a param?
bool
SignatureExpression::IsParam(string paramName) const
{
   for (unsigned int i = 0; i < params.size(); ++i)
   {
      if (get<1>(params[i]) == paramName)
      {
	 return true;
      }
   }

   return false;
}

bool
SignatureExpression::IsVoid() const
{
   return !rets.size();
}

unique_ptr<Expression>
SignatureExpression::Parse(token_stream& str,
			   ParseInfo info)
{
   vector<string> rs;
   vector<tuple<string, string>> args;

   string name;

   //Return values, and function name

   /*
     For ease of parsing, multiple return types must be enclosed in ().
    */

   if (str.cur_tok().GetKind() == token_kind::PAREN_OPEN)
   {
      //Eat (
      str.get();

      while (true)
      {
	 token_kind kind = str.cur_tok().GetKind();

	 switch (kind)
	 {
	    case token_kind::NAME:
	    {
	       if (!info.is_valid_type_name(str.cur_tok().GetValue()))
	       {
		  Log::log_error(Error(0, 0,
				       string("Invalid type name for return type list of function.")));

		  return nullptr;
	       }

	       rs.push_back(str.cur_tok().GetValue());

	       //Eat type
	       str.get();
	    }
	    break;

	    case token_kind::TYPE_VOID:
	    {
	       Log::log_error(Error(0, 0,
				    string("'void' included in function return type list.")));
	       return nullptr;
	    }
	    break;

	    case token_kind::TYPE_INT:
	    {
	       rs.emplace_back("int");
	       
	       str.get();
	    }
	    break;

	    case token_kind::TYPE_FLOAT:
	    {
	       rs.emplace_back("float");
	       
	       str.get();
	    }
	    break;

	    case token_kind::TYPE_STRING:
	    {
	       rs.emplace_back("string");
	       
	       str.get();
	    }
	    break;

	    case token_kind::PAREN_CLOSE:
	    {
	       if (!rs.size())
	       {
		  //Might get rid of this, it's a bit OTT
		  Log::log_error(Error(0, 0,
				       string("No return types mentioned in a function return type list. Perhaps you meant 'void'?")));
		  return nullptr;
	       }

	       //Eat )
	       str.get();

	       //Break while()
	       break;
	    }
	    break;

	    default:
	    {
	       Log::log_error(Error(0, 0,
				    string("Expected end or continuation of a function return list.")));
	       
	       return nullptr;
	    }
	    break;
	 }

	 //Immediately after
	 if (str.cur_tok().GetKind() != token_kind::COMMA)
	 {
	    Log::log_error(Error(0, 0,
				 string("Expected comma delimiting function returns.")));
	    return nullptr;
	 }

	 //Eat ,
	 else { str.get(); }
      }
   }

   else
   {
      //Get just one return
      switch (str.cur_tok().GetKind())
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
	    if (!info.is_valid_type_name(str.cur_tok().GetValue()))
	    {
	       Log::log_error(Error(0, 0,
				    string("Invalid return type name.")));
	       return nullptr;
	    }

	    rs.push_back(str.cur_tok().GetValue());
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
      str.get();
   }

   //Either way, function name is next
   if (str.cur_tok().GetKind() != token_kind::NAME)
   {
      Log::log_error(Error(0, 0,
			   string("Expected function name after return list.")));
      return nullptr;
   }

   if (!info.is_valid_func_name(str.cur_tok().GetValue()))
   {
      Log::log_error(Error(0, 0,
			   string("Invalid function name.")));
      return nullptr;
   }

   //Save name for later
   name = str.cur_tok().GetValue();

   //Eat function name
   str.get();

   //Eat (
   if (str.cur_tok().GetKind() != token_kind::PAREN_OPEN)
   {
      Log::log_error(Error(0, 0,
			   string("Expected ( opening function parameter list.")));
      return nullptr;
   }

   str.get();

   //Params
   while (true)
   {
      if (str.cur_tok().GetKind() == token_kind::PAREN_CLOSE)
      {
	 //Eat )
	 str.get();

	 break;
      }

      else if (!info.is_type_token(str.cur_tok()))
      {
	 Log::log_error(Error(0, 0,
			      string("Expected type name of a function signature parameter.")));
	 return nullptr;
      }

      //Eat a type name
      const string& typeName = str.cur_tok().GetValue();

      str.get();

      //Check param name
      if (str.cur_tok().GetKind() != token_kind::NAME)
      {
	 Log::log_error(Error(0, 0,
			      string("Expected name of a function signature parameter.")));
	 return nullptr;
      }

      args.push_back(tuple<string, string>(typeName,
					   str.cur_tok().GetValue()));

      //Eat name
      str.get();

      if (str.cur_tok().GetKind() == token_kind::COMMA)
      {
	 //Eat ,
	 str.get();

	 continue;
      }

      else if (str.cur_tok().GetKind() == token_kind::PAREN_CLOSE)
      {
	 //Eat )
	 str.get();

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
