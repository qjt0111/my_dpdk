# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/qjt/my_dpdk

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/qjt/my_dpdk/build

# Include any dependencies generated for this target.
include crc32/CMakeFiles/crc_key.dir/depend.make

# Include the progress variables for this target.
include crc32/CMakeFiles/crc_key.dir/progress.make

# Include the compile flags for this target's objects.
include crc32/CMakeFiles/crc_key.dir/flags.make

crc32/CMakeFiles/crc_key.dir/crc_key.c.o: crc32/CMakeFiles/crc_key.dir/flags.make
crc32/CMakeFiles/crc_key.dir/crc_key.c.o: ../crc32/crc_key.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/qjt/my_dpdk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object crc32/CMakeFiles/crc_key.dir/crc_key.c.o"
	cd /home/qjt/my_dpdk/build/crc32 && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/crc_key.dir/crc_key.c.o   -c /home/qjt/my_dpdk/crc32/crc_key.c

crc32/CMakeFiles/crc_key.dir/crc_key.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/crc_key.dir/crc_key.c.i"
	cd /home/qjt/my_dpdk/build/crc32 && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/qjt/my_dpdk/crc32/crc_key.c > CMakeFiles/crc_key.dir/crc_key.c.i

crc32/CMakeFiles/crc_key.dir/crc_key.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/crc_key.dir/crc_key.c.s"
	cd /home/qjt/my_dpdk/build/crc32 && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/qjt/my_dpdk/crc32/crc_key.c -o CMakeFiles/crc_key.dir/crc_key.c.s

# Object files for target crc_key
crc_key_OBJECTS = \
"CMakeFiles/crc_key.dir/crc_key.c.o"

# External object files for target crc_key
crc_key_EXTERNAL_OBJECTS =

crc32/libcrc_key.a: crc32/CMakeFiles/crc_key.dir/crc_key.c.o
crc32/libcrc_key.a: crc32/CMakeFiles/crc_key.dir/build.make
crc32/libcrc_key.a: crc32/CMakeFiles/crc_key.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/qjt/my_dpdk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libcrc_key.a"
	cd /home/qjt/my_dpdk/build/crc32 && $(CMAKE_COMMAND) -P CMakeFiles/crc_key.dir/cmake_clean_target.cmake
	cd /home/qjt/my_dpdk/build/crc32 && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/crc_key.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
crc32/CMakeFiles/crc_key.dir/build: crc32/libcrc_key.a

.PHONY : crc32/CMakeFiles/crc_key.dir/build

crc32/CMakeFiles/crc_key.dir/clean:
	cd /home/qjt/my_dpdk/build/crc32 && $(CMAKE_COMMAND) -P CMakeFiles/crc_key.dir/cmake_clean.cmake
.PHONY : crc32/CMakeFiles/crc_key.dir/clean

crc32/CMakeFiles/crc_key.dir/depend:
	cd /home/qjt/my_dpdk/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/qjt/my_dpdk /home/qjt/my_dpdk/crc32 /home/qjt/my_dpdk/build /home/qjt/my_dpdk/build/crc32 /home/qjt/my_dpdk/build/crc32/CMakeFiles/crc_key.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : crc32/CMakeFiles/crc_key.dir/depend

