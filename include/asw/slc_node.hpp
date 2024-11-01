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

#ifndef ASW__SLC_NODE_HPP_
#define ASW__SLC_NODE_HPP_

#include <concepts>
#include <functional>
#include <memory>
#include <variant>
#include <vector>
#include <string>

#include <asw/location_info.hpp>
#include <asw/scope.hpp>
#include <asw/type_info.hpp>
#include <asw/visitor.hpp>
#include <asw/llvm_codegen.hpp>

/* add helper functions for casting */
#define utilities(type) \
  bool is_ ## type(); \
  type * as_ ## type();

namespace asw::slc
{

enum class visiting_state
{
  NOT_VISITED,
  VISITING,
  VISITED,
};

enum class op_id
{
  PLUS,
  MINUS,
  TIMES,
  DIVIDE,
  GREATER,
  GREATER_EQ,
  LESS,
  LESS_EQ,
  EQUAL,
  NOT,
  OR,
  AND,
  XOR,
  CAR,
  CDR,
  CONS,
  PRINT,
  INVALID,
};

inline std::string op_to_str(op_id id)
{
  using namespace std::string_literals;
  switch (id) {
    case op_id::PLUS:
      return "+"s;
    case op_id::MINUS:
      return "-"s;
    case op_id::TIMES:
      return "'*'"s;
    case op_id::DIVIDE:
      return "/"s;
    case op_id::GREATER:
      return "'>'"s;
    case op_id::GREATER_EQ:
      return ">="s;
    case op_id::LESS:
      return "<"s;
    case op_id::LESS_EQ:
      return "<="s;
    case op_id::EQUAL:
      return "="s;
    case op_id::NOT:
      return "not"s;
    case op_id::OR:
      return "or"s;
    case op_id::AND:
      return "and"s;
    case op_id::XOR:
      return "xor"s;
    case op_id::CAR:
      return "car"s;
    case op_id::CDR:
      return "cdr"s;
    case op_id::CONS:
      return "cons"s;
    case op_id::PRINT:
      return "print"s;
    case op_id::INVALID:
      return "invalid"s;
  }
  return "unknown_op"s;
}

struct node
{
  node() = default;
  virtual ~node()
  {
    delete location_;
    delete tid;
    for (auto * const child : children) {
      delete child;
    }
  }

  virtual bool accept(const visitor * v)
  {
    return v->visit_node(this);
  }

  virtual llvm::Value * accept(const llvm_visitor * v)
  {
    return v->visit_node(this);
  }
  /**
   * returns a path to the root node
   */
  std::vector<std::string> get_path_from_root() const
  {
    std::vector<std::string> ret;
    if (this->is_root()) {
      return {name};
    }
    for (node * parent = get_parent(); !parent->is_root(); parent = parent->get_parent()) {
      ret.insert(std::begin(ret), parent->get_name());
    }
    return ret;
  }

  std::string get_fqn(const std::string & delim = "::") const
  {
    if (fqn.empty()) {
      auto vec{get_path_from_root()};
      for (const auto & n : vec) {
        fqn += delim + n;
      }
    }
    return fqn + delim + this->get_name();
  }

  const std::string & get_name() const
  {
    return name;
  }

  void set_name(const std::string & _name)
  {
    name = _name;
  }

  node * get_parent() const
  {
    return parent_;
  }

  const std::vector<node *> & get_children() const
  {
    return children;
  }

  bool is_root() const
  {
    /* this is not a great check, but it's sufficient for now */
    return nullptr == parent_ && name == "Root";
  }

  void add_child(node * child)
  {
    child->parent_ = this;
    children.emplace_back(child);
  }

  void remove_child(node * child, bool free = true)
  {
    size_t x = 0;
    for (; x < children.size(); ++x) {
      if (child == children[x]) {
        if (free) {
          delete children[x];
        }
        children.erase(std::begin(children) + x);
        return;
      }
    }
  }

  void prepend_child(node * child)
  {
    child->parent_ = this;
    children.insert(std::begin(children), child);
  }

  std::string print() const
  {
    std::string text = "Root:\n";
    for (const auto & child : children) {
      if (nullptr != child) {
        text += child->print_node(1);
      }
    }
    return text;
  }

  std::string get_indent(size_t indent_level) const
  {
    std::string indent = "";
    for (size_t x = indent_level; x-- > 0; ) {
      indent += "  ";
    }
    return (indent_level >= 1) ? (indent + "- ") : (indent);
  }

  virtual std::string print_node(size_t indent_level) const
  {
    std::string ret = get_indent(indent_level) + name + ":\n";
    for (const auto * child : this->get_children()) {
      child->print_node(indent_level + 1);
    }
    return ret;
  }

  void set_scope(std::shared_ptr<scope> _scope)
  {
    scope_ = std::move(_scope);
#ifdef DEBUG
    fprintf(
      stderr, "scope for '%s' set to %p\n",
      this->get_name().c_str(), scope_.get());
#endif
  }

