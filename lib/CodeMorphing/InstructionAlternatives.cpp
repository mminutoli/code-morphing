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


#include "cmp/InstructionAlternatives.h"

#include "cmp/InstructionAlternativeUtils.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"


using namespace cmp;
using namespace llvm;


template <InstructionTy>
std::vector<llvm::BasicBlock *> buildAlternatives(llvm::Instruction & I);


// Do not define it.
template <>
std::vector<llvm::BasicBlock *>
buildAlternatives<LastInstructionTy>(llvm::Instruction & I);


#include <generated/BuildAlternatives.def>


std::vector<llvm::BasicBlock *>
InstructionAlternatives::Build(llvm::Instruction & I)
{
  std::vector<BasicBlock *> alternatives;

  InstructionTy type = getInstTy(&I);

#define HANDLE_ISTRUCTION_TYPE(TYPE)            \
  case TYPE:                                    \
    alternatives = buildAlternatives<TYPE>(I);  \
    break

  switch (type) {
#include <generated/HandleInstruction.def>
    case LastInstructionTy:
      break;
  }

#undef HANDLE_ISTRUCTION_TYPE

  return alternatives;
}
