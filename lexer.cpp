#include "lexer.hpp"

token_kind token::GetKind() const
{
   return kind;
}

string token::GetValue() const
{
   return value;
}

void token_string::push(token tok)
{
   toks.push_back(tok);
}

size_t token_string::size() const
{
   return toks.size();
}

token token_string::operator[] (size_t index)
{
   return toks[index];
}

char lexer::next_char()
{
   char res;

   res = fl.get();

   if (res != char_traits<char>::eof())
      return res;

   //You could do whitespace checking here, and simply return a
   //negative value that meant whitespace. That way the
   //semantics could be preserved by the caller.

   //End of file
   else return -1;
}

void lexer::skip_line()
{
   char cur = fl.get();

   while (cur != '\n')
   {
      //Leave as eof token
      if (cur == char_traits<char>::eof())
      {
	 fl.unget();
	 break;
      }

      else cur = fl.get();
   }
}

void lexer::skip_closed_comment()
{
   char cur;

   bool possible_close = false;

   while (cur = next_char(), cur != -1)
   {
      switch(cur)
      {
	 case '*':
	 {
	    possible_close = true;
	    break;
	 }

	 case '/':
	 {
	    if (possible_close)
	    {
	       //End of comment
	       goto skip_done;
	    }
	 }

	 default:
	 {
	    possible_close = false;
	    break;
	 }
      }
   }

  skip_done:
   return;
}

token_kind lexer::is_literal(const string& str)
{
   if ((str[0] == '\"') && (*str.end() == '\"'))
   {
      return token_kind::LIT_STRING;
   }

   else
   {
      //TODO: Add hexadecimal?

      //Either it's numeric, or false
      if (str[0] == '.')
      {
	 //Could only be float
	 for (unsigned int i = 1; i < str.size() - 1; ++i)
	 {
	    if (!isdigit(str[i]))
	       return token_kind::INVALID;
	 }

	 //Is float
	 return token_kind::LIT_FLOAT;
      }

      else if (str[0] == '-')
      {
	 //Could be int or float	    
	 for (unsigned int i = 1; i < str.size() - 1; ++i)
	 {
	    //NB could even be -.01020 etc.
	    if (str[i] == '.')
	    {
	       for (unsigned int j = i + 1; j < str.size() - 1; ++j)
	       {
		  if (!isdigit(str[j]))
		     return token_kind::INVALID;
	       }

	       return token_kind::LIT_FLOAT;
	    }

	    else if (!isdigit(str[i]))
	       return token_kind::INVALID;
	 }

	 return token_kind::LIT_FLOAT;
      }

      else if (isdigit(str[0]))
      {
	 //Could be float or int
	 for (unsigned int i = 1; i < str.size() - 1; ++i)
	 {
	    if (str[i] == '.')
	    {
	       //Could only be float
	       for (unsigned int j = i + 1; j < str.size() - 1; ++j)
	       {
		  if (!isdigit(str[j]))
		     return token_kind::INVALID;
	       }

	       return token_kind::LIT_FLOAT;
	    }

	    else if (!isdigit(str[i]))
	       return token_kind::INVALID;
	 }

	 return token_kind::LIT_INT;
      }

      else return token_kind::INVALID;
   }
}

bool lexer::is_valid_name(const string& str)
{
   if (str.size() < 1)
      return false;

   //First char has to be alphabetic or underscore
   //NB very important it can't have ' anywhere
   if (!isalpha(str[0]) &&
       (str[0] != '_'))
      return false;
      
   for (unsigned int i = 1; i < str.size() - 1; ++i)
   {
      if (!isalnum(str[i]) &&
	  (str[i] != '_') &&
	  (str[i] != '-'))
	 return false;
   }

   //Last character can't have a -
   if (*str.end() == '-')
      return false;

   return true;
}

