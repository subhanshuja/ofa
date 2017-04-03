
ifeq ($(SWIG),)
SWIG = $(SOURCE_ROOT)/mobile/tools/swig/swig
export SWIG_LIB = $(SOURCE_ROOT)/mobile/tools/swig/Lib
endif

SWIG_OUT := $(WAM_ROOT)/bin/swig
SWIG_JARS := $(addprefix $(WAM_ROOT)/libs/swig-,$(addsuffix .jar,$(foreach Package,$(SWIG_PACKAGES),$(notdir $(Package)))))
SWIG_CPPS := $(addprefix $(SWIG_OUT)/,$(addsuffix .cpp,$(SWIG_PACKAGES)))

# Include generated dependencies
$(foreach Package,$(SWIG_PACKAGES),$(eval -include $(SWIG_OUT)/$(Package).d))

# The dependency file includes a variable SwigFiles.$(package) that lists the
# secondary files generated when generating the cpp file.
define SWIG_DEPS
$(eval $(call MULTI_OUTPUT_RULE,$(SWIG_OUT)/$1.cpp,$(SwigFiles.$(notdir $1)),$(SWIG_OUT)/$1.d))

# Cause swig to rerun if we lose the java files
$(WAM_ROOT)/libs/swig-$(notdir $1).jar: $(filter %.java, $(SwigFiles.$(notdir $1)))

# Add phony targets to handle lost files
$(SwigFiles.$(notdir $1)):
endef

$(foreach Package,$(SWIG_PACKAGES),$(eval $(call SWIG_DEPS,$(Package))))


.PHONY: swig
swig: $(SWIG_JARS) $(SWIG_CPPS)

$(SWIG_OUT)/%.cpp: $(WAM_ROOT)/src/%.i $(SWIG)
	@echo "SWIG generate $@"
	$H rm -fr $(SWIG_OUT)/$(dir $*)
	$H mkdir -p $(SWIG_OUT)/$(dir $*)
	$H rm -fr $(SWIG_OUT)/$(CODE_PACKAGE_PATH)/$(notdir $*)
	$H mkdir -p $(SWIG_OUT)/$(CODE_PACKAGE_PATH)/$(notdir $*)
	$H $(SWIG) -Wextra -Werror -c++ -java -package $(CODE_PACKAGE).$(notdir $*) -MP -MD -I$(CHROMIUM_ROOT) -o $@ -oh $(SWIG_OUT)/$*.h -outdir $(SWIG_OUT)/$(CODE_PACKAGE_PATH)/$(notdir $*) $<
	$H echo SwigFiles.$(notdir $*) += $(SWIG_OUT)/$(CODE_PACKAGE_PATH)/$(notdir $*)/*.java >> $(SWIG_OUT)/$*.d
	$H if [ -f $(SWIG_OUT)/$*.h ]; then echo SwigFiles.$(notdir $*) += $(SWIG_OUT)/$*.h >> $(SWIG_OUT)/$*.d; fi

define SWIG_JAR_COMPILE
$$(WAM_ROOT)/libs/swig-$(notdir $1).jar: $$(SWIG_OUT)/$1.cpp $$(WAM_OUT)/classes-api.jar
	@echo "SWIG compile  $$@"
	$$H $$(JAVAC) $$(JAVAC_FLAGS) -cp $$(subst $$(space),:,$$(run_classpath.api)) $$(SWIG_OUT)/$$(CODE_PACKAGE_PATH)/$(notdir $1)/*.java
	$$H jar cf $$@ -C $$(SWIG_OUT) $$(CODE_PACKAGE_PATH)/$(notdir $1)
endef

$(foreach Package,$(SWIG_PACKAGES),$(eval $(call SWIG_JAR_COMPILE,$(Package))))

.SECONDARY: $(SWIG_CPPS)
