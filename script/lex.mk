# Lex rules
LEXFLAGS +=

$(TMP_DIR)/%.yy.hpp $(TMP_DIR)/%.yy.cpp: %.l
	@echo '+ LEX $<'
	@mkdir -p $(dir $@)
	@$(LEX) $(LEXFLAGS) --header-file=$(@:%.cpp=%.hpp) -o $@ $<

TMPCSRCS += $(LEXSRC:%.l=$(TMP_DIR)/%.yy.cpp)
