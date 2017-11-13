#include "Parser.hpp"
#include "log.hpp"

#include "llvm/IR/Verifier.h"

//Here and eg in subexpr .cpp files subexprs are included to use their
//static functions/make_uniques without incurring cost of including in .hpps.
#include "exprs/subexprs/VarExpression.hpp"
#include "exprs/subexprs/LitIntExpression.hpp"
#include "exprs/subexprs/NameExpression.hpp"
#include "exprs/subexprs/BinaryExpression.hpp"
#include "exprs/subexprs/StatementExpression.hpp"
#include "exprs/subexprs/RHSExpression.hpp"
#include "exprs/subexprs/CallExpression.hpp"
#include "exprs/subexprs/ReturnExpression.hpp"
#include "exprs/subexprs/SignatureExpression.hpp"
#include "exprs/subexprs/FunctionExpression.hpp"
#include "exprs/subexprs/ParenExpression.hpp"
#include "exprs/subexprs/AssignExpression.hpp"
#include "exprs/subexprs/InitVarExpression.hpp"

void Parser::Generate()
{
   for (unsigned int i = 0; i < parsed.size(); ++i)
   {
      generated.push_back(parsed[i]->Generate(scope, build, ParseInfo(build)));
   }
}

llvm::Value* LitIntExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   return llvm::ConstantInt::get(build.GetContext(),
				 llvm::APInt(32, (uint64_t) value));
				 //Must also specify if signed
}

llvm::Value* VarExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   //By default return value, not pointer; exceptions won't call
   //Generate, but GenerateLHS.
   return GenerateRHS(scope, build, info);
}

llvm::Value* VarExpression::GenerateLHS(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   llvm::AllocaInst* addr = scope.is_in_scope(varName);

   if (!addr)
   {
      Log::log_error(Error(0, 0,
			   string("Variable name '" +
				  varName +
				  "' not in scope.")));
      return nullptr;
   }

   //Return pointer to value, to be changed.
   else return addr;
}

llvm::Value* VarExpression::GenerateRHS(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   llvm::AllocaInst* addr = scope.is_in_scope(varName);

   if (!addr)
   {
      Log::log_error(Error(0, 0,
			   string("Variable name '" +
				  varName +
				  "' not in scope.")));
      return nullptr;
   }

   else
   {
      //TODO: use the type-specific CreateLoad
      return build.GetBuilder().CreateLoad(addr, varName);
   }
}

/*
  NB: following two should not have a value to initialise to!! 
  The init value is assigned with an enclosing Assign expression 
  of some kind. These just allocate things to be assigned to.
*/

llvm::Value* InitVarExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   //These are for inline declarations of variables.

   //Check not already in scope
   if (scope.is_in_scope(varName))
   {
      Log::log_error(Error(0, 0,
			   string("Variable name '" + varName + "' is already used in the scope it is initialised in.")));
      return nullptr;
   }

   //This adds to scope, too.
   llvm::AllocaInst* addr = build.allocate_instruction(scope,
						       info.GetType(typName),
						       varName);

   //Return pointer, not value, because if anything it will be on the
   //left hand of an assign; a value will be dumped in the pointer.
   return addr;
}

llvm::Value* InitVarExpression::GenerateLHS(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   return Generate(scope, build, info);
}

llvm::Value* CallExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   //This is a global function table. Could add checks (possibly in
   //the llvm API?) for privacy etc.
   llvm::Function* called = build.GetModule()->getFunction(name);

   if (!called)
   {
      Log::log_error(Error(0, 0,
			   string("Unable to find name of function being called in function table.")));
      return nullptr;
   }

   if (called->arg_size() != args.size())
   {
      Log::log_error(Error(0, 0,
			   string("Not the right number of arguments in function call.")));
      return nullptr; //TODO format in # args
   }

   vector<llvm::Value*> argValues;

   for (unsigned int i = 0; i < args.size(); ++i)
   {
      argValues.push_back(args[i]->Generate(scope, build, info));

      //Check each as you go along
      if (!argValues.back())
      {
	 Log::log_error(Error(0, 0,
			      string("Failure generating argument to function call."))); //TODO more informative
	 
	 return nullptr; //TODO log - but then, shouldn't the above Generate()?
      }
   }

   //Indices into struct returned by call
   vector<unsigned int> indices = {0};

   //ExtractValue because functions return structs (in all
   //cases). This will need to be changed once tuples are properly
   //supported (because you should need to extract more than one
   //value); see comments in AssignExpression::Generate.
   return build.GetBuilder().CreateExtractValue(build.GetBuilder().CreateCall(called, argValues, name),
						indices,
						name + "_res");
}

