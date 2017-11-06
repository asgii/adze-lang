#include "AssignExpression.hpp"

#include "RHSExpression.hpp"

AssignExpression::AssignExpression(unique_ptr<Expression> left,
				   unique_ptr<Expression> right)
   : lhs (move(left))
   , rhs (move(right))
{
}

ostream&
AssignExpression::print (ostream& stream)
{
   stream << "AssignExpression: " <<  endl;

   stream << "[Assign left hand side:]" << endl << *lhs;
   stream << "[Assign right hand side:]" << endl << *rhs;

   return stream << "AssignExpression end" << endl;
}

string
AssignExpression::GetSubject()
{
   if (!lhs)
      return string();

   else
      return lhs->GetSubject();
}

unique_ptr<Expression>
AssignExpression::Parse(token_stream& str,
			ParseInfo info,
			unique_ptr<Expression> left)
{
   //Eat =
   str.get();

   //TODO: should these checks even be done, if they've been passed
   //here as if they are valid already? Caller should really be the
   //one checking, since implicitly it checks for correctness as part
   //of an AssignExpression
   if (!left)
   {
      Log::log_error(Error(0, 0,
			   string("Failure parsing left-hand side of assignment.")));
      
      return nullptr;
   }
   
   unique_ptr<Expression> right = RHSExpression::Parse(str,
						       info);

   if (right == nullptr)
   {
      //TODO: log specifics somehow
      Log::log_error(Error(0, 0,
			   string("Failure parsing right-hand side of assignment.")));
      
      return nullptr;
   }

   //Don't check for SEMICOLON here; do it in StatementExpression,
   //which is what calls this.
   
   return make_unique<AssignExpression>(move(left), move(right));
}
