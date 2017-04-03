TARGET_ARCH ?= armv7

ifneq ($(TARGET_ARCHS),)
$(error TARGET_ARCHS is deprecated (along with multiarch builds). Update your scripts to use TARGET_ARCH instead)
endif

KnownTargetArchs := armv5 armv7 armv8 x86 x86_64

Unknown = $(filter-out $(KnownTargetArchs), $(TARGET_ARCH))

ifneq ($(Unknown),)
$(error $(Unknown) is not a valid TARGET_ARCH, must be one of: $(KnownTargetArchs))
endif

# Note! Don't add spaces to the subst function calls.
fnArchToAbi = $(subst armv5,armeabi,$(subst armv7,armeabi-v7a,$(subst armv8,arm64-v8a,$1)))

KNOWN_ABIS := $(foreach Target,$(KnownTargetArchs),$(call fnArchToAbi,$(Target)))

APP_ABI = $(call fnArchToAbi,$(TARGET_ARCH))
