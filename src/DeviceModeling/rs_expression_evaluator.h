/**
 * @file rs_expression_evaluator.h
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <iostream>
#include "exprtk.hpp"
#include "spdlog/spdlog.h"

using namespace std;
/**
 * @brief A C++ class for expression evaluation using the exprtk library.
 *
 * @tparam T The type of the values used in the expression evaluation (e.g., double, float, etc.).
 * @tparam E The type of the value associated with the symbols in the symbol table (e.g., double, float, etc.).
 */
template <typename T, typename E>
class rs_expression_evaluator
{
   std::string expr_str_ = "";
   E value_;
   bool evaluated_ = false;
   std::set<std::string> symbol_set_;

public:
   /**
    * @brief Constructs an rs_expression_evaluator object with a given expression string.
    *
    * @param exp The expression string.
    */
   rs_expression_evaluator(std::string exp) : expr_str_(exp) {}

   /**
    * @brief Sets the value associated with a given symbol in a symbol table, creating the symbol if necessary.
    *
    * @tparam T The type of the values stored in the symbol table.
    * @tparam E The type of the value associated with the symbol to set.
    * @param smb The name of the symbol to set.
    * @param s_table The symbol table to modify.
    * @param val The new value to associate with the symbol.
    * @return True if the symbol was newly created, false otherwise.
    */
   static bool set_symbol(const string &smb, exprtk::symbol_table<T> &s_table, E val)
   {
      auto s_ptr = s_table.get_variable(smb.c_str());
      bool new_symbol = false;
      if (!s_ptr)
      {
         new_symbol = true;
         s_table.create_variable(smb.c_str());
         s_ptr = s_table.get_variable(smb.c_str());
      }
      s_ptr->ref() = T(val);
      return new_symbol;
   }

