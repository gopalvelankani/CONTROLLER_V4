# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build

# Include any dependencies generated for this target.
include examples/CMakeFiles/commandline.dir/depend.make

# Include the progress variables for this target.
include examples/CMakeFiles/commandline.dir/progress.make

# Include the compile flags for this target's objects.
include examples/CMakeFiles/commandline.dir/flags.make

examples/CMakeFiles/commandline.dir/commandline.cc.o: examples/CMakeFiles/commandline.dir/flags.make
examples/CMakeFiles/commandline.dir/commandline.cc.o: ../examples/commandline.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object examples/CMakeFiles/commandline.dir/commandline.cc.o"
	cd /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/examples && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/commandline.dir/commandline.cc.o -c /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/examples/commandline.cc

examples/CMakeFiles/commandline.dir/commandline.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/commandline.dir/commandline.cc.i"
	cd /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/examples && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/examples/commandline.cc > CMakeFiles/commandline.dir/commandline.cc.i

examples/CMakeFiles/commandline.dir/commandline.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/commandline.dir/commandline.cc.s"
	cd /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/examples && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/examples/commandline.cc -o CMakeFiles/commandline.dir/commandline.cc.s

examples/CMakeFiles/commandline.dir/commandline.cc.o.requires:

.PHONY : examples/CMakeFiles/commandline.dir/commandline.cc.o.requires

examples/CMakeFiles/commandline.dir/commandline.cc.o.provides: examples/CMakeFiles/commandline.dir/commandline.cc.o.requires
	$(MAKE) -f examples/CMakeFiles/commandline.dir/build.make examples/CMakeFiles/commandline.dir/commandline.cc.o.provides.build
.PHONY : examples/CMakeFiles/commandline.dir/commandline.cc.o.provides

examples/CMakeFiles/commandline.dir/commandline.cc.o.provides.build: examples/CMakeFiles/commandline.dir/commandline.cc.o


# Object files for target commandline
commandline_OBJECTS = \
"CMakeFiles/commandline.dir/commandline.cc.o"

# External object files for target commandline
commandline_EXTERNAL_OBJECTS =

examples/commandline: examples/CMakeFiles/commandline.dir/commandline.cc.o
examples/commandline: examples/CMakeFiles/commandline.dir/build.make
examples/commandline: examples/CMakeFiles/commandline.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable commandline"
	cd /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/commandline.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/CMakeFiles/commandline.dir/build: examples/commandline

.PHONY : examples/CMakeFiles/commandline.dir/build

examples/CMakeFiles/commandline.dir/requires: examples/CMakeFiles/commandline.dir/commandline.cc.o.requires

.PHONY : examples/CMakeFiles/commandline.dir/requires

examples/CMakeFiles/commandline.dir/clean:
	cd /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/examples && $(CMAKE_COMMAND) -P CMakeFiles/commandline.dir/cmake_clean.cmake
.PHONY : examples/CMakeFiles/commandline.dir/clean

examples/CMakeFiles/commandline.dir/depend:
	cd /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/examples /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/examples /home/user/Music/new_build_git/CONTROLLER_V4/pistache-master/build/examples/CMakeFiles/commandline.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/CMakeFiles/commandline.dir/depend