  std::shared_ptr<scope> & get_scope()
  {
    return scope_;
  }

  void set_location(int line, int col, const char * text)
  {
    location_ = new location_info(line, col, text);
  }

  location_info * get_location() const
  {
    if (nullptr == location_) {
      fprintf(
        stderr, "internal compiler error: location unset for node '%s'\n",
        this->get_fqn().c_str());
    }
    return location_;
  }

  void set_type(type_info * id)
  {
    delete this->tid;
    this->tid = id;
  }

  void set_type(type_id id)
  {
    if (this->tid) {
      delete tid;
    }
    this->tid = new type_info;
    tid->type = id;
  }

  type_info * get_type() const
  {
    return this->tid;
  }

  void mark_visiting() const
  {
    this->visit_state = visiting_state::VISITING;
  }

  void mark_visited() const
  {
    this->visit_state = visiting_state::VISITED;
  }

  bool visiting() const
  {
    return this->visit_state == visiting_state::VISITING;
  }

  bool visited() const
  {
    return this->visit_state == visiting_state::VISITED;
  }

  bool is_descendent(node * other) const
  {
    if (is_root()) {
      return other == this;
    }
    for (node * n = parent_; !n->is_root(); n = n->parent_) {
      if (n == other) {
        return true;
      }
    }
    return false;
  }

  bool is_anscestor(node * other) const
  {
    if (is_root()) {
      return other == this;
    }
    for (node * n = other; !n->is_root(); n = n->parent_) {
      if (n == this) {
        return true;
      }
    }
    return false;
  }

  utilities(binary_op)
  utilities(expression)
  utilities(extern_function)
  utilities(list_op)
  utilities(unary_op)
  utilities(if_expr)
  utilities(lambda)
  utilities(list)
  utilities(literal)
  utilities(formal)
  utilities(function_call)
  utilities(function_body)
  utilities(variable_definition)
  utilities(function_definition)

protected:
  node * parent_ = nullptr;
  location_info * location_ = nullptr;
  type_info * tid = nullptr;
  std::shared_ptr<scope> scope_ = nullptr;

