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
  _insert_slc_double_list_functions();
  llvm::Value * ret = n->accept(this);
  n->mark_visited();
  return ret;
}

llvm::Value * codegen::visit_extern_function(extern_function * const func_) const
{
  std::vector<llvm::Type *> formals;
  formals.reserve(func_->get_formals().size());
  for (const formal * param : func_->get_formals()) {
    formals.push_back(_type_id_to_llvm(param->get_type()->type));
  }
  llvm::FunctionType * func__ = llvm::FunctionType::get(
    _type_id_to_llvm(func_->get_type()->type), formals, false);
  return llvm::Function::Create(
    func__, llvm::Function::ExternalLinkage, func_->get_name(), module_.get());
}

llvm::Value * codegen::LogErrorV(const char * s) const
{
  std::string err_text = std::string("\e[1;31mllvm error:\e[0m ") + s + "\n";
  fprintf(stderr, err_text.c_str());
  return nullptr;
}

llvm::Value * codegen::visit_literal(literal * const l) const
{
  switch (l->get_type()->type) {
    case type_id::INT:
      return llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(*context_), l->get_int());
    case type_id::FLOAT:
      return llvm::ConstantFP::get(*context_, llvm::APFloat(l->get_double()));
    case type_id::STRING:
      return builder_->CreateGlobalString(l->get_str(), l->get_fqn("."));
    case type_id::NIL:
      return llvm::ConstantPointerNull::get(llvm::PointerType::get(*context_, 0));
    default:
      break;
  }
  return LogErrorV("unknown literal");
}

llvm::Value * codegen::visit_do_loop(do_loop * const _loop) const
{
  /* save iter variable in case it shadows another variable */
  llvm::Value * old_iter_val = nullptr;
  if (auto it = named_values_.find(_loop->get_iterator()->get_name()); it != named_values_.end()) {
    old_iter_val = it->second;
  }
  llvm::Function * func = builder_->GetInsertBlock()->getParent();
  llvm::BasicBlock * check_bb = llvm::BasicBlock::Create(*context_, "check", func);
  llvm::BasicBlock * loop_bb = llvm::BasicBlock::Create(*context_, "loop", func);
  llvm::BasicBlock * update_bb = llvm::BasicBlock::Create(*context_, "update", func);
  llvm::BasicBlock * loop_end_bb = llvm::BasicBlock::Create(*context_, "loopend", func);
  llvm::Type * iter_t = _type_id_to_llvm(_loop->get_iterator()->get_type()->type);
  llvm::Value * null = llvm::ConstantPointerNull::get(llvm::PointerType::get(*context_, 0));
  llvm::AllocaInst * ret_alloca =
    builder_->CreateAlloca(
    _type_id_to_llvm(
      _loop->get_loop_body()->get_return_expression()->get_type()->type),
    nullptr, "loopret");
  llvm::AllocaInst * iter_alloca = builder_->CreateAlloca(iter_t, nullptr, "iter_head");
  /* reserve space for iterator */
  llvm::AllocaInst * list_iter_alloca =
    builder_->CreateAlloca(llvm::PointerType::get(*context_, 0), nullptr, "iter_tail");
  /* store the tail of the list in the pointer for the iterator */
  llvm::Value * init = _loop->get_iterator()->get_list()->accept(this);
  builder_->CreateStore(_do_cdr(init, _loop->get_iterator()->get_type()->type), list_iter_alloca);
  /* get the value of the head of the list, and store it in iter */
  builder_->CreateStore(_do_car(init, _loop->get_iterator()->get_type()->type), iter_alloca);
  /* insert explicit fall-through to the check block */
  builder_->CreateBr(check_bb);
  builder_->SetInsertPoint(check_bb);
  /* load the tail */
  llvm::Value * tail = builder_->CreateLoad(
    list_iter_alloca->getAllocatedType(), list_iter_alloca, "tail_iter");
  /* check if tail is null */
  llvm::Value * cond = builder_->CreateCmp(
    llvm::CmpInst::Predicate::ICMP_EQ, tail, null,
    "nullcheck");
  /* if tail is null, branch to the end of the loop. otherwise jump to body */
  builder_->CreateCondBr(cond, loop_end_bb, loop_bb);
  builder_->SetInsertPoint(loop_bb);
  /* initialize the iterator to the head of the list */
  named_values_[_loop->get_iterator()->get_name()] =
    builder_->CreateLoad(iter_alloca->getAllocatedType(), iter_alloca, "iter_head");
  /* insert explicit fall-through to the loop block */
  builder_->SetInsertPoint(loop_bb);
  /* emit the body */
  builder_->CreateStore(_loop->get_loop_body()->accept(this), ret_alloca);
  /* fall-through to the update step */
  builder_->CreateBr(update_bb);
  builder_->SetInsertPoint(update_bb);
  /* update the tail iterator */
  const type_id list_t = _loop->get_iterator()->get_type()->type;
  builder_->CreateStore(_do_cdr(tail, list_t), list_iter_alloca);
  builder_->CreateStore(_do_car(tail, list_t), iter_alloca);
  /* brach to check step */
  builder_->CreateBr(check_bb);
  builder_->SetInsertPoint(loop_end_bb);
  named_values_.erase(_loop->get_iterator()->get_name());
  named_values_[_loop->get_iterator()->get_name()] = old_iter_val;
  return builder_->CreateLoad(
    ret_alloca->getAllocatedType(), ret_alloca, "loopret");
}

