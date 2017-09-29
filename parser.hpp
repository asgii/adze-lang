#pragma once

#include "lexer.hpp"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

using namespace std;

class Expression;

class VarExpression;
class LitIntExpression;

class NameExpression;

class BinaryExpression;
class StatementExpression;
class RHSExpression;
class ReturnExpression;

class CallExpression;
class FunctionExpression;
class SignatureExpression;

class ParenExpression;

class AssignExpression;

class InitVarExpression;

//Temporary, until lexer streams tokens...?
class token_stream
{
private:
   token_string str;
   size_t pos;

public:
   token_stream()
      : pos (0)
   {
   }

   token_stream(token_string nStr)
      : str (nStr)
      , pos (0)
   {
   }
   
   token get()
   {
      token tok = token(token_kind::END);

      if (pos <= (str.size() - 1))
      {
	 tok = str[pos];

	 ++pos;
      }
      
      return tok;
   }

   token peek()
   {
      if (pos < (str.size() - 1))
	 return str[pos + 1];

      else return token(token_kind::END);
   }
};

class Parser
{
private:
   //Token stream stuff
   token cur_tok;
   token_stream str;

   //Currently just functions
   vector<unique_ptr<Expression>> parsed;
   vector<llvm::Value*> generated;

   map<token_kind, int> precedences = {{token_kind::OP_ADD, 8},
				       {token_kind::OP_SUB, 7},
				       {token_kind::OP_MUL, 9},
				       {token_kind::OP_DIV, 11},
				       {token_kind::OP_MOD, 10},
				       {token_kind::OP_EXP, 13},
				       {token_kind::OP_ROOT, 12}};

   //LLVM stuff
   llvm::LLVMContext context;
   llvm::IRBuilder<> builder;
   unique_ptr<llvm::Module> module;

   //Stack of names in scope
   //(can't be std::stack because is_in_scope accesses all indices)
   vector<map<string, llvm::AllocaInst*>> names;
   //TODO: will probably need another for refs

   void init_LLVM();

   //Operations on stack of names
   /*
     Note w/r/t scope: -generation- is never going to 'go back' to
     scopes that have collapsed, so scope really can be a stack, with 
     previously higher level sections of the stack being discarded.
    */

   map<string, llvm::AllocaInst*>& push_scope()
   {
      names.emplace_back();

      return names.back();
   }

   void pop_scope()
   {
      names.pop_back();
   }

   void push_to_scope(const string& name, llvm::AllocaInst* var)
   {
      names.back()[name] = var;
   }

   //pop_from_scope if there's some kind of delete operation? But then
   //you'd want some other mechanism for error info - when the
   //user refers to something that's been deleted. Another names, for
   //deleted ones? It might not need be a stack.

   llvm::AllocaInst* is_in_scope(const string& nm)
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

   llvm::AllocaInst* allocate_instruction(llvm::Type* typ,
					  const string& nam)
   {
      //Insert instruction to allocate stack space for (mutable)
      //variable - but insert it at the start of the current block,
      //since that's what LLVM advises to do.

      //Note it'd be nice if it still inserted them sequentially after
      //the previous allocations. That way there'd be an obvious
      //order, in assembly, from params to inits.
      //You'd have to do a while() with the iterator, checking for
      //non-allocates.
      //(Or you could save and increment a pointer/iterator while
      //making allocates - here, maybe?)

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
      push_to_scope(nam, alloc);

      return alloc;
   }

   //Operations on stream
   
   token get()
   {
      return cur_tok = str.get();
   }

   token peek()
   {
      return str.peek();
   }

   //Miscellaneous helpers

   bool get_literal_int(const string& str,
			int& result);

   int get_binary_precedence(const token& tok);

   bool is_valid_func_name(const string& str) const
   {
      //TODO

      return true;
   }

   bool is_valid_type_name(const string& str) const
   {
      //TODO
      
      return true;
   }

   bool is_type_token(const token& tok) const
   {
      //NB excludes 'void'
      
      switch (tok.GetKind())
      {
	 case token_kind::TYPE_INT:
	 case token_kind::TYPE_FLOAT:
	 case token_kind::TYPE_STRING:
	    return true;

	 case token_kind::NAME:
	 {
	    return is_valid_func_name(tok.GetValue());
	 }
	 
	 default:
	    return false;
      }
   }

