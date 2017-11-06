#include "CallExpression.hpp"

CallExpression::CallExpression(const string& funcName,
	       vector<unique_ptr<Expression>> argsGiven)
   : name (funcName)
   , args (move(argsGiven))
{
}

ostream&
CallExpression::print (ostream& stream)
{
   stream << "CallExpression: " << name << endl;

   for (unsigned int i = 0; i < args.size(); ++i)
   {
      stream << "[Call argument " << i << ":]" << endl << *args[i];
   }

   return stream << "CallExpression end" << endl;
}

