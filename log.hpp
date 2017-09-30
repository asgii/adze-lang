#pragma once

#include <string>
#include <vector>
#include <iostream>

/*
TODO
Each token should have a place in buffer, a line and a char number.
How would the generator know though?
It could if an Expression carried the (first) token along with it.
*/

class Error
{
private:
   size_t line;
   size_t letter;

   string msg;

public:
   Error(size_t li, size_t le, const string& message)
      : line (li)
      , letter (le)
      , msg (message)
   {
   }

   friend ostream& operator<< (ostream& stream, Error err)
   {
      return stream << err.line << ", " << err.letter << ": " << err.msg;
   }
};

class Log
{
private:
   vector<Error> errors;

   Log()
   {
   }

   static Log& getInstance()
   {
      static Log instance;

      return instance;
   }

public:

   static void log_error(const Error& err)
   {
      Log& instance = getInstance();

      instance.errors.push_back(err);
   }

   static void print()
   {
      Log& instance = getInstance();

      for (unsigned int i = 0; i < instance.errors.size(); ++i)
      {
	 cout << instance.errors[i] << endl;
      }

      cout << instance.errors.size() << " errors total." << endl;
   }
};
