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


#ifndef _DECLARERANDOMIZEFUNCTION_H_
#define _DECLARERANDOMIZEFUNCTION_H_

#include <llvm/Pass.h>

namespace cmp
{

class DeclareRandomizeFunction : public llvm::ModulePass
{
 public:
  static char ID;

  DeclareRandomizeFunction();

  bool runOnModule(llvm::Module & M);

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
};

}

#endif /* _DECLARERANDOMIZEFUNCTION_H_ */
