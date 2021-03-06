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


#ifndef _CODEMORPHING_H_
#define _CODEMORPHING_H_

#include "llvm/Pass.h"

namespace cmp
{

class CodeMorphing : public llvm::FunctionPass
{
 public:
  static char ID;

  CodeMorphing();

  bool runOnFunction(llvm::Function &F);

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

  const char * getPassName() const
  {
    return "Code Morphing Pass";
  }
};

}

#endif /* _CODEMORPHING_H_ */
