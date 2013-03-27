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


#ifndef _INSTRUCTIONALTERNATIVEUTILS_H_
#define _INSTRUCTIONALTERNATIVEUTILS_H_

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"


namespace cmp
{

// We are intrested in giving alternatives to a subset of the LLVM IR
// instructions. This enumeration defines the considered subset.
// *DO NOT* put holes in the enumeration.
enum InstructionTy
{
  FirstInstructionTy = 0,
  Xor = FirstInstructionTy,
  LastInstructionTy
};

template <unsigned Type> class NumberOfAlternativesFor;

#define CMP_SET_ALTERNATIVE_NUMBER(Inst, Num)   \
  template <>                                   \
  class NumberOfAlternativesFor<Inst>           \
  {                                             \
   public:                                      \
    static int const value = Num;               \
  }

// Alternative Number table
CMP_SET_ALTERNATIVE_NUMBER(Xor, 1);
// End of Alternative Number table

#undef CMP_SET_ALTERNATIVE_NUMBER


InstructionTy
getInstTy(llvm::Instruction *);


namespace
{

struct InitializeAlternativesVectorBase
{
  typedef llvm::SmallVector<llvm::Constant *, LastInstructionTy> vector_type;
  typedef llvm::Type * element_type_ptr;
};

}


// This Meta-Function loops over the element of the enum InstructionTy
// and insert inside the vector passed the initializer of the
// Alternative Vector. The loop is not built in the usual way going
// from MAX to MIN but in the opposite way. The reason is that going
// this way we can use push_back to insert the element in the correct
// order. In the other way around we should have used push_front that
// for every insertion need to move forward all the elements already
// in the vector.
template <unsigned InstTy = FirstInstructionTy>
struct  InitializeAlternativesVector : InitializeAlternativesVectorBase
{
  static void exec(vector_type & SV, element_type_ptr ElementTy)
  {
    llvm::Constant * value =
        llvm::Constant::getIntegerValue(
            ElementTy,
            llvm::APInt(ElementTy->getIntegerBitWidth(),
                        NumberOfAlternativesFor<InstTy>::value));
    SV.push_back(value);
    InitializeAlternativesVector<InstTy + 1>::exec(SV, ElementTy);
  }
};

template <>
struct InitializeAlternativesVector<LastInstructionTy>
    : InitializeAlternativesVectorBase
{
  static void exec(vector_type & SV, element_type_ptr ElementTy)
  {}
};

}

#endif /* _INSTRUCTIONALTERNATIVEUTILS_H_ */