   bool is_literal(const token_kind& tok) const
   {
      switch (tok)
      {
	 case token_kind::LIT_INT:
	 case token_kind::LIT_FLOAT:
	 case token_kind::LIT_STRING:
	    return true;

	    //case token_kind::NAME

	 default:
	    return false;
      }
   }

   bool is_rhs_end(const token_kind& tok) const
   {
      //This is mainly for RHSExpression::Parse, which needs to know
      //when the rhs has stopped.
      //Would be easier just to have a token_kind::SEMICOLON!

      switch (tok)
      {
	 case token_kind::BRACE_CLOSE:
	 case token_kind::OP_ASSIGN_VAL:
	 case token_kind::OP_ASSIGN_REF:
	 case token_kind::END:
	    return true;

	 default:
	    return false;
      }
   }

   llvm::Type* GetType(const string str)
   {
      llvm::Type* ty = nullptr;

      if (str == "int")
	 ty = llvm::Type::getInt32Ty(context);

      else if (str == "float")
	 ty = llvm::Type::getFloatTy(context);

      else if (str == "string")
      {
	 //uhhhh - TODO
      }

      return ty;
   }

   llvm::Type* GetType(const token_kind tok)
   {
      switch (tok)
      {
	 case token_kind::TYPE_INT:
	    return llvm::Type::getInt32Ty(context);

	 case token_kind::TYPE_FLOAT:
	    return llvm::Type::getFloatTy(context);

	    //TODO string

	 default:
	    return nullptr;
      }
   }

   size_t GetTypeSize(const string& str) const
   {
      if (str == "int")
	 return 32;

      else if (str == "float")
	 return 32;

      else if (str == "string")
	 //TODO
	 return 0;

      else return 0;
   }
   
public:

   Parser()
      : cur_tok (token_kind::INVALID)
      , builder (context)
   {
   }

   void Parse(token_string toks);
   void Generate();

   void print();

   friend class Expression;
   
   friend class LitIntExpression;
   friend class VarExpression;

   friend class NameExpression;
   friend class BinaryExpression;
   friend class RHSExpression;
   friend class StatementExpression;
   friend class ReturnExpression;

   friend class CallExpression;
   friend class FunctionExpression;
   friend class SignatureExpression;
   
   friend class ParenExpression;
   
   friend class AssignExpression;

   friend class InitVarExpression;
};

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

     However, it does mean they all have to be friends, rather than
     children, of the Parser, since, clearly, as static they can't
     have a parent whose members they could call.

     NB also it's consistent with the actual code generation being virtual/override.
   */

   friend ostream& operator<< (ostream& stream, Expression& expr)
   {
      return expr.print(stream);
   }

   virtual ostream& print(ostream& stream) = 0;
   virtual llvm::Value* Generate(Parser& prs) = 0;

   static unique_ptr<Expression> Parse(Parser& prs);

   //Temporarily a token. Might ditch altogether, dependent on
   //TypeExpression etc.
   virtual token GetType()
   {
      //This is the default, overrided by expressions for which it makes sense
      return token(token_kind::INVALID, string());
   }

   //Again, not necessarily implemented
   virtual string GetSubject()
   {
      return string();
   }

   virtual string GetFuncName() const
   {
      return string();
   }

   virtual string GetParamName(size_t index)
   {
      return string();
   }

   //These two are for VarExpressions, which should generate
   //differently dependent on l- or r-value.

   virtual llvm::Value* GenerateLHS(Parser& prs)
   { return nullptr; }
   
   virtual llvm::Value* GenerateRHS(Parser& prs)
   { return nullptr; }

   virtual token_kind GetParamType(size_t index) const
   {
      return token_kind::INVALID;
   }

   virtual bool IsParam(string paramName) const
   {
      return false;
   }

   virtual bool IsVoid() const
   {
      return true;
   }
};

class VarExpression : public Expression
{
private:
   string varName;
   string typeName;
      
public:
   VarExpression(const string& varNm)
      : varName (varNm)
   {
   }

