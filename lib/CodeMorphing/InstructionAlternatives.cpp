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


template <>
std::vector<llvm::BasicBlock *> buildAlternatives<Xor>(llvm::Instruction & I)
{
  std::vector<BasicBlock *> alternatives;

  Value * firstOperand = I.getOperand(0);
  Value * secondOperand = I.getOperand(1);

  // First Xor alternative
  // Implements : A xor B = (A or B) and not(A and B)
  // The not operator isn't present in the LLVM IR for this reason I
  // have used xor with a constant of all ones to obtain the not.
  BasicBlock * BB = BasicBlock::Create(I.getContext());
  BinaryOperator * orA = BinaryOperator::Create(Instruction::Or, firstOperand,
                                                secondOperand, "", BB);
  BinaryOperator * andA = BinaryOperator::Create(Instruction::And, firstOperand,
                                                 secondOperand, "", BB);
  IntegerType * type = IntegerType::get(I.getContext(),
                                        andA->getType()->getIntegerBitWidth());
  Constant * allOnes = ConstantInt::get(type, type->getMask());
  BinaryOperator * notA = BinaryOperator::Create(Instruction::Xor, andA,
                                                 allOnes, "", BB);
  BinaryOperator::Create(Instruction::And, notA, orA, "", BB);

  alternatives.push_back(BB);

  // Second Xor alternative
  // Implements : A xor B = (A and not B) or (not A and B)
  BB = BasicBlock::Create(I.getContext());
  BinaryOperator * notB = BinaryOperator::Create(Instruction::Xor, secondOperand,
                                                 allOnes, "", BB);
  andA = BinaryOperator::Create(Instruction::And, firstOperand,
                                notB, "", BB);
  notA = BinaryOperator::Create(Instruction::Xor, firstOperand,
                                allOnes, "", BB);
  BinaryOperator * andB = BinaryOperator::Create(Instruction::And, secondOperand,
                                                 notA, "", BB);
  orA = BinaryOperator::Create(Instruction::Or, andA, andB, "", BB);

  alternatives.push_back(BB);

  return alternatives;
}


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
    HANDLE_ISTRUCTION_TYPE(Xor);
    case LastInstructionTy:
      break;
  }

#undef HANDLE_ISTRUCTION_TYPE

  return alternatives;
}
