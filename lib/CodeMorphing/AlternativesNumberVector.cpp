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


#include "cmp/AlternativesNumberVector.h"

#include "cmp/AllPasses.h"
#include "cmp/InstructionAlternativeUtils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"


using namespace cmp;
using namespace llvm;


char AlternativesNumberVector::ID = 0;


AlternativesNumberVector::AlternativesNumberVector() :
    llvm::ModulePass(ID)
{}


bool
AlternativesNumberVector::runOnModule(llvm::Module &M)
{
  Type * ElementTy = Type::getInt32Ty(M.getContext());
  ArrayType * IntArrayTy =
      llvm::ArrayType::get(ElementTy, LastInstructionTy);

  SmallVector<Constant *, LastInstructionTy> RegRegConstructor;
  InitializeAlternativesVector<RegReg>::exec(RegRegConstructor, ElementTy);
  Constant * regRegInitList = ConstantArray::get(IntArrayTy, RegRegConstructor);
  (void)new GlobalVariable(M,
                           IntArrayTy,
                           false,
                           GlobalVariable::InternalLinkage,
                           regRegInitList,
                           "RegRegAlternativeNumber");

  SmallVector<Constant *, LastInstructionTy> RegConstConstructor;
  InitializeAlternativesVector<RegConst>::exec(RegConstConstructor, ElementTy);
  Constant * regConstInitList = ConstantArray::get(IntArrayTy, RegConstConstructor);
  (void)new GlobalVariable(M,
                           IntArrayTy,
                           false,
                           GlobalVariable::InternalLinkage,
                           regConstInitList,
                           "RegConstAlternativeNumber");
  return true;
}


void
AlternativesNumberVector::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{}


AlternativesNumberVector *
CreateAlternativesNumberVectorPass()
{
  return new AlternativesNumberVector();
}


INITIALIZE_PASS(AlternativesNumberVector, "",
                "Alternative Number Vector",
                false,
                false)
