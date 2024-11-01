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

#ifndef ASW__TYPE_INFO_HPP_
#define ASW__TYPE_INFO_HPP_

namespace asw::slc
{
enum class type_id
{
  INT,
  FLOAT,
  STRING,
  BOOL,
  LAMBDA,
  VARIABLE,
  NIL,
  LIST,
  INVALID,
};

struct type_info
{
  type_info() = default;
  type_info(const type_info & other)
  {
    type = other.type;
    if (nullptr != other.subtype) {
      subtype = new type_info(*other.subtype);
    }
  }

  ~type_info()
  {
    delete subtype;
  }

  type_info & operator=(const type_info & other)
  {
    this->type = other.type;
    delete subtype;
    if (nullptr != other.subtype) {
      subtype = new type_info();
      *subtype = *other.subtype;
    } else {
      subtype = nullptr;
    }
    return *this;
  }

  bool operator==(const type_info & rhs) const
  {
    if (type != rhs.type) {
      return false;
    } else if (type != type_id::LIST) {
      return true;
    }
    /* both are lists, compare subtypes */
    return subtype && rhs.subtype && (*subtype == *rhs.subtype);
  }

  bool operator!=(const type_info & rhs) const
  {
    return !(*this == rhs);
  }

  template<class ... Args>
  bool compatible(type_id arg, Args ... args) const
  {
    /* *INDENT-OFF* */
    return ((arg == args) || ...);
    /* *INDENT-ON* */
  }

  bool converts_to(type_info * other) const
  {
    if ((this->type == type_id::LIST) && (other->type == type_id::LIST)) {
      return *this == *other || (this->subtype && other->subtype &&
             this->subtype->converts_to(other->subtype));
    }
    switch (type) {
      case type_id::INT:
        return compatible(
          other->type, type_id::STRING, type_id::INT, type_id::FLOAT, type_id::BOOL);
      case type_id::FLOAT:
        return compatible(
          other->type, type_id::STRING, type_id::INT, type_id::FLOAT, type_id::BOOL);
      case type_id::BOOL:
        return compatible(
          other->type, type_id::STRING, type_id::INT, type_id::FLOAT, type_id::BOOL);
      case type_id::LAMBDA:
        return other->type == type_id::LAMBDA;
      case type_id::STRING:
        return compatible(
          other->type, type_id::STRING, type_id::BOOL);
      case type_id::VARIABLE:
      case type_id::NIL:
      case type_id::LIST:
        return compatible(
          other->type, type_id::BOOL);
      case type_id::INVALID:
        return false;
    }
    return false;
  }

  /* stores the type of this component */
  type_id type = type_id::INVALID;
  /* stores the inner type (for lists) */
  type_info * subtype = nullptr;
};

using namespace std::string_literals;

inline std::string type_id_to_str(type_id id)
{
  using namespace std::string_literals;
  switch (id) {
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
    case type_id::INVALID:
      return "invalid"s;
  }
  return "unknown_type"s;
}

inline std::string type_to_str(type_info * _type)
{
  if (_type->type == type_id::LIST) {
    return "list<"s + type_to_str(_type->subtype) + ">"s;
  }
  return type_id_to_str(_type->type);
}
}  // namespace asw::slc
#endif  // ASW__TYPE_INFO_HPP_
