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


#include "cmp/AllPasses.h"

#include <cstdlib>

using namespace cmp;

// I have used the following layout for the project. Each pass is a static
// library. All passes are contained in a loadable module, allowing using them
// with opt. In order to achieve this behaviour, we need to perform some
// black-magic with the linker.
namespace {

// First of all, we have to reference all pass creation functions, in order to
// force linking them inside the loadable module. Using a static object with a
// constructor referencing those functions in an unreachable code section is a
// standard trick.
class ForceLinking {
public:
  ForceLinking() {
    // We must reference the passes in such a way that compilers will not
    // delete it all as dead code, even with whole program optimization,
    // yet is effectively a NO-OP. As the compiler isn't smart enough
    // to know that getenv() never returns -1, this will do the job.
    if(std::getenv("bar") != (char*) -1)
      return;

    // Analysis.

    // Transformations.
    CreateCodeMorphingPass();
    CreateDeclareRandomizeFunctionPass();
  }
};

ForceLinking Link;

// The second step is registering all passes to the LLVM pass registry. We can
// still use the static-object-constructor trick, but this time you have to
// really execute functions!
class ForceInitialization {
public:
  ForceInitialization() {
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();

    // Analysis

    // Dot Viewer Passes

    // Dot Printer Passes

    // Transformations
    initializeCodeMorphingPass(Registry);
    initializeDeclareRandomizeFunctionPass(Registry);
  }
};

ForceInitialization FI;

} // End anonymous namespace