llvm::Value* BinaryExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   llvm::Value* left = lhs->Generate(scope, build, info);
   llvm::Value* right = rhs->Generate(scope, build, info);

   if (!left)
   {
      if (!right)
      {
	 Log::log_error(Error(0, 0,
			      string("Failure generating either side of a binary expression.")));
	 return nullptr;
      }

      Log::log_error(Error(0, 0,
			   string("Failure generating left-hand side of a binary expression.")));
      return nullptr;
   }

   else if (!right)
   {
      Log::log_error(Error(0, 0,
			   string("Failure generating right-hand side of a binary expression.")));
      return nullptr;
   }

   /*
     TODO: some of these include type assumptions. So this function
     should check type requirements: that left and right have the
     same type, or equivalent, etc.

     Should probably also check values, since some of these have
     undefined behaviour, e.g. division by 0.
    */

   switch(op)
   {
      case token_kind::OP_ADD:
	 return build.GetBuilder().CreateAdd(left, right, "add");
	 
      case token_kind::OP_SUB:
	 return build.GetBuilder().CreateSub(left, right, "sub");

      case token_kind::OP_MUL:
	 return build.GetBuilder().CreateMul(left, right, "mul");

      case token_kind::OP_DIV:
	 return build.GetBuilder().CreateSDiv(left, right, "div");

      case token_kind::OP_MOD:
	 //NB that 'signed remainder' is not the same as modulo.
	 return build.GetBuilder().CreateURem(left, right, "mod");

      case token_kind::OP_EXP:
	 //TODO: for loop
      case token_kind::OP_ROOT:
	 //TODO: ditto
      default:
      {
	 Log::log_error(Error(0, 0,
			      string("A binary expression was parsed that wasn't recognised...")));
	 //TODO not very useful. Again, have to distinguish parser's and user's errors
	 return nullptr;
      }
   }

   //TODO Could handle overloads, etc.
}

llvm::Value* FunctionExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   //Only generate the signature if it hasn't already been done.
   llvm::Function* func = (llvm::Function*) build.GetModule()->getFunction(signature->GetFuncName());

   if (!func)
   {
      func = (llvm::Function*) signature->Generate(scope, build, info);

      if (!func)
      {
	 Log::log_error(Error(0, 0,
			      string("Failure generating function signature.")));
	 return nullptr;
      }
   }

   else
   {
      //If the signature has already been generated, better make sure
      //the body hasn't been too!
      if (func->isDeclaration())
      {
	 Log::log_error(Error(0, 0,
			      string("A function was wrongly redeclared.")));
	 //TODO more informative - get line from func.
	 //(How would llvm know...?)
	 return nullptr;
      }
   }

   build.BuildFunction(scope, func);

   //Special handling for declaration of params from signature.
   //(Declarations of temporaries in body are done on the fly.)

   //This should be different depending on whether it's a value or a ref...

   int i = 0;
   
   for (llvm::Function::arg_iterator it = func->arg_begin();
	it != func->arg_end();
	++it, ++i)
   {
      /*
	NB following is odd because allocas implies they're going
	to be -stored- to. But params shouldn't really be mutable

	Rather than manage a further scope map (names -> Value*s),
	though, I will do it as if they were allocas. Mutability
	can just be a separate constraint. The use of allocas,
	if it's worse, will presumably be optimised by LLVM, or it
	might be better anyway (if use of a Value implies
	pass-by-copy).

	(I believe LLVM advises you do this anyway (even if they
	shouldn't be mutable), for ease of optimisation.)
      */

      token_kind paramType = signature->GetParamType(i);

      //(This adds to scope too)
      llvm::AllocaInst* alloc = build.allocate_instruction(scope,
							   info.GetType(paramType),
							   signature->GetParamName(i));

      //Store to that variable
      //Tbh might want to just replace this with a const variable
      //stack,
      //since it's not obvious how extra instructions are helping here.
      build.GetBuilder().CreateStore(&(*it), alloc);
   }

   //Actual generation of the statements
   //Naive implementation: just 1:1 replicate calls.
   //You're leaving optimisations up to llvm in that case.

   for (unsigned int i = 0; i < statements.size(); ++i)
   {
      //These Generate()s should handle scope on their own - e.g., if
      //an expression involves a block, it will make its own IR block.
      //NB they should go back to previous block, too.
      //(or, make a temporary Builder with its own insert-point- but
      //then they'd have to pass them on and back)

      statements[i]->Generate(scope, build, info);
   }

   //Should now be back in entry block.

   //Pop scoped names
   scope.pop_scope();

   //Might already be a return (in this block).
   //TODO: since stmts are meant to be expressions, they should all be
   //at this scope (only subordinate expressions won't be). So you
   //should be able to just check the last one is a return.
   if (signature->IsVoid())
   {
      build.GetBuilder().CreateRetVoid();
   }

   //NB if it isn't void, requires explicit mention of what to return
   //- so should be in the statements anyway

   //TODO This has an output stream if you want a debug message
   //NB the weird T/F conditions here
   if (llvm::verifyFunction(*func, &llvm::errs()))
   {
      //func->eraseFromParent(); //TODO reinstate this (though good for debug)

      //TODO: make more informative? Ideally it'd work out -why-
      Log::log_error(Error(0, 0,
			   string("Function '" +
				  signature->GetFuncName()  +
				  "' failed to be verified.")));
      return nullptr;
   }

   else return func;
}

