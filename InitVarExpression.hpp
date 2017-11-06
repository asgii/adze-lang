#pragma once

#include "Expression.hpp"

/*
  Probably just a definition of a variable: its type and its name.
  Not a real 'initialisation' ie setting it the first time.
  TODO rename?

  This is parsed by a caller, which just uses its constructor.
*/

class InitVarExpression : public Expression
{
private:
   string varName;
   string typName;

public:
   InitVarExpression(const string& varNm,
		     const string& typNm);

   //TODO why is this available? Just notekeeping?
   InitVarExpression(const string& varNm);

   ostream& print (ostream& stream) override;

   llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) override;
   llvm::Value* GenerateLHS(ParseScope& scope, ParseBuild& build, ParseInfo info) override;
};
