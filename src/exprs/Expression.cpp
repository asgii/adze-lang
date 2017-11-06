#include "Expression.hpp"

ostream&
operator<< (ostream& stream, Expression& expr)
{
   return expr.print(stream);
}

token
Expression::GetType()
{
   //This is the default, overrided by expressions for which it makes sense
   return token(token_kind::INVALID, string());
}

string
Expression::GetSubject()
{
   return string();
}

string
Expression::GetFuncName() const
{
   return string();
}

string
Expression::GetParamName(size_t index)
{
   return string();
}

llvm::Value*
Expression::GenerateLHS(ParseScope& scope, ParseBuild& build, ParseInfo info)
{ return nullptr; }
   
llvm::Value*
Expression::GenerateRHS(ParseScope& scope, ParseBuild& build, ParseInfo info)
{ return nullptr; }

token_kind
Expression::GetParamType(size_t index) const
{
   return token_kind::INVALID;
}

bool
Expression::IsParam(string paramName) const
{
   return false;
}

bool
Expression::IsVoid() const
{
   return true;
}
