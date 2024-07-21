clang++ -std=c++20 -O2 -ggdb -lm -L/extlibs -I/extlibs -lantlr4-runtime -lpthread \
src/main.cpp \
src/ast/ast.cpp \
src/symbol/type.cpp \
src/symbol/var.cpp \
src/symbol/func.cpp \
src/symbol/str.cpp \
src/ir/IRGener.cpp \
src/ir/BasicBlock.cpp \
src/ir/Module.cpp \
src/ir/User.cpp \
src/ir/Value.cpp \
src/ir/GlobalValue.cpp \
src/ir/Function.cpp \
src/ir/Constant.cpp \
src/ir/Edge.cpp \
src/ir/GlobalVariable.cpp \
src/ir/Type.cpp \
src/ir/Instruction.cpp \
src/util/list.cpp \
src/util/log.cpp \
src/util/hlist.cpp \
src/util/RPO.cpp \
src/frontend/syntax_type_array.cpp \
src/frontend/syntax_decl.cpp \
src/frontend/use.cpp \
src/frontend/syntax_info.cpp \
src/frontend/syntax_decl_head.cpp \
src/frontend/log.cpp \
src/frontend/symbol_table.cpp \
src/frontend/syntax_init.cpp \
src/lir/immeFloatToLoad.cpp \
src/lir/ArmStand.cpp \
src/lir/GEPToALU.cpp \
src/lir/GVtoAssgin.cpp \
src/lir/immeIntTomove.cpp \
src/lir/LargeToGlobal.cpp \
src/lir/modTosubmul.cpp \
src/ir_opt/SCEV.cpp \
src/ir_opt/loopVarReduce.cpp \
src/ir_opt/LivenessAnalysis.cpp \
src/ir_opt/SCCP.cpp \
src/ir_opt/IRCopy.cpp \
src/ir_opt/ConstFold.cpp \
src/ir_opt/THBalancing.cpp \
src/ir_opt/SimplifyCFG.cpp \
src/ir_opt/loopDrop.cpp \
src/ir_opt/Manager.cpp \
src/ir_opt/GVN.cpp \
src/ir_opt/DPE.cpp \
src/ir_opt/loopVarMove.cpp \
src/ir_opt/uselessBBDel.cpp \
src/ir_opt/SSARegisterAlloc.cpp \
src/ir_opt/DCE.cpp \
src/ir_opt/DomTree.cpp \
src/ir_opt/GCM.cpp \
src/ir_opt/Inline.cpp \
src/ir_opt/Associate.cpp \
src/ir_opt/BitVector.cpp \
src/ir_opt/GlobalVariableopt.cpp \
src/ir_opt/Mem2Reg.cpp \
src/ir_opt/Loop.cpp \
src/grammar/SysY.tab.cpp \
src/grammar/SysY.yy.cpp \
src/program/program.cpp \
src/backend/arm/ArmGen.cpp \
src/backend/arm/BlockMerge.cpp \
src/backend/arm/arm.cpp \
-I include/program \
-I include/ir \
-I include/util \
-I include/symbol \
-I include/lir \
-I include/ast \
-I include/frontend \
-I include/ir_opt \
-I include/backend/arm \
-I src/grammar/ \
-o compiler
