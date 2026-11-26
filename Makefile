# ERL-HUB build file. Works with GNU make / mingw32-make.
ifeq ($(OS),Windows_NT)
  EXE := .exe
  MKDIRS = if not exist build mkdir build & if not exist bin mkdir bin
  CLEAN_OBJS := build\*.o
  CLEAN_TARGET := bin\erlhub.exe
  CLEAN_TESTS := bin\tests.exe
  CLEAN_ASAN := bin\erlhub_asan.exe
  CLEAN_ASANTESTS := bin\tests_asan.exe
  CLEAN_DEBUGSTL := bin\tests_debugstl.exe
  RM := del /Q
else
  EXE :=
  MKDIRS = mkdir -p build bin
  CLEAN_OBJS := build/*.o
  CLEAN_TARGET := bin/erlhub
  CLEAN_TESTS := bin/tests
  CLEAN_ASAN := bin/erlhub_asan
  CLEAN_ASANTESTS := bin/tests_asan
  CLEAN_DEBUGSTL := bin/tests_debugstl
  RM := rm -f
endif

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -Iinclude
LDFLAGS  ?=

SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
TARGET := bin/erlhub$(EXE)
TEST_TARGET := bin/tests$(EXE)

.PHONY: all tests run-tests asan asan-tests clean check-dir

all: $(TARGET) tests

check-dir:
	@$(MKDIRS)

$(TARGET): check-dir main.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) main.cpp $(OBJS) $(LDFLAGS) -o $@

tests: $(TEST_TARGET)

$(TEST_TARGET): check-dir tests/test_hub.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_hub.cpp $(OBJS) $(LDFLAGS) -o $@

build/%.o: src/%.cpp check-dir
	$(CXX) $(CXXFLAGS) -c $< -o $@

run-tests: tests
	$(TEST_TARGET)

# AddressSanitizer + UBSan builds (both the app and the test harness).
asan: check-dir
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 main.cpp $(SRCS) -o bin/erlhub_asan$(EXE)

asan-tests: check-dir
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 tests/test_hub.cpp $(SRCS) -o bin/tests_asan$(EXE)

# Portable alternative when sanitizer runtimes are unavailable.
debugstl: check-dir
	$(CXX) $(CXXFLAGS) -D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS -g tests/test_hub.cpp $(SRCS) -o bin/tests_debugstl$(EXE)

clean:
	-$(RM) $(CLEAN_OBJS) $(CLEAN_TARGET) $(CLEAN_TESTS) $(CLEAN_ASAN) $(CLEAN_ASANTESTS) $(CLEAN_DEBUGSTL)
