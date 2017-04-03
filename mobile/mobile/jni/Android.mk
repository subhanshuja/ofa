LOCAL_PATH := $(call my-dir)
# Not actually the root but the mobile/ subdirectory...
SRCROOT := $(LOCAL_PATH)/../..

include $(CLEAR_VARS)

ifeq ($(NDK_DEBUG),1)
GENDIR ?= ../bin/debug/include
LOCAL_CFLAGS += -DBREAM_ENTRY_THREAD_CHECK
else
GENDIR ?= ../bin/release/include
endif
GENSRC := $(LOCAL_PATH)/$(GENDIR)

# Makefile.sources will use "$(PROJDIR)/shared/..." to refer to the sources.
# Should be relative to mobile/mobile/jni/
PROJDIR := ../mini/c
include $(LOCAL_PATH)/$(PROJDIR)/shared/core/build/Makefile.sources

GENERIC_SOURCES := \
	$(PROJDIR)/shared/generic/debuginfo/VMDIHelpers.c \
	$(PROJDIR)/shared/generic/pi_impl/Graphics.c \
	$(PROJDIR)/shared/generic/vm_integ/ContextMenuInfo.c \
	$(PROJDIR)/shared/generic/vm_integ/ObjectWrapper.c \
	$(PROJDIR)/shared/generic/vm_integ/ObmlSelectWidget.c \
	$(PROJDIR)/shared/generic/vm_integ/SavedPages.c \
	$(PROJDIR)/shared/generic/vm_integ/ScrollView.c \
	$(PROJDIR)/shared/generic/vm_integ/ThumbnailData.c \
	$(PROJDIR)/shared/generic/vm_integ/TilesContainer.c \
	$(PROJDIR)/shared/generic/vm_integ/VMFunctions.c \
	$(PROJDIR)/shared/generic/vm_integ/VMHelpers.c \
	$(PROJDIR)/shared/generic/vm_integ/VMInstance.c \
	$(PROJDIR)/shared/generic/vm_integ/VMNativeRequest.c \
	$(PROJDIR)/shared/generic/vm_integ/VMString.c

# Reachability.cpp implements MOpReachability_get by its own
# and zlib we already have
CORE_EXCLUDES := %/Reachability.c $(PROJDIR)/shared/core/zlib/%.c

LOCAL_MODULE    := om
LOCAL_SRC_FILES := \
	$(GENERIC_SOURCES) \
	$(filter-out $(CORE_EXCLUDES), $(CORE_SOURCES)) \
	handle_invoke.c \
	src_bream.c \
	src_bream_graphics.c \
	canvascontext.cpp \
	data_store.cpp \
	font.cpp \
	favorites.cpp \
	socket.cpp \
	resolve.cpp \
	HttpHeaders.c \
	GlobalMemory.c \
	LowLevelFile.cpp \
	placeholders.c \
	SystemInfo.cpp \
	Reachability.cpp \
	Resources.cpp \
	platform.cpp \
	pushedcontent.cpp \
	obml_request.c \
	obmlview.cpp \
	bream.cpp \
	reksio.cpp \
	workerthread.cpp \
	vmentry_utils.cpp \
	jniutils.cpp \
	$(GENDIR)/mini/c/shared/generic/vm_integ/VMEntry.cpp \
	$(GENDIR)/mini/c/shared/generic/vm_integ/VMEntries.c \
	$(GENDIR)/VMInvokes.cpp \
	$(GENDIR)/VMCxxInvokes.cpp

LOCAL_C_INCLUDES :=
# Generated files: must be first in the path
LOCAL_C_INCLUDES += $(GENSRC)
LOCAL_C_INCLUDES += $(GENSRC)/mobile
LOCAL_C_INCLUDES += $(GENSRC)/mobile/mini/c/shared
LOCAL_C_INCLUDES += $(GENSRC)/bream/vm/c/inc
LOCAL_C_INCLUDES += $(GENSRC)/mini/c/shared

LOCAL_C_INCLUDES += $(SRCROOT)/mobile
LOCAL_C_INCLUDES += $(SRCROOT)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/config
LOCAL_C_INCLUDES += $(PRODUCT_INCLUDE_DIR)
LOCAL_C_INCLUDES += $(SRCROOT)/mobile/bream/vm/c/inc
LOCAL_C_INCLUDES += $(SRCROOT)/mobile/bream/vm/c # For some includes of bream graphics
LOCAL_C_INCLUDES += $(SRCROOT)/mobile/mini/c/shared
LOCAL_C_INCLUDES += $(SRCROOT)/..

LOCAL_CFLAGS += -fvisibility=hidden -Werror -Wno-psabi -Wreturn-type -include buildconfig.h
LOCAL_CFLAGS += -g
LOCAL_CXXFLAGS += -std=c++11 -Wall -Wextra -Wno-unused-function -fvisibility-inlines-hidden -Wmissing-declarations
LOCAL_LDLIBS := -lz
LOCAL_LDLIBS += -llog
LOCAL_LDLIBS += -ljnigraphics
LOCAL_LDLIBS += -lGLESv1_CM
LOCAL_LDFLAGS += -Wl,--gc-sections
C_SPECIFIC_FLAGS += -std=c99 -Wpointer-sign

ifneq ($(CC_wrapper),)
TARGET_CC := $(CC_wrapper) $(TARGET_CC)
endif

ifneq ($(CXX_wrapper),)
TARGET_CXX := $(CXX_wrapper) $(TARGET_CXX)
endif

# Workaround: LOCAL_CFLAGS gets included in C++ compilation and makes it
# impossible to add any C-specific flags or warnings.
TARGET_CC := $(TARGET_CC) $(C_SPECIFIC_FLAGS)

include $(BUILD_SHARED_LIBRARY)