llvm::Value * codegen::visit_collect_loop(collect_loop * const) const
{
  return LogErrorV("visit_collect_loop");
}

llvm::Value * codegen::visit_when_loop(when_loop * const) const
{
  return LogErrorV("visit_when_loop");
}

llvm::Value * codegen::visit_infinite_loop(infinite_loop * const) const
{
  return LogErrorV("visit_infinite_loop");
}

llvm::Value * codegen::_maybe_convert(node * const n, node * const match) const
{
  if (n->get_type()->type != match->get_type()->type) {
    /* convert first */
    switch (match->get_type()->type) {
      case type_id::INT:
        return _convert_to_int(n->accept(this), n->get_type()->type);
      case type_id::FLOAT:
        return _convert_to_float(n->accept(this), n->get_type()->type);
      case type_id::BOOL:
        return _convert_to_bool(n->accept(this), n->get_type()->type);
      default:
        return LogErrorV("unknown conversion function");
    }
  }
  return n->accept(this);
}

llvm::Value * codegen::_maybe_convert(node * const n, const type_id tid) const
{
  if (n->get_type()->type != tid) {
    /* convert first */
    switch (tid) {
      case type_id::INT:
        return _convert_to_int(n->accept(this), n->get_type()->type);
      case type_id::FLOAT:
        return _convert_to_float(n->accept(this), n->get_type()->type);
      case type_id::BOOL:
        return _convert_to_bool(n->accept(this), n->get_type()->type);
      default:
        return LogErrorV("cannot convert to requested type");
    }
  }
  return n->accept(this);
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
    case type_id::LIST:
      return val;
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
    default:
      return LogErrorV("conversion from invalid type");
  }
  return LogErrorV("unknown error");
}

llvm::Value * codegen::visit_binary_op(binary_op * const op) const
{
  expression * lhs = op->get_children()[0]->as_expression();
  expression * rhs = op->get_children()[1]->as_expression();
  /* get codegen for lhs and rhs */
  if (op->get_op() == op_id::CONS) {
    return _create_cons(lhs, rhs);
  }
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
    case type_id::NIL:
      R = _convert_to_bool(R, rhs->get_type()->type);
      predicates = {
        llvm::CmpInst::Predicate::ICMP_EQ,
        llvm::CmpInst::Predicate::ICMP_UGT,
        llvm::CmpInst::Predicate::ICMP_ULT,
        llvm::CmpInst::Predicate::ICMP_UGE,
        llvm::CmpInst::Predicate::ICMP_ULE,
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
      return builder_->CreateCmp(predicates[1], L, R, "cmptmp");
    case op_id::LESS:
      return builder_->CreateCmp(predicates[2], L, R, "cmptmp");
    case op_id::GREATER_EQ:
      return builder_->CreateCmp(predicates[3], L, R, "cmptmp");
    case op_id::LESS_EQ:
      return builder_->CreateCmp(predicates[4], L, R, "cmptmp");
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
    /* visit every node except the return expression */
    if (child != body->get_return_expression()) {
      child->accept(this);
    }
  }
  return body->get_return_expression()->accept(this);
}

