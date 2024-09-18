LLVM_PATH := $(shell llvm-config --bindir)
CXX := ${LLVM_PATH}/clang++
CXX_FLAGS := $(shell llvm-config --cxxflags)
LD_FLAGS := $(shell llvm-config --ldflags)
LIBS := $(shell llvm-config --libs)

# Detect the operating system
UNAME_S := $(shell uname -s)

# Set platform-specific variables
ifeq ($(UNAME_S),Darwin)
    SHARED_LIB_EXT := dylib
    SHARED_LIB_FLAG := -dynamiclib
    PLATFORM_FLAGS := -mmacosx-version-min=13.6.0
else
    SHARED_LIB_EXT := so
    SHARED_LIB_FLAG := -shared
    PLATFORM_FLAGS :=
endif

# Update CXX_FLAGS with platform-specific flags
CXX_FLAGS += $(PLATFORM_FLAGS)

TARGET := NodeMatcherPlugin.$(SHARED_LIB_EXT)

$(TARGET): Query.o QueryParser.o NodeMatcherPlugin.o Query.h QueryParser.h QuerySession.h
	$(CXX) $(CXX_FLAGS) $(LD_FLAGS) $(LIBS) $(SHARED_LIB_FLAG) ./NodeMatcherPlugin.o ./QueryParser.o ./Query.o \
		-lclang-cpp \
		-o $@

.cpp.o:
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)

.PHONY: clean
