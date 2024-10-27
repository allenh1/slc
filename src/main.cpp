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

#include <sys/wait.h>
#include <unistd.h>

extern FILE * yyin;
extern FILE * yyout;

extern "C" {
int yyparse();
void yyrestart(FILE * in);
void yyerror(YYLTYPE *, asw::slc::node *, const char * s);
}

int main(int argc, char ** argv)
{
  if (!(argc == 2 || argc == 4 || (argc > 5 && (std::string_view(argv[4]) == "--gcc-opts")))) {
    fprintf(stderr, "Invalid args.\n");
    fprintf(stderr, "Usage:\n%s [file]:\t\t create llvm intermediate\n", argv[0]);
    fprintf(stderr, "%s [file] -o [output]:\t\t compile to executable\n", argv[0]);
    fprintf(stderr, "%s [file] -o [output] --gcc-opts [opts]*:\t compile to executable, pass anything after gcc opts to gcc\n", argv[0]);
    return 1;
  }
  yyin = fopen(argv[1], "r");
  if (NULL == yyin) {
    fprintf(stderr, "Cannot read input from '%s'.\n", argv[1]);
    return 2;
  }
  asw::slc::node root;
  int ret = yyparse(&root);
  if (0 != ret) {
    return ret;
  }
  std::string outfile_name{argv[1]};
  outfile_name += ".yml";
  std::string llvm_out = std::string(argv[1]) + ".ll";
  std::string llvm_asm_out = std::string(argv[1]) + ".s";
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
  /* convert to IR */
  asw::slc::LLVM::codegen llvm_codegen;
  llvm::Value * ir = llvm_codegen.visit(&root);
  if (!ir) {
    return 1;
  }
  FILE * llvm_out_f = fopen(llvm_out.c_str(), "w");
  auto file_out = llvm::raw_fd_ostream(fileno(llvm_out_f), true);
  /* write IR to file */
  llvm_codegen.get_mod()->print(file_out, nullptr);
  /* call llc */
  pid_t child = fork();
  if (0 == child) {
    execlp("llc", "llc", llvm_out.c_str(), nullptr);
  } else {
    int status = -1;
    if (child != waitpid(child, &status, 0)) {
      return 2;
    }
  }
  /* call gcc */
  pid_t child2 = fork();
  if (0 == child2) {
    std::vector<const char *> args;
    args.emplace_back("gcc");
    args.emplace_back(llvm_asm_out.c_str());
    std::string lib_path = std::string("-L") + RUNTIME_PREFIX + "/";
    args.emplace_back(lib_path.c_str());
    args.emplace_back("-lslc_runtime");
    args.emplace_back("-o");
    args.emplace_back(argv[3]);
    args.emplace_back(nullptr);
    /* todo: propagate gcc args */
    execvp("gcc", (char * const *)args.data());
  } else {
    int status = -1;
    if (child2 != waitpid(child2, &status, 0)) {
      return 2;
    }
  }
  return 0;
}