llvm::Value * codegen::visit_function_call(function_call * const call) const
{
  llvm::Function * func = nullptr;
  lambda * as_lambda = dynamic_cast<lambda *>(call->get_resolution());
  if (nullptr != as_lambda) {
    /* this is a safe cast */
    func = module_->getFunction(as_lambda->get_name());
  } else {
    func = module_->getFunction(call->get_name());
  }
  if (nullptr == func) {
    /* look for a lambda */
    return LogErrorV("Unknown function called");
  }
  std::vector<llvm::Value *> args;
  callable * resolved = call->get_resolution();
  args.reserve(call->get_children().size());
  for (size_t x = 0; x < call->get_children().size(); ++x) {
    args.emplace_back(_maybe_convert(call->get_children()[x], resolved->get_formals()[x]));
  }
  std::string call_name = "calltmp";
  return builder_->CreateCall(func, args, call_name);
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
    func__, llvm::Function::ExternalLinkage, func->get_name(), module_.get());
  func_->addFnAttrs(
    llvm::AttrBuilder(*context_)
    .addAttribute(llvm::Attribute::AttrKind::NoInline)
    .addAttribute(llvm::Attribute::AttrKind::OptimizeNone)
  );
  std::string label = (func->get_name() + "_impl");
  /* record formal names */
  std::size_t x = 0;
  for (auto & arg : func_->args()) {
    arg.setName(func->get_formals()[x++]->get_name());
  }
  /* map formal names */
  for (auto & arg : func_->args()) {
    named_values_[std::string(arg.getName())] = &arg;
  }
  llvm::BasicBlock * bb = llvm::BasicBlock::Create(*context_, label, func_);
  builder_->SetInsertPoint(bb);
  llvm::Value * ret = func->get_body()->accept(this);
  builder_->CreateRet(ret);
  return func_;
}

llvm::Value * codegen::visit_if_expr(if_expr * const if_stmt) const
{
  llvm::Function * func = builder_->GetInsertBlock()->getParent();
  llvm::Value * condition =
    _maybe_convert(if_stmt->get_condition(), type_id::BOOL);

  /* create blocks for different cases */
  llvm::BasicBlock * bb_then = llvm::BasicBlock::Create(*context_, "then", func);
  llvm::BasicBlock * bb_else = llvm::BasicBlock::Create(*context_, "else");
  llvm::BasicBlock * bb_cont = llvm::BasicBlock::Create(*context_, "cont");

  /* branch based on the expression */
  builder_->CreateCondBr(condition, bb_then, bb_else);

  /* emit value for the affirmitive case */
  builder_->SetInsertPoint(bb_then);
  llvm::Value * affirmative = _maybe_convert(if_stmt->get_affirmative(), if_stmt);
  if (nullptr == affirmative) {
    return LogErrorV("error generating affirmative branch");
  }
  /* branch back to the continuation point */
  builder_->CreateBr(bb_cont);
  /* codegen of 'Then' can change the current block, update bb_then for the PHI */
  bb_then = builder_->GetInsertBlock();

  /* emit else block at the end of the function (so far), and repoint the builder's insert point */
  func->insert(func->end(), bb_else);
  builder_->SetInsertPoint(bb_else);

  llvm::Value * else_value = _maybe_convert(if_stmt->get_else(), if_stmt);
  if (nullptr == else_value) {
    return LogErrorV("error generating else branch");
  }
  builder_->CreateBr(bb_cont);
  bb_else = builder_->GetInsertBlock();

  /* emit the merge block */
  func->insert(func->end(), bb_cont);
  builder_->SetInsertPoint(bb_cont);
  llvm::PHINode * phi = builder_->CreatePHI(
    _type_id_to_llvm(if_stmt->get_type()->type), 2, "iftmp");
  phi->addIncoming(affirmative, bb_then);
  phi->addIncoming(else_value, bb_else);
  return phi;
}

llvm::Value * codegen::visit_iterator_definition(iterator_definition * const iter) const
{
  /* store the variable in the scope with no value */
  named_values_[iter->get_name()] = nullptr;
  return named_values_[iter->get_name()];
}

