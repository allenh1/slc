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
    if (nullptr == impl) {
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
      internal_compiler_error("visit_node called for non-root node: '%s'\n", n->get_fqn().c_str());
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

  bool visit_children(node * const n, const std::vector<node *> & skip_list) const
  {
    std::vector<node *> to_visit;
    to_visit.reserve(n->get_children().size());
    for (node * const child : n->get_children()) {
      if (std::find(skip_list.begin(), skip_list.end(), child) == skip_list.end()) {
        to_visit.emplace_back(child);
      }
    }
    for (node * const child : to_visit) {
      if (!child->accept(this)) {
        return false;
      }
    }
    return true;
  }

  bool visit_function_body(function_body * const body) const override
  {
    return visit_children(body);
  }

  bool visit_function_call(function_call * const call_) const override
  {
    /* set up this function call's scope */
    node * parent = nullptr;
    for (parent = call_->get_parent(); parent && (parent->get_scope() == nullptr); ) {
      parent = parent->get_parent();
    }
    if (nullptr == parent) {
      /* made it to root, this should not happen */
      internal_compiler_error(
        "traversed to root node before finding a scope for function '%s'\n", call_->get_fqn());
      return false;
    }
    call_->set_scope(parent->get_scope());
    function_definition * resolved = nullptr;
    auto & scope = call_->get_scope();
    /* search for the requested variable in this scope, or parent scopes */
    for (; scope && !scope_has_function(call_->get_name(), scope, &resolved);
      scope = scope->parent)
    {
    }
    if (!resolved) {
      error("undefined reference to function '%s'\n", call_, call_->get_name().c_str());
      return false;
    } else {
      auto * loc = resolved->get_location();
    }
    return true;
  }

  bool visit_function_definition(function_definition * const func_) const override
  {
    /* create a new scope under the parent scope */
    auto * parent = func_->get_parent();
    func_->set_scope(parent->get_scope());
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(func_->get_name(), parent->get_scope(), &f_conflict)) {
      location_info & loc = *f_conflict->get_location();
      error(
        "conflicting definition for function '%s' (original on line %d column %d): %s\n",
        func_,
        func_->get_name().c_str(),
        loc.line, loc.column, loc.text.c_str());
      return false;
    } else if (scope_has_variable(func_->get_name(), parent->get_scope(), &v_conflict)) {
      location_info & loc = *v_conflict->get_location();
      error(
        "conflicting definition for function '%s' (defined as variable on line %d column %d)\n",
        func_,
        func_->get_name().c_str(),
        loc.line, loc.column);
      return false;
    }
    func_->get_scope()->functions.emplace_back(func_);
    if (nullptr != func_->get_argument_list()) {
      /* visit argument list first */
      if (!func_->get_argument_list()->accept(this)) {
        return false;
      }
    }
    return visit_children(func_, {func_->get_argument_list()});
  }

  bool visit_variable_definition(variable_definition * const var_) const override
  {
    variable_definition & var = *var_;
    auto * parent = var.get_parent();
    if (nullptr == parent) {
      internal_compiler_error("parent is null visiting variable definition");
      return false;
    }
    var.set_scope(parent->get_scope());
    /* check scope for redefinition */
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(var.get_name(), parent->get_scope(), &f_conflict)) {
      location_info & loc = *f_conflict->get_location();
      error(
        "conflicting definition for variable '%s' (original on line %d column %d): %s\n",
        &var,
        var.get_name().c_str(),
        loc.line, loc.column, loc.text.c_str());
      return false;
    } else if (scope_has_variable(var.get_name(), parent->get_scope(), &v_conflict)) {
      location_info & loc = *v_conflict->get_location();
      error(
        "conflicting definition for variable '%s' (original on line %d column %d): %s\n",
        &var,
        var.get_name().c_str(),
        loc.line, loc.column);
      return false;
    }
    /* otherwise, append the variable to the scope */
    var.get_scope()->variables.push_back(&var);
    if (var.get_children().size() > 1) {
      location_info & loc = *var.get_location();
      error(
        "too many expressions provided for variable definition on line %d column %d: %s\n",
        &var,
        loc.line, loc.column, loc.text.c_str());
      return false;
    }
    return visit_children(&var);
  }


  bool visit_list_op(list_op * const op) const override
  {
    /* child (singular) should be an expression */
    if (op->get_children().size() > 1) {
      internal_compiler_error(
        "too many children (%zd) for list operation", op,
        op->get_children().size());
      return false;
    } else if (nullptr == dynamic_cast<list *>(op->get_children()[0])) {
      error("invalid arguments for list operation", op);
      return false;
    }
    return visit_children(op);
  }

  bool visit_list(list * const _list) const override
  {
    return visit_children(_list);
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
    /* set up this variable's scope */
    node * parent = nullptr;
    for (parent = var->get_parent(); parent && (parent->get_scope() == nullptr); ) {
      parent = parent->get_parent();
    }
    if (nullptr == parent) {
      /* made it to root, this should not happen */
      internal_compiler_error(
        "traversed to root node before finding a scope for variable '%s'\n", var->get_fqn());
      return false;
    }
    var->set_scope(parent->get_scope());
    variable_definition * resolved = nullptr;
    auto & scope = var->get_scope();
    /* search for the requested variable in this scope, or parent scopes */
    for (; scope && !scope_has_variable(var->get_name(), scope, &resolved); scope = scope->parent) {
    }
    if (!resolved) {
      error("undefined reference to variable '%s'\n", var, var->get_name().c_str());
      return false;
    }
    var->resolve(resolved);
    return true;
  }

  bool scope_has_variable(
    const std::string & name, const std::shared_ptr<scope> & s,
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
    const std::string & name, const std::shared_ptr<scope> & s,
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

  template<class ... Args>
  void internal_compiler_error(const char * fmt, Args && ... args) const
  {
    std::string err_text = std::string("\e[31minternal compiler error:\e[0m ") + fmt;
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
    std::string err_text = "\e[31merror (" + location_text + "):\e[0m " + fmt;
    fprintf(stderr, err_text.c_str(), args ...);
  }

private:
  SemanticAnalyzer() = default;
  inline static SemanticAnalyzer * impl = nullptr;
};

}  // namespace asw::slc

#endif  // ASW__SEMANTICS_HPP_
