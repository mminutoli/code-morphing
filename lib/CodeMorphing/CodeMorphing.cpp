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


#include "cmp/CodeMorphing.h"

#include "cmp/AllPasses.h"
#include "cmp/AlternativesNumberVector.h"
#include "cmp/DeclareRandomizeFunction.h"
#include "cmp/InstructionAlternatives.h"
#include "cmp/InstructionAlternativeUtils.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"


using namespace cmp;
using namespace llvm;


char CodeMorphing::ID = 0;


CodeMorphing::CodeMorphing() :
    llvm::FunctionPass(ID)
{}


namespace {
typedef std::pair<Instruction *, std::vector<BasicBlock *> > replacement;
}

static bool
BuildReplacementList(Function & , std::vector<replacement> &);
static bool
InsertAlternativeBlocks(Function &, std::vector<replacement> &, Pass *);
bool CodeMorphing::runOnFunction(Function & F)
{
  bool result = false;
  std::vector<replacement> replacementList;
  result |= BuildReplacementList(F, replacementList);
  if (result)
  {
    result |= InsertAlternativeBlocks(F, replacementList, this);
  }
  return result;
}


void CodeMorphing::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<DeclareRandomizeFunction>();
  AU.addRequired<AlternativesNumberVector>();
}


CodeMorphing * CreateCodeMorphingPass()
{
  return new CodeMorphing();
}


INITIALIZE_PASS(CodeMorphing, "static-morphing",
                "Static Code Morphing Pass",
                false,
                false)


static bool
BuildReplacementList(Function & F, std::vector<replacement> & V)
{
  typedef Function::BasicBlockListType::iterator BlockListIterator;
  for (BlockListIterator block = F.begin(), lastBlock = F.end();
       block != lastBlock; ++block)
  {
    typedef BasicBlock::iterator InstructionIterator;
    for (InstructionIterator instruction = block->begin(),
                         lastInstruction = block->end();
         instruction != lastInstruction; ++instruction)
    {
      std::vector<BasicBlock *> alternativeBlocks =
          InstructionAlternatives::Build(*instruction);
      if (!alternativeBlocks.empty())
      {
        replacement rep = std::make_pair(&*instruction, alternativeBlocks);
        V.push_back(rep);
      }
    }
  }

  return !V.empty();
}


static std::string alternativesVectorName(Instruction * i)
{
  Value * firstOperand = i->getOperand(0);
  Value * secondOperand = i->getOperand(1);

  if (isa<Constant>(firstOperand) || isa<Constant>(secondOperand))
    return "RegConstAlternativeNumber";
  return "RegRegAlternativeNumber";
}


static bool
InsertAlternativeBlocks(Function & F, std::vector<replacement> & V, Pass * P)
{
  bool result = false;

  typedef std::vector<replacement>::iterator ReplacementItr;
  for (ReplacementItr I = V.begin(), E = V.end(); I != E; ++I)
  {
    Instruction * i = I->first;
    std::vector<BasicBlock *> & replacementList = I->second;

    // Split the block containing I
    BasicBlock * upperBlock = i->getParent();
    BasicBlock * originalBlock = SplitBlock(upperBlock, i);
    originalBlock->setName("");
    BasicBlock * lowerBlock = SplitBlock(originalBlock, i->getNextNode());
    lowerBlock->setName("");

    IntegerType * Int32Ty = Type::getInt32Ty(F.getContext());
    Value * alternativesVector =
        F.getParent()->getValueSymbolTable().lookup(alternativesVectorName(i));
    assert(alternativesVector && "Alternatives vector not found!");

    ConstantInt * zero = ConstantInt::get(Int32Ty, 0);
    ConstantInt * index = ConstantInt::get(Int32Ty, getInstTy(i));
    std::vector<Value *> indexes;
    indexes.push_back(zero);
    indexes.push_back(index);

    GetElementPtrInst * alternativesPtr =
        GetElementPtrInst::CreateInBounds(alternativesVector, indexes, "",
                                          upperBlock);
    LoadInst * numberOfAlternatives =
        new LoadInst(alternativesPtr, "", upperBlock->getTerminator());

    const ValueSymbolTable & VST = F.getParent()->getValueSymbolTable();
    Value * randomize = VST.lookup("randomize");
    CallInst * choice =
        CallInst::Create(randomize, numberOfAlternatives,
                         "", upperBlock->getTerminator());
    SwitchInst * test = SwitchInst::Create(choice, originalBlock, 0);
    ReplaceInstWithInst(upperBlock->getTerminator(), test);

    test->addCase(zero, originalBlock);
    PHINode * phi = PHINode::Create(i->getType(), 0);
    i->replaceAllUsesWith(phi);
    phi->addIncoming(&*originalBlock->begin(), originalBlock);

    lowerBlock->getInstList().push_front(phi);

    unsigned int alternative = 1;
    for (std::vector<BasicBlock *>::iterator
             replacement = replacementList.begin(),
             lastReplacement = replacementList.end();
         replacement != lastReplacement; ++replacement)
    {
      ConstantInt * alternativeNumber =
          ConstantInt::get(Int32Ty, alternative);
      test->addCase(alternativeNumber, *replacement);

      phi->addIncoming(&(*replacement)->back(), *replacement);

      BranchInst::Create(lowerBlock, *replacement);
      F.getBasicBlockList().push_back(*replacement);

      ++alternative;
    }
  }

  return result;
}
