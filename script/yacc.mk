# Yacc rules
YACCFLAGS += -d

$(TMP_DIR)/%.tab.hpp $(TMP_DIR)/%.tab.cpp: %.y
	@echo '+ YACC $<'
	@mkdir -p $(dir $@)
	@$(YACC) $(YACCFLAGS) -o $(@:%.hpp=%.cpp) $<

TMPCSRCS += $(YACCSRC:%.y=$(TMP_DIR)/%.tab.cpp)
