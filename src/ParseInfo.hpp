#pragma once

#include "lexer.hpp"

#include "ParseBuild.hpp"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

#include <string>
#include <map>

class ParseInfo
/*
  Helpers for Expressions.
  TODO Move these to Expression:: ?
*/
{
private:
   //This is somewhat redundant with the token_kind::OP_*s...
   static const std::map<token_kind, int> precedences;

   //TODO refigure this structure somehow
   llvm::LLVMContext& context;
   
public:

   //(This is because of 'context', above; it's bad
   ParseInfo(ParseBuild& build);

   bool get_literal_int(const std::string& str, int& result);
   //TODO: get_literal_float, get_literal_string (not sure how latter works)
   int get_binary_precedence(const token& tok) const;
   bool is_valid_func_name(const std::string& str) const;
   bool is_valid_type_name(const std::string& str) const;
   //TODO this one is badly named; includes NAME tokens that could be types
   bool is_type_token(const token& tok) const;
   bool is_literal(const token_kind& tok) const;
   bool is_rhs_end(const token_kind& tok) const;

   //Messy helpers. TODO just bundle stuff with tokens instead?
   llvm::Type* GetType(const std::string str);
   llvm::Type* GetType(const token_kind tok);
   size_t GetTypeSize(const std::string& str) const;
};
