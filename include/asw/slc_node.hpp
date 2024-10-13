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

namespace asw::slc
{
enum class type_id
{
  INT,
  FLOAT,
  STRING,
  BOOL,
  LAMBDA,
  LIST,
  VARIABLE,
  NIL,
};

inline std::string type_to_str(type_id id)
{
  using namespace std::string_literals;
  switch(id)
  {
  case type_id::INT:
    return "int"s;
  case type_id::FLOAT:
    return "float"s;
  case type_id::STRING:
    return "string"s;
  case type_id::BOOL:
    return "bool"s;
  case type_id::LAMBDA:
    return "lambda"s;
  case type_id::LIST:
    return "list"s;
  case type_id::VARIABLE:
    return "variable"s;
  case type_id::NIL:
    return "nil"s;
  }
  return "unknown_type"s;
}

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
  INVALID,
};

inline std::string op_to_str(op_id id)
{
  using namespace std::string_literals;
  switch(id)
  {
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
    for (auto * const child : children) {
      delete child;
    }
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
    return parent;
  }

  const std::vector<node *> & get_children() const
  {
    return children;
  }

  bool is_root() const
  {
    /* this is not a great check, but it's sufficient for now */
    return nullptr == parent && name == "Root";
  }

  void add_child(node * child)
  {
    fprintf(stderr, "add_child(%p)\n", child);
    child->parent = this;
    children.emplace_back(child);
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
    for (size_t x = indent_level; x-- > 0;) {
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

protected:
  node * parent = nullptr;
  std::vector<node *> children{};
  std::string name = "Root";
};

struct expression : public node {
  ~expression() override = default;

  // std::string print_node(size_t indent_level) const override
  // {
  //   std::string indent = get_indent(indent_level);
  //   std::string ret = indent + "expression:\n";
  //   for (const auto * child : this->get_children()) {
  //     ret += child->print_node(indent_level + 1);
  //   }
  //   std::string indent2 = get_indent(indent_level + 1);
  //   ret += indent2 + "name: " + this->name + "\n";
  //   return ret;
  // }
};

struct simple_expression : public expression
{
  ~simple_expression() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + type_to_str(id) + ":\n";
    for (const auto & child : this->children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  void set_tid(const type_id id)
  {
    this->id = id;
  }

protected:
  type_id id;
  /* name */
  /* child */
};

struct literal : public simple_expression
{
  ~literal() override = default;

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
  void set_value(const T & val) requires (
    std::is_same_v<T, int> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, std::string>) 
  {
    this->value = val;
  }

protected:
  std::variant<int, double, std::string> value;
};

struct variable : public simple_expression
{
  ~variable() override = default;

  void resolve(node * /* scope */)
  {
    /* find the referenced variable's definition in the scope */
  }

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + this->get_name() + "\n";
    return ret;
  }

  bool is_resolved() const
  {
    return nullptr != resolved_type;
  }

protected:
  node * resolved_type = nullptr;
  /* referenced var stored in name */
};

struct binary_op : public expression
{
  ~binary_op() override = default;

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

protected:
  op_id op = op_id::INVALID;
  /* name */
  /* children (lhs, rhs) */
};

struct list_op : public expression
{
  ~list_op() override = default;
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

protected:
  op_id op = op_id::INVALID;
  /* name */
  /* children: list */
};

struct unary_op : public expression
{
  ~unary_op() override = default;
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

protected:
  op_id op = op_id::INVALID;
  /* name */
  /* children: list */
};

struct if_expr : public expression
{
  ~if_expr() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + "if:\n";
    for (const auto & child : this->children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

protected:
  /* name */
  /* children (condition, expression, expression) */
};

struct list : public expression
{
  ~list() override = default;
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
    this->add_child(h);
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

struct formal : public node
{
  ~formal() override = default;
  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string indent2 = get_indent(indent_level + 1);
    std::string ret = indent + "formal:\n";
    ret += indent2 + "name: " + this->name + "\n";
    ret += indent2 + "type: " + type_to_str(this->type) + "\n";
    return ret;
  }

  void set_type(type_id id)
  {
    type = id;
  }

protected:
  /* name */
  type_id type;
};

struct function_call : public expression
{
  ~function_call() override = default;
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
  /* name: function to call */
  /* children: arguments */
};

struct definition : public node{};

struct function_body : public node
{
  ~function_body() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + "function_body(" + this->name + "):\n";
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

struct function_definition : public definition
{
  ~function_definition() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string ret = get_indent(indent_level) + "function_definition(" + this->name + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

  void set_body(function_body * body)
  {
    this->add_child(body);
    this->impl = body;
  }

  function_body * get_body() const
  {
    return impl;
  }

  void set_argument_list(variable * list)
  {
    this->add_child(list);
    this->argument_list = list;
  }

  variable * get_argument_list() const
  {
    return argument_list;
  }

protected:
  function_body * impl = nullptr;
  variable * argument_list = nullptr;
  /* name */
  /* value (expression) stored as child */  
};

struct variable_definition : public definition
{
  ~variable_definition() override = default;

  std::string print_node(size_t indent_level) const override
  {
    std::string indent = get_indent(indent_level);
    std::string ret = indent + "variable_definition(" + this->name + "):\n";
    for (const auto & child : children) {
      ret += child->print_node(indent_level + 1);
    }
    return ret;
  }

protected:
  /* name */
  /* value (expression) stored as child */
};

} // namespace asw::slc
#endif  // ASW__SLC_NODE_HPP_
