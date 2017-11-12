#include "ParseInfo.hpp"

const std::map<token_kind, int> ParseInfo::precedences =
{{token_kind::OP_ADD, 8},
 {token_kind::OP_SUB, 7},
 {token_kind::OP_MUL, 9},
 {token_kind::OP_DIV, 11},
 {token_kind::OP_MOD, 10},
 {token_kind::OP_EXP, 13},
 {token_kind::OP_ROOT, 12}};

ParseInfo::ParseInfo(ParseBuild& build)
   : context (build.GetContext())
{
}

bool
ParseInfo::get_literal_int(const std::string& str,
			     int& result)
{
   //These could be replaced by checking exceptions on stoi().  But
   //as it is, might be that there are more stringent requirements
   //for ints in adze than in C++.

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

   result = std::stoi(str);

   return false;
}

int
ParseInfo::get_binary_precedence(const token& tok) const
{
   if (precedences.count(tok.GetKind()))
      return precedences.at(tok.GetKind());

   else return 0;
}


bool
ParseInfo::is_valid_func_name(const std::string& str) const
{
   //TODO

   return true;
}

bool
ParseInfo::is_valid_type_name(const std::string& str) const
{
   //TODO
      
   return true;
}

bool
ParseInfo::is_type_token(const token& tok) const
{
   //NB excludes 'void'
      
   switch (tok.GetKind())
   {
      case token_kind::TYPE_INT:
      case token_kind::TYPE_FLOAT:
      case token_kind::TYPE_STRING:
	 return true;

      case token_kind::NAME:
      {
	 return is_valid_func_name(tok.GetValue());
      }
	 
      default:
	 return false;
   }
}

bool
ParseInfo::is_literal(const token_kind& tok) const
{
   switch (tok)
   {
      case token_kind::LIT_INT:
      case token_kind::LIT_FLOAT:
      case token_kind::LIT_STRING:
	 return true;

	 //case token_kind::NAME

      default:
	 return false;
   }
}

bool
ParseInfo::is_rhs_end(const token_kind& tok) const
{
   //This is mainly for RHSExpression::Parse, which needs to know
   //when the rhs has stopped.
   //Would be easier just to have a token_kind::SEMICOLON!

   switch (tok)
   {
      case token_kind::BRACE_CLOSE:
      case token_kind::OP_ASSIGN_VAL:
      case token_kind::OP_ASSIGN_REF:
      case token_kind::END:
	 return true;

      default:
	 return false;
   }
}

llvm::Type*
ParseInfo::GetType(const std::string str)
{
   llvm::Type* ty = nullptr;

   if (str == "int")
      ty = llvm::Type::getInt32Ty(context);

   else if (str == "float")
      ty = llvm::Type::getFloatTy(context);

   else if (str == "string")
   {
      //uhhhh - TODO
   }

   return ty;
}

llvm::Type*
ParseInfo::GetType(const token_kind tok)
{
   switch (tok)
   {
      case token_kind::TYPE_INT:
	 return llvm::Type::getInt32Ty(context);

      case token_kind::TYPE_FLOAT:
	 return llvm::Type::getFloatTy(context);

	 //TODO string

      default:
	 return nullptr;
   }
}

size_t
ParseInfo::GetTypeSize(const std::string& str) const
{
   if (str == "int")
      return 32;

   else if (str == "float")
      return 32;

   else if (str == "string")
      //TODO
      return 0;

   else return 0;
}