   /**
    * @brief Retrieves the value associated with a given symbol in a symbol table.
    *
    * @tparam T The type of the values stored in the symbol table. (The expression evaluation type)
    * @tparam E The type of the value associated with the symbol to retrieve.
    * @param smb The name of the symbol to retrieve.
    * @param s_table The symbol table to search in.
    * @return The value associated with the symbol.
    * @throws std::runtime_error if the symbol is not found in the symbol table.
    */
   static E get_symbol_val(const string &smb, const exprtk::symbol_table<T> &s_table)
   {
      auto s_ptr = s_table.get_variable(smb.c_str());
      if (!s_ptr)
      {
         throw std::runtime_error("Symbol " + smb + " not found in symbol table.");
      }
      E ret_val = static_cast<E>(s_ptr->value());
      return ret_val;
   }
   /**
    * @brief Evaluates the expression using the provided symbol values and stores the result in the given reference.
    *
    * @param expression_str The expression string to evaluate.
    * @param value_map A map of symbol names and their corresponding values.
    * @param result A reference to store the result of the evaluated expression.
    * @return True if the expression is successfully evaluated, false otherwise.
    */
   static bool evaluate_expression(const std::string &expression_str, map<string, E> &value_map, E &result)
   {
      typedef exprtk::symbol_table<T> symbol_table_t;
      typedef exprtk::expression<T> expression_t;
      typedef exprtk::parser<T> parser_t;
      typedef exprtk::parser_error::type error_t;
      typedef typename parser_t::settings_store settings_t;
      symbol_table_t symbol_table;
      expression_t expression;
      expression.register_symbol_table(symbol_table);
      parser_t parser(settings_t(settings_t::compile_all_opts + settings_t::e_disable_usr_on_rsrvd)
                          .disable_all_base_functions()
                          .disable_all_control_structures());
      parser.enable_unknown_symbol_resolver();
      parser.dec().collect_variables() = true;
      parser.dec().collect_functions() = true;
      if (!parser.compile(expression_str, expression))
      {
         for (std::size_t i = 0; i < parser.error_count(); ++i)
         {
            error_t error = parser.get_error(i);
            spdlog::error("Error: {} Position: {} Type: [{}] Message: {} Expression: {}",
                          i,
                          error.token.position,
                          exprtk::parser_error::to_str(error.mode).c_str(),
                          error.diagnostic.c_str(),
                          expression_str.c_str());
         }
         return false;
      }

      typedef typename parser_t::dependent_entity_collector::symbol_t dec_symbol_t;
      std::deque<dec_symbol_t> symbol_list;
      parser.dec().symbols(symbol_list);
      // allow only variables and the function not()
      for (std::size_t i = 0; i < symbol_list.size(); ++i)
      {
         if (exprtk::details::imatch(symbol_list[i].first, "not"))
         {
            symbol_list.erase(symbol_list.begin() + i);

            if (i >= symbol_list.size())
               break;
         }
         if (parser_t::e_st_function == symbol_list[i].second)
         {
            spdlog::error("Error: call to function '{}' not allowed.\n", symbol_list[i].first.c_str());
            return false;
         }
      }

      for (auto &s : symbol_list)
      {
         // DBG spdlog::info("Symbol {} \n", (s.first).c_str());
         auto it = value_map.find(s.first);
         if (end(value_map) != it)
            continue;
         cout << "Enter the missing value of symbol " << s.first << ": ";
         cin >> value_map[s.first];
      }
      for (auto &s : value_map)
      {
         spdlog::info("{}  : {}", s.first, s.second);
         set_symbol(s.first, symbol_table, s.second);
      }
      result = static_cast<E>(expression.value());
      return true;
   }
   /**
    * @brief Retrieves the symbols from an expression string and stores them in a set.
    *
    * @param expression_str The expression string to extract symbols from.
    * @param res A reference to a set in which the extracted symbols will be stored.
    * @return True if symbols are successfully extracted, false otherwise.
    */
   static bool symbols_of_expression(const std::string &expression_str, std::set<std::string> &res)
   {
      typedef exprtk::symbol_table<T> symbol_table_t;
      typedef exprtk::expression<T> expression_t;
      typedef exprtk::parser<T> parser_t;
      typedef exprtk::parser_error::type error_t;
      typedef typename parser_t::settings_store settings_t;
      symbol_table_t symbol_table;
      expression_t expression;
      expression.register_symbol_table(symbol_table);
      parser_t parser(settings_t(settings_t::compile_all_opts + settings_t::e_disable_usr_on_rsrvd)
                          .disable_all_base_functions()
                          .disable_all_control_structures());
      parser.enable_unknown_symbol_resolver();
      parser.dec().collect_variables() = true;
      parser.dec().collect_functions() = true;
      if (!parser.compile(expression_str, expression))
      {
         for (std::size_t i = 0; i < parser.error_count(); ++i)
         {
            error_t error = parser.get_error(i);
            spdlog::error("Error: {} Position: {} Type: [{}] Message: {} Expression: {}",
                          i,
                          error.token.position,
                          exprtk::parser_error::to_str(error.mode).c_str(),
                          error.diagnostic.c_str(),
                          expression_str.c_str());
         }
         return false;
      }

      typedef typename parser_t::dependent_entity_collector::symbol_t dec_symbol_t;
      std::deque<dec_symbol_t> symbol_list;
      parser.dec().symbols(symbol_list);
      // allow only variables and the function not()
      for (std::size_t i = 0; i < symbol_list.size(); ++i)
      {
         if (exprtk::details::imatch(symbol_list[i].first, "not"))
         {
            symbol_list.erase(symbol_list.begin() + i);
            if (i >= symbol_list.size())
               break;
         }
         if (parser_t::e_st_function == symbol_list[i].second)
         {
            spdlog::error("Error: call to function '{}' not allowed.\n", symbol_list[i].first.c_str());
            return false;
         }
      }
      for (auto &s : symbol_list)
      {
         res.insert(s.first);
      }
      return true;
   }
   /**
    * @brief Infers the symbol set from the stored expression string and updates the symbol_set_ member variable.
    *
    * @return True if the symbol set is successfully inferred, false otherwise.
    */
   bool infer_symbol_set()
   {
      return symbols_of_expression(expr_str_, symbol_set_);
   }
   /**
    * @brief Getter method for the symbol set of the stored expression.
    *
    * @return A constant reference to the symbol set.
    */
   const std::set<std::string> &get_symbol_set() const
   {
      return symbol_set_;
   }
   /**
    * @brief Evaluates the stored expression using the given symbol values.
    *
    * @param value_map A map of symbol names and their corresponding values.
    * @return True if the expression is successfully evaluated, false otherwise.
    */
   bool evaluate(map<string, E> &value_map)
   {
      return evaluated_ = evaluate_expression(expr_str_, value_map, value_);
   }
   /**
    * @brief Getter method for the evaluated value of the stored expression.
    *
    * @return The evaluated value of the expression.
    * @throws std::runtime_error if the expression is not yet evaluated.
    */
   E get_value()
   {
      if (!evaluated_)
         throw std::runtime_error("Accessing value of not evaluated expression " + expr_str_);
      return value_;
   }
   /**
    * @brief Sets the expression string for the evaluator and marks it as unevaluated.
    *
    * @param expr The new expression string.
    */
   void set_expr_string(std::string expr)
   {
      evaluated_ = false;
      expr_str_ = expr;
   }
   /**
    * @brief Getter method for the stored expression string.
    *
    * @return A constant reference to the expression string.
    */
   const std::string &get_expr_string() const
   {
      return expr_str_;
   }
   /**
    * @brief Checks whether the stored expression has been evaluated.
    *
    * @return True if the expression has been evaluated, false otherwise.
    */
   bool is_evaluated() const
   {
      return evaluated_;
   }
   /**
    * @brief A REPL (Read-Eval-Print Loop) function to interactively evaluate expressions.
    */
   static void repl()
   {
      std::string expr_str;
      std::cout << "Expression (\"exit\" to exit) : ";
      getline(std::cin >> std::ws, expr_str);
      while ("exit" != expr_str)
      {
         std::set<std::string> symbols;
         rs_expression_evaluator<T, E> ee(expr_str);
         ee.infer_symbol_set();
         symbols = ee.get_symbol_set();
         map<string, E> value_map;
         for (auto &s : symbols)
         {
            std::cout << "Please, provide the value of " << s << " : ";
            std::cin >> value_map[s];
         };
         E res(0);
         if (ee.evaluate(value_map))
            res = ee.get_value();
         std::cout << "The expression value is : " << res << std::endl;
         std::cout << "Expression (\"exit\" to exit) : ";
         getline(std::cin >> std::ws, expr_str);
      }
   }
};
