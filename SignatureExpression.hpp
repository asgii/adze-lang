#pragma once

#include "Expression.hpp"

/*
  A function signature.
*/

class SignatureExpression : public Expression
{
private:
   //Name goes here bc you could define (and then parse) the sig
   //without the body. Whereas the body will never be defined without
   //an accompanying sig to parse.
   string funcName;

   vector<string> rets;
   vector<tuple<string, string>> params;

public:
   SignatureExpression(const string& name,
		       vector<string> rs,
		       vector<tuple<string, string>> args);

   ostream& print (ostream& stream) override;

   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);
   
   llvm::Function* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;

   string GetFuncName() const override;
   string GetParamName(size_t index) override;

   //TODO: will have to be changed for custom types...
   token_kind GetParamType(size_t index) const override;

   //ie has this name for a param already been used as a name for a param?
   bool IsParam(string paramName) const override;

   bool IsVoid() const override;
};
