#pragma once

#include "Expression.hpp"

/*
  Specifically the name of a variable.

  Old, possibly outdated notes:

  VarExpression shouldn't just be some notekeeping for making the 
  expression tree - it should primarily be a generator.

  It should represent specifically (some grammatical expression
  that resolves to) an address at the time of the
  enclosing Expressions' (real, not IR) instructions' execution
  (and therefore that address should have a value specific to that
  time). It should be able to be used both as an lvalue and
  rvalue. It should have a type (or an intended type - after all,
  compilation might fail), so the value can be used in both those
  ways.

  int a = b + 1020303;
  ^^^^^   ^   ^not a VarExpression
  ^^^^^^^^^two VarExpressions; b implicitly carries a type too

  Note that other expressions, e.g. subordinate to a
  SignatureExpression, can simply be for notekeeping; recording the
  type of returns, for example, doesn't involve any address.
  (Though equally, you could just have the SignatureExpression
  parse those types itself without calling a subordinate expression
  to do it).
*/

class VarExpression : public Expression
{
private:
   string varName;
   string typeName;
      
public:
   VarExpression(const string& varNm);
   VarExpression(const string& varNm,
		 const string& typeNm);

   ostream& print (ostream& stream) override;

   static unique_ptr<Expression> Parse(token_stream& str,
				       ParseInfo info);

   llvm::Value* Generate(ParseScope& scope,
			 ParseBuild& build,
			 ParseInfo info) override;
   llvm::Value* GenerateLHS(ParseScope& scope,
			 ParseBuild& build,
			 ParseInfo info) override;
   llvm::Value* GenerateRHS(ParseScope& scope,
			 ParseBuild& build,
			 ParseInfo info) override;

   string GetSubject() override;
};
