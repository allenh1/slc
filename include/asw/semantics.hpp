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

  bool visit(node * const n) const
  {
    if (n->visited()) {
      return true;
    }
    n->mark_visiting();
    bool ret = n->accept(this);
    n->mark_visited();
    return ret;
  }

  bool visit_children(node * const n) const
  {
    for (node * const child : n->get_children()) {
      if (child->visited()) {
        continue;
      }
      child->mark_visiting();
      if (!child->accept(this)) {
        return false;
      }
      child->mark_visited();
    }
    return true;
  }

  bool visit_binary_op(binary_op * const op) const override
  {
    if (!visit_children(op)) {
      return false;
    }
    expression * lhs = dynamic_cast<expression *>(op->get_children()[0]);
    expression * rhs = dynamic_cast<expression *>(op->get_children()[1]);
    auto is_int = [](expression * expr) -> bool {return expr->get_type()->type == type_id::INT;};
    auto is_float =
      [](expression * expr) -> bool {return expr->get_type()->type == type_id::FLOAT;};
    auto is_list = [](expression * expr) -> bool {return expr->get_type()->type == type_id::LIST;};
    switch (op->get_op()) {
      case op_id::GREATER:
      case op_id::GREATER_EQ:
      case op_id::LESS:
      case op_id::LESS_EQ:
      case op_id::EQUAL:
        {
          if (!((is_int(lhs) || is_float(lhs)) && (is_int(rhs) || is_float(rhs)))) {
            error(
              "invalid operands for binary operator '%s'\n", op,
              op_to_str(op->get_op()).c_str());
            return false;
          }
          op->set_type(type_id::BOOL);
          return true;
        }
      case op_id::CONS:
        {
          if (!(!lhs->is_list() && is_list(rhs))) {
            error("invalid operands for binary operator 'cons'\n", op);
            return false;
          }
          type_info tmp = *lhs->get_type();
          if (!tmp.converts_to(rhs->get_type()->subtype)) {
            error(
              "cannot convert type '%s' to '%s' in 'cons'\n",
              lhs, type_to_str(lhs->get_type()).c_str(), type_to_str(rhs->get_type()->subtype).c_str()
            );
            return false;
          }
          op->set_type(new type_info(*rhs->get_type()));
          return true;
        }
      default:
        debug("operator\n", op);
        internal_compiler_error("operator is not a binary operator\n");
        return false;
    }
    return true;
  }

  bool visit_node(node * const n) const override
  {
    /* root node */
    n->mark_visiting();
    if (n->is_root()) {
      /* create the global scope */
      n->set_scope(std::make_shared<scope>());
    } else {
      internal_compiler_error("visit_node called for non-root node: '%s'\n", n->get_fqn().c_str());
      return false;
    }
    bool ret = visit_children(n);
    n->mark_visited();
    return ret;
  }

  bool visit_function_body(function_body * const body) const override
  {
    body->set_scope(body->get_parent()->get_scope());
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
    function_definition * resolved = nullptr;
    scope * scope = parent->get_scope().get();
    for (; scope && !scope_has_function(call_->get_name(), scope, &resolved);
      scope = scope->parent.get())
    {
      /* search for the requested function in this scope, or parent scopes */
    }
    if (!resolved) {
      error("undefined reference to function '%s'\n", call_, call_->get_name().c_str());
      return false;
    }
    call_->resolve(resolved);
    /* check arguments match */
    if (call_->get_children().size() < resolved->get_formals().size()) {
      error(
        "too few arguments for function '%s': got '%zd' expected '%zd'\n",
        call_, call_->get_name().c_str(), call_->get_children().size(),
        resolved->get_formals().size());
      return false;
    } else if (call_->get_children().size() > resolved->get_formals().size()) {
      error(
        "too many arguments for function '%s': got '%zd' expected '%zd'\n",
        call_, call_->get_name().c_str(), call_->get_children().size(),
        resolved->get_formals().size());
      return false;
    }
    /* check types on arguments */
    for (std::size_t x = 0; x < resolved->get_formals().size(); ++x) {
      if (!call_->get_children()[x]->accept(this)) {
        return false;
      }
      if (!call_->get_children()[x]->get_type()->converts_to(resolved->get_formals()[x]->get_type()))
      {
        error(
          "invalid argument passed to function '%s': got '%s' expected '%s'\n",
          call_->get_children()[x], call_->get_name().c_str(),
          type_to_str(call_->get_children()[x]->get_type()).c_str(),
          type_to_str(resolved->get_formals()[x]->get_type()).c_str());
        return false;
      }
    }
    /* check for recursion */
    if (!resolved->visiting()) {
      call_->set_type(new type_info(*resolved->get_type()));
      return true;
    }
    /* confirm this is recursive */
    if (!resolved->is_anscestor(call_)) {
      internal_compiler_error("visiting function in a non-recursive context\n");
      return false;
    }
    /**
     * in order to resolve the type, we crawl back up and find a branch that
     * resolves to a concrete type.
     */
    if_expr * pif = nullptr;
    for (node * n = call_; nullptr != n && !n->is_function_body(); n = n->get_parent()) {
      if (n->is_if_expr()) {
        pif = n->as_if_expr();
      }
    }
    if (nullptr == pif) {
      error("detected recursive call without any if statements\n", call_);
      return false;
    }
    /* our result depends on the other branch */
    if (pif->get_affirmative()->is_anscestor(call_)) {
      /* visit the else branch first, and take that branch's value */
      if (nullptr != pif->get_else()->get_type()) {
        call_->set_type(new type_info(*pif->get_else()->get_type()));
        return true;
      } else if (pif->get_else()->visiting()) {
        error("no type resolution for either branch in recursive call\n", pif);
        return false;
      } else if (!visit(pif->get_else())) {
        return false;
      }
      call_->set_type(new type_info(*pif->get_else()->get_type()));
      return true;
    }
    /* visit the affirmative branch first, and take that value. */
    if (nullptr != pif->get_affirmative()->get_type()) {
      call_->set_type(new type_info(*pif->get_affirmative()->get_type()));
      return true;
    } else if (pif->get_affirmative()->visiting()) {
      error("no type resolution for either branch in recursive call\n", pif);
      return false;
    } else if (!visit(pif->get_affirmative())) {
      return false;
    }
    call_->set_type(new type_info(*pif->get_else()->get_type()));
    return true;
  }

  bool visit_extern_function(extern_function * const func_) const override
  {
    auto * parent = func_->get_parent();
    const auto & p_scope = parent->get_scope();
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(func_->get_name(), parent->get_scope().get(), &f_conflict)) {
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
    /* add this function to the parent's scope */
    parent->get_scope()->functions.emplace_back(func_);
    return true;
  }

  bool visit_function_definition(function_definition * const func_) const override
  {
    auto * parent = func_->get_parent();
    const auto & p_scope = parent->get_scope();
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(func_->get_name(), parent->get_scope().get(), &f_conflict)) {
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
    /* add this function to the parent's scope */
    parent->get_scope()->functions.emplace_back(func_);
    /* create new scope under the parent scope */
    func_->set_scope(std::make_shared<scope>());
    debug("create scope for function: %p\n", func_, func_->get_scope().get());
    func_->get_scope()->parent = p_scope;
    /* visit children */
    if (!visit_children(func_)) {
      return false;
    }
    /* set type to the last expression's type */
    expression * ret = func_->get_body()->get_return_expression();
    if (nullptr == ret) {
      /* no return expression for this function, throw an error */
      internal_compiler_error("missing return expression for function\n");
      return false;
    }
    func_->set_type(new type_info(*ret->get_type()));
    return true;
  }

  bool visit_if_expr(if_expr * const if_stmt) const override
  {
    /* verify we have three children */
    if (if_stmt->get_children().size() != 3) {
      internal_compiler_error(
        "too many children ('%zd') processing if statement", if_stmt->get_children().size());
    }
    /* verify each child is an expression */
    for (node * const child : if_stmt->get_children()) {
      if (!child->is_expression()) {
        auto * loc = child->get_location();
        error("expected expression on line %d column %d\n", if_stmt, loc->line, loc->column);
      }
    }
    /* visit the children */
    if (!visit_children(if_stmt)) {
      return false;
    }
    /* check the condition evaluates to a bool */
    type_info bool_t;
    bool_t.type = type_id::BOOL;
    if (!if_stmt->get_condition()->get_type()->converts_to(&bool_t)) {
      error("expression does not evaluate to a boolean\n", if_stmt->get_condition());
      return false;
    }
    /* check that both results evaluate to the same type */
    auto * affirmative_t = if_stmt->get_affirmative()->get_type();
    auto * else_t = if_stmt->get_else()->get_type();
    if (!else_t->converts_to(affirmative_t)) {
      error(
        "type of else expression ('%s') does not convert to expected type '%s'\n",
        if_stmt->get_else(), type_to_str(else_t).c_str(), type_to_str(affirmative_t).c_str());
      return false;
    }
    if_stmt->set_type(new type_info(*affirmative_t));
    return true;
  }

  bool visit_variable_definition(variable_definition * const var_) const override
  {
    variable_definition & var = *var_;
    auto * parent = var.get_parent();
    if (nullptr == parent) {
      internal_compiler_error("parent is null visiting variable definition");
      return false;
    }
    for (; nullptr == parent->get_scope(); parent = parent->get_parent()) {
      /* crawl up the tree to find the first parent with a scope available */
    }
    var.set_scope(parent->get_scope());
    /* check scope for redefinition */
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(var.get_name(), parent->get_scope().get(), &f_conflict)) {
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
    if (!visit_children(&var)) {
      /* visit children */
      return false;
    }
    /* take type from first child */
    if (!var_->is_formal() && !var_->get_children().empty()) {
      var_->set_type(new type_info(*var_->get_children()[0]->get_type()));
    }
    return true;
  }

  bool visit_formal(formal * const var) const override
  {
    auto * parent = var->get_parent();
    if (nullptr == parent) {
      internal_compiler_error("parent is null visiting formal");
      return false;
    } else if (!parent->is_function_definition()) {
      internal_compiler_error("parent is not a function definition visiting formal");
    }
    /* check scope for redefinition */
    function_definition * f_conflict;
    variable_definition * v_conflict;
    if (scope_has_function(var->get_name(), parent->get_scope().get(), &f_conflict)) {
      location_info & loc = *f_conflict->get_location();
      error(
        "conflicting definition for parameter '%s' (original on line %d column %d): %s\n",
        var,
        var->get_name().c_str(),
        loc.line, loc.column, loc.text.c_str());
      return false;
    } else if (scope_has_variable(var->get_name(), parent->get_scope(), &v_conflict)) {
      location_info & loc = *v_conflict->get_location();
      error(
        "conflicting definition for parameter '%s' (original on line %d column %d): %s\n",
        var,
        var->get_name().c_str(),
        loc.line, loc.column);
      return false;
    }
    /* otherwise, append the formal to the function's scope */
    parent->get_scope()->variables.push_back(var);
    debug(
      "adding definition for formal '%s' (type: '%s')\n", var,
      var->get_name().c_str(), type_to_str(var->get_type()).c_str());
    return true;
  }

  bool visit_list_op(list_op * const op) const override
  {
    /* child (singular) should be a list */
    if (op->get_children().size() > 1) {
      internal_compiler_error(
        "too many children (%zd) for list operation\n", op,
        op->get_children().size());
      return false;
    } else if (nullptr == dynamic_cast<list *>(op->get_children()[0])) {
      error("invalid arguments for list operation\n", op);
      return false;
    }
    /* traverse through our children */
    bool ret = visit_children(op);
    if (!ret) {
      /* error already set */
      return false;
    }
    /* get the type of the head */
    list * const list_ = dynamic_cast<list *>(op->get_children()[0]);
    type_info * list_t = list_->get_type();
    if (nullptr == list_t->subtype) {
      internal_compiler_error("unresolved subtype for list '%s'\n", list_->get_fqn().c_str());
      return false;
    }
    switch (op->get_op()) {
      case op_id::PLUS:
        if (list_t->subtype->type == type_id::INT ||
          list_t->subtype->type == type_id::FLOAT ||
          list_t->subtype->type == type_id::BOOL ||
          list_t->subtype->type == type_id::STRING ||
          list_t->subtype->type == type_id::LIST)
        {
          op->set_type(new type_info(*list_t->subtype));
          return true;
        }
        error("invalid operands for list operator '%s'\n", op, op_to_str(op->get_op()).c_str());
        return false;
      case op_id::MINUS:
      case op_id::TIMES:
      case op_id::DIVIDE:
        if (list_t->subtype->type == type_id::INT ||
          list_t->subtype->type == type_id::FLOAT)
        {
          op->set_type(new type_info(*list_t->subtype));
          return true;
        }
        error(
          "invalid operands for list operator '%s': expected list, but got '%s'\n",
          op, op_to_str(op->get_op()).c_str(), type_to_str(list_t).c_str());
        return false;
      case op_id::OR:
      case op_id::AND:
      case op_id::XOR:
      case op_id::NOT:
        if (list_t->subtype->type == type_id::INT ||
          list_t->subtype->type == type_id::FLOAT ||
          list_t->subtype->type == type_id::BOOL ||
          list_t->subtype->type == type_id::STRING ||
          list_t->subtype->type == type_id::LIST)
        {
          op->set_type(type_id::BOOL);
          return true;
        }
        error("invalid operands for list operator '%s'\n", op, op_to_str(op->get_op()).c_str());
        return false;
      case op_id::PRINT:
        /* check the first argument is a string */
        /* int because printf returns an int */
        op->set_type(type_id::INT);
        return true;
      default:
        debug("invalid operation\n", op);
        internal_compiler_error("operator is not a list operator\n");
        return false;
    }
    return ret;
  }

  bool visit_list(list * const _list) const override
  {
    if (!visit_children(_list)) {
      return false;
    }
    type_info * subtype = nullptr;
    if (nullptr != _list->get_type()->subtype) {
      /* expliticly labled */
      subtype = _list->get_type()->subtype;
    } else {
      /* derived from first element */
      _list->get_type()->subtype = new type_info(*_list->get_head()->get_type());
      subtype = _list->get_type()->subtype;
    }
    /* check all list types are compatible */
    if (!_list->get_head()->get_type()->converts_to(subtype)) {
      error(
        "child type '%s' is incompatible with list of type '%s'\n",
        _list->get_head(),
        type_to_str(_list->get_head()->get_type()).c_str(),
        type_to_str(_list->get_type()).c_str());
      return false;
    }
    for (list * iter = _list->get_tail(); iter != nullptr; iter = iter->get_tail()) {
      if (!iter->get_head()->get_type()->converts_to(subtype)) {
        error(
          "child type '%s' is incompatible with list of type '%s'\n",
          iter->get_head(),
          type_to_str(iter->get_head()->get_type()).c_str(),
          type_to_str(_list->get_type()).c_str());
        return false;
      }
    }
    return true;
  }

  bool visit_literal(literal * const l) const override
  {
    l->set_name(type_to_str(l->get_type()));
    return true;
  }

  bool visit_simple_expression(simple_expression * const s) const override
  {
    return visit_children(s);
  }

  bool visit_unary_op(unary_op * const op) const override
  {
    if (!visit_children(op)) {
      return false;
    } else if (op->get_children().size() > 1) {
      error("too many operands for unary operator\n", op);
      return false;
    }
    type_info & child_t = *op->get_children()[0]->get_type();
    if (op->get_op() == op_id::NOT) {
      if (child_t.type == type_id::INVALID ||
        child_t.type == type_id::VARIABLE)
      {
        internal_compiler_error("unresolved type for not operator\n");
        return false;
      }
      op->set_type(type_id::BOOL);
      return true;
    } else if (op->get_op() == op_id::CAR) {
      if (op->get_children()[0]->get_type()->type != type_id::LIST) {
        error(
          "attempted car operation on non-list type '%s'\n",
          op, type_to_str(op->get_children()[0]->get_type()).c_str());
        return false;
      }
      op->set_type(new type_info(*op->get_children()[0]->get_type()->subtype));
      return true;
    } else if (op->get_op() == op_id::CDR) {
      if (op->get_children()[0]->get_type()->type != type_id::LIST) {
        error(
          "attempted cdr operation on non-list type '%s'\n",
          op, type_to_str(op->get_children()[0]->get_type()).c_str());
        return false;
      }
      op->set_type(new type_info(*op->get_children()[0]->get_type()));
      return true;
    }
    internal_compiler_error("invalid unary operator '%s'", op_to_str(op->get_op()).c_str());
    return false;
  }

  bool visit_variable(variable * const var) const override
  {
    /* set up this variable's scope */
    node * parent;
    for (parent = var->get_parent(); parent && (parent->get_scope() == nullptr); ) {
      parent = parent->get_parent();
    }
    if (nullptr == parent) {
      /* made it to root, this should not happen */
      internal_compiler_error(
        "traversed to root node before finding a scope to lookup variable '%s'\n", var->get_fqn());
      return false;
    }
    variable_definition * resolved = nullptr;
    auto scope = parent->get_scope();
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
