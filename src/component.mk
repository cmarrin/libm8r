#
# "libm8r" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

CXXFLAGS += -std=c++14

COMPONENT_ADD_INCLUDEDIRS := . littlefs
COMPONENT_EXTRA_INCLUDES := 
COMPONENT_SRCDIRS := . littlefs
COMPONENT_OBJS := \
    Application.o \
    Atom.o \
    Base64.o \
    Containers.o \
    Error.o \
    Executable.o \
    HTTPServer.o \
    IPAddr.o \
    Mallocator.o \
    MFS.o \
    MString.o \
    MUDP.o \
    Scanner.o \
    Shell.o \
    SystemInterface.o \
    SystemTime.o \
    Task.o \
    TaskManager.o \
    TCP.o \
    TCPServer.o \
    Telnet.o \
    Terminal.o \
    Timer.o \
    littlefs/MLittleFS.o \
    littlefs/lfs.o \
    littlefs/lfs_util.o \
    RtosGPIOInterface.o \
    RtosSystemInterface.o \
    RtosTCP.o \
    RtosWifi.o \
