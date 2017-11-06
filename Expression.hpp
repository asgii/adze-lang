#pragma once

#include "parser.hpp"
#include "log.hpp"

#include "llvm/IR/Value.h"

#include <iostream>
#include <string>
#include <memory>

using namespace std;

class Expression
{
public:
   /*
     NB the structure here: Parse() isn't virtual/override, but is
     actually static: it can't depend on the type of its caller,
     and doesn't even have one. You have to know when calling it
     what Expression you want.

     This has the advantage that it can encode an error, by
     returning nullptr (the only alternative within constructors
     being exceptions, perhaps?), while still being encapsulated
     somewhat rather than being methods of a monolithic Parser.

     NB also it's consistent with the actual code generation being
     virtual/override.
   */

   friend ostream& operator<< (ostream& stream, Expression& expr);

   virtual ostream& print(ostream& stream) = 0;
   virtual llvm::Value* Generate(ParseScope& scope, ParseBuild& build, ParseInfo info) = 0;

   //These two are for VarExpressions, which need to call different
   //Generate()s depending on l- or r-value.
   virtual llvm::Value* GenerateLHS(ParseScope& scope, ParseBuild& build, ParseInfo info);
   virtual llvm::Value* GenerateRHS(ParseScope& scope, ParseBuild& build, ParseInfo info);

   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);

   //Kind of messy: these are just for expressions where
   //'subject' etc are meaningful concepts, but where you don't know
   //whether an exception is -that kind- of exception.
   //TODO would at least be better if the default threw. That way you
   //could only get something you were expecting.
   virtual string GetSubject();
   virtual string GetFuncName() const;
   virtual string GetParamName(size_t index);
   virtual token_kind GetParamType(size_t index) const;
   virtual bool IsParam(string paramName) const;
   virtual bool IsVoid() const;
   //Temporarily a token. Might ditch altogether, dependent on
   //TypeExpression etc.
   virtual token GetType();
};
