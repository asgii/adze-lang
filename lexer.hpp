#pragma once

#include <fstream>

#include <iostream>
#include <cstring>

#include <string>
#include <map>
#include <set>
#include <vector>

using namespace std;

enum class token_kind
{
   KEY_MAIN, //TODO: remove this (it's just a function NAME)
   KEY_RETURN,

   COMMA,
   SEMICOLON,

   OP_ASSIGN_VAL,
   OP_ASSIGN_REF,

   OP_ADD,
   OP_SUB,
   OP_MUL,
   OP_DIV,
   OP_MOD,
   OP_EXP,
   OP_ROOT,

   //Unlike semicolons, these are meaningful (scoping)
   BRACE_OPEN,
   BRACE_CLOSE,
   PAREN_OPEN,
   PAREN_CLOSE,

   LIT_FLOAT,
   LIT_INT,
   LIT_STRING,

   TYPE_VOID,
   TYPE_FLOAT, //TODO: remove this stuff?
   TYPE_INT,
   TYPE_STRING,

   //TODO: remove
   TYPE_INT_REF,

   NAME,
   INVALID,

   //Needn't be written to vector, but useful for lexer
   END,
};


static map<string, token_kind> keywords = {{"main", token_kind::KEY_MAIN},
					   {"return", token_kind::KEY_RETURN},
					   {"=", token_kind::OP_ASSIGN_VAL},
					   {"'=", token_kind::OP_ASSIGN_REF},
					   {"+", token_kind::OP_ADD},
					   {"-", token_kind::OP_SUB},
					   {"*", token_kind::OP_MUL},
					   {"/", token_kind::OP_DIV},
					   {"%", token_kind::OP_MOD},
					   {"^", token_kind::OP_EXP},
					   {"¬/", token_kind::OP_ROOT}};

static map<string, token_kind> primitives = {{"void", token_kind::TYPE_VOID},
					     {"int", token_kind::TYPE_INT},
					     {"float", token_kind::TYPE_FLOAT},
					     {"string", token_kind::TYPE_STRING},
					     //TODO: These should be done programatically
					     {"int'", token_kind::TYPE_INT_REF}};

class token
{
private:
   token_kind kind;

   string value;

public:

   token(token_kind k)
      : kind (k)
      , value ()
   {
   }

   token(token_kind k, const string& v)
      : kind (k)
      , value (v)
   {
   }

   token_kind GetKind() const;
   string GetValue() const;

   friend ostream& operator<< (ostream& stream, token& tok)
   {
      switch (tok.GetKind())
      {
	 //This would be easier if you just had a map<token_kind, string>.
	 
	 case token_kind::KEY_MAIN:
	    return stream << "KEY_MAIN";
	 case token_kind::KEY_RETURN:
	    return stream << "KEY_RETURN";

	 case token_kind::COMMA:
	    return stream << "COMMA";
	 case token_kind::SEMICOLON:
	    return stream << "SEMICOLON";
	 
	 case token_kind::OP_ASSIGN_VAL:
	    return stream << "OP_ASSIGN_VAL";
	 case token_kind::OP_ASSIGN_REF:
	    return stream << "OP_ASSIGN_REF";

	 case token_kind::OP_ADD:
	    return stream << "OP_ADD";
	 case token_kind::OP_SUB:
	    return stream << "OP_SUB";
	 case token_kind::OP_MUL:
	    return stream << "OP_MUL";
	 case token_kind::OP_DIV:
	    return stream << "OP_DIV";
	 case token_kind::OP_MOD:
	    return stream << "OP_MOD";
	 case token_kind::OP_EXP:
	    return stream << "OP_EXP";
	 case token_kind::OP_ROOT:
	    return stream << "OP_ROOT";
	    
	 case token_kind::PAREN_OPEN:
	    return stream << "PAREN_OPEN";
	 case token_kind::PAREN_CLOSE:
	    return stream << "PAREN_CLOSE";
	 case token_kind::BRACE_OPEN:
	    return stream << "BRACE_OPEN";
	 case token_kind::BRACE_CLOSE:
	    return stream << "BRACE_CLOSE";
	    
	 case token_kind::LIT_FLOAT:
	    return stream << "LIT_FLOAT";
	 case token_kind::LIT_INT:
	    return stream << "LIT_INT";
	 case token_kind::LIT_STRING:
	    return stream << "LIT_STRING";

	 case token_kind::TYPE_VOID:
	    return stream << "TYPE_VOID";
	 case token_kind::TYPE_INT:
	    return stream << "TYPE_INT";
	 case token_kind::TYPE_FLOAT:
	    return stream << "TYPE_FLOAT";
	 case token_kind::TYPE_STRING:
	    return stream << "TYPE_STRING";

	    //TODO: remove
	 case token_kind::TYPE_INT_REF:
	    return stream << "TYPE_INT_REF";
	    
	 case token_kind::NAME:
	    return stream << "NAME";
	 case token_kind::INVALID:
	    return stream << "INVALID";

	 default:
	    return stream;
      }
   }
};

