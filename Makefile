CXX := clang++

VERSION_HEADER := base/version.generated.h

#TESTING CRAP REMOVE
b.test : a.test
	echo "b?"
	if test -e b.test; then echo "b exists"; else touch b.test; echo "made b"; fi
c.test : b.test
	echo "making c"
	touch c.test

PLUGIN_TRANSLATION_UNITS      := $(wildcard ksp_plugin/*.cpp)
PLUGIN_TEST_TRANSLATION_UNITS := $(wildcard ksp_plugin_test/*_test.cpp) $(wildcard ksp_plugin_test/mock_*.cpp)
JOURNAL_TRANSLATION_UNITS     := $(wildcard journal/*.cpp)
TEST_TRANSLATION_UNITS        := $(wildcard */*_test.cpp) $(wildcard */mock_*.cpp)
TOOLS_TRANSLATION_UNITS       := $(wildcard tools/*.cpp)
NON_TEST_TRANSLATION_UNITS    := $(filter-out $(TEST_TRANSLATION_UNITS), $(wildcard */*.cpp))
PROTO_FILES                   := $(wildcard */*.proto)
PROTO_TRANSLATION_UNITS       := $(PROTO_FILES:.proto=.pb.cc)
PROTO_HEADERS                 := $(PROTO_FILES:.proto=.pb.h)

GENERATED_PROFILES :=                    \
	journal/profiles.generated.h     \
	journal/profiles.generated.cc    \
	journal/player.generated.cc      \
	ksp_plugin/interface.generated.h \
	ksp_plugin_adapter/interface.generated.cs

TEST_DIRS := astronomy base geometry integrators journal ksp_plugin_test numerics physics quantities testing_utilities
TEST_BINS := $(addsuffix /test,$(TEST_DIRS))

PROJECT_DIR := ksp_plugin_adapter/
SOLUTION_DIR := ./
ADAPTER_BUILD_DIR := ksp_plugin_adapter/obj
ADAPTER_CONFIGURATION := Debug
FINAL_PRODUCTS_DIR := Debug
ADAPTER := $(ADAPTER_BUILD_DIR)/$(ADAPTER_CONFIGURATION)/ksp_plugin_adapter.dll

LIB_DIR := $(FINAL_PRODUCTS_DIR)/GameData/Principia/Linux64
LIB := $(LIB_DIR)/principia.so

DEP_DIR := deps
LIBS := $(DEP_DIR)/protobuf/src/.libs/libprotobuf.a $(DEP_DIR)/glog/.libs/libglog.a -lpthread -lc++ -lc++abi
TEST_INCLUDES := -I$(DEP_DIR)/googlemock/include -I$(DEP_DIR)/googletest/include -I$(DEP_DIR)/googlemock/ -I$(DEP_DIR)/googletest/ 
INCLUDES := -I. -I$(DEP_DIR)/glog/src -I$(DEP_DIR)/protobuf/src -I$(DEP_DIR)/benchmark/include -I$(DEP_DIR)/Optional -I$(DEP_DIR)/eggsperimental_filesystem/
SHARED_ARGS := -std=c++14 -stdlib=libc++ -O3 -g -fPIC -fexceptions -ferror-limit=1 -fno-omit-frame-pointer -Wall -Wpedantic \
	-DPROJECT_DIR='std::experimental::filesystem::path("$(PROJECT_DIR)")'\
	-DSOLUTION_DIR='std::experimental::filesystem::path("$(SOLUTION_DIR)")' \
	-DNDEBUG

COMPILER_OPTIONS = -c $(SHARED_ARGS) $(INCLUDES)

BUILD_DIRECTORY := build/
BIN_DIRECTORY   := bin/

# We don't do dependency resolution on the protos; we compile them all at once.
$(PROTO_HEADERS) $(PROTO_TRANSLATION_UNITS): $(PROTO_FILES)
	$(DEP_DIR)/protobuf/src/protoc -I $(DEP_DIR)/protobuf/src/ -I . $^ --cpp_out=.

TEST_DEPENDENCIES        := $(addprefix $(BUILD_DIRECTORY), $(TEST_TRANSLATION_UNITS:.cpp=.d))
TOOLS_DEPENDENCIES       := $(addprefix $(BUILD_DIRECTORY), $(TOOLS_TRANSLATION_UNITS:.cpp=.d))
NON_TEST_DEPENDENCIES    := $(addprefix $(BUILD_DIRECTORY), $(NON_TEST_TRANSLATION_UNITS:.cpp=.d))
PLUGIN_DEPENDENCIES      := $(addprefix $(BUILD_DIRECTORY), $(PLUGIN_TRANSLATION_UNITS:.cpp=.d))
PLUGIN_TEST_DEPENDENCIES := $(addprefix $(BUILD_DIRECTORY), $(PLUGIN_TEST_TRANSLATION_UNITS:.cpp=.d))
JOURNAL_DEPENDENCIES     := $(addprefix $(BUILD_DIRECTORY), $(JOURNAL_TRANSLATION_UNITS:.cpp=.d))

# As a prerequisite for listing the includes of things that depend on
# generated headers, we must generate said code.
# Note that the prerequisites for dependency files are order-only: once
# we have the dependency files, their own actual dependency on generated headers
# and on the translation unit will be listed there and recomputed as needed.
$(PLUGIN_DEPENDENCIES)      : | $(GENERATED_PROFILES)
$(PLUGIN_TEST_DEPENDENCIES) : | $(GENERATED_PROFILES)
$(JOURNAL_DEPENDENCIES)     : | $(GENERATED_PROFILES)

$(NON_TEST_DEPENDENCIES): $(BUILD_DIRECTORY)%.d: %.cpp | $(PROTO_HEADERS)
	@mkdir -p $(@D)
	$(CXX) -M $(COMPILER_OPTIONS) $< > $@.temp
	sed 's!.*\.o[ :]*!$(BUILD_DIRECTORY)$*.o $@ : !g' < $@.temp > $@
	rm -f $@.temp

$(TEST_DEPENDENCIES): $(BUILD_DIRECTORY)%.d: %.cpp | $(PROTO_HEADERS)
	@mkdir -p $(@D)
	$(CXX) -M $(COMPILER_OPTIONS) $(TEST_INCLUDES) $< > $@.temp
	sed 's!.*\.o[ :]*!$(BUILD_DIRECTORY)$*.o $@ : !g' < $@.temp > $@
	rm -f $@.temp

TEST_OBJECTS := $(addprefix $(BUILD_DIRECTORY), $(TEST_TRANSLATION_UNITS:.cpp=.o))
NON_TEST_OBJECTS := $(addprefix $(BUILD_DIRECTORY), $(NON_TEST_TRANSLATION_UNITS:.cpp=.o))
TOOLS_OBJECTS := $(addprefix $(BUILD_DIRECTORY), $(TOOLS_TRANSLATION_UNITS:.cpp=.o))
PROTO_OBJECTS := $(addprefix $(BUILD_DIRECTORY), $(PROTO_TRANSLATION_UNITS:.cc=.o))

include $(NON_TEST_DEPENDENCIES)
include $(TEST_DEPENDENCIES)

TOOLS_BIN := $(BIN_DIRECTORY)tools

$(TOOLS_BIN): $(TOOLS_OBJECTS) $(PROTO_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

$(GENERATED_PROFILES) : $(TOOLS_BIN)
	$^ generate_profiles

$(TEST_OBJECTS): $(BUILD_DIRECTORY)%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(COMPILER_OPTIONS) $(TEST_INCLUDES) $< -o $@

$(NON_TEST_OBJECTS): $(BUILD_DIRECTORY)%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(COMPILER_OPTIONS) $(INCLUDES) $< -o $@

$(PROTO_OBJECTS): $(BUILD_DIRECTORY)%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(COMPILER_OPTIONS) $(INCLUDES) $< -o $@

# detect OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    UNAME_M := $(shell uname -m)
    ifeq ($(UNAME_M),x86_64)
        SHARED_ARGS += -m64
    else
        SHARED_ARGS += -m32
    endif
    MDTOOL := mdtool
endif
ifeq ($(UNAME_S),Darwin)
    SHARED_ARGS += -mmacosx-version-min=10.7 -arch i386
    MDTOOL ?= "/Applications/Xamarin Studio.app/Contents/MacOS/mdtool"
endif

CXXFLAGS := -c $(SHARED_ARGS) $(INCLUDES)
LDFLAGS := $(SHARED_ARGS)


.PHONY: all adapter lib tests tools check plugin run_tests clean
.PRECIOUS: %.o $(PROTO_HEADERS) $(PROTO_CC_SOURCES) $(GENERATED_SOURCES)
.DEFAULT_GOAL := plugin
.SUFFIXES:

##### CONVENIENCE TARGETS #####
all: $(LIB) $(ADAPTER) tests

adapter: $(ADAPTER)
lib: $(LIB)

tests: $(TEST_BINS)

tools: $(TOOLS_BIN)

check: run_tests

##### CORE #####
$(ADAPTER): $(GENERATED_SOURCES)
	$(MDTOOL) build -c:$(ADAPTER_CONFIGURATION) ksp_plugin_adapter/ksp_plugin_adapter.csproj

#$(TOOLS_BIN): $(PROTO_OBJECTS) $(TOOLS_OBJECTS) $(STATUS_OBJECTS)
#	$(CXX) $(LDFLAGS) $(PROTO_OBJECTS) $(TOOLS_OBJECTS) -o $(TOOLS_BIN) $(LIBS)

.SECONDEXPANSION:
$(LIB): $(PROTO_OBJECTS) $$(ksp_plugin_objects) $$(journal_objects) $(LIB_DIR) $(STATUS_OBJECTS)
	$(CXX) -shared $(LDFLAGS) $(PROTO_OBJECTS) $(STATUS_OBJECTS) $(ksp_plugin_objects) $(journal_objects) -o $(LIB) $(LIBS)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(VERSION_HEADER): .git
	./generate_version_header.sh

$(GENERATED_SOURCES): $(TOOLS_BIN) serialization/journal.proto
	tools/tools generate_profiles

#%.pb.cc %.pb.h: %.proto
#	$(DEP_DIR)/protobuf/src/protoc -I $(DEP_DIR)/protobuf/src/ -I . $< --cpp_out=.

%.pb.o: %.pb.cc $(PROTO_HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@ 

tools/%.o: tools/%.cpp $(VERSION_HEADER) $(PROTO_HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

%.o: %.cpp $(VERSION_HEADER) $(PROTO_HEADERS) $(GENERATED_SOURCES)
	$(CXX) $(CXXFLAGS) $< -o $@ 

%.o: %.cc $(VERSION_HEADER) $(PROTO_HEADERS) $(GENERATED_SOURCES)
	$(CXX) $(CXXFLAGS) $< -o $@ 

##### DISTRIBUTION #####
plugin: $(ADAPTER) $(LIB)
	cd $(FINAL_PRODUCTS_DIR); zip -r Principia-$(UNAME_S)-$(shell git describe)-$(shell date "+%Y-%m-%d").zip GameData/

##### TESTS #####
run_tests: tests
	@echo "Cake, and grief counseling, will be available at the conclusion of the test."
	-astronomy/test
	-base/test
	-geometry/test
	-integrators/test
	-ksp_plugin_test/test
	-physics/test
	-quantities/test
	-testing_utilities/test
	-numerics/test

TEST_LIBS=$(DEP_DIR)/protobuf/src/.libs/libprotobuf.a $(DEP_DIR)/glog/.libs/libglog.a -lpthread

GMOCK_SOURCE=$(DEP_DIR)/googlemock/src/gmock-all.cc $(DEP_DIR)/googlemock/src/gmock_main.cc $(DEP_DIR)/googletest/src/gtest-all.cc
GMOCK_OBJECTS=$(GMOCK_SOURCE:.cc=.o)

test_objects = $(patsubst %.cpp,%.o,$(wildcard $(@D)/*.cpp))
ksp_plugin_objects = $(patsubst %.cpp,%.o,$(wildcard ksp_plugin/*.cpp))
journal_objects = journal/profiles.o journal/recorder.o

# We need to special-case ksp_plugin_test and journal because they require object files from ksp_plugin
# and journal.  The other tests don't do this.
.SECONDEXPANSION:
ksp_plugin_test/test: $$(ksp_plugin_objects) $$(journal_objects) $$(test_objects) $(GMOCK_OBJECTS) $(PROTO_OBJECTS) $(STATUS_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(TEST_LIBS) -o $@

# We cannot link the player test because we do not have the benchmarks.  We only build the recorder test.
.SECONDEXPANSION:
journal/test: $$(ksp_plugin_objects) $$(journal_objects) journal/player.o journal/recorder_test.o $(GMOCK_OBJECTS) $(PROTO_OBJECTS) $(STATUS_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(TEST_LIBS) -o $@

.SECONDEXPANSION:
%/test: $$(test_objects) $(GMOCK_OBJECTS) $(PROTO_OBJECTS) $(STATUS_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(TEST_LIBS) -o $@

##### CLEAN #####
clean:
	rm -rf $(ADAPTER_BUILD_DIR) $(FINAL_PRODUCTS_DIR)
	rm -f $(LIB) $(VERSION_HEADER) $(PROTO_HEADERS) $(PROTO_CC_SOURCES) $(GENERATED_SOURCES) $(TEST_BINS) $(TOOLS_BIN) $(LIB) */*.o

##### EVERYTHING #####
# Compiles everything, but does not link anything.  Used to check standard compliance on code that we don't want to run on *nix.
compile_everything: $(patsubst %.cpp,%.o,$(wildcard */*.cpp))

##### IWYU #####
IWYU := deps/include-what-you-use/bin/include-what-you-use
IWYU_FLAGS := -Xiwyu --max_line_length=200 -Xiwyu --mapping_file="iwyu.imp" -Xiwyu --check_also=*/*.hpp
IWYU_NOSAFE_HEADERS := --nosafe_headers
REMOVE_BOM := for f in `ls */*.hpp && ls */*.cpp`; do awk 'NR==1{sub(/^\xef\xbb\xbf/,"")}1' $$f | awk NF{p=1}p > $$f.nobom; mv $$f.nobom $$f; done
RESTORE_BOM := for f in `ls */*.hpp && ls */*.cpp`; do awk 'NR==1{sub(/^/,"\xef\xbb\xbf\n")}1' $$f > $$f.withbom; mv $$f.withbom $$f; done
FIX_INCLUDES := deps/include-what-you-use/bin/fix_includes.py
IWYU_CHECK_ERROR := tee /dev/tty | test ! "`grep ' error: '`"
IWYU_TARGETS := $(wildcard */*.cpp)
IWYU_CLEAN := rm iwyu_generated_mappings.imp; rm */*.iwyu

iwyu_generate_mappings:
	{ ls */*_body.hpp && ls */*.generated.h; } | awk -f iwyu_generate_mappings.awk > iwyu_generated_mappings.imp

%.cpp!!iwyu: iwyu_generate_mappings
	$(IWYU) $(CXXFLAGS) $(subst !SLASH!,/, $*.cpp) $(IWYU_FLAGS) 2>&1 | tee $(subst !SLASH!,/, $*.iwyu) | $(IWYU_CHECK_ERROR)
	$(REMOVE_BOM) 
	$(FIX_INCLUDES) < $(subst !SLASH!,/, $*.iwyu) | cat
	$(RESTORE_BOM)

iwyu: $(subst /,!SLASH!, $(addsuffix !!iwyu, $(IWYU_TARGETS)))
	$(IWYU_CLEAN)

%.cpp!!iwyu_unsafe: iwyu_generate_mappings
	$(IWYU) $(CXXFLAGS) $(subst !SLASH!,/, $*.cpp) $(IWYU_FLAGS) 2>&1 | tee $(subst !SLASH!,/, $*.iwyu) | $(IWYU_CHECK_ERROR)
	$(REMOVE_BOM) 
	$(FIX_INCLUDES) $(IWYU_NOSAFE_HEADERS) < $(subst !SLASH!,/, $*.iwyu) | cat
	$(RESTORE_BOM)

iwyu_unsafe: $(subst /,!SLASH!, $(addsuffix !!iwyu_unsafe, $(IWYU_TARGETS)))
	$(IWYU_CLEAN)

iwyu_clean:
	$(IWYU_CLEAN)

normalize_bom:
	$(REMOVE_BOM)
	$(RESTORE_BOM)
