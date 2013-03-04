##===- Makefile --------------------------------------------*- Makefile -*-===##
#
# Main Makefile.
#
##===----------------------------------------------------------------------===##

#
# Indicates our relative path to the top of the project's root directory.
#
LEVEL = .
DIRS = lib tools
EXTRA_DIST = include test

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common
