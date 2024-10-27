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

#include <asw/slc_node.hpp>

#define utilities_impl(type) \
  bool node::is_ ## type() {return this->as_ ## type() != nullptr;} \
  type * node::as_ ## type() {return dynamic_cast<type *>(this);}

namespace asw::slc
{
utilities_impl(binary_op)
utilities_impl(list_op)
utilities_impl(unary_op)
utilities_impl(if_expr)
utilities_impl(expression)
utilities_impl(list)
utilities_impl(literal)
utilities_impl(function_call)
utilities_impl(function_body)
utilities_impl(variable_definition)
utilities_impl(function_definition)
}  // namespace asw::slc