llvm::Value * codegen::visit_lambda(lambda * const lambda) const
{
  std::vector<llvm::Type *> formals;
  formals.reserve(lambda->get_formals().size());
  for (const formal * param : lambda->get_formals()) {
    formals.push_back(_type_id_to_llvm(param->get_type()->type));
  }
  llvm::FunctionType * func__ = llvm::FunctionType::get(
    _type_id_to_llvm(lambda->get_type()->type), formals, false);
  llvm::Function * func_ = llvm::Function::Create(
    func__, llvm::Function::ExternalLinkage, lambda->get_name(), module_.get());
  func_->addFnAttrs(
    llvm::AttrBuilder(*context_)
    .addAttribute(llvm::Attribute::AttrKind::NoInline)
    .addAttribute(llvm::Attribute::AttrKind::OptimizeNone)
  );
  std::string label = (lambda->get_name() + "_impl");
  /* record formal names */
  std::size_t x = 0;
  for (auto & arg : func_->args()) {
    arg.setName(lambda->get_formals()[x++]->get_name());
  }
  /* map formal names */
  for (auto & arg : func_->args()) {
    named_values_[std::string(arg.getName())] = &arg;
  }
  llvm::BasicBlock * bb_old = builder_->GetInsertBlock();
  llvm::BasicBlock * bb = llvm::BasicBlock::Create(*context_, label, func_);
  builder_->SetInsertPoint(bb);
  llvm::Value * ret = lambda->get_body()->accept(this);
  builder_->CreateRet(ret);
  builder_->SetInsertPoint(bb_old);  /* continue with parent function */
  return func_;
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
    case type_id::STRING:
      return llvm::Type::getInt8Ty(*context_)->getPointerTo();
    case type_id::LIST:
      return llvm::Type::getInt8Ty(*context_)->getPointerTo();
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
  if (auto it = scope_to_alloca_map_.find(v->get_parent()->get_scope().get());
    it == scope_to_alloca_map_.end())
  {
    /* create scope map if one does not already exist */
    scope_to_alloca_map_[v->get_parent()->get_scope().get()] =
      std::make_unique<name_to_alloca_map_t>();
  }
  name_to_alloca_map_t & name_to_alloca_map =
    *(scope_to_alloca_map_[v->get_parent()->get_scope().get()]);
  /* generate the initial value */
  llvm::Value * val = v->get_children()[0]->accept(this);
  /* create alloca for the value */
  llvm::AllocaInst * var_alloca = builder_->CreateAlloca(val->getType(), nullptr, v->get_name());
  /* store the value in the allocated spot */
  builder_->CreateStore(val, var_alloca);
  /* update the map */
  name_to_alloca_map[v->get_name()] = var_alloca;
  return val;
}

llvm::Value * codegen::_load_var(scope * const s, const std::string & name) const
{
  /* check the specified scope for the variable */
  name_to_alloca_map_t & name_map = *scope_to_alloca_map_[s];
  if (name_map.find(name) == name_map.end()) {
    return LogErrorV("unable to locate variable in requested scope");
  }
  llvm::AllocaInst * alloc = name_map[name];
  return builder_->CreateLoad(alloc->getAllocatedType(), alloc, name);
}

llvm::Value * codegen::_store_var(
  scope * const s, const std::string & name,
  llvm::Value * val) const
{
  /* check the specified scope for the variable */
  name_to_alloca_map_t & name_map = *scope_to_alloca_map_[s];
  if (name_map.find(name) == name_map.end()) {
    return LogErrorV("unable to locate variable in requested scope");
  }
  llvm::AllocaInst * alloc = name_map[name];
  return builder_->CreateStore(val, alloc), val;
}