llvm::Value* ReturnExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   //Check # of args and types, too
   //But this can only be done in the context of a function...
   //So I'll do it in FunctionExpression...?
   
   //Or you could do it like the scope 'stack', and keep a
   //record of current function's expected returns

   if (rets.size())
   {
      llvm::Value* vals[rets.size()];

      for (unsigned int i = 0; i < rets.size(); ++i)
      {
	 vals[i] = rets[i]->Generate(scope, build, info);

	 if (!vals[i])
	 {
	    //TODO: more informative?
	    Log::log_error(Error(0, 0,
				 string("Failed to generate expression being returned.")));
	    return nullptr;
	 }
      }

      return build.GetBuilder().CreateAggregateRet(vals, rets.size());
   }

   else
   {
      return build.GetBuilder().CreateRetVoid();
   }
}

llvm::Function* SignatureExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   vector<llvm::Type*> parArgs;
   
   for (unsigned int i = 0; i < params.size(); ++i)
   {
      const token& typ = GetParamType(i);

      llvm::Type* typePtr = info.GetType(typ.GetKind());

      if (!typePtr)
	 break;

      else parArgs.push_back(typePtr);
	 
       /*
	 default:
	    //If not primitive type, find it in parser names
	 {
	    if (prs.names.count(typ.GetValue()))
	    {
	       //TODO
	    }

	    else
	    {
	       //TODO log
	       return nullptr;
	    }
	 }
       */
   }

   llvm::FunctionType* funcType = nullptr;

   //Compose struct type for multiple returns
   if (rets.size())
   {
      vector<llvm::Type*> returnTypes;

      returnTypes.resize(rets.size());

      for (unsigned int i = 0; i < rets.size(); ++i)
      {
	 if (rets[i] == "int")
	 { returnTypes[i] = llvm::Type::getInt32Ty(build.GetContext()); }
      
	 else if (rets[i] == "float")
	 { returnTypes[i] = llvm::Type::getFloatTy(build.GetContext()); }

	 //TODO string
      }

      funcType = llvm::FunctionType::get(llvm::StructType::get(build.GetContext(),
							       returnTypes,
							       false), //whether packed or not
					 parArgs,
					 false);
   }

   else
   {
      funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(build.GetContext()),
					 parArgs,
					 false);
   }

   llvm::Function* func = llvm::Function::Create(funcType,
						 llvm::Function::ExternalLinkage,
						 funcName,
						 build.GetModule().get());

   //Set name of params
   unsigned int i = 0;
   
   for (auto &it : func->args())
   {
      const string& paramName = GetParamName(i);

      it.setName(paramName);

      ++i;
   }

   return func;
}

llvm::Value* AssignExpression::Generate(ParseScope& scope, ParseBuild& build, ParseInfo info)
{
   /*
     As above, this can either be a var assigned to a var, or a ref
     ref-assigned to a ref (int a = 0, int a = b; or int' a, int' b '=
     a;) This currently works with LLVM because all a ref is is a ptr
     value, kept on the stack.

     They will probably need to be distinguished, though, since if a ref
     is meant to be like a smart pointer, changing it should have
     side-effects (it should deallocate if it's unique).

     At the moment all variables are mutable (for LLVM's purposes, not
     necessarily the parser's) and so are treated with a degree of
     indirection: what you end up generating from a VarExpression is a
     load from a pointer, a pointer to the actual value.

     Here, we want to store a value to the -pointer-, not the value
     itself. So the lhs has to -not- generate a load after all.

     Do lhs first in case it's an init; it will add to scope.

     Things are further complicated by tuples, e.g., (a, b) = (c, d);
     and functions which return tuples, since they'll return structs
     (which can't simply be stored, even if they have the same bit-
     width in principle).

     In effect the assign has to match struct members on the rhs with
     variables on the left. Eventually it will have to check the size
     of the lhs and rhs (they'll return vector<llvm::Value*>).

     The most simple way of doing this would be to store everything,
     even singletons, in structs, and automatically
     extract/insertvalue. Then these could be optimised away.

     For now though (without tuples), I will just put the extractvalue
     into CallExpression::Generate.
    */
   llvm::Value* l = lhs->GenerateLHS(scope, build, info);
   llvm::Value* r = rhs->Generate(scope, build, info);

   if (!r)
   {
      if (!l)
      {
	 Log::log_error(Error(0, 0,
			      string("Failure generating either side of assignment.")));
	 return nullptr;
      }

      else
      {
	 Log::log_error(Error(0, 0,
			      string("Failure generating right-hand side of assignment.")));
	 return nullptr;
      }
   }

   else if (!l)
   {
      Log::log_error(Error(0, 0,
			   string("Failure generating left-hand side of assignment.")));
      return nullptr;
   }
   
   return build.GetBuilder().CreateStore(r, //val
					 l); //ptr
}
