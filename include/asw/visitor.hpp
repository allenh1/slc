// Copyright 2024 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ASW__VISITOR_HPP_
#define ASW__VISITOR_HPP_

namespace llvm
{
struct Value;
}  // namespace llvm

namespace asw::slc
{

struct binary_op;
struct collect_loop;
struct do_loop;
struct expression;
struct extern_function;
struct formal;
struct function_body;
struct function_call;
struct function_definition;
struct if_expr;
struct infinite_loop;
struct iterator_definition;
struct lambda;
struct list;
struct list_op;
struct literal;
struct loop;
struct node;
struct set_expression;
struct simple_expression;
struct unary_op;
struct variable;
struct variable_definition;
struct when_loop;

template<class T>
struct visitor_interface
{
  virtual ~visitor_interface() = default;
  using return_type = std::conditional_t<std::is_trivial_v<T>, T, const T &>;
  virtual return_type visit_binary_op(binary_op * const) const = 0;
  virtual return_type visit_collect_loop(collect_loop * const) const = 0;
  virtual return_type visit_do_loop(do_loop * const) const = 0;
  virtual return_type visit_extern_function(extern_function * const) const = 0;
  virtual return_type visit_formal(formal * const) const = 0;
  virtual return_type visit_function_body(function_body * const) const = 0;
  virtual return_type visit_function_call(function_call * const) const = 0;
  virtual return_type visit_function_definition(function_definition * const) const = 0;
  virtual return_type visit_if_expr(if_expr * const) const = 0;
  virtual return_type visit_infinite_loop(infinite_loop * const) const = 0;
  virtual return_type visit_iterator_definition(iterator_definition * const) const = 0;
  virtual return_type visit_variable_definition(variable_definition * const) const = 0;
  virtual return_type visit_lambda(lambda * const) const = 0;
  virtual return_type visit_list(list * const) const = 0;
  virtual return_type visit_list_op(list_op * const) const = 0;
  virtual return_type visit_literal(literal * const) const = 0;
  virtual return_type visit_node(node * const) const = 0;
  virtual return_type visit_set_expression(set_expression * const) const = 0;
  virtual return_type visit_simple_expression(simple_expression * const) const = 0;
  virtual return_type visit_unary_op(unary_op * const) const = 0;
  virtual return_type visit_variable(variable * const) const = 0;
  virtual return_type visit_when_loop(when_loop * const) const = 0;
};

struct visitor : public visitor_interface<bool> {};
struct llvm_visitor : public visitor_interface<llvm::Value *> {};

}  // namespace asw::slc
#endif  // ASW__VISITOR_HPP_
