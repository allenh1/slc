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

#ifndef ASW__LOCATION_INFO_HPP_
#define ASW__LOCATION_INFO_HPP_

#include <string>

namespace asw::slc
{

struct location_info
{
  location_info(
    int _line, int _column, const char * _text)
  {
    line = _line;
    column = _column;
    text = _text;
  }

  int line = 0;
  int column = 0;
  std::string text;
};

}  // namespace asw::slc
#endif  // ASW__LOCATION_INFO_HPP_
