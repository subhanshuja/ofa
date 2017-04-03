# Configure gn variables here.

GnVars = opera_wam=true target_os="android" opera="$(SOURCE_ROOT)"

GnVars += host_clang=false use_sysroot=false cc_wrapper="ccache" linux_use_bundled_binutils=false

GnVars += avoid_google_play_services=true

GnVars += package_name="$(PACKAGE_NAME)"

GnVars += brand_res_dir="$(BRAND_RES_DIR)"

# Disable PNG crunching since we do it ourselves when packaging resources
# and it can not be done twice on the same PNGs
GnVars += use_png_crunch=false

# Enable search on dns lookup failure
GnVars += enable_search_in_error_page=true

# Custom Burmese translations for the Zawgyi font.
GnVars += use_zawgyi_encoding=$(call fnToBool,$(ENABLE_ZAWGYI_ENCODING))

GnVars += is_debug=$(call fnToBool,$(CR_DEBUG))

GnVars += is_official_build=$(call fnToBool,$(OFFICIAL))
GnVars += is_beta_build=$(call fnToBool,$(BETA))
GnVars += is_component_build=$(call fnToBool,$(COMPONENT_BUILD))
GnVars += is_developer_build=$(call fnToBool,$(DEVELOPER_BUILD))

# Disable optimizations for proper native debugging, except if:
# - FAST_DEBUG is enabled:
#     Stepping through code will be unpredicatable but all other debug help is
#     available. Run-time performance will be close to release through.
# - targeting x86:
#     We've repeatedly encountered compilation issues due to this not being a
#     configuration that upstream builds regularly.
#     See https://code.google.com/p/libyuv/issues/detail?id=517 for details.
ifeq ($(CR_DEBUG), YES)
ifeq ($(FAST_DEBUG), NO)
ifneq ($(TARGET_ARCH), x86)
GnVars += android_full_debug=true
endif
endif
endif

# We can't afford to use the full symbol information for non-component builds
# since the library can grow above 4gb in 32-bit builds.
# See https://bugs.chromium.org/p/chromium/issues/detail?id=648948 for details.
ifeq ($(COMPONENT_BUILD), YES)
GnVars += symbol_level=2
else
GnVars += symbol_level=1
endif

ifdef ICECC_PREFIX
GnVars += icecc_dir="$(ICECC_PREFIX)"
endif

GnTarget.armv5 := target_cpu="arm" arm_version=6 arm_use_neon=false
GnTarget.armv7 := target_cpu="arm" arm_version=7
GnTarget.armv8 := target_cpu="arm64"
GnTarget.x86 := target_cpu="x86"
GnTarget.x86_64 := target_cpu="x64"

GnVars += $(GnTarget.$(TARGET_ARCH))

GnVars += turbo_brand="$(TURBO_BRANDING)"
GnVars += turbo_version="$(VERSION_NUMBER)"
GnVars += turbo2_proxy_auth_secret="$(TURBO2_PROXY_AUTH_SECRET)"

ifdef TURBO2_PROXY
GnVars += turbo2_proxy="$(TURBO2_PROXY)"
endif

ifdef TURBO2_PROXY_HOST_ZERO_RATE_SLOT
GnVars += turbo2_proxy_host_zero_rate_slot="$(TURBO2_PROXY_HOST_ZERO_RATE_SLOT)"
endif

GnVars += opera_sync=true
GnVars += turbo_product="OperaMobile"
GnVars += sync_auth_service="$(SYNC_CLIENT_PRODUCT)"
GnVars += sync_auth_application="$(SYNC_APPLICATION)"
GnVars += sync_auth_application_key="$(SYNC_APPLICATION_KEY)"
GnVars += sync_auth_client_key="$(AUTH_CLIENT_KEY)"
GnVars += sync_auth_client_secret="$(AUTH_CLIENT_SECRET)"
GnVars += enable_supervised_users=false

GnVars += chr_version="$(CHROMIUM_VERSION)"
GnVars += last_change="$(LASTCHANGE)"
GnVars += opr_version="$(VERSION_NUMBER)"
GnVars += opr_build_number="$(BUILDNUMBER)"
GnVars += opr_public_build=$(call fnToBool,$(PUBLIC_BUILD))

GnVars += socorro_product_name="$(SOCORRO_PRODUCT_NAME)"

GnVars += enable_browser_provider=$(call fnToBool, $(ENABLE_BROWSER_PROVIDER))

GnVars += enable_sitepatcher_test=$(call fnToBool,$(ENABLE_SITEPATCHER_TEST))

GnVars += root_extra_deps=["$(WAM_ROOT)/build"]

GnVars += ${GN_FLAGS}

GnCheckVars = $(CHROMIUM_OUT_FULL)/wam-gn.vars
GN_STAMP = $(CHROMIUM_OUT_FULL)/wam-gn.stamp

CREATE_DIRS += $(CHROMIUM_OUT_FULL)

.PHONY: gn force_check

$(GnCheckVars): force_check | $(CHROMIUM_OUT_FULL)
	$H echo '$(GnVars)' > $@.tmp
	$H cmp -s $@.tmp $@ || mv $@.tmp $@

$(GN_STAMP): $(GnCheckVars)
	@echo Running gn for $(CHROMIUM_OUT)
	$H env SKIP_CLANG_DOWNLOAD=1 $(CHROMIUM_ROOT)/build/run_opera_hooks.py
	$H cd $(CHROMIUM_ROOT) ; buildtools/linux64/gn gen --args='$(GnVars)' $(CHROMIUM_OUT)
	$H touch $@

gn: $(GN_STAMP)
