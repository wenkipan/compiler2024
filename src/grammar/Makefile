# Makefile for compiling SysY.l and SysY.y

# 定义工具路径
FLEX = flex
BISON = bison

# 源文件和目标文件
LEX_SRC = SysY.l
YACC_SRC = SysY.y

LEX_OUTPUT_CPP = SysY.yy.cpp
LEX_OUTPUT_HPP = SysY.yy.hpp
YACC_OUTPUT_CPP = SysY.tab.cpp
YACC_OUTPUT_HPP = SysY.tab.hpp

# 默认目标
all: $(LEX_OUTPUT_CPP) $(LEX_OUTPUT_HPP) $(YACC_OUTPUT_CPP) $(YACC_OUTPUT_HPP)

# 生成 SysY.yy.cpp 和 SysY.yy.hpp
$(LEX_OUTPUT_CPP): $(LEX_SRC)
	@echo "Generating $@ from $<"
	@mkdir -p $(dir $@)
	@$(FLEX) --header-file=$(LEX_OUTPUT_HPP) -o $@ $<

# 生成 SysY.tab.cpp 和 SysY.tab.hpp
$(YACC_OUTPUT_CPP): $(YACC_SRC)
	@echo "Generating $@ from $<"
	@mkdir -p $(dir $@)
	@$(BISON) -d -o $@ $<

# 清理生成的文件
clean:
	@echo "Cleaning up..."
	@rm -f $(LEX_OUTPUT_CPP) $(LEX_OUTPUT_HPP) $(YACC_OUTPUT_CPP) $(YACC_OUTPUT_HPP)

# 强制执行默认目标
.PHONY: all clean