   VarExpression(const string& varNm,
		 const string& typeNm)
      : varName (varNm)
      , typeName (typeNm)
   {
   }

   ostream& print (ostream& stream) override
   {
      return stream << "VarExpression: " << varName <<
	 ", type: " << typeName << endl;
   }

   static unique_ptr<Expression> Parse(Parser& prs);
   llvm::Value* Generate(Parser& prs) override;   
   llvm::Value* GenerateLHS(Parser& prs) override;   
   llvm::Value* GenerateRHS(Parser& prs) override;

   string GetSubject() override
   {
      return varName;
   }
};

class CallExpression : public Expression
{
private:
   string name;
   vector<unique_ptr<Expression>> args;
      
public:
   CallExpression(const string& funcName,
		  vector<unique_ptr<Expression>> argsGiven)
      : name (funcName)
      , args (move(argsGiven))
   {
   }

   ostream& print (ostream& stream) override
   {
      stream << "CallExpression: " << name << endl;

      for (unsigned int i = 0; i < args.size(); ++i)
      {
	 stream << "[Call argument " << i << ":]" << endl << *args[i];
      }

      return stream << "CallExpression end" << endl;
   }

   llvm::Value* Generate(Parser& prs) override;
};

class NameExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(Parser& prs);
};

class LitIntExpression : public Expression
{
private:
   int value;
   
public:
   LitIntExpression(int val)
      : value (val)
   {
   }

   ostream& print (ostream& stream) override
   {
      return stream << "LitIntExpression: " << value << endl;
   }
   
   static unique_ptr<Expression> Parse(Parser& prs);
   llvm::Value* Generate(Parser& prs) override;
};

class BinaryExpression : public Expression
{
private:
   token_kind op;
      
   unique_ptr<Expression> lhs;
   unique_ptr<Expression> rhs;

public:
   BinaryExpression(token_kind opKind,
		    unique_ptr<Expression> left,
		    unique_ptr<Expression> right)
      : op (opKind)
      , lhs (move(left))
      , rhs (move(right))
   {
   }

   ostream& print (ostream& stream) override
   {
      token tok = token(op);

      stream << "BinaryExpression: " << tok << endl;

      stream << "[Binary left hand side:]" << endl << *lhs;
      stream << "[Binary right hand side:]" << endl << *rhs;

      return stream << "BinaryExpression end" << endl;
   }
   
   static unique_ptr<Expression> Parse(Parser& prs,
				       unique_ptr<Expression> left);
   llvm::Value* Generate(Parser& prs) override;
};

class SignatureExpression : public Expression
{
private:
   //Name goes here bc you could define (and then parse) the sig
   //without the body. Whereas the body will never be defined without
   //an accompanying sig to parse.
   string funcName;
   
   //vector<unique_ptr<Expression>> rets;
   //vector<unique_ptr<Expression>> params;

   vector<string> rets;
   vector<tuple<string, string>> params;

public:
   SignatureExpression(const string& name,
		       vector<string> rs,
		       vector<tuple<string, string>> args)
      : funcName (name)
      , rets (move(rs))
      , params (move(args))
   {
   }

   ostream& print (ostream& stream) override
   {
      stream << "SignatureExpression: " << funcName << endl;

      for (unsigned int i = 0; i < rets.size(); ++i)
      {
	 stream << "[Signature return " << i << ":]" << endl << rets[i];
      }

      stream << endl;

      for (unsigned int i = 0; i < rets.size(); ++i)
      {
	 stream << "[Signature param " << i << ":]" <<
	    get<0>(params[i]) << " " << get<1>(params[i]) << endl;
      }

      stream << endl;

      return stream << "SignatureExpression end" << endl;
   }

   static unique_ptr<Expression> Parse(Parser& prs);
   llvm::Function* Generate(Parser& prs) override;

   string GetFuncName() const override
   {
      return funcName;
   }

   string GetParamName(size_t index) override
   {
      return get<1>(params[index]);
   }

