#include "VarExpression.hpp"

VarExpression::VarExpression(const string& varNm)
   : varName (varNm)
{
}

VarExpression::VarExpression(const string& varNm,
			     const string& typeNm)
   : varName (varNm)
   , typeName (typeNm)
{
}

ostream&
VarExpression::print (ostream& stream)
{
   return stream << "VarExpression: " << varName <<
      ", type: " << typeName << endl;
}

string
VarExpression::GetSubject()
{
   return varName;
}

unique_ptr<Expression>
VarExpression::Parse(token_stream& str,
		     ParseInfo info)
{
   const string& name = str.cur_tok().GetValue();

   //Eat NAME
   str.get();

   //TODO: Check validity of name before committing to construction?
   //or in the lexer?

   return make_unique<VarExpression>(name);
}
