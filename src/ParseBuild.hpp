#pragma once

//More LLVM headers in ParseBuild.cpp
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "ParseScope.hpp"

class ParseBuild
/*
  All of the LLVM stuff required for generation from a finished parse tree.
*/
{
private:
   llvm::LLVMContext context;
   llvm::IRBuilder<> builder;
   unique_ptr<llvm::Module> module;

   //Insertion point after last alloc in this block
   //(actually, it's one before that- see .cpp)
   llvm::BasicBlock::iterator allocInsert;
   
public:

   ParseBuild();
   /*
     TODO: instead of just exposing these, build an interface on top
     of LLVM? But that seems OTT- it will surely just be exposing LLVM
     functions one after another.
   */
   llvm::LLVMContext& GetContext();
   llvm::IRBuilder<>& GetBuilder();
   unique_ptr<llvm::Module>& GetModule();

   llvm::AllocaInst* allocate_instruction(ParseScope& scope,
					  llvm::Type* typ, const string& nam);

   //Advantage of having this here is it can initialise allocInsert
   void BuildFunction(ParseScope& scope, llvm::Function* func);
};
