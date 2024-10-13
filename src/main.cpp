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

#include "slc_bison.hh"
#include <asw/slc_node.hpp>
#include <asw/semantics.hpp>

extern FILE * yyin;
extern FILE * yyout;

extern "C" {
  int yyparse();
  void yyrestart(FILE * in);
  void yyerror(YYLTYPE *, asw::slc::node *, const char * s);
}

int main(int argc, char ** argv)
{
  if (argc != 2) {
    fprintf(stderr, "Invalid args.\n");
    return 1;
  }
  yyin = fopen(argv[1], "r");
  if (NULL == yyin) {
    fprintf(stderr, "Cannot read input from '%s'.\n", argv[1]);
  }
  asw::slc::node root;
  int ret = yyparse(&root);
  if (0 != ret) {
    return ret;
  }
  std::string outfile_name{argv[1]};
  outfile_name += ".yml";
  FILE * out = fopen(outfile_name.c_str(), "w");
  if (nullptr != out) {
    fputs(root.print().c_str(), out);
    fclose(out);
  }
  /* semantic analysis */
  asw::slc::SemanticAnalyzer & a = asw::slc::SemanticAnalyzer::get_instance();
  if (!a.visit(&root)) {
    return 1;
  }
  return 0;
}
