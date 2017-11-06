#include "BinaryExpression.hpp"

#include "RHSExpression.hpp"


BinaryExpression::BinaryExpression(token_kind opKind,
				   unique_ptr<Expression> left,
				   unique_ptr<Expression> right)
   : op (opKind)
   , lhs (move(left))
   , rhs (move(right))
{
}

ostream&
BinaryExpression::print (ostream& stream)
{
   token tok = token(op);

   stream << "BinaryExpression: " << tok << endl;

   stream << "[Binary left hand side:]" << endl << *lhs;
   stream << "[Binary right hand side:]" << endl << *rhs;

   return stream << "BinaryExpression end" << endl;
}

unique_ptr<Expression>
BinaryExpression::Parse(token_stream& str,
			ParseInfo info,
			unique_ptr<Expression> left)
{
   /*
     For clarity: this function starts with the lhs already parsed
     (left), with cur_tok on a binary expression. get() gets the rhs
     of the original binary expression (right). After that, we check
     for -another- binary expression occurring without parentheses;
     that's a situation which needs disambiguating.
   */

   token_kind kind = str.cur_tok().GetKind();
   int precedence = info.get_binary_precedence(kind);

   //Eat binary op
   str.get();
   
   unique_ptr<Expression> right = RHSExpression::Parse(str,
						       info);

   if (!right)
   {
      Log::log_error(Error(0, 0,
			   string("Failure parsing right-hand side of a binary operation.")));
      
      return nullptr;
   }
   
   //Parse should eat the last relevant token; no get().

   token_kind furtherKind = str.cur_tok().GetKind();
   int furtherPrecedence = info.get_binary_precedence(furtherKind);

   //Don't get() here either; it's to be parsed by another call
   //whether or not it's another binary op.

   //ie if it's a binary op
   if (furtherPrecedence)
   {
      if (precedence >= furtherPrecedence)
      {
	 //Eat up this binary op and feed into a new BinaryExpression, as
	 //new left
	 return BinaryExpression::Parse(str,
					info,
					make_unique<BinaryExpression>(kind,
								      move(left),
								      move(right)));
								      
      }

      else
      {
	 /*
	   Eat up the next one, by making a new parse of it, and feed
	   it into this one.

	   Whichever 'goes first', ie has the highest precedence, is
	   not the one that's returned directly, since height in the
	   tree represents backward-order of operations; the leaves
	   are evaluated first.
	  */
	 return make_unique<BinaryExpression>(furtherKind,
					      BinaryExpression::Parse(str,
								      info,
								      move(right)),
					      move(left));
      }
   }

   //i.e., if the now current token isn't another binary op.
   else return make_unique<BinaryExpression>(kind, move(left), move(right));
}
