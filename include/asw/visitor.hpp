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

namespace asw::slc
{

struct node;
struct function_definition;
struct variable_definition;
struct literal;
struct list_op;
struct simple_expression;
struct variable;

struct visitor
{
  virtual bool visit_node(node * const) const = 0;
  virtual bool visit_simple_expression(simple_expression * const) const = 0;
  virtual bool visit_literal(literal * const) const = 0;
  virtual bool visit_variable(variable * const) const = 0;
  virtual bool visit_function_definition(function_definition * const) const = 0;
  virtual bool visit_variable_definition(variable_definition * const) const = 0;
  virtual bool visit_list_op(list_op * const) const = 0;

};

}  // namespace asw::slc
#endif  // ASW__VISITOR_HPP_
