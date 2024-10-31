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

#ifndef ASW__SCOPE_HPP_
#define ASW__SCOPE_HPP_

#include <memory>
#include <vector>

namespace asw::slc
{

/* forward declarations */
struct variable_definition;
struct function_definition;

struct scope
{
  std::shared_ptr<scope> parent = nullptr;
  std::vector<variable_definition *> variables;
  std::vector<function_definition *> functions;
};

}  // namespace asw::slc

#endif  // ASW__SCOPE_HPP_
