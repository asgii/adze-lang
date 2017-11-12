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

     Actually it's not quite the
     start - it's at the end of a section, at the start of the block,
     where all such allocate instructions are put.
   */

   //Get current location for insertion of instructions, to return
   //to
   llvm::BasicBlock::iterator oldLoc = builder.GetInsertPoint();

   /*
     NB: same block. So this works with blocks other than function
     entries; scope would be affected by inserting an init into the
     parent block
     
     ++allocInsert because allocInsert is the one previous.
     The alternative doesn't work bc inserting directly 'after' the
     allocInsert point changes it (perhaps because LLVM thinks of
     itself as inserting instructions 'before' a point, not after an
     instruction).
     As a result allocInsert would equal oldLoc at all points,
     defeating the entire point.
   */
   builder.SetInsertPoint(builder.GetInsertBlock(),
			  ++allocInsert);
   
   llvm::AllocaInst* alloc = builder.CreateAlloca(typ,
						  nullptr,
						  nam);

   //set allocInsert to -before- the last alloc instruction.
   allocInsert = alloc->getIterator();

   //Restore old point of insertion (which hasn't changed relative to
   //the instructions)
   builder.SetInsertPoint(builder.GetInsertBlock(),
			  oldLoc);

   scope.push_to_scope(nam, alloc);

   return alloc;
}

void
ParseBuild::BuildFunction(ParseScope& scope, llvm::Function* func)
{
   llvm::BasicBlock* block = llvm::BasicBlock::Create(context,
						      "entry",
						      func);

   builder.SetInsertPoint(block);

   //Initialise allocInsert (pointer to last alloc at start of the
   //entry block)
   allocInsert = block->begin();

   scope.push_scope();
}
