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

static bool InsertChoiceVector(Function &, Pass *);
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
    InsertChoiceVector(F, this);
    InsertAlternativeBlocks(F, replacementList, this);
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
InsertChoiceVector(Function & F, Pass * P)
{
  // Build basic block structure for the initialization loop.  The
  // entry block is splitted in a way that will produce the following
  // blocks:
  //   1. A new empty entry block (entry)
  //   2. A block hosting the loop condition (loopCondition)
  //   3. A block hosting the loop body (loopBody)
  //   4. The old entry block reached at loop exit (oldEntry)
  BasicBlock * entry = F.begin();
  BasicBlock * loopCondition =
      SplitBlock(entry, entry->begin(), P);
  loopCondition->setName("loopCondition");

  BasicBlock * oldEntry = SplitEdge(entry, loopCondition, P);
  oldEntry->setName("oldEntry");
  BasicBlock * loopBody = SplitBlock(loopCondition, loopCondition->begin(), P);
  loopBody->setName("loopBody");

  // Fill the new entry block with instruction that:
  //   1. allocate space for the choice vector
  //   2. allocate space for the index variable and initialize it.
  Type * Int32Ty = Type::getInt32Ty(F.getContext());
  ArrayType * Int32ArrayTy =
      ArrayType::get(Int32Ty, LastInstructionTy);
  AllocaInst * choiceVector =
      new AllocaInst(Int32ArrayTy, "choice.vector", entry->getTerminator());
  choiceVector->setAlignment(4);

  AllocaInst * index =
      new AllocaInst(Int32Ty, "idx.vector", entry->getTerminator());
  index->setAlignment(4);
  Constant * lastInstrConstant = ConstantInt::get(Int32Ty, LastInstructionTy);
  (void)new StoreInst(lastInstrConstant, index, entry->getTerminator());

  // Fill the loopCondition block with instruction that:
  //   1. decrese the index
  //   2. test the exit condition
  //   3. jump to the appropriate block
  Instruction * originalJump = loopCondition->getTerminator();

  LoadInst * loadIdx = new LoadInst(index, "", originalJump);
  Constant * minusOne = ConstantInt::get(Int32Ty, -1, true);
  BinaryOperator * decInst =
      BinaryOperator::CreateAdd(loadIdx, minusOne, ".dec", originalJump);
  StoreInst * newIdxValue = new StoreInst(decInst, index, originalJump);

  Constant * zero = ConstantInt::get(Int32Ty, 0, true);
  ICmpInst * loopTest =
      new ICmpInst(originalJump, ICmpInst::ICMP_NE, loadIdx, zero);

  BranchInst * conditionalBranch =
      BranchInst::Create(loopBody, oldEntry, loopTest);
  ReplaceInstWithInst(originalJump, conditionalBranch);

  // Fill the loopBody block with instruction that:
  //   1. retrieve index
  //   2. retrieve the value to be passed to randomize
  //   3. call randomize
  //   4. store the result in the choice vector
  //   5. jump back to the loopCondition block
  const ValueSymbolTable & VST = F.getParent()->getValueSymbolTable();
  Value * randomize = VST.lookup("randomize");
  Value * alternativesVector = VST.lookup("AlternativeNumber");

  originalJump = loopBody->getTerminator();
  loadIdx = new LoadInst(index, "", originalJump);

  std::vector<Value *> indexes;
  indexes.push_back(zero);
  indexes.push_back(loadIdx);
  GetElementPtrInst * alternativesVecElemPtr =
      GetElementPtrInst::Create(alternativesVector, indexes, "", originalJump);
  alternativesVecElemPtr->setIsInBounds(true);
  LoadInst * loadArg = new LoadInst(alternativesVecElemPtr, "", originalJump);

  CallInst * randomizeCall =
      CallInst::Create(randomize, loadArg, "", originalJump);

  GetElementPtrInst * choiceVecElemPtr =
      GetElementPtrInst::Create(choiceVector, indexes, "", originalJump);
  choiceVecElemPtr->setIsInBounds(true);
  newIdxValue = new StoreInst(randomizeCall, choiceVecElemPtr, originalJump);

  BranchInst * jumpBackToCond = BranchInst::Create(loopCondition);
  ReplaceInstWithInst(originalJump, jumpBackToCond);

  return true;
}


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
    BasicBlock * originalBlock = SplitBlock(upperBlock, i, P);
    originalBlock->setName("");
    BasicBlock * lowerBlock = SplitBlock(originalBlock, i->getNextNode(), P);
    lowerBlock->setName("");

    IntegerType * Int32Ty = Type::getInt32Ty(F.getContext());
    Value * choiceVector = F.getValueSymbolTable().lookup("choice.vector");
    assert(choiceVector && "choice.vector not found!");
    ConstantInt * zero = ConstantInt::get(Int32Ty, 0);
    ConstantInt * index = ConstantInt::get(Int32Ty, getInstTy(i));
    std::vector<Value *> indexes;
    indexes.push_back(zero);
    indexes.push_back(index);
    GetElementPtrInst * choicePtr =
        GetElementPtrInst::Create(choiceVector, indexes, "",
                                  upperBlock->getTerminator());
    LoadInst * choice =
        new LoadInst(choicePtr, "", upperBlock->getTerminator());
    SwitchInst * test = SwitchInst::Create(choice, originalBlock, 0);
    ReplaceInstWithInst(upperBlock->getTerminator(), test);

    test->addCase(zero, originalBlock);
    PHINode * phi = PHINode::Create(i->getType(), 0);
    i->replaceAllUsesWith(phi);
    phi->addIncoming(originalBlock->begin(), originalBlock);

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