  std::vector<node *> children{};
  std::string name = "Root";
  mutable visiting_state visit_state = visiting_state::NOT_VISITED;
  mutable std::string fqn{};
};

struct expression : public node
{
  ~expression() override = default;
};

struct definition : public node
{
  ~definition() override = default;
};

struct simple_expression : public expression
{
  ~simple_expression() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_simple_expression(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_simple_expression(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + type_to_str(get_type()) + ":\n";
    for (const auto & child : this->children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

protected:
  /* name */
  /* child */
};

struct literal : public simple_expression
{
  ~literal() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_literal(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_literal(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent;
    std::visit(
      [&ret](const auto & arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same<T, std::string>::value) {
          ret += arg;
        } else {
          ret += std::to_string(arg);
        }
      }, this->value);
    /* this is a terminal node, so we have no children */
    return ret + "\n";
  }

  template<class T>
  void set_value(const T & val) requires(
    std::is_same_v<T, int>||
    std::is_same_v<T, double>||
    std::is_same_v<T, std::string>)
  {
    this->value = val;
  }

  int get_int() const
  {
    return std::get<0>(value);
  }

  double get_double() const
  {
    return std::get<1>(value);
  }

  std::string get_str() const
  {
    return std::get<2>(value);
  }

  const std::variant<int, double, std::string> & get_value()
  {
    return value;
  }

protected:
  std::variant<int, double, std::string> value;
};

struct variable : public simple_expression
{
  ~variable() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_variable(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_variable(this);
  }

  void resolve(definition * def)
  {
    /* set pointer to the variable or function this node references */
    resolved_definition = def;
    type_info * tid = new type_info();
    *tid = *def->get_type();
    this->set_type(tid);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + this->get_name() + "\n";
    return ret;
  }

  bool is_resolved() const
  {
    return nullptr != resolved_definition;
  }

  definition * get_resolution() const
  {
    return resolved_definition;
  }

protected:
  definition * resolved_definition = nullptr;
  /* referenced var stored in name */
};

struct binary_op : public expression
{
  ~binary_op() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_binary_op(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_binary_op(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + op_to_str(op) + ":\n";
    for (const auto & child : this->children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  void set_op(op_id id)
  {
    this->op = id;
  }

  op_id get_op() const
  {
    return op;
  }

protected:
  op_id op = op_id::INVALID;
  /* name */
  /* children (lhs, rhs) */
};

struct list_op : public expression
{
  ~list_op() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_list_op(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_list_op(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + op_to_str(oid) + ":\n";
    for (const auto & child : this->children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  void set_op(op_id id)
  {
    this->oid = id;
  }

  op_id get_op() const
  {
    return this->oid;
  }

protected:
  op_id oid = op_id::INVALID;
  /* name */
  /* children: list */
};

struct unary_op : public expression
{
  ~unary_op() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_unary_op(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_unary_op(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + op_to_str(op) + ":\n";
    for (const auto & child : this->children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  void set_op(op_id id)
  {
    this->op = id;
  }

  op_id get_op() const
  {
    return op;
  }

protected:
  op_id op = op_id::INVALID;
  /* name */
  /* children: list */
};

struct if_expr : public expression
{
  ~if_expr() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_if_expr(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_if_expr(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + "if:\n";
    for (const auto & child : this->children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  expression * get_condition() const
  {
    return get_children()[0]->as_expression();
  }

  expression * get_affirmative() const
  {
    return get_children()[1]->as_expression();
  }

  expression * get_else() const
  {
    return get_children()[2]->as_expression();
  }

protected:
  /* name */
  /* children (condition, expression, expression) */
};

struct list : public expression
{
  ~list() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_list(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_list(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + "list:\n";
    if (head) {
      ret += head->print_node(indent_level + 1);
    }
    if (tail) {
      ret += tail->print_node(indent_level + 1);
    } else {
      ret += get_indent(indent_level + 1) + "~\n";
    }
    return ret;
  }

  void set_head(expression * h)
  {
    if (nullptr != h) {
      this->add_child(h);
    }
    head = h;
  }

  void set_tail(list * t)
  {
    this->add_child(t);
    tail = t;
  }

  expression * get_head() const
  {
    return head;
  }

  list * get_tail() const
  {
    return tail;
  }

  list ** get_iter()
  {
    return &tail;
  }

protected:
  /* first element in the list */
  expression * head = nullptr;
  /* rest of the list */
  list * tail = nullptr;
};

using formals = std::vector<formal *>;

struct callable
{
  virtual void set_body(function_body * body) = 0;
  virtual function_body * get_body() const = 0;
  virtual void set_formals(formals * list) = 0;
  virtual formals get_formals() const = 0;
};

struct function_call : public expression
{
  ~function_call() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_function_call(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_function_call(this);
  }

  callable * get_resolution() const
  {
    return resolved_;
  }

  void resolve(callable * func) const
  {
    resolved_ = func;
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + this->name + ":\n";
    for (const auto * child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

protected:
  mutable callable * resolved_ = nullptr;
  /* name: function to call */
  /* children: arguments */
};

struct function_body : public node
{
  ~function_body() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_function_body(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_function_body(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + "function_body(" + this->get_fqn() + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  expression * get_return_expression() const
  {
    return return_expression;
  }

  void set_return_expression(expression * expr)
  {
    this->add_child(expr);
    return_expression = expr;
  }

protected:
  expression * return_expression = nullptr;
};

struct variable_definition : public definition
{
  ~variable_definition() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + "variable_definition(" + this->get_fqn() + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  bool accept(const visitor * v) override
  {
    return v->visit_variable_definition(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_variable_definition(this);
  }

protected:
  /* name */
  /* value (expression) stored as child */
};

struct formal : public variable_definition
{
  ~formal() override = default;

  bool accept(const visitor * v) override
  {
    return v->visit_formal(this);
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + "variable_definition(" + this->get_fqn() + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

protected:
  /* name */
};

struct function_definition : public definition, public callable
{
  ~function_definition() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + "function_definition(" + this->get_fqn() + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  bool accept(const visitor * v) override
  {
    return v->visit_function_definition(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_function_definition(this);
  }

  void set_body(function_body * body) final
  {
    this->add_child(body);
    this->impl = body;
  }

  function_body * get_body() const final
  {
    return impl;
  }

  void set_formals(formals * list) final
  {
    for (formal * f : *list) {
      this->add_child(f);
    }
    this->parameters = *list;
  }

  formals get_formals() const final
  {
    return parameters;
  }

protected:
  function_body * impl = nullptr;
  formals parameters;
  /* name */
  /* value (expression) stored as child */
};

struct lambda : public expression, public callable
{
  ~lambda() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + "lambda(" + this->get_fqn() + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  bool accept(const visitor * v) override
  {
    return v->visit_lambda(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_lambda(this);
  }

  void set_body(function_body * body) final
  {
    this->add_child(body);
    this->impl = body;
  }

  function_body * get_body() const final
  {
    return impl;
  }

  void set_formals(formals * list) final
  {
    for (formal * f : *list) {
      this->add_child(f);
    }
    this->parameters = *list;
  }

  formals get_formals() const final
  {
    return parameters;
  }

protected:
  function_body * impl = nullptr;
  formals parameters;
  /* name */
  /* value (expression) stored as child */
};

struct extern_function : public function_definition
{
  ~extern_function() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + "extern_function(" + this->get_fqn() + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  bool accept(const visitor * v) override
  {
    return v->visit_extern_function(this);
  }

  llvm::Value * accept(const llvm_visitor * v) override
  {
    return v->visit_extern_function(this);
  }
};

} // namespace asw::slc
#endif  // ASW__SLC_NODE_HPP_
