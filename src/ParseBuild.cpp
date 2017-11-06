#include "ParseBuild.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Verifier.h"

ParseBuild::ParseBuild()
   : builder (context)
{
   module = llvm::make_unique<llvm::Module>("adze", context);
}

llvm::LLVMContext&
ParseBuild::GetContext() { return context; }

llvm::IRBuilder<>&
ParseBuild::GetBuilder() { return builder; }

unique_ptr<llvm::Module>&
ParseBuild::GetModule() { return module; }

llvm::AllocaInst*
ParseBuild::allocate_instruction(ParseScope& scope,
				 llvm::Type* typ, const string& nam)
{
   /*
     Insert instruction to allocate stack space for (mutable)
     variable - but insert it at the start of the current block,
     since that's what LLVM advises to do.

     Note it'd be nice if it still inserted them sequentially after
     the previous allocations. That way there'd be an obvious
     order, in assembly, from params to inits.
     You'd have to do a while() with the iterator, checking for
     non-allocates.
     (Or you could save and increment a pointer/iterator while
     making allocates - here, maybe?)
   */

   //Get current location for insertion of instructions, to return
   //to
   llvm::BasicBlock::iterator oldLoc = builder.GetInsertPoint();

   builder.SetInsertPoint(builder.GetInsertBlock(),
			  builder.GetInsertBlock()->begin());

   llvm::AllocaInst* alloc = builder.CreateAlloca(typ,
						  nullptr, //array
						  //size
						  nam);

   //Restore old point of insertion
   //NB: same block. So this works with blocks other than function
   //entries; scope would be affected by inserting an init into the
   //parent block
   builder.SetInsertPoint(builder.GetInsertBlock(),
			  oldLoc);

   //Add to scope
   scope.push_to_scope(nam, alloc);

   return alloc;
}
