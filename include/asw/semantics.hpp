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

#ifndef ASW__SEMANTICS_HPP_
#define ASW__SEMANTICS_HPP_

#include <asw/slc_node.hpp>

namespace asw::slc
{

class SemanticAnalyzer : public visitor
{
public:
  SemanticAnalyzer(const SemanticAnalyzer &) = delete;
  SemanticAnalyzer & operator=(const SemanticAnalyzer &) = delete;

  static SemanticAnalyzer & get_instance();

  bool visit(node * const n) const;
  bool visit_children(node * const n) const;

  bool visit_binary_op(binary_op * const op) const override;
  bool visit_node(node * const n) const override;
  bool visit_function_body(function_body * const body) const override;
  bool visit_function_call(function_call * const call_) const override;
  bool visit_extern_function(extern_function * const func_) const override;
  bool visit_function_definition(function_definition * const func_) const override;
  bool visit_if_expr(if_expr * const if_stmt) const override;
  bool visit_variable_definition(variable_definition * const var_) const override;
  bool visit_formal(formal * const var) const override;
  bool visit_lambda(lambda * const lambda) const override;
  bool visit_list_op(list_op * const op) const override;
  bool visit_list(list * const _list) const override;
  bool visit_literal(literal * const l) const override;
  bool visit_simple_expression(simple_expression * const s) const override;
  bool visit_unary_op(unary_op * const op) const override;
  bool visit_variable(variable * const var) const override;

  bool scope_has_variable(
    const std::string & name, const scope * s,
    variable_definition ** p_var = nullptr) const
  {
    for (variable_definition * const v : s->variables) {
      if (v->get_name() == name) {
        if (nullptr != p_var) {
          *p_var = v;
        }
        return true;
      }
    }
    return false;
  }

  bool scope_has_function(
    const std::string & name, const scope * s,
    function_definition ** p_func = nullptr) const
  {
    for (function_definition * f : s->functions) {
      if (f->get_name() == name) {
        if (nullptr != p_func) {
          *p_func = f;
        }
        return true;
      }
    }
    return false;
  }

  bool scope_has_definition(
    const std::string & name, const scope * s,
    definition ** p_def = nullptr) const
  {
    function_definition * func;
    variable_definition * var;
    if (scope_has_function(name, s, &func)) {
      *p_def = func;
      return true;
    } else if (scope_has_variable(name, s, &var)) {
      *p_def = var;
      return true;
    }
    return false;
  }

  template<class ... Args>
  void internal_compiler_error(const char * fmt, Args && ... args) const
  {
    std::string err_text = std::string("\e[1;31minternal compiler error:\e[0m ") + fmt;
    fprintf(stderr, err_text.c_str(), args ...);
  }

  template<class ... Args>
  void error(const char * fmt, node * const node, Args && ... args) const
  {
    location_info * loc = node->get_location();
    std::string location_text;
    if (nullptr == loc) {
      location_text = "location unavailable";
    } else {
      location_text = std::string("line ") + std::to_string(loc->line) + " column " +
        std::to_string(loc->column);
    }
    std::string err_text = "\e[1;31merror (" + location_text + "):\e[0m " + fmt;
    fprintf(stderr, err_text.c_str(), args ...);
  }

#ifdef DEBUG
  template<class ... Args>
  void debug(const char * fmt, node * const node, Args && ... args) const
  {
    location_info * loc = node->get_location();
    std::string location_text;
    if (nullptr == loc) {
      location_text = "location unavailable";
    } else {
      location_text = std::string("line ") + std::to_string(loc->line) + " column " +
        std::to_string(loc->column);
    }
    std::string err_text = "\e[1;35minfo (" + location_text + "):\e[0m " + fmt;
    fprintf(stderr, err_text.c_str(), args ...);
  }
#else
  template<class ... Args>
  void debug(const char *, node * const, Args && ...) const {}
#endif  // DEBUG

private:
  mutable std::size_t str_counter{0};
  SemanticAnalyzer() = default;
  ~SemanticAnalyzer() override = default;
  inline static SemanticAnalyzer * impl = nullptr;
};

}  // namespace asw::slc

#endif  // ASW__SEMANTICS_HPP_