token lexer::next_token()
{
   string lit;
   char cur;

   //Take care for possible comments since they can't be delimited
   //exclusively by whitespace
   bool possible_comment = false;

   cur = next_char();

   //next_char() doesn't (and shouldn't really) do this
   while ((cur == ' ') ||
	  (cur == '\n')/* ||
	  (cur == '\r') ||
	  (cur == '\t') ||
	  (cur == '\v') ||
	  (cur == '\f')*/)
   {
      cur = next_char();
   }

   //First pass, for things which are meaningful at start
   switch (cur)
   {	 
      case -1:
	 return token(token_kind::END);

      case ';':
	 return token(token_kind::SEMICOLON);
      case ',':
	 return token(token_kind::COMMA);

      case '(':
	 return token(token_kind::PAREN_OPEN);
      case ')':
	 return token(token_kind::PAREN_CLOSE);

      case '{':
	 return token(token_kind::BRACE_OPEN);
      case '}':
	 return token(token_kind::BRACE_CLOSE);
   }
      
   while (true)
   {
      //next_char() afterwards, so this works on first character

      switch (cur)
      {
	 //End of file; abort current
	 case -1:
	    goto abort_lit;

	    //End of word
	 case ' ':
	 case '\n':
	 case '\r':
	 case '\t':
	 case '\v':
	 case '\f':
	 {
	    goto abort_lit;
	 }

	 case ';':
	 case ',':
	 case '(':
	 case ')':
	 case '{':
	 case '}':
	 {
	    /*
	      This is not the first pass (for these specific
	      characters, anyway). So we're mid-word - and previous
	      word has to be taken as a token on its own.
	      So decrease pos, so that (/{ are added as tokens, too
	      (this is what the first pass is for).
	    */
	    fl.unget();

	    goto abort_lit;
	 }

	 case '/':
	 {
	    if (possible_comment)
	    {
	       //Definitely a comment; abort and skip line
	       skip_line();

	       //Get rid of / starting comment
	       lit.pop_back();
		  
	       goto abort_lit;
	    }

	    else
	    {
	       possible_comment = true;
	       break;
	    }
	 }

	 case '*':
	 {
	    if (possible_comment)
	    {
	       //Definitely a comment; abort and skip til end
	       skip_closed_comment();

	       //Get rid of / starting comment
	       lit.pop_back();
		  
	       goto abort_lit;
	    }
	 }
      }

      lit += cur;

      cur = next_char();
   }

  abort_lit:
      
   //Check it's empty (covering above's back)
   if (lit.size() == 0)
      return next_token();

   //Check for obvious tokens, otherwise save as string (name - of
   //variable, or custom type)
            
   if (keywords.count(lit))
   {
      return token(keywords.find(lit)->second, lit);
   }

   if (primitives.count(lit))
   {
      return token(primitives.find(lit)->second, lit);
   }

   if (is_literal(lit) != token_kind::INVALID)
   {
      if ((is_literal(lit) == token_kind::LIT_INT) ||
	  (is_literal(lit) == token_kind::LIT_FLOAT))
      {
	 return token(is_literal(lit), lit);
      }

      if (is_literal(lit) == token_kind::LIT_STRING)
      {
	 //Return without enclosing quotes
	 return token(token_kind::LIT_STRING, string(lit, 1, lit.size() - 2));
      }
   }

   else
   {
      /*
	All you have to do here is check that it's a valid
	-possible- name.

	Note also that the validity of the name in its place will
	be determined by the parser, not the lexer; better not to
	reject file outright, to have useful errors.
      */
      if (is_valid_name(lit))
      {
	 return token(token_kind::NAME, lit);
      }
   }

   //Makes sense to do error stuff in the parser instead.
   return token(token_kind::INVALID, lit);
}

token_string lexer::lex(char* str)
{
   token_string result;

   fl.open(str, ifstream::in);
      
//      buf = str;

   token tok = next_token();

   while (tok.GetKind() != token_kind::END)
   {
      result.push(tok);

      tok = next_token();
   }
      
   return result;
}

/*
//This is just a test
int main(int argc, char** argv)
{
   lexer lexer;

   token_string toks = lexer.lex(argv[1]);

   cout << toks;
}
*/
