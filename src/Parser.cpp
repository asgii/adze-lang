#include "Parser.hpp"

#include "log.hpp"

//for top-level parsing
#include "exprs/subexprs/FunctionExpression.hpp"

//for llvm::errs() - hopefully temporary
#include "llvm/Support/raw_ostream.h"

using namespace std;

token_stream::token_stream()
   : pos (0)
   , cur (token(token_kind::INVALID))
{
}

token_stream::token_stream(token_string nStr)
   : str (nStr)
   , pos (0)
   , cur (str.size() ? str[0] : token(token_kind::END))
{
}

const token
token_stream::get()
{
   token tok = token(token_kind::END);
   
   if ((pos + 1) < str.size())
   {
      ++pos;

      tok = str[pos];
   }
      
   return cur = tok;
}

const token
token_stream::peek() const
{
   if ((pos + 1) < str.size())
      return str[pos + 1];

   else return token(token_kind::END);
}

const token
token_stream::cur_tok() const
{
   return cur;
}

//

Parser::Parser()
{
}

void
Parser::printTree()
{
   for (unsigned int i = 0; i < parsed.size(); ++i)
   {
      cout << *parsed[i].get();
   }
}

void
Parser::printIR()
{
   build.GetModule()->print(llvm::errs(), nullptr);
}

void
Parser::Parse(token_string toks)
{   
   //TODO Might isolate this
   str = token_stream(toks);

   while (str.cur_tok().GetKind() != token_kind::END)
   {
      unique_ptr<Expression> func = FunctionExpression::Parse(str,
							      ParseInfo(build));

      if (!func)
      {
	 Log::log_error(Error(0, 0, "Failed to parse FunctionExpression."));
	 
	 break;
      }

      else parsed.push_back(move(func));
   }
}

//TODO remove
int main(int argc, char** argv)
{
   lexer lexer;

   if (argc <= 1)
   {
      return 1;
   }
   
   token_string toks = lexer.lex(argv[1]);
   
   //cout << toks;
   
   Parser prs;

   prs.Parse(toks);

   //prs.printTree();

   prs.Generate();

   //prs.printIR();

   Log::print();

   return 0;
}
