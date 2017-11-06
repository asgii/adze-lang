#pragma once

#include "lexer.hpp"

#include "ParseBuild.hpp"
#include "ParseScope.hpp"
#include "ParseInfo.hpp"

class token_stream;

#include "Expression.hpp"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

using namespace std;

class Expression;

//Temporary, until lexer streams tokens...?
class token_stream
{
private:
   token_string str;
   size_t pos;

   token cur; //Current token

public:
   token_stream();
   token_stream(token_string nStr);
   
   const token get(); //Get and advance stream
   const token peek() const; //Get but don't advance stream
   const token cur_tok() const; //Get current token
};

class Parser
/*
  Top-level wrapper for ParseInfo, ParseScope, ParseBuild: interface
  for doing entire parsing pipeline.
*/
{
private:
   vector<unique_ptr<Expression>> parsed;
   vector<llvm::Value*> generated;

   token_stream str;

   ParseScope scope; //Scope, for generation
   ParseBuild build; //LLVM stuff

public:
   Parser();
   
   void Parse(token_string toks);
   void Generate();

   void printTree(); //Print a representation of the tree. Very rough
   void printIR(); //Dump LLVM IR generated
};
