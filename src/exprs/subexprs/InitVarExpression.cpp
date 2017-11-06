#include "InitVarExpression.hpp"

InitVarExpression::InitVarExpression(const string& varNm,
				     const string& typNm)
   : varName (varNm)
   , typName (typNm)
{
}

InitVarExpression::InitVarExpression(const string& varNm)
   : varName (varNm)
{
}

ostream&
InitVarExpression::print (ostream& stream)
{
   return stream << "InitVarExpression: " << typName << " " << varName << endl;
}
