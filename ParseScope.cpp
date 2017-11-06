#include "ParseScope.hpp"

map<string, llvm::AllocaInst*>&
ParseScope::push_scope()
{
   names.emplace_back();

   return names.back();
}

void
ParseScope::pop_scope()
{
   names.pop_back();
}

void
ParseScope::push_to_scope(const string& name, llvm::AllocaInst* var)
{
   names.back()[name] = var;
}

//pop_from_scope if there's some kind of delete operation? But then
//you'd want some other mechanism for error info - when the
//user refers to something that's been deleted. Another names, for
//deleted ones? It might not need be a stack.

llvm::AllocaInst*
ParseScope::is_in_scope(const string& nm)
{
   //Proceed from nearest scope out
   for (int i = names.size() - 1; i > -1; --i)
   {
      if (names[i].count(nm))
      {
	 return names[i].at(nm); //because at() can be const
      }
   }
      
   return nullptr;

   /*
     TODO: what about overloading? i.e., the scope is wider than
     just public variables. Presumably I must allow names for
     variables that have been taken in other, 'invisible' scopes.
	
     In that case I would probably have to look at the payload at
     names[i][nm] - it would have to include access constraints.
   */
}