   //TODO: will have to be changed for custom types...
   token_kind GetParamType(size_t index) const override
   {
      const string& typeName = get<0>(params[index]);

      if (typeName == "int")
	 return token_kind::TYPE_INT;

      if (typeName == "float")
	 return token_kind::TYPE_FLOAT;

      if (typeName == "string")
	 return token_kind::TYPE_STRING;

      else return token_kind::INVALID;
   }

   //ie has this name for a param already been used as a name for a param?
   bool IsParam(string paramName) const override
   {
      for (unsigned int i = 0; i < params.size(); ++i)
      {
	 if (get<1>(params[i]) == paramName)
	 {
	    return true;
	 }
      }

      return false;
   }

   bool IsVoid() const override
   {
      return !rets.size();
   }
};

class FunctionExpression : public Expression
{
private:
   unique_ptr<Expression> signature;
   vector<unique_ptr<Expression>> statements;

public:
   FunctionExpression(unique_ptr<Expression> sig,
		      vector<unique_ptr<Expression>>& stmts)
      : signature (move(sig))
      , statements (move(stmts))
   {
   }

   ostream& print (ostream& stream) override
   {
      stream << "FunctionExpression: " << endl;

      stream << "[Function signature:]" << endl << *signature;

      for (unsigned int i = 0; i < statements.size(); ++i)
      {
	 stream << "[Function statement " << i << ":]" << *statements[i];
      }

      return stream << endl << "FunctionExpression end" << endl;
   }

   static unique_ptr<Expression> Parse(Parser& prs);
   llvm::Value* Generate(Parser& prs) override;
};

class ParenExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(Parser& prs);
};

class ReturnExpression : public Expression
{
private:
   vector<unique_ptr<Expression>> rets;

public:
   ReturnExpression(vector<unique_ptr<Expression>> rs)
      : rets (move(rs))
   {
   }
   
   static unique_ptr<Expression> Parse(Parser& prs);
   llvm::Value* Generate(Parser& prs) override;

   ostream& print (ostream& stream) override
   {
      stream << "ReturnExpression: " << endl;

      for (unsigned int i = 0; i < rets.size(); ++i)
      {
	 stream << "[Return " << i << ":]" << endl << *rets[i];
      }

      return stream << "ReturnExpression end" << endl;
   }
};

class StatementExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(Parser& prs);
};

class RHSExpression : public Expression
{
public:
   static unique_ptr<Expression> Parse(Parser& prs);
};

//TODO: merge these next two into one Expression, with two Generating
//expressions?

class AssignExpression : public Expression
{
private:
   unique_ptr<Expression> lhs;
   unique_ptr<Expression> rhs;
   
public:
   AssignExpression(unique_ptr<Expression> left,
		    unique_ptr<Expression> right)
      : lhs (move(left))
      , rhs (move(right))
   {
   }

   ostream& print (ostream& stream) override
   {
      stream << "AssignExpression: " <<  endl;

      stream << "[Assign left hand side:]" << endl << *lhs;
      stream << "[Assign right hand side:]" << endl << *rhs;

      return stream << "AssignExpression end" << endl;
   }

   static unique_ptr<Expression> Parse(Parser& prs,
				       unique_ptr<Expression> left);
   llvm::Value* Generate(Parser& prs) override;

   string GetSubject() override
   {
      if (!lhs)
	 return string();

      else
      	 return lhs->GetSubject();
   }
};

class InitVarExpression : public Expression
{
private:
   string varName;
   string typName;

public:
   InitVarExpression(const string& varNm,
		     const string& typNm)
      //unique_ptr<Expression> in)
      : varName (varNm)
      , typName (typNm)
	// init (move(in))
   {
   }

   InitVarExpression(const string& varNm)
		     //	     unique_ptr<Expression> in)
      : varName (varNm)
	// , init (move(in))
   {
   }

   ostream& print (ostream& stream) override
   {
      return stream << "InitVarExpression: " << typName << " " << varName << endl;
   }

   llvm::Value* Generate(Parser& prs) override;
   llvm::Value* GenerateLHS(Parser& prs) override;
};

//TODO: control flow Expressions: return, if...

/*
a = 0;
b = 1;
a = b;
b = 2;
*/
