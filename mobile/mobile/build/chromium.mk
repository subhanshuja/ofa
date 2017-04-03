# Chromium built jar files.
ChromiumJars := \
    android_swipe_refresh_java.jar \
    base_java.jar \
    battery_monitor_android.jar \
    battery_mojo_bindings_java.jar \
    bindings.jar \
    blink_headers_java.jar \
    bluetooth_java.jar \
    capture_java.jar \
    chill_java.jar \
    content_java.jar \
    gamepad_java.jar \
    geolocation_java.jar \
    generic_sensor_interfaces_java.jar \
    generic_sensor_java.jar \
    location_java.jar \
    media_java.jar \
    midi_java.jar \
    mojo_bindings_java.jar \
    mojo_custom_types_java.jar \
    navigation_interception_java.jar \
    net_java.jar \
    nfc_java.jar \
    power_save_blocker_java.jar \
    screen_capture_java.jar \
    shell_interfaces_java.jar \
    shell_java.jar \
    system.jar \
    system_java.jar \
    time_zone_java.jar \
    ui_accessibility_java.jar \
    ui_java.jar \
    vibration_manager_android.jar \
    vibration_mojo_bindings_java.jar \
    web_contents_delegate_android_java.jar \
    usb_java.jar \
    ui_mojo_bindings_java.jar

# Jars needed to build and run JUnit tests on the host JVM
TestLibJars := \
    third_party/guava/guava_java.jar \
    third_party/hamcrest/hamcrest_core_java.jar \
    third_party/hamcrest/hamcrest_integration_java.jar \
    third_party/hamcrest/hamcrest_library_java.jar \
    third_party/junit/junit.jar \
    third_party/robolectric/android-all-5.0.0_r2-robolectric-1.jar \
    third_party/mockito/mockito_java.jar \
    third_party/robolectric/json-20080701.jar \
    third_party/robolectric/robolectric_annotations_java.jar \
    third_party/robolectric/robolectric_java.jar \
    third_party/robolectric/robolectric_processor_java.jar \
    third_party/robolectric/robolectric_resources_java.jar \
    third_party/robolectric/robolectric_template_processor.jar \
    third_party/robolectric/robolectric_utils_java.jar \
    third_party/robolectric/shadows-core-3.0-21.jar \
    third_party/robolectric/shadows-multidex-3.0.jar \
    third_party/robolectric/tagsoup-1.2.jar

ChromiumPaks = opera.pak icudtl.dat

ChromiumPaks += natives_blob.bin
ifneq ($(filter x86_64 armv8, $(TARGET_ARCH)),)
ChromiumPaks += snapshot_blob_64.bin
else
ChromiumPaks += snapshot_blob_32.bin
endif

CHROMIUM_JARS := $(addprefix $(WAM_ROOT)/libs/,$(ChromiumJars))
CHROMIUM_PAKS := $(addprefix $(WAM_ASSETS)/,$(ChromiumPaks))
TEST_LIB_JARS := $(addprefix $(CHROMIUM_OUT_FULL)/lib.java/,$(TestLibJars))

ifeq ($(USE_SUMO), YES)
BuildCmd := sumo
else
BuildCmd := ninja
endif

ifeq ($(VERBOSE), YES)
VerboseNinja := -v
endif

CHROMIUM_LIB_DIR = $(CHROMIUM_OUT_FULL)
WAM_LIB_DIR = $(WAM_ROOT)/chromium_libs/$(APP_ABI)
WamMapDir = $(WAM_ROOT)/bin/maps-$(APP_ABI)

CHROMIUM_UNITTESTS ?= android_builder_tests

CR_LIBS := libopera

CR_LIBS := $(addsuffix $(CR_LIB_EXT),$(CR_LIBS))

