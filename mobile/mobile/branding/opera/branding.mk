
# Used to set MOP_PRODUCT_NAME
PRODUCT_NAME := Opera

# Base name for application package
#
# Depending on build settings additional suffixes
# will be appended to create a unique name.
BASE_PACKAGE_NAME := com.opera.browser

# Do not use additional "Powered by Opera" branding in
# splash screen, introduction and main menu when
# building normal Opera branded builds. This will
# default to YES for all other brands.
SHOW_POWERED_BY := NO

# Tracks installs via advertising campaigns.
ENABLE_APPSFLYER := YES

# Brand identifers communicated to the mini and turbo servers
ifeq ($(BETA), YES)
DEFAULT_BRANDING := new_opera_beta
TURBO_BRANDING := new_opera_beta
else
DEFAULT_BRANDING := new_opera
TURBO_BRANDING := new_opera
endif

# Native library storage
#
# One or more of of:
#  ziplib    Libs are deflated in the APK and extracted on install.
#  strlib    Libs are stored in the APK and loaded without extraction.
#  extlib    Libs are excluded from the APK and stored separately, for preloads.
#
# Multiple APKs will be built using the rules in build/output_apks.py
#
LIB_STORAGE ?= ziplib strlib

# Distribution source and medium
#
# List of values on the form SOURCE:MEDIUM
#
# SOURCE is the name of an app-store, OEM or operator customer.
# MEDIUM is assigned to each store or customer according to:
#   https://docs.google.com/spreadsheets/d/1FwLCGD9p7alm3VkXbsv_9dKGV-fsm_JVaTNg4MGETIQ/edit#gid=0
#
# One APK will be built per value.
#
# NOTE! The google_play source will activate the "please rate us" dialog.
#
DISTRIBUTION := google_play:aso opera:doc

ifeq ($(PUBLIC_BUILD), YES)
DISTRIBUTION += operastore:tp

ifeq ($(TARGET_ARCH), armv7)
DISTRIBUTION += 1mobile:tp 9apps:tp amazon:tp appitalism:tp aptoide:tp bemobi:tp
DISTRIBUTION += dinga:tp getjar:tp indosat_aplikazone:tp lgsmartworld:tp
DISTRIBUTION += mobango:tp mobile24:tp mobile9:tp mobogenie:tp mobomarket:tp
DISTRIBUTION += samsungappstore:tp slideme:tp vodacom:to yandex_store:tp
endif
endif

# WAM-9164: It's cool =)
WAIT_FOR_EULA_APPROVAL := NO
