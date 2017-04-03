# Download and package Google Play services

# The output is a directory with the following structure:
#
# OUT_DIR
# +-- res
# |   +-- CLIENT_1
# |   |   +-- color
# |   |   +-- values
# |   |   +-- etc.
# |   +-- CLIENT_2
# |       +-- ...
# +-- stub
# |   +-- libs/google-play-services.jar
# |   +-- res/
# |   +-- src/android/UnusedStub.java

# For more information about the structure of the generated
# Google Play Services directory check:
# $(WAM_ROOT)/build/preprocess_google_play_services.py

ifneq (,$(ANDROID_SDK_MIRROR_HOST))
ANDROID_SDK_MIRROR_PORT ?= 80
AndroidSdkProxyParam = -s --proxy-host $(ANDROID_SDK_MIRROR_HOST) --proxy-port $(ANDROID_SDK_MIRROR_PORT)
endif

GooglePlayServices = $(WAM_ROOT)/bin/google-play-services
GooglePlayServicesConfig = $(WAM_ROOT)/build/preprocess_google_play_services.config.json
GOOGLE_PLAY_SERVICES_JAR := $(GooglePlayServices)/stub/libs/google-play-services.jar
AndroidM2Repository = extra-google-m2repository
AndroidM2RepositoryDir = $(CHROMIUM_ROOT)/third_party/android_tools/sdk/extras/google/m2repository

$(GOOGLE_PLAY_SERVICES_JAR): $(AndroidM2RepositoryDir) $(GooglePlayServicesConfig)
	@echo Packaging $@
	$H mkdir -p $(GooglePlayServices)
	$H $(WAM_ROOT)/build/preprocess_google_play_services.py \
		-o $(GooglePlayServices) \
		-r $(AndroidM2RepositoryDir) \
		-c $(GooglePlayServicesConfig)

AndroidBinary = $(CHROMIUM_ROOT)/third_party/android_tools/sdk/tools/android

$(AndroidM2RepositoryDir):
	@echo Downloading $(AndroidM2Repository)...
	@[ -z $(ANDROID_SDK_MIRROR_HOST) ] || echo Using proxy $(ANDROID_SDK_MIRROR_HOST):$(ANDROID_SDK_MIRROR_PORT)
	$H echo "y" | $(AndroidBinary) update sdk --all --filter $(AndroidM2Repository) --no-ui $(AndroidSdkProxyParam) > /dev/null

clean::
	rm -fr $(AndroidM2RepositoryDir)
