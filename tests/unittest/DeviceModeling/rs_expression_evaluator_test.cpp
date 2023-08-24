/**
 * @file rs_expression_evaluator_test.cpp
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "DeviceModeling/rs_expression_evaluator.h"

#include <gtest/gtest.h>

#include <map>

using namespace std;

// Define the types you want to use in the rs_expression_evaluator.
typedef double T;
typedef double E;

// Typedef for convenience.
typedef rs_expression_evaluator<T, E> ExprEval;

TEST(RSExpressionEvaluatorTest, SetAndGetSymbol) {
  exprtk::symbol_table<T> symbol_table;
  string symbol = "x";
  E value = 5.0;

  bool new_symbol = ExprEval::set_symbol(symbol, symbol_table, value);
  ASSERT_TRUE(new_symbol);

  E retrieved_value = ExprEval::get_symbol_val(symbol, symbol_table);
  ASSERT_EQ(value, retrieved_value);
}

TEST(RSExpressionEvaluatorTest, EvaluateExpression) {
  string expr_str = "2 * x + 3 * y";
  map<string, E> value_map = {{"x", 3.0}, {"y", 4.0}};
  E expected_result = 18.0;
  E result;

  bool success = ExprEval::evaluate_expression(expr_str, value_map, result);
  ASSERT_TRUE(success);
  ASSERT_EQ(expected_result, result);
}

TEST(RSExpressionEvaluatorTest, SymbolsOfExpression) {
  string expr_str = "x * y + z";
  set<string> expected_symbols = {"x", "y", "z"};
  set<string> symbols;

  bool success = ExprEval::symbols_of_expression(expr_str, symbols);
  ASSERT_TRUE(success);
  ASSERT_EQ(expected_symbols, symbols);
}

TEST(RSExpressionEvaluatorTest, InferSymbolSet) {
  string expr_str = "a * b - c";
  ExprEval ee(expr_str);

  bool success = ee.infer_symbol_set();
  ASSERT_TRUE(success);

  set<string> expected_symbols = {"a", "b", "c"};
  const set<string> &symbols = ee.get_symbol_set();
  ASSERT_EQ(expected_symbols, symbols);
}

TEST(RSExpressionEvaluatorTest, Evaluate) {
  string expr_str = "x * y + z";
  ExprEval ee(expr_str);
  map<string, E> value_map = {{"x", 2.0}, {"y", 3.0}, {"z", 4.0}};
  E expected_result = 10.0;

  bool success = ee.evaluate(value_map);
  ASSERT_TRUE(success);

  E result = ee.get_value();
  ASSERT_EQ(expected_result, result);
}

// int main(int argc, char **argv)
// {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
