#include <asw/llvm_codegen.hpp>

namespace asw::slc::LLVM
{

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
  llvm::FunctionType * print_int = llvm::FunctionType::get(
    llvm::Type::getInt32Ty(*context_), {llvm::Type::getInt64Ty(*context_)}, false);
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

void codegen::_insert_slc_double_list_functions() const
{
  /* slc_int_list */
  llvm::Type * slc_double_list_type = llvm::PointerType::getInt64Ty(*context_)->getPointerTo();
  std::vector<llvm::Type *> args_slc_double_list_destroy = {slc_double_list_type};
  std::vector<llvm::Type *> args_slc_double_list_set_head =
  {slc_double_list_type, llvm::Type::getDoubleTy(
      *context_)};
  std::vector<llvm::Type *> args_slc_double_list_cons =
  {llvm::Type::getDoubleTy(*context_), slc_double_list_type};
  llvm::FunctionType * slc_double_list_create = llvm::FunctionType::get(
    slc_double_list_type, false);
  llvm::FunctionType * slc_double_list_destroy = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_fini = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_init = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_set_head = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context_), args_slc_double_list_set_head, false);
  llvm::FunctionType * slc_double_list_car = llvm::FunctionType::get(
    llvm::Type::getDoubleTy(*context_)->getPointerTo(), args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_cdr = llvm::FunctionType::get(
    slc_double_list_type, args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_cons = llvm::FunctionType::get(
    slc_double_list_type, args_slc_double_list_cons, false);
  llvm::FunctionType * slc_double_list_add = llvm::FunctionType::get(
    llvm::Type::getDoubleTy(*context_), args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_subtract = llvm::FunctionType::get(
    llvm::Type::getDoubleTy(*context_), args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_multiply = llvm::FunctionType::get(
    llvm::Type::getDoubleTy(*context_), args_slc_double_list_destroy, false);
  llvm::FunctionType * slc_double_list_divide = llvm::FunctionType::get(
    llvm::Type::getDoubleTy(*context_), args_slc_double_list_destroy, false);
  /* utility */
  llvm::Function::Create(
    slc_double_list_create, llvm::Function::ExternalLinkage,
    "slc_double_list_create", module_.get());
  llvm::Function::Create(
    slc_double_list_destroy, llvm::Function::ExternalLinkage,
    "slc_double_list_destroy", module_.get());
  llvm::Function::Create(
    slc_double_list_init, llvm::Function::ExternalLinkage, "slc_double_list_init",
    module_.get());
  llvm::Function::Create(
    slc_double_list_fini, llvm::Function::ExternalLinkage, "slc_double_list_fini",
    module_.get());
  llvm::Function::Create(
    slc_double_list_set_head, llvm::Function::ExternalLinkage,
    "slc_double_list_set_head", module_.get());
  /* unary ops */
  llvm::Function::Create(
    slc_double_list_car, llvm::Function::ExternalLinkage, "slc_double_list_car",
    module_.get());
  llvm::Function::Create(
    slc_double_list_cdr, llvm::Function::ExternalLinkage, "slc_double_list_cdr",
    module_.get());
  /* binary ops */
  llvm::Function::Create(
    slc_double_list_cons, llvm::Function::ExternalLinkage, "slc_double_list_cons",
    module_.get());
  /* list ops */
  llvm::Function::Create(
    slc_double_list_add, llvm::Function::ExternalLinkage, "slc_double_list_add",
    module_.get());
  llvm::Function::Create(
    slc_double_list_subtract, llvm::Function::ExternalLinkage,
    "slc_double_list_subtract", module_.get());
  llvm::Function::Create(
    slc_double_list_multiply, llvm::Function::ExternalLinkage,
    "slc_double_list_multiply", module_.get());
  llvm::Function::Create(
    slc_double_list_divide, llvm::Function::ExternalLinkage,
    "slc_double_list_divide", module_.get());
}

}  // namespace asw::slc::LLVM
