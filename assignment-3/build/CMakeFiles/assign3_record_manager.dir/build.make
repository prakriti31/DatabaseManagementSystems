# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build

# Include any dependencies generated for this target.
include CMakeFiles/assign3_record_manager.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/assign3_record_manager.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/assign3_record_manager.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/assign3_record_manager.dir/flags.make

CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr.c
CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o -MF CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o.d -o CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr.c

CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr.c > CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.i

CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr.c -o CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.s

CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr_stat.c
CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o -MF CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o.d -o CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr_stat.c

CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr_stat.c > CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.i

CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/buffer_mgr_stat.c -o CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.s

CMakeFiles/assign3_record_manager.dir/dberror.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/dberror.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/dberror.c
CMakeFiles/assign3_record_manager.dir/dberror.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/assign3_record_manager.dir/dberror.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/dberror.c.o -MF CMakeFiles/assign3_record_manager.dir/dberror.c.o.d -o CMakeFiles/assign3_record_manager.dir/dberror.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/dberror.c

CMakeFiles/assign3_record_manager.dir/dberror.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/dberror.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/dberror.c > CMakeFiles/assign3_record_manager.dir/dberror.c.i

CMakeFiles/assign3_record_manager.dir/dberror.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/dberror.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/dberror.c -o CMakeFiles/assign3_record_manager.dir/dberror.c.s

CMakeFiles/assign3_record_manager.dir/expr.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/expr.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/expr.c
CMakeFiles/assign3_record_manager.dir/expr.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/assign3_record_manager.dir/expr.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/expr.c.o -MF CMakeFiles/assign3_record_manager.dir/expr.c.o.d -o CMakeFiles/assign3_record_manager.dir/expr.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/expr.c

CMakeFiles/assign3_record_manager.dir/expr.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/expr.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/expr.c > CMakeFiles/assign3_record_manager.dir/expr.c.i

CMakeFiles/assign3_record_manager.dir/expr.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/expr.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/expr.c -o CMakeFiles/assign3_record_manager.dir/expr.c.s

CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/rm_serializer.c
CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o -MF CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o.d -o CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/rm_serializer.c

CMakeFiles/assign3_record_manager.dir/rm_serializer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/rm_serializer.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/rm_serializer.c > CMakeFiles/assign3_record_manager.dir/rm_serializer.c.i

CMakeFiles/assign3_record_manager.dir/rm_serializer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/rm_serializer.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/rm_serializer.c -o CMakeFiles/assign3_record_manager.dir/rm_serializer.c.s

CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/storage_mgr.c
CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o -MF CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o.d -o CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/storage_mgr.c

CMakeFiles/assign3_record_manager.dir/storage_mgr.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/storage_mgr.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/storage_mgr.c > CMakeFiles/assign3_record_manager.dir/storage_mgr.c.i

CMakeFiles/assign3_record_manager.dir/storage_mgr.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/storage_mgr.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/storage_mgr.c -o CMakeFiles/assign3_record_manager.dir/storage_mgr.c.s

CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/test_assign3_1.c
CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o -MF CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o.d -o CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/test_assign3_1.c

CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/test_assign3_1.c > CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.i

CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/test_assign3_1.c -o CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.s

CMakeFiles/assign3_record_manager.dir/record_mgr.c.o: CMakeFiles/assign3_record_manager.dir/flags.make
CMakeFiles/assign3_record_manager.dir/record_mgr.c.o: /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/record_mgr.c
CMakeFiles/assign3_record_manager.dir/record_mgr.c.o: CMakeFiles/assign3_record_manager.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/assign3_record_manager.dir/record_mgr.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/assign3_record_manager.dir/record_mgr.c.o -MF CMakeFiles/assign3_record_manager.dir/record_mgr.c.o.d -o CMakeFiles/assign3_record_manager.dir/record_mgr.c.o -c /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/record_mgr.c

CMakeFiles/assign3_record_manager.dir/record_mgr.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/assign3_record_manager.dir/record_mgr.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/record_mgr.c > CMakeFiles/assign3_record_manager.dir/record_mgr.c.i

CMakeFiles/assign3_record_manager.dir/record_mgr.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/assign3_record_manager.dir/record_mgr.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/record_mgr.c -o CMakeFiles/assign3_record_manager.dir/record_mgr.c.s

# Object files for target assign3_record_manager
assign3_record_manager_OBJECTS = \
"CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o" \
"CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o" \
"CMakeFiles/assign3_record_manager.dir/dberror.c.o" \
"CMakeFiles/assign3_record_manager.dir/expr.c.o" \
"CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o" \
"CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o" \
"CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o" \
"CMakeFiles/assign3_record_manager.dir/record_mgr.c.o"

# External object files for target assign3_record_manager
assign3_record_manager_EXTERNAL_OBJECTS =

assign3_record_manager: CMakeFiles/assign3_record_manager.dir/buffer_mgr.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/buffer_mgr_stat.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/dberror.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/expr.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/rm_serializer.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/storage_mgr.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/test_assign3_1.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/record_mgr.c.o
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/build.make
assign3_record_manager: CMakeFiles/assign3_record_manager.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Linking C executable assign3_record_manager"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/assign3_record_manager.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/assign3_record_manager.dir/build: assign3_record_manager
.PHONY : CMakeFiles/assign3_record_manager.dir/build

CMakeFiles/assign3_record_manager.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/assign3_record_manager.dir/cmake_clean.cmake
.PHONY : CMakeFiles/assign3_record_manager.dir/clean

CMakeFiles/assign3_record_manager.dir/depend:
	cd /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3 /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3 /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build /home/yash/Desktop/IIT/ADO/fall_2024_32/assignment-3/build/CMakeFiles/assign3_record_manager.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/assign3_record_manager.dir/depend

