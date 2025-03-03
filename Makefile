CPPOPT=-g -Og -D_DEBUG
# -O2 -Os -Ofast
# -fprofile-generate -fprofile-use
CPPFLAGS=$(CPPOPT) -Wall -ansi -pedantic -std=c++17
# -Wparentheses -Wno-unused-parameter -Wformat-security
# -fno-rtti -std=c++11 -std=c++98

# documents and scripts
DOCS=Tasks.txt
SCRS=

# headers and code sources
HDRS=	defs.h \
		Iterator.h Scan.h Filter.h Sort.h Witness.h \
		DRAM.h HDD.h TreeOfLosers.h
SRCS=	defs.cpp Assert.cpp \
		Iterator.cpp Scan.cpp Filter.cpp Sort.cpp Witness.cpp \
		DRAM.cpp HDD.cpp TreeOfLosers.cpp

# Find all test source files
TEST_SRCS= 	Test0.cpp Test1.cpp Test2.cpp Test3.cpp Test4.cpp Test5.cpp Test6.cpp Test7.cpp \
			Test8.cpp Test9.cpp Test10.cpp Test11.cpp Test12.cpp Test13.cpp Test14.cpp
TEST_EXES=$(TEST_SRCS:.cpp=.exe)

# compilation targets for common objects
OBJS=	defs.o Assert.o \
		Iterator.o Scan.o Filter.o Sort.o Witness.o \
		DRAM.o HDD.o TreeOfLosers.o

# RCS assists
REV=-q -f
MSG=no message

# default target
all: $(TEST_EXES)

# Build rule for each Test*.exe
%.exe: %.cpp $(OBJS)
	g++ $(CPPFLAGS) -o $@ $^ 

# Rule to run all tests
run_all: all
	@for exe in $(TEST_EXES); do \
		echo "\n\n\n\n\nRunning $$exe..."; \
		./$$exe; \
	done

# Dependencies for object files
$(OBJS): Makefile defs.h
Test.o: Iterator.h Scan.h Filter.h Sort.h Witness.h
Iterator.o Scan.o Filter.o Sort.o : Iterator.h
Scan.o : Scan.h
Filter.o : Filter.h
Sort.o : Sort.h
Witness.o : Witness.h
DRAM.o : DRAM.h
HDD.o : HDD.h
TreeOfLosers.o: TreeOfLosers.h

# Additional targets
list: Makefile
	echo Makefile $(HDRS) $(SRCS) $(TEST_SRCS) $(DOCS) $(SCRS) > list
count: list
	@wc `cat list`

ci:
	ci $(REV) -m"$(MSG)" $(HDRS) $(SRCS) $(TEST_SRCS) $(DOCS) $(SCRS)
	ci -l $(REV) -m"$(MSG)" Makefile
co:
	co $(REV) -l $(HDRS) $(SRCS) $(TEST_SRCS) $(DOCS) $(SCRS)

clean:
	@rm -rf $(OBJS) $(TEST_EXES) *.stackdump trace *.o *.exe *.exe.dSYM
