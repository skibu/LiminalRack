ifndef RACK_DIR
$(error RACK_DIR is not defined)
endif

include $(RACK_DIR)/arch.mk

OBJCOPY ?= objcopy
STRIP ?= strip
INSTALL_NAME_TOOL ?= install_name_tool
OTOOL ?= otool

# Generate dependency files alongside the object files
FLAGS += -MMD -MP
# Debugger symbols. These are removed with `strip`.
# For development, using -g3 to include macro definitions in 
# debug info. But for production should use -g1 for minimal 
# debug info or -g2 (same as -g) for standard debug info.
FLAGS += -g3
# Optimization. -Og is good for development because it does 
# some optimizations but does not hinder debugging. For production
# builds should use -O3 for maximum optimization.
FLAGS += -Og -funsafe-math-optimizations -fno-omit-frame-pointer
# Warnings
FLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-vla-extension
# Needed because of hack at dep/include/nanovg_gl_utils.h:46:11
#FLAGS += -Wmacro-redefined
# C++ standard
CXXFLAGS += -std=c++17

# Define compiler/linker target if cross-compiling
ifdef CROSS_COMPILE
	FLAGS += --target=$(MACHINE)
endif

# Architecture-independent flags
ifdef ARCH_X64
	FLAGS += -march=nehalem
endif
ifdef ARCH_ARM64
	FLAGS += -march=armv8-a+fp+simd
endif

ifdef ARCH_LIN
	CXXFLAGS += -Wsuggest-override
# Unfortunately no way to tell if raspberry pi so turning on 
# WAYLAND_TOUCHSCREEN_SUPPORT for all linux builds
	CXXFLAGS += -DWAYLAND_TOUCHSCREEN_SUPPORT
endif
ifdef ARCH_MAC
	CXXFLAGS += -stdlib=libc++
	MAC_SDK_FLAGS := -mmacosx-version-min=10.9
#       -Wmacro-redefined needed because of hack at dep/include/nanovg_gl_utils.h:46:11
        FLAGS += $(MAC_SDK_FLAGS) -Wmacro-redefined
endif
ifdef ARCH_WIN
	FLAGS += -D_USE_MATH_DEFINES
	FLAGS += -municode
	CXXFLAGS += -Wsuggest-override
endif

# Allow *appending* rather than prepending to common flags.
# This is useful to force-redefine compiler settings instead of merely setting defaults that may be overwritten.
FLAGS += $(EXTRA_FLAGS)
CFLAGS += $(EXTRA_CFLAGS)
CXXFLAGS += $(EXTRA_CXXFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

# Apply FLAGS to language-specific flags
CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)

# Derive object files from sources and place them before user-defined objects
OBJECTS := $(patsubst %, build/%.o, $(SOURCES)) $(OBJECTS)
OBJECTS += $(patsubst %, build/%.bin.o, $(BINARIES))
DEPENDENCIES := $(patsubst %, build/%.d, $(SOURCES))

# Final targets

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

-include $(DEPENDENCIES)

build/%.c.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/%.cc.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/%.m.o: %.m
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.mm.o: %.mm
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/%.bin.o: %
	@mkdir -p $(@D)
	xxd -i $< | $(CC) $(CFLAGS) -c -o $@ -xc -

build/%.html: %.md
	markdown $< > $@
