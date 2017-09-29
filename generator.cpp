#include "parser.hpp"
#include "log.hpp"

void Parser::init_LLVM()
{
   module = llvm::make_unique<llvm::Module>("adze", context);
}

void Parser::Generate()
{
   init_LLVM();

   for (unsigned int i = 0; i < parsed.size(); ++i)
   {
      generated.push_back(parsed[i]->Generate(*this));
   }

   //
   module->print(llvm::errs(), nullptr); //
   //
}

llvm::Value* LitIntExpression::Generate(Parser& prs)
{
   return llvm::ConstantInt::get(prs.context,
				 llvm::APInt(32, (uint64_t) value));
				 //Must also specify if signed
}

llvm::Value* VarExpression::Generate(Parser& prs)
{
   /*
     Current thoughts: VarExpression shouldn't just be some
     notekeeping for making the expression tree - it should primarily
     be a generator.

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

   //By default return value, not pointer; exceptions won't call
   //Generate, but GenerateLHS.
   return GenerateRHS(prs);
}

llvm::Value* VarExpression::GenerateLHS(Parser& prs)
{
   llvm::AllocaInst* addr = prs.is_in_scope(varName);

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

llvm::Value* VarExpression::GenerateRHS(Parser& prs)
{
   llvm::AllocaInst* addr = prs.is_in_scope(varName);

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
      return prs.builder.CreateLoad(addr, varName);
   }
}

/*
  NB: following two should not have a value to initialise to!! 
  The init value is assigned with an enclosing Assign expression 
  of some kind. These just allocate things to be assigned to.
*/

llvm::Value* InitVarExpression::Generate(Parser& prs)
{
   //These are for inline declarations of variables.

   //Check not already in scope
   if (prs.is_in_scope(varName))
   {
      Log::log_error(Error(0, 0,
			   string("Variable name '" + varName + "' is already used in the scope it is initialised in.")));
      return nullptr;
   }

   //This adds to scope, too.
   llvm::AllocaInst* addr = prs.allocate_instruction(prs.GetType(typName),
						     varName);

   //Return pointer, not value, because if anything it will be on the
   //left hand of an assign; a value will be dumped in the pointer.
   return addr;
}

llvm::Value* InitVarExpression::GenerateLHS(Parser& prs)
{
   return Generate(prs);
}

llvm::Value* CallExpression::Generate(Parser& prs)
{
   //This is a global function table. Could add checks (possibly in
   //the llvm API?) for privacy etc.
   llvm::Function* called = prs.module->getFunction(name);

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
      argValues.push_back(args[i]->Generate(prs));

      //Check each as you go along
      if (!argValues.back())
      {
	 Log::log_error(Error(0, 0,
			      string("Failure generating argument to function call."))); //TODO more informative
	 
	 return nullptr; //TODO log - but then, shouldn't the above Generate()?
      }
   }

   return prs.builder.CreateCall(called, argValues, "call");
}

llvm::Value* BinaryExpression::Generate(Parser& prs)
{
   llvm::Value* left = lhs->Generate(prs);
   llvm::Value* right = rhs->Generate(prs);

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
	 return prs.builder.CreateAdd(left, right, "add");
	 
      case token_kind::OP_SUB:
	 return prs.builder.CreateSub(left, right, "sub");

      case token_kind::OP_MUL:
	 return prs.builder.CreateMul(left, right, "mul");

      case token_kind::OP_DIV:
	 return prs.builder.CreateSDiv(left, right, "div");

      case token_kind::OP_MOD:
	 //NB that 'signed remainder' is not the same as modulo.
	 return prs.builder.CreateURem(left, right, "mod");

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

   //Could handle overloads, etc.
}

llvm::Value* FunctionExpression::Generate(Parser& prs)
{
   //Only generate the signature if it hasn't already been done.
   llvm::Function* func = (llvm::Function*) prs.module->getFunction(signature->GetFuncName());

   if (!func)
   {
      func = (llvm::Function*) signature->Generate(prs);

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

   //Body (don't do it in separate function because needs to call sig)
   llvm::BasicBlock* block = llvm::BasicBlock::Create(prs.context,
						      "entry",
						      (llvm::Function*) func);

   prs.builder.SetInsertPoint(block);

   //Special handling for declaration of params from signature.
   //(Declarations of temporaries in body are done on the fly.)

   //You have to put the params into scope.
   //(TODO: make names include type and mutability info.)
   prs.push_scope();
   //Isn't this overkill unless there's nesting -functions-...?
   //Remember though C scopes {..}

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

	(I believe LLVM advises you do this anyway, even if they
	shouldn't be mutable, for ease of optimisation.)
      */

      token_kind paramType = signature->GetParamType(i);

      //(This adds to scope too)
      llvm::AllocaInst* alloc = prs.allocate_instruction(prs.GetType(paramType),
							 signature->GetParamName(i));

      //Store to that variable
      //Tbh might want to just replace this with a const variable
      //stack,
      //since it's not obvious how extra instructions are helping here.
      prs.builder.CreateStore(&(*it), alloc);
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

      statements[i]->Generate(prs);
   }

   //Should now be back in entry block.

   //Pop scoped names
   prs.pop_scope();

   //Might already be a return (in this block).
   //TODO: since stmts are meant to be expressions, they should all be
   //at this scope (only subordinate expressions won't be). So you
   //should be able to just check the last one is a return.
   if (signature->IsVoid())
   {
      prs.builder.CreateRetVoid();
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

llvm::Value* ReturnExpression::Generate(Parser& prs)
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
	 vals[i] = rets[i]->Generate(prs);

	 if (!vals[i])
	 {
	    //TODO: more informative?
	    Log::log_error(Error(0, 0,
				 string("Failed to generate expression being returned.")));
	    return nullptr;
	 }
      }

      return prs.builder.CreateAggregateRet(vals, rets.size());
   }

   else
   {
      return prs.builder.CreateRetVoid();
   }
}

llvm::Function* SignatureExpression::Generate(Parser& prs)
{
   vector<llvm::Type*> parArgs;
   
   for (unsigned int i = 0; i < params.size(); ++i)
   {
      const token& typ = GetParamType(i);

      llvm::Type* typePtr = prs.GetType(typ.GetKind());

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
	 { returnTypes[i] = llvm::Type::getInt32Ty(prs.context); }
      
	 else if (rets[i] == "float")
	 { returnTypes[i] = llvm::Type::getFloatTy(prs.context); }

	 //TODO string
      }

      funcType = llvm::FunctionType::get(llvm::StructType::get(prs.context,
										   returnTypes,
										   false), //whether packed or not
							     parArgs,
							     false);
   }

   else
   {
      funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(prs.context),
					 parArgs,
					 false);
   }

   llvm::Function* func = llvm::Function::Create(funcType,
						 llvm::Function::ExternalLinkage,
						 funcName,
						 prs.module.get());

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

llvm::Value* AssignExpression::Generate(Parser& prs)
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
    */
   llvm::Value* l = lhs->GenerateLHS(prs);
   llvm::Value* r = rhs->Generate(prs);

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
   
   return prs.builder.CreateStore(r, //val
				  l); //ptr
}
