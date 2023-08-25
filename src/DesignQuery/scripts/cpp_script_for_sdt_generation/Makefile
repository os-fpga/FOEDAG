FILENAME = main_sdtgen_cpp_nlohman_lib_v5
	# in order to run this make the below variable DEPENDENCY = 1
	# further modularizing the code with a separate main file

DEPENDENCY_FILE = sdtgen_cpp_nlohman_lib_v5

DEPENDENCY = 1

SOURCE	= ./$(FILENAME).cpp

# LIBS = -lm -lSDL -lpthread -ldl

LIBS = -lm

all: executable
debug: debugging


.PHONY: all reset_handler

ifeq ($(DEPENDENCY), 1)
executable: DEPENDENCY_FILE_INCLUDED_RULE
else 
executable: $(FILENAME)
endif

debugging: $(SOURCE)
	g++ -Wall -Wextra -pedantic-errors -ggdb -O2 $< -o $(FILENAME)  && gdb $(FILENAME)
	# look at debugging comment at the bottom

DEPENDENCY_FILE_INCLUDED_RULE: $(FILENAME).o $(DEPENDENCY_FILE).o
	g++ -std=c++17 -o $(FILENAME) $(FILENAME).o $(DEPENDENCY_FILE).o && ./$(FILENAME) 

$(FILENAME).o: $(FILENAME).cpp $(DEPENDENCY_FILE).h
	g++ -std=c++17 -c $(FILENAME).cpp

$(DEPENDENCY_FILE).o: $(DEPENDENCY_FILE).cpp $(DEPENDENCY_FILE).h
	g++ -std=c++17 -c $(DEPENDENCY_FILE).cpp
	 

clean:
	-rm $(FILENAME)

# # Debugging using GDB
# 			§ https://cs.franklin.edu/~shaffstj/oldcs2/debugging.htm#:~:text=Use%20a%20symbolic%20debugger%3A&text=You%20can%20examine%20the%20contents,code%20that%20caused%20the%20crash.
# 				□ v cool for line by line debugging of C++ code using breakpoint from gdb API
# 				□ used -g flag to use gdb functionality after checking that gdb is installed in my Ubuntu OS
# 				□ use "break (line number) to set up break points.
# 				□ Use "run" to start prog execution until first breakpoint
# 				□ Then use "next" to step through breakpoints.
# more functionality can be checked here https://web.eecs.umich.edu/~sugih/pointers/summary.html#:~:text=Gdb%20is%20a%20debugger%20for,variable%20after%20executing%20each%20line.