USE_ICECC ?= $(ICECC)
ifeq ($(USE_ICECC), YES)
ifdef ICECC_PREFIX
CcachePrefix := $(ICECC_PREFIX)/bin/icecc
else
CcachePrefix := icecc
endif
IceccEnv = CCACHE_PREFIX=$(CcachePrefix) ICECC_VERSION=`cat $(CHROMIUM_OUT_FULL)/gen/icecc_version`
Jobs := -j$(ICECC_JOBS)
else
IceccEnv :=
Jobs :=
endif

# Dummy target for source tarball.
$(SOURCE_ROOT)/.git/logs/HEAD:

.PHONY: chromium chromium_tests

.PHONY: all_libs
all_libs: $(GN_STAMP) | $(CHROMIUM_OUT_FULL)/gen
	@echo Running ninja for $@
ifeq ($(USE_ICECC), YES)
	$H $(BuildCmd) $(VerboseNinja) -C $(CHROMIUM_OUT_FULL) icecc_create_env
endif
	@echo make[$(MAKELEVEL)]: Entering directory \`$(CHROMIUM_OUT_FULL)\'
	$H env $(IceccEnv) $(BuildCmd) $(VerboseNinja) $(Jobs) -C $(CHROMIUM_OUT_FULL) wam_libs
	@echo make[$(MAKELEVEL)]: Leaving directory \`$(CHROMIUM_OUT_FULL)\'

CREATE_DIRS += $(CHROMIUM_OUT_FULL)/gen

# Add the Chromium build into the dependency chain.
all_libs: $(SWIG_CPPS)
$(addprefix $(CHROMIUM_LIB_DIR)/,$(CR_LIBS)): all_libs
	@
$(addprefix $(CHROMIUM_LIB_DIR)/lib.unstripped/,$(CR_LIBS)): all_libs
	@

# STRIP_RULE: Strip library into output dir.
# $1 library
define STRIP_RULE
$(WAM_LIB_DIR)/$1: $(CHROMIUM_LIB_DIR)/$1 | $(WAM_LIB_DIR)
	@echo Stripping $$< to $$@
	$H $(STRIP) --strip-unneeded --remove-section=.gdb_index -o $$@ $$<

CREATE_DIRS += $(WAM_LIB_DIR)
chromium: $(WAM_LIB_DIR)/$1
endef

# Instantiate STRIP_RULE for Chromium built libs.
$(foreach Lib,$(CR_LIBS),$(eval $(call STRIP_RULE,$(Lib))))

CREATE_DIRS += $(WAM_ROOT)/libs $(WAM_ASSETS)

# Pattern rule to copy Chromium jars into library output dir.
$(WAM_ROOT)/libs/%.jar: $(CHROMIUM_OUT_FULL)/lib.java/%.jar | $(WAM_ROOT)/libs
	$(call fnLink,$<,$@)

# Pattern rule to copy Chromium paks into output dir.
$(WAM_ASSETS)/%.pak: $(CHROMIUM_OUT_FULL)/%.pak | $(WAM_ASSETS)
	$(call fnLink,$<,$@)

# Pattern rule to copy Chromium dats into output dir.
$(WAM_ASSETS)/%.dat: $(CHROMIUM_OUT_FULL)/%.dat | $(WAM_ASSETS)
	$(call fnLink,$<,$@)

# Pattern rules to put Chromium natives_blob.bin and snapshot_blob.bin into output dir.
$(WAM_ASSETS)/natives_blob.bin: $(CHROMIUM_OUT_FULL)/natives_blob.bin | $(WAM_ASSETS)
	$(call fnLink,$<,$@)

$(WAM_ASSETS)/snapshot_blob_%.bin: $(CHROMIUM_OUT_FULL)/snapshot_blob.bin | $(WAM_ASSETS)
	$(call fnLink,$<,$@)

$(CHROMIUM_OUT_FULL)/snapshot_blob.bin: $(CHROMIUM_LIB_DIR)/libopera$(CR_LIB_EXT)
	@

# $1: source
# $2: destination
define COPY_JAR
$(WAM_ROOT)/libs/$2: $(CHROMIUM_OUT_FULL)/lib.java/$1
	$(call fnLink,$$<,$$@)

$(CHROMIUM_OUT_FULL)/lib.java/$1: $(CHROMIUM_LIB_DIR)/libopera$(CR_LIB_EXT)
	@
endef

# Copy and rename jars with potential name conflicts.
$(eval $(call COPY_JAR,components/location/android/location_java.jar,location_java.jar))
$(eval $(call COPY_JAR,device/battery/mojo_bindings_java.jar,battery_mojo_bindings_java.jar))
$(eval $(call COPY_JAR,device/bluetooth/java.jar,bluetooth_java.jar))
$(eval $(call COPY_JAR,device/gamepad/java.jar,gamepad_java.jar))
$(eval $(call COPY_JAR,device/generic_sensor/java.jar,generic_sensor_java.jar))
$(eval $(call COPY_JAR,device/generic_sensor/public/interfaces/interfaces_java.jar,generic_sensor_interfaces_java.jar))
$(eval $(call COPY_JAR,device/geolocation/geolocation_java.jar,geolocation_java.jar))
$(eval $(call COPY_JAR,device/nfc/android/java.jar,nfc_java.jar))
$(eval $(call COPY_JAR,device/nfc/mojo_bindings_java.jar,mojo_bindings_java.jar))
$(eval $(call COPY_JAR,device/power_save_blocker/java.jar,power_save_blocker_java.jar))
$(eval $(call COPY_JAR,device/usb/java.jar,usb_java.jar))
$(eval $(call COPY_JAR,device/vibration/mojo_bindings_java.jar,vibration_mojo_bindings_java.jar))
$(eval $(call COPY_JAR,device/time_zone_monitor/java.jar,time_zone_java.jar))
$(eval $(call COPY_JAR,mojo/common/common_custom_types_java.jar,mojo_custom_types_java.jar))
$(eval $(call COPY_JAR,services/shell/public/interfaces/interfaces_java.jar,shell_interfaces_java.jar))
$(eval $(call COPY_JAR,services/shell/public/java/shell_java.jar,shell_java.jar))
$(eval $(call COPY_JAR,ui/base/mojo/mojo_bindings_java.jar,ui_mojo_bindings_java.jar))

ChromiumSrcJars = $(addprefix $(CHROMIUM_OUT_FULL)/lib.java/,$(ChromiumJars))
ChromiumSrcPaks = $(addprefix $(CHROMIUM_OUT_FULL)/,$(ChromiumPaks))

# This rule tells make that the jars and paks are created as a side effect.
$(ChromiumSrcJars) $(ChromiumSrcPaks): $(CHROMIUM_LIB_DIR)/libopera$(CR_LIB_EXT)
	@

# Depend on jars and paks to initiate copy.
chromium: $(CHROMIUM_JARS)
chromium: $(CHROMIUM_PAKS)

clean-product::
	rm -f $(CHROMIUM_PAKS) $(CHROMIUM_JARS)

.PHONY: force_ut
$(ChromiumOutRoot)/test.stamp: $(GN_STAMP) force_ut
	@echo Running ninja for $(TARGET_ARCH) unit tests
	$H env PATH=$(COMPILER_PATH):$(PATH) $(CcachePrefix) ICECC_VERSION=$(IceccVersion) \
	$(BuildCmd) $(VerboseNinja) $(Jobs) -C $(CHROMIUM_OUT_FULL) host_forwarder md5sum $(CHROMIUM_UNITTESTS)
	touch $@

chromium_tests: $(ChromiumOutRoot)/test.stamp

TestStamp = $(CHROMIUM_OUT_FULL)/wam-test.stamp

$(TestStamp): $(GN_STAMP)
	$H $(BuildCmd) $(VerboseNinja) $(Jobs) -C $(CHROMIUM_OUT_FULL) junit_test_support
	$H touch $@

# This rule tells make that the jars are created as a side effect.
$(TEST_LIB_JARS): $(TestStamp)
	@
