// Copyright 2013 Marco Minutoli <mminutoli@gmail.com>

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU  General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA 02110-1301, USA.


#include "cmp/DeclareRandomizeFunction.h"

#include "cmp/AllPasses.h"
#include "cmp/InstructionAlternatives.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"


using namespace cmp;
using namespace llvm;


char DeclareRandomizeFunction::ID = 0;


DeclareRandomizeFunction::DeclareRandomizeFunction() :
    llvm::ModulePass(ID)
{}


bool
DeclareRandomizeFunction::runOnModule(llvm::Module & M)
{
  llvm::Function * randomize = M.getFunction("randomize");

  // Maybe the function type should be checked
  if (randomize != NULL) return false;

  llvm::LLVMContext &Ctx = M.getContext();
  llvm::Type * IntTy = llvm::Type::getInt32Ty(Ctx);
  llvm::FunctionType * RandomizeTy =
      llvm::FunctionType::get(IntTy, IntTy, false);

  // Insert a declaration of randomize
  M.getOrInsertFunction("randomize", RandomizeTy);
  return true;
}


void
DeclareRandomizeFunction::getAnalysisUsage(llvm::AnalysisUsage & AU) const
{
  AU.setPreservesAll();
}


DeclareRandomizeFunction *
CreateDeclareRandomizeFunctionPass()
{
  return new DeclareRandomizeFunction();
}


INITIALIZE_PASS(DeclareRandomizeFunction, "",
                "Declare Randomize",
                false,
                false)
