#pragma once

#include "llvm/IR/Instructions.h"

#include <string>
#include <map>

using namespace std;

class ParseScope
/*
  Structure for holding data about names already used at any point of
  generation.  
  Generation is never going to 'go back' to scopes that have 
  collapsed, so scope really can be a stack, with previously 
  higher level sections of the stack being discarded.
*/
{
private:
   //Stack of names in scope
   //(can't be std::stack because is_in_scope accesses all indices)
   vector<map<string, llvm::AllocaInst*>> names;
   //TODO: will probably need another for refs
   //TODO: Might need another, for variables that have been
   //implicitly deleted. Needn't be a stack...?

public:

   //Push/pop an entire level of scope
   map<string, llvm::AllocaInst*>& push_scope();
   void pop_scope();
   //Push a name to the current top level scope
   void push_to_scope(const string& name, llvm::AllocaInst* var);
   //No need for pop_from_scope

   llvm::AllocaInst* is_in_scope(const string& nm);
};