llvm::Value * codegen::_create_cons(expression * const e, expression * const l) const
{
  std::vector<llvm::Value *> args = {
    _maybe_convert(e, l->get_type()->subtype->type),
    l->accept(this)
  };
  llvm::Function * cons;
  switch (l->get_type()->subtype->type) {
    case type_id::INT:
      cons = module_->getFunction("slc_int_list_cons");
      break;
    case type_id::FLOAT:
      cons = module_->getFunction("slc_double_list_cons");
      break;
    default:
      return LogErrorV("Unimplemented list type in create_cons");
  }
  return builder_->CreateCall(cons, args, "binop_cons");
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

llvm::Value * codegen::_visit_float_list(list * const l) const
{
  llvm::Function * create = module_->getFunction("slc_double_list_create");
  llvm::Function * cons = module_->getFunction("slc_double_list_cons");
  llvm::Function * set_head = module_->getFunction("slc_double_list_set_head");
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
  } else if (l->get_type()->subtype->type == type_id::FLOAT) {
    return _visit_float_list(l);
  }

  return LogErrorV("unimplemented list type in visit_list");
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

llvm::Value * codegen::_visit_list_op_float(list_op * const op) const
{
  std::vector<llvm::Value *> args = {
    op->get_children()[0]->accept(this),
  };
  llvm::Function * op_impl;
  switch (op->get_op()) {
    case op_id::PLUS:
      op_impl = module_->getFunction("slc_double_list_add");
      break;
    case op_id::MINUS:
      op_impl = module_->getFunction("slc_double_list_subtract");
      break;
    case op_id::TIMES:
      op_impl = module_->getFunction("slc_double_list_multiply");
      break;
    case op_id::DIVIDE:
      op_impl = module_->getFunction("slc_double_list_divide");
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
  } else if (op->get_type()->type == type_id::FLOAT) {
    return _visit_list_op_float(op);
  }
  return LogErrorV("unimplemented list type in visit_list_op");
}

llvm::Value * codegen::visit_node(node * const n) const
{
  llvm::Value * ret{nullptr};
  for (node * const child : n->get_children()) {
    ret = child->accept(this);
  }
  return ret;
}

llvm::Value * codegen::visit_set_expression(set_expression * const expr) const
{
  scope * const s = expr->get_resolution()->get_scope().get();
  return _store_var(s, expr->get_name(), expr->get_children()[0]->accept(this));
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
    } else if (op->get_children()[0]->get_type()->subtype->type == type_id::FLOAT) {
      return _visit_unary_op_float_list(op);
    }

  }
  return LogErrorV("unimplemented unary op");
}

llvm::Value * codegen::_do_car(llvm::Value * l, const type_id list_type) const
{
  llvm::Function * op_impl;
  std::vector<llvm::Value *> args = {l};

  switch (list_type) {
    case type_id::INT:
      op_impl = module_->getFunction("slc_int_list_car");
      return builder_->CreateLoad(
        llvm::Type::getInt64Ty(*context_),
        builder_->CreateCall(op_impl, args));
    case type_id::FLOAT:
      op_impl = module_->getFunction("slc_double_list_car");
      return builder_->CreateCall(op_impl, args);
    default:
      return LogErrorV("unimplemented car type");
  }
  return LogErrorV("unexpected return in _do_car");
}

llvm::Value * codegen::_do_car(expression * const l) const
{
  return _do_car(l->accept(this), l->get_type()->subtype->type);
}

llvm::Value * codegen::_do_cdr(llvm::Value * l, const type_id list_type) const
{
  llvm::Function * op_impl;
  std::vector<llvm::Value *> args = {l};
  switch (list_type) {
    case type_id::INT:
      op_impl = module_->getFunction("slc_int_list_cdr");
      break;
    case type_id::FLOAT:
      op_impl = module_->getFunction("slc_double_list_cdr");
      break;
    default:
      return LogErrorV("unimplemented cdr type in _do_cdr");
  }
  return builder_->CreateCall(op_impl, args);
}

llvm::Value * codegen::_do_cdr(expression * const l) const
{
  return _do_cdr(l->accept(this), l->get_type()->subtype->type);
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
      return builder_->CreateLoad(
        llvm::Type::getInt64Ty(*context_),
        builder_->CreateCall(op_impl, args));
    case op_id::CDR:
      op_impl = module_->getFunction("slc_int_list_cdr");
      return builder_->CreateCall(op_impl, args);
    default:
      break;
  }
  return LogErrorV("unimplemented unary op");
}

llvm::Value * codegen::_visit_unary_op_float_list(unary_op * const op) const
{
  llvm::Function * op_impl;
  llvm::Value * arg = op->get_children()[0]->accept(this);
  std::vector<llvm::Value *> args = {
    arg
  };
  switch (op->get_op()) {
    case op_id::CAR:
      op_impl = module_->getFunction("slc_double_list_car");
      break;
    case op_id::CDR:
      op_impl = module_->getFunction("slc_double_list_cdr");
      break;
    default:
      return LogErrorV("unimplemented unary op");
  }
  return builder_->CreateCall(op_impl, args);
}

llvm::Value * codegen::visit_variable(variable * const var) const
{
  if (auto it = named_values_.find(var->get_name()); it != named_values_.end()) {
    return it->second;
  }
  /* otherwise, call load */
  return _load_var(var->get_resolution()->get_scope().get(), var->get_name());
}
}  // namespace asw::slc::LLVM
