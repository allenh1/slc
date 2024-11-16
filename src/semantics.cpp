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

#include <asw/semantics.hpp>

namespace asw::slc
{

SemanticAnalyzer & SemanticAnalyzer::get_instance()
{
  if (nullptr == impl) {
    impl = new SemanticAnalyzer();
  }
  return *impl;
}

bool SemanticAnalyzer::visit(node * const n) const
{
  if (n->visited()) {
    return true;
  }
  n->mark_visiting();
  bool ret = n->accept(this);
  n->mark_visited();
  return ret;
}

bool SemanticAnalyzer::visit_children(node * const n) const
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


bool SemanticAnalyzer::visit_binary_op(binary_op * const op) const
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
  auto is_nil = [](expression * expr) -> bool {return expr->get_type()->type == type_id::NIL;};
  switch (op->get_op()) {
    case op_id::GREATER:
    case op_id::GREATER_EQ:
    case op_id::LESS:
    case op_id::LESS_EQ:
    case op_id::EQUAL:
      {
        if (((is_int(lhs) && is_float(rhs)) || (is_int(rhs) && is_float(lhs)))) {
          op->set_type(type_id::BOOL);
          return true;
        } else if ((is_nil(lhs) && is_list(rhs)) || (is_nil(rhs) && is_list(lhs))) {
          op->set_type(type_id::BOOL);
          return true;
        } else if (lhs->get_type()->type == rhs->get_type()->type) {
          op->set_type(type_id::BOOL);
          return true;
        }
        error(
          "invalid operands for binary operator '%s'\n", op,
          op_to_str(op->get_op()).c_str());
        return false;
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

bool SemanticAnalyzer::visit_node(node * const n) const
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

bool SemanticAnalyzer::visit_function_body(function_body * const body) const
{
  body->set_scope(body->get_parent()->get_scope());
  return visit_children(body);
}

bool SemanticAnalyzer::visit_function_call(function_call * const call_) const
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
  definition * resolved_ = nullptr;
  scope * scope = parent->get_scope().get();
  for (; scope && !scope_has_definition(call_->get_name(), scope, &resolved_);
    scope = scope->parent.get())
  {
    /* search for the requested function in this scope, or parent scopes */
  }
  if (!resolved_) {
    error("undefined reference to function '%s'\n", call_, call_->get_name().c_str());
    return false;
  }

  callable * resolved = nullptr;
  if (resolved_->is_variable_definition()) {
    if (!resolved_->get_children()[0]->is_lambda()) {
      /* expected a lambda */
      error("attempted to call a variable as a function\n", call_);
      return false;
    }
    resolved = resolved_->get_children()[0]->as_lambda();
  } else {
    resolved = resolved_->as_function_definition();
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
  if (!resolved_->visiting()) {
    call_->set_type(new type_info(*resolved_->get_type()));
    return true;
  }
  /* confirm this is recursive */
  if (!resolved_->is_anscestor(call_)) {
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
    if (pif->get_else()->visited()) {
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
  call_->set_type(new type_info(*pif->get_affirmative()->get_type()));
  return true;
}

bool SemanticAnalyzer::visit_extern_function(extern_function * const func_) const
{
  auto * parent = func_->get_parent();
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
  } else if (scope_has_variable(func_->get_name(), parent->get_scope().get(), &v_conflict)) {
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

bool SemanticAnalyzer::visit_function_definition(function_definition * const func_) const
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
  } else if (scope_has_variable(func_->get_name(), parent->get_scope().get(), &v_conflict)) {
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

bool SemanticAnalyzer::visit_if_expr(if_expr * const if_stmt) const
{
  auto * parent = if_stmt->get_parent();
  const auto & p_scope = parent->get_scope();
  if_stmt->set_scope(std::make_shared<scope>());
  if_stmt->get_scope()->parent = p_scope;
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

bool SemanticAnalyzer::visit_iterator_definition(iterator_definition * const iter) const
{
  if (!visit_children(iter)) {
    return false;
  }
  if (iter->get_children()[0]->get_type()->type != type_id::LIST) {
    error(
      "cannot iterate over type '%s'\n",
      iter->get_children()[0],
      type_to_str(iter->get_children()[0]->get_type()).c_str());
    return false;
  }
  if (nullptr == iter->get_children()[0]->as_expression()) {
    error(
      "expected an expression for list, but node is not an expression\n",
      iter->get_children()[0]
    );
    return false;
  }
  iter->set_list(iter->get_children()[0]->as_expression());
  /* resolve to the type inside the list */
  iter->set_type(new type_info(*iter->get_children()[0]->get_type()->subtype));
  auto * parent = iter->get_parent();
  iter->set_scope(parent->get_scope());
  /* check scope for redefinition */
  function_definition * f_conflict;
  variable_definition * v_conflict;
  if (scope_has_function(iter->get_name(), parent->get_scope().get(), &f_conflict)) {
    location_info & loc = *f_conflict->get_location();
    error(
      "conflicting definition for variable '%s' (original on line %d column %d): %s\n",
      iter,
      iter->get_name().c_str(),
      loc.line, loc.column, loc.text.c_str());
    return false;
  } else if (scope_has_variable(iter->get_name(), parent->get_scope().get(), &v_conflict)) {
    location_info & loc = *v_conflict->get_location();
    error(
      "conflicting definition for variable '%s' (original on line %d column %d): %s\n",
      iter,
      iter->get_name().c_str(),
      loc.line, loc.column);
    return false;
  }
  /* otherwise, append the variable to the scope */
  iter->get_scope()->variables.push_back(iter);
  return true;
}

bool SemanticAnalyzer::visit_variable_definition(variable_definition * const var_) const
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
  } else if (scope_has_variable(var.get_name(), parent->get_scope().get(), &v_conflict)) {
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

bool SemanticAnalyzer::visit_formal(formal * const var) const
{
  auto * parent = var->get_parent();
  if (nullptr == parent) {
    internal_compiler_error("parent is null visiting formal");
    return false;
  } else if (!parent->is_function_definition() && !parent->is_lambda()) {
    internal_compiler_error("parent is not a function definition or lambda visiting formal");
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
  } else if (scope_has_variable(var->get_name(), parent->get_scope().get(), &v_conflict)) {
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
  return true;
}

bool SemanticAnalyzer::visit_lambda(lambda * const lambda) const
{
  auto * parent = lambda->get_parent();
  for (; parent->get_scope() == nullptr; parent = parent->get_parent()) {
    /* find the first non-null scope */
  }
  const auto & p_scope = parent->get_scope();
  /* create new scope under the parent scope */
  lambda->set_scope(std::make_shared<scope>());
  lambda->get_scope()->parent = p_scope;
  /* visit children */
  if (!visit_children(lambda)) {
    return false;
  }
  /* set type to the last expression's type */
  expression * ret = lambda->get_body()->get_return_expression();
  if (nullptr == ret) {
    /* no return expression for this lambda, throw an error */
    internal_compiler_error("missing return expression for lambda\n");
    return false;
  }
  lambda->set_type(new type_info(*ret->get_type()));
  return true;
}

bool SemanticAnalyzer::visit_list_op(list_op * const op) const
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

bool SemanticAnalyzer::visit_list(list * const _list) const
{
  if (!visit_children(_list)) {
    return false;
  }
  type_info * subtype = nullptr;
  if (nullptr != _list->get_type()->subtype) {
    /* expliticly labled */
    subtype = _list->get_type()->subtype;
  } else {
    subtype = _list->get_type()->subtype;
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

bool SemanticAnalyzer::visit_literal(literal * const l) const
{
  l->set_name(type_to_str(l->get_type()));
  return true;
}

bool SemanticAnalyzer::visit_do_loop(do_loop * const _loop) const
{
  auto * parent = _loop->get_parent();
  const auto & p_scope = parent->get_scope();
  /* create new scope under the parent scope */
  _loop->set_scope(std::make_shared<scope>());
  _loop->get_scope()->parent = p_scope;
  if (!visit_children(_loop)) {
    return false;
  }
  _loop->set_type(new type_info(*_loop->get_loop_body()->get_return_expression()->get_type()));
  return true;
}

bool SemanticAnalyzer::visit_collect_loop(collect_loop * const _loop) const
{
  auto * parent = _loop->get_parent();
  const auto & p_scope = parent->get_scope();
  /* create new scope under the parent scope */
  _loop->set_scope(std::make_shared<scope>());
  _loop->get_scope()->parent = p_scope;
  if (!visit_children(_loop)) {
    return false;
  }
  type_info * subtype = new type_info(*_loop->get_loop_body()->get_return_expression()->get_type());
  type_info * type = new type_info;
  type->type = type_id::LIST;
  type->subtype = subtype;
  _loop->set_type(type);
  return true;
}

bool SemanticAnalyzer::visit_when_loop(when_loop * const _loop) const
{
  return visit_children(_loop);
}

bool SemanticAnalyzer::visit_infinite_loop(infinite_loop * const _loop) const
{
  return visit_children(_loop);
}

bool SemanticAnalyzer::visit_set_expression(set_expression * const s) const
{
  if (!visit_children(s)) {
    return false;
  }
  /* set up this variable's scope */
  node * parent;
  for (parent = s->get_parent(); parent && (parent->get_scope() == nullptr); ) {
    parent = parent->get_parent();
  }
  if (nullptr == parent) {
    /* made it to root, this should not happen */
    internal_compiler_error(
      "traversed to root node before finding a scope to lookup variable '%s'\n", s->get_fqn());
    return false;
  }
  variable_definition * resolved = nullptr;
  auto scope = parent->get_scope();
  for (; scope && !scope_has_variable(s->get_name(), scope.get(), &resolved);
    scope = scope->parent)
  {
    /* search for the requested variable in this scope, or parent scopes */
  }
  if (!resolved) {
    error("undefined reference to variable '%s'\n", s, s->get_name().c_str());
    return false;
  }
  s->set_type(new type_info(*resolved->get_type()));
  s->resolve(resolved);
  return true;
}

bool SemanticAnalyzer::visit_simple_expression(simple_expression * const s) const
{
  if (!visit_children(s)) {
    return false;
  }
  return true;
}

bool SemanticAnalyzer::visit_unary_op(unary_op * const op) const
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

bool SemanticAnalyzer::visit_variable(variable * const var) const
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
  for (; scope && !scope_has_variable(var->get_name(), scope.get(), &resolved);
    scope = scope->parent)
  {
  }
  if (!resolved) {
    error("undefined reference to variable '%s'\n", var, var->get_name().c_str());
    return false;
  }
  var->resolve(resolved);
  return true;
}

}  // namespace asw::slc
