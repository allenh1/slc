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

#include <asw/llvm_codegen.hpp>
#include <asw/slc_node.hpp>

namespace asw::slc::LLVM
{

codegen::codegen()
{
  context_ = std::make_unique<llvm::LLVMContext>();
  module_ = std::make_unique<llvm::Module>("slc", *context_);
  builder_ = std::make_unique<llvm::IRBuilder<llvm::NoFolder>>(*context_);
}

llvm::Value * codegen::visit(node * const n) const
{
  n->mark_visiting();
  _insert_slc_int_list_functions();
  llvm::Value * ret = n->accept(this);
  n->mark_visited();
  return ret;
}

void codegen::_insert_slc_int_list_functions() const
{
  /* slc_int_list */
  llvm::Type * slc_int_list_type = llvm::PointerType::getInt64Ty(*context_)->getPointerTo();
  std::vector<llvm::Type *> args_slc_int_list_destroy = {slc_int_list_type};
  std::vector<llvm::Type *> args_slc_int_list_set_head = {slc_int_list_type, llvm::Type::getInt64Ty(
      *context_)};
  std::vector<llvm::Type *> args_slc_int_list_cons =
  {llvm::Type::getInt64Ty(*context_), slc_int_list_type};
  llvm::FunctionType * slc_int_list_create = llvm::FunctionType::get(
    slc_int_list_type, false);
  llvm::FunctionType * slc_int_list_destroy = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_fini = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_init = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_set_head = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_int_list_set_head, false);
  llvm::FunctionType * slc_int_list_car = llvm::FunctionType::get(
    llvm::Type::getInt64Ty(*context_)->getPointerTo(), args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_cdr = llvm::FunctionType::get(
    slc_int_list_type, args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_cons = llvm::FunctionType::get(
    slc_int_list_type, args_slc_int_list_cons, false);
  llvm::FunctionType * slc_int_list_add = llvm::FunctionType::get(
    llvm::Type::getInt64Ty(*context_), args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_subtract = llvm::FunctionType::get(
    llvm::Type::getInt64Ty(*context_), args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_multiply = llvm::FunctionType::get(
    llvm::Type::getInt64Ty(*context_), args_slc_int_list_destroy, false);
  llvm::FunctionType * slc_int_list_divide = llvm::FunctionType::get(
    llvm::Type::getInt64Ty(*context_), args_slc_int_list_destroy, false);
  /* utility */
  llvm::Function::Create(
    slc_int_list_create, llvm::Function::ExternalLinkage,
    "slc_int_list_create", module_.get());
  llvm::Function::Create(
    slc_int_list_destroy, llvm::Function::ExternalLinkage,
    "slc_int_list_destroy", module_.get());
  llvm::Function::Create(
    slc_int_list_init, llvm::Function::ExternalLinkage, "slc_int_list_init",
    module_.get());
  llvm::Function::Create(
    slc_int_list_fini, llvm::Function::ExternalLinkage, "slc_int_list_fini",
    module_.get());
  llvm::Function::Create(
    slc_int_list_set_head, llvm::Function::ExternalLinkage,
    "slc_int_list_set_head", module_.get());
  /* unary ops */
  llvm::Function::Create(
    slc_int_list_car, llvm::Function::ExternalLinkage, "slc_int_list_car",
    module_.get());
  llvm::Function::Create(
    slc_int_list_cdr, llvm::Function::ExternalLinkage, "slc_int_list_cdr",
    module_.get());
  /* binary ops */
  llvm::Function::Create(
    slc_int_list_cons, llvm::Function::ExternalLinkage, "slc_int_list_cons",
    module_.get());
  /* list ops */
  llvm::Function::Create(
    slc_int_list_add, llvm::Function::ExternalLinkage, "slc_int_list_add",
    module_.get());
  llvm::Function::Create(
    slc_int_list_subtract, llvm::Function::ExternalLinkage,
    "slc_int_list_subtract", module_.get());
  llvm::Function::Create(
    slc_int_list_multiply, llvm::Function::ExternalLinkage,
    "slc_int_list_multiply", module_.get());
  llvm::Function::Create(
    slc_int_list_divide, llvm::Function::ExternalLinkage,
    "slc_int_list_divide", module_.get());
}

llvm::Value * codegen::LogErrorV(const char * s) const
{
  std::string err_text = std::string("\e[1;31mllvm error:\e[0m ") + s + "\n";
  fprintf(stderr, err_text.c_str());
  return nullptr;
}

llvm::Value * codegen::visit_literal(literal * const l) const
{
  debug("visit_literal\n", l);
  switch (l->get_type()->type) {
    case type_id::INT:
      return llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(*context_), l->get_int());
    case type_id::FLOAT:
      return llvm::ConstantFP::get(*context_, llvm::APFloat(l->get_double()));
    case type_id::STRING:
      /* TODO */
      return LogErrorV("strings are not implemented");
    default:
      break;
  }
  return LogErrorV("unknown literal");
}

llvm::Value * codegen::_convert_to_float(llvm::Value * val, const type_id _type) const
{
  switch (_type) {
    case type_id::INT:
      return builder_->CreateSIToFP(val, llvm::Type::getDoubleTy(*context_), "doubletmp");
    case type_id::BOOL:
      return builder_->CreateUIToFP(val, llvm::Type::getDoubleTy(*context_), "booltmp");
    case type_id::FLOAT:
      return val;
    case type_id::STRING:
      /* TODO */
      return LogErrorV("strings are not implemented");
    default:
      return LogErrorV("conversion from invalid type");
  }
  return LogErrorV("unknown error");
}

llvm::Value * codegen::_convert_to_bool(llvm::Value * val, const type_id _type) const
{
  switch (_type) {
    case type_id::INT:
      return val;
    case type_id::BOOL:
      return val;
    case type_id::FLOAT:
      return builder_->CreateFPToUI(val, llvm::Type::getInt1Ty(*context_), "booltmp");
    case type_id::STRING:
      /* TODO */
      return LogErrorV("strings are not implemented");
    default:
      return LogErrorV("conversion from invalid type");
  }
  return LogErrorV("unknown error");
}

llvm::Value * codegen::_convert_to_int(llvm::Value * val, const type_id _type) const
{
  switch (_type) {
    case type_id::INT:
      return val;
    case type_id::BOOL:
      return val;
    case type_id::FLOAT:
      return builder_->CreateFPToSI(val, llvm::Type::getInt64Ty(*context_), "inttmp");
    case type_id::STRING:
      /* TODO */
      return LogErrorV("strings are not implemented");
    default:
      return LogErrorV("conversion from invalid type");
  }
  return LogErrorV("unknown error");
}

llvm::Value * codegen::visit_binary_op(binary_op * const op) const
{
  debug("visit_binary_op\n", op);
  expression * lhs = op->get_children()[0]->as_expression();
  expression * rhs = op->get_children()[1]->as_expression();
  /* get codegen for lhs and rhs */
  llvm::Value * L = lhs->accept(this);
  llvm::Value * R = rhs->accept(this);
  /* 0: eq, 1: gt, 2: lt, 3: ge, 4: le */
  std::vector<llvm::CmpInst::Predicate> predicates(5);
  /* check if the types are consistent, and see if we need to convert */
  switch (lhs->get_type()->type) {
    case type_id::INT:
      R = _convert_to_int(R, rhs->get_type()->type);
      predicates = {
        llvm::CmpInst::Predicate::ICMP_EQ,
        llvm::CmpInst::Predicate::ICMP_SGT,
        llvm::CmpInst::Predicate::ICMP_SLT,
        llvm::CmpInst::Predicate::ICMP_SGE,
        llvm::CmpInst::Predicate::ICMP_SLE,
      };
      break;
    case type_id::BOOL:
      R = _convert_to_bool(R, rhs->get_type()->type);
      predicates = {
        llvm::CmpInst::Predicate::ICMP_EQ,
        llvm::CmpInst::Predicate::ICMP_UGT,
        llvm::CmpInst::Predicate::ICMP_ULT,
        llvm::CmpInst::Predicate::ICMP_UGE,
        llvm::CmpInst::Predicate::ICMP_ULE,
      };
      break;
    case type_id::FLOAT:
      R = _convert_to_float(R, rhs->get_type()->type);
      predicates = {
        llvm::CmpInst::Predicate::FCMP_UEQ,
        llvm::CmpInst::Predicate::FCMP_UGT,
        llvm::CmpInst::Predicate::FCMP_ULT,
        llvm::CmpInst::Predicate::FCMP_UGE,
        llvm::CmpInst::Predicate::FCMP_ULE,
      };
      break;
    default:
      break;
  }
  /* types are consistent, do the comparison */
  switch (op->get_op()) {
    case op_id::EQUAL:
      return builder_->CreateCmp(predicates[0], L, R, "cmptmp");
    case op_id::GREATER:
      {
        return builder_->CreateCmp(predicates[1], L, R, "cmptmp");
      }
    case op_id::LESS:
      return builder_->CreateCmp(predicates[2], L, R, "cmptmp");
    case op_id::GREATER_EQ:
      return builder_->CreateCmp(predicates[3], L, R, "cmptmp");
    case op_id::LESS_EQ:
      return builder_->CreateCmp(predicates[4], L, R, "cmptmp");
    case op_id::CONS:
      return LogErrorV("unimplemented binary operation");
    default:
      return LogErrorV("invalid binary operation");
  }
  return LogErrorV("unknown error");
}

llvm::Value * codegen::visit_formal(formal * const) const
{
  return LogErrorV("call to visit_formal");
}

llvm::Value * codegen::visit_function_body(function_body * const body) const
{
  for (node * const child : body->get_children()) {
    if (child != body->get_return_expression()) {
      child->accept(this);
    }
  }
  return body->get_return_expression()->accept(this);
}

llvm::Value * codegen::visit_function_call(function_call * const) const
{
  return LogErrorV("visit_function_call");
}

llvm::Value * codegen::visit_function_definition(function_definition * const func) const
{
  std::vector<llvm::Type *> formals;
  formals.reserve(func->get_formals().size());
  for (const formal * param : func->get_formals()) {
    formals.push_back(_type_id_to_llvm(param->get_type()->type));
  }
  llvm::FunctionType * func__ = llvm::FunctionType::get(
    _type_id_to_llvm(func->get_type()->type), formals, false);
  llvm::Function * func_ = llvm::Function::Create(
    func__, llvm::Function::PrivateLinkage, func->get_name(), module_.get());
  std::string label = (func->get_name() + "_impl");
  llvm::BasicBlock * bb = llvm::BasicBlock::Create(*context_, label, func_);
  builder_->SetInsertPoint(bb);
  llvm::Value * ret = func->get_body()->accept(this);
  builder_->CreateRet(ret);
  return func_;
}

llvm::Value * codegen::visit_if_expr(if_expr * const) const
{
  return LogErrorV("visit_if_expr");
}

llvm::Type * codegen::_type_id_to_llvm(const type_id id) const
{
  switch (id) {
    case type_id::INT:
      return llvm::Type::getInt64Ty(*context_);
    case type_id::FLOAT:
      return llvm::Type::getDoubleTy(*context_);
    case type_id::BOOL:
      return llvm::Type::getInt1Ty(*context_);
    default:
      return nullptr;
  }
  return nullptr;
}

llvm::Value * codegen::visit_variable_definition(variable_definition * const v) const
{
  llvm::Type * type_ = _type_id_to_llvm(v->get_type()->type);
  if (v->get_parent()->get_scope()->parent == nullptr) {
    /* declare a global */
    if (v->get_type()->type == type_id::LIST) {
      return LogErrorV("global lists unimplemented");
    }
    llvm::FunctionType * init_impl;
    switch (v->get_type()->type) {
      case type_id::INT:
        type_ = llvm::Type::getInt64Ty(*context_);
        break;
      case type_id::FLOAT:
        type_ = llvm::Type::getDoubleTy(*context_);
        break;
      default:
        return LogErrorV("unimplemented global type");
    }
    llvm::GlobalVariable * gv = new llvm::GlobalVariable(
      *module_, type_, false, llvm::GlobalValue::CommonLinkage, 0, v->get_name());
    return gv;
  }
  return LogErrorV("visit_variable_definition");
}

llvm::Value * codegen::_visit_int_list(list * const l) const
{
  llvm::Function * create = module_->getFunction("slc_int_list_create");
  llvm::Function * cons = module_->getFunction("slc_int_list_cons");
  llvm::Function * set_head = module_->getFunction("slc_int_list_set_head");
  if (l->get_tail() == nullptr) {
    llvm::Value * ret = builder_->CreateCall(create);
    std::vector<llvm::Value *> args = {
      ret,
      l->get_head()->accept(this),
    };
    /* call set head */
    builder_->CreateCall(set_head, args, "calltmp");
    return ret;
  }
  std::vector<llvm::Value *> args = {
    l->get_head()->accept(this),
    l->get_tail()->accept(this),
  };
  /* call cons */
  return builder_->CreateCall(cons, args, "constmp");
}

llvm::Value * codegen::visit_list(list * const l) const
{
  if (l->get_type()->subtype->type == type_id::INT) {
    return _visit_int_list(l);
  }
  return LogErrorV("unimplemented list type");
}

llvm::Value * codegen::_visit_list_op_int(list_op * const op) const
{
  std::vector<llvm::Value *> args = {
    op->get_children()[0]->accept(this),
  };
  llvm::Function * op_impl;
  switch (op->get_op()) {
    case op_id::PLUS:
      op_impl = module_->getFunction("slc_int_list_add");
      break;
    case op_id::MINUS:
      op_impl = module_->getFunction("slc_int_list_subtract");
      break;
    case op_id::TIMES:
      op_impl = module_->getFunction("slc_int_list_multiply");
      break;
    case op_id::DIVIDE:
      op_impl = module_->getFunction("slc_int_list_divide");
      break;
    default:
      return LogErrorV("not a list op");
  }
  return builder_->CreateCall(op_impl, args);
}

llvm::Value * codegen::visit_list_op(list_op * const op) const
{
  if (op->get_type()->type == type_id::INT) {
    return _visit_list_op_int(op);
  }
  return LogErrorV("unimplemented list type");
}

llvm::Value * codegen::visit_node(node * const n) const
{
  llvm::Value * ret{nullptr};
  for (node * const child : n->get_children()) {
    ret = child->accept(this);
  }
  return ret;
}

llvm::Value * codegen::visit_simple_expression(simple_expression * const) const
{
  return LogErrorV("visit_simple_expression");
}

llvm::Value * codegen::visit_unary_op(unary_op * const op) const
{
  if (op->get_children()[0]->get_type()->type == type_id::LIST) {
    if (op->get_children()[0]->get_type()->subtype->type == type_id::INT) {
      return _visit_unary_op_int_list(op);
    }
  }
  return LogErrorV("unimplemented unary op");
}

llvm::Value * codegen::_visit_unary_op_int_list(unary_op * const op) const
{
  llvm::Function * op_impl;
  llvm::Value * arg = op->get_children()[0]->accept(this);
  std::vector<llvm::Value *> args = {
    arg
  };
  switch (op->get_op()) {
    case op_id::CAR:
      op_impl = module_->getFunction("slc_int_list_car");
      break;
    case op_id::CDR:
      op_impl = module_->getFunction("slc_int_list_cdr");
      break;
    default:
      return LogErrorV("unimplemented unary op");
  }
  return builder_->CreateCall(op_impl, args);
}

llvm::Value * codegen::visit_variable(variable * const) const
{
  return LogErrorV("visit_variable");
}
}  // namespace asw::slc::LLVM
