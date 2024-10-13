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

  static SemanticAnalyzer & get_instance()
  {
    if (nullptr == impl)
    {
      impl = new SemanticAnalyzer();
    }
    return *impl;
  }

  bool visit(node * const n)
  {
    return n->accept(this);
  }

  bool visit_node(node * const n) const override
  {
    /* root node */
    if (n->is_root()) {
      /* create the global scope */
      n->set_scope(std::make_shared<scope>());
    } else {
      fprintf(stderr, "internal compiler error: visit_node called for non-root node\n");
      return false;
    }
    return visit_children(n);
  }

  bool visit_children(node * const n) const
  {
    for (node * const child : n->get_children()) {
      if (!child->accept(this)) {
        return false;
      }
    }
    return true;
  }

  bool visit_function_definition(function_definition * const func_) const override
  {
    function_definition & func = *func_;
    /* create a new scope under the parent scope */
    auto * parent = func.get_parent();
    auto s = std::make_shared<scope>();
    s->parent = parent->get_scope();
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(func.get_name(), parent->get_scope(), &f_conflict)) {
      location_info & loc = *f_conflict->get_location();
      fprintf(
        stderr,
        "error: conflicting definition for function '%s' (original on line %d column %d): %s\n",
        func.get_name().c_str(),
        loc.line, loc.column, loc.text.c_str());
      return false;
    } else if (scope_has_variable(func.get_name(), parent->get_scope(), &v_conflict)) {
      location_info & loc = *v_conflict->get_location();
      fprintf(
        stderr,
        "error: conflicting definition for function '%s' (original on line %d column %d): %s\n",
        func.get_name().c_str(),
        loc.line, loc.column, loc.text.c_str());
      return false;
    }
    return visit_children(&func);
  }

  bool visit_variable_definition(variable_definition * const var_) const override
  {
    variable_definition & var = *var_;
    auto * parent = var.get_parent();
    if (nullptr == parent) {
      fprintf(stderr, "internal compiler error: parent is null visiting variable definition\n");
    }
    var.set_scope(parent->get_scope());
    /* check scope for redefinition */
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(var.get_name(), parent->get_scope(), &f_conflict)) {
      location_info & loc = *f_conflict->get_location();
      fprintf(
        stderr,
        "error: conflicting definition for variable '%s' (original on line %d column %d): %s\n",
        var.get_name().c_str(),
        loc.line, loc.column, loc.text.c_str());
      return false;
    } else if (scope_has_variable(var.get_name(), parent->get_scope(), &v_conflict)) {
      location_info & loc = *v_conflict->get_location();
      fprintf(
        stderr,
        "error: conflicting definition for variable '%s' (original on line %d column %d): %s\n",
        var.get_name().c_str(),
        loc.line, loc.column, loc.text.c_str());
      return false;
    }
    /* otherwise, append the variable to the scope */
    var.get_scope()->variables.push_back(&var);
    if (var.get_children().size() > 1) {
      location_info & loc = *var.get_location();
      fprintf(
        stderr,
        "error: too many expressions provided for variable definition on line %d column %d: %s\n",
        loc.line, loc.column, loc.text.c_str());
      return false;
    }
    return visit_children(&var);
  }


  bool visit_list_op(list_op * const /* op */) const override
  {
    return true;
  }

  static bool scope_has_variable(
    const std::string & name, const std::shared_ptr<scope> & s, variable_definition ** p_var = nullptr)
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

  bool visit_literal(literal * const) const override
  {
    /* nothing to do here */
    return true;
  }

  bool visit_simple_expression(simple_expression * const) const override
  {
    return true;
  }

  bool visit_variable(variable * const var) const override
  {
    /* find the variable in the scope of this node's parent */
    return true;
  }

  bool scope_has_function(
    const std::string & name, const std::shared_ptr<scope> & s, function_definition ** p_func = nullptr) const
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

private:
  SemanticAnalyzer() = default;
  inline static SemanticAnalyzer * impl = nullptr;
};

}  // namespace asw::slc

#endif  // ASW__SEMANTICS_HPP_