class token_string
{
private:
   vector<token> toks;

public:

   void push(token tok);
   size_t size() const;
   token operator[] (size_t index);

   friend ostream& operator<< (ostream& stream, token_string& tokens)
   {
      cout << "Size of token string: " << tokens.toks.size() << '\n'; //

      for (unsigned int i = 0; i < tokens.toks.size(); ++i)
      {
	 stream << tokens.toks[i] << '\n';
      }

      return stream;
   }
};


//TODO: do programatic ' detection for types - including custom types,
//so you have to have a token_kind::NAME_REF which saves the name (not
//the ')

class lexer
{
private:
   string buf;
   ifstream fl;

   char next_char();
   token next_token();
   
   void skip_line();
   void skip_closed_comment();

   token_kind is_literal(const string& str);
   bool is_valid_name(const string& str);

public:
   lexer()
   {
   }

   token_string lex(char* str);
};

/*
Features:
-Everything is on the heap; variables are references.
(Except things for which it'd be redundant - things >= size of a
pointer. But they are treated as if they are references.)
-(Therefore obviously) only one kind of passing, pass-by-reference.
-Idea is thread-safety. Alternative to 'lifetimes' (whereby referee has to live longer than referrer):
referee -can- be destroyed before referrer, but only if it's replaced atomically.

int' a '= b; //assign
int a = b; //copy

//spitballing:
int. a .= b;
int:a := b;

int, a ,= b;
int' a '= b;

If you pass by reference (assign), then implicitly the referee is volatile.
So all future writes are atomic?

Be nice to know which are volatile...

int' a '= b;
const int' a '= b; //Can still be useful: effectively volatile read-only.

-GC: since everything is a reference, if it goes out of scope, you can bin it.
This isn't true if it's a int' though. So are they ref-counted?
If you can '= to -any- thing, even not declared with ', doesn't that
make everything vulnerable?

-Further addition to mut: vis, a warning you might '= to it.
That way any function without 'vis' needn't change its ref-count.
Anything that is vis (') will have a ref-count.
(Couldn't this work just with ', even in functions?)
I think my point here was specifically -threading- functions.
In a linear control flow, it doesn't matter whether something is mut:
it won't make the original variable volatile. It's as if the
function was inline.
The danger is when you pass to a function which can't be inlined,
because it, in turn, splits the control flow into threads.
So functions have to be marked when they branch a (specific) variable.
I think I can still use ' for that though, because in a linear
function, ' is not really necessary.
Const/mut might still be useful to remind people that a linear
function writes, though.

You could make the language dynamic easily; the only difference would
be that you couldn't optimise by replacing atomic ops with non-atomic
ops in the case of a ' that was never threaded.


some_fn(int' x, int y);

int a = 1;
int' b = a;

int' c '= a; //ERROR
int' d '= b;

some_fn(a, a); //ERROR
some_fn(b, a);
*/
