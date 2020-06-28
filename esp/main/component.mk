#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

SRC_DIR := ../../lib/m8rscript/src

CXXFLAGS += -std=c++14

COMPONENT_ADD_INCLUDEDIRS := $(SRC_DIR) $(SRC_DIR)/littlefs
COMPONENT_EXTRA_INCLUDES := 
COMPONENT_SRCDIRS := . $(SRC_DIR) $(SRC_DIR)/littlefs
COMPONENT_OBJS := m8rscript.o \
    RtosGPIOInterface.o \
    RtosSystemInterface.o \
    RtosTCP.o \
    RtosWifi.o \
    $(SRC_DIR)/Application.o \
    $(SRC_DIR)/Atom.o \
    $(SRC_DIR)/Base64.o \
    $(SRC_DIR)/Closure.o \
    $(SRC_DIR)/CodePrinter.o \
    $(SRC_DIR)/Error.o \
    $(SRC_DIR)/ExecutionUnit.o \
    $(SRC_DIR)/Function.o \
    $(SRC_DIR)/GC.o \
    $(SRC_DIR)/Global.o \
    $(SRC_DIR)/GPIO.o \
    $(SRC_DIR)/HTTPServer.o \
    $(SRC_DIR)/IPAddr.o \
    $(SRC_DIR)/Iterator.o \
    $(SRC_DIR)/JSON.o \
    $(SRC_DIR)/Mallocator.o \
    $(SRC_DIR)/MFS.o \
    $(SRC_DIR)/MString.o \
    $(SRC_DIR)/MUDP.o \
    $(SRC_DIR)/Object.o \
    $(SRC_DIR)/Parser.o \
    $(SRC_DIR)/ParseEngine.o \
    $(SRC_DIR)/Program.o \
    $(SRC_DIR)/ROMString.o \
    $(SRC_DIR)/Scanner.o \
    $(SRC_DIR)/GeneratedValues.o \
    $(SRC_DIR)/Shell.o \
    $(SRC_DIR)/SystemInterface.o \
    $(SRC_DIR)/SystemTime.o \
    $(SRC_DIR)/Task.o \
    $(SRC_DIR)/TaskManager.o \
    $(SRC_DIR)/TCP.o \
    $(SRC_DIR)/TCPServer.o \
    $(SRC_DIR)/Telnet.o \
    $(SRC_DIR)/Terminal.o \
    $(SRC_DIR)/Timer.o \
    $(SRC_DIR)/Value.o \
    $(SRC_DIR)/slre.o \
    $(SRC_DIR)/littlefs/MLittleFS.o \
    $(SRC_DIR)/littlefs/lfs.o \
    $(SRC_DIR)/littlefs/lfs_util.o \
