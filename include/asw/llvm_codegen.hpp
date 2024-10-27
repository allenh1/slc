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

#ifndef ASW__LLVM_CODEGEN_HPP_
#define ASW__LLVM_CODEGEN_HPP_

#include <asw/location_info.hpp>
#include <asw/type_info.hpp>
#include <asw/visitor.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/IR/NoFolder.h>
#pragma GCC diagnostic pop

namespace asw::slc::LLVM
{

struct codegen : public llvm_visitor
{
  codegen();
  ~codegen() override = default;

  llvm::Value * LogErrorV(const char * s) const;

  llvm::Value * visit(node * const n) const;
  llvm::Value * visit_binary_op(binary_op * const) const override;
  llvm::Value * visit_literal(literal * const) const override;

  llvm::Value * visit_formal(formal * const) const override;
  llvm::Value * visit_function_body(function_body * const) const override;
  llvm::Value * visit_function_call(function_call * const) const override;
  llvm::Value * visit_function_definition(function_definition * const) const override;
  llvm::Value * visit_if_expr(if_expr * const) const override;
  llvm::Value * visit_variable_definition(variable_definition * const) const override;
  llvm::Value * visit_list(list * const) const override;
  llvm::Value * visit_list_op(list_op * const) const override;

  llvm::Value * visit_node(node * const) const override;
  llvm::Value * visit_simple_expression(simple_expression * const) const override;
  llvm::Value * visit_unary_op(unary_op * const) const override;
  llvm::Value * visit_variable(variable * const) const override;

  std::unique_ptr<llvm::Module> & get_mod() const
  {
    return module_;
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
  llvm::Value * _convert_to_bool(llvm::Value * val, const type_id _type) const;
  llvm::Value * _convert_to_int(llvm::Value * val, const type_id _type) const;
  llvm::Value * _convert_to_float(llvm::Value * val, const type_id _type) const;
  llvm::Value * _visit_int_list(list * const l) const;
  llvm::Value * _visit_list_op_int(list_op * const op) const;
  llvm::Value * _visit_unary_op_int_list(unary_op * const op) const;

  void _insert_slc_int_list_functions() const;

  llvm::Type * _type_id_to_llvm(const type_id id) const;

  inline static std::unique_ptr<llvm::LLVMContext> context_ = nullptr;
  inline static std::unique_ptr<llvm::Module> module_ = nullptr;
  inline static std::unique_ptr<llvm::IRBuilder<llvm::NoFolder>> builder_ = nullptr;
};

}  // namespace asw::slc::LLVM

#endif  // ASW__LLVM_CODEGEN_HPP_
