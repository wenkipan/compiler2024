#pragma once
/*
x0	zero	常量零
x1	ra	返回地址
x2	sp	堆栈指针
x3	gp	全局指针
x4	tp	线程指针
x5-x7	t0-t2	临时寄存器
x8	s0/fp	保存寄存器/帧指针
x9	s1	保存寄存器
x10-x11	a0-a1	函数参数/返回值
x12-x17	a2-a7	函数参数
x18-x27	s2-s11	保存寄存器
x28-x31	t3-t6	临时寄存器


f0-f7	ft0-ft7	临时浮点寄存器
f8-f9	fs0-fs1	保存浮点寄存器
f10-f17	fa0-fa7	函数参数/返回值浮点寄存器
f18-f27	fs2-fs11	保存浮点寄存器
f28-f31	ft8-ft11	临时浮点寄存器
*/

#define zero 0 // x0 常量零
#define ra 1   // x1 返回地址
#define sp 2   // x2 堆栈指针
#define gp 3   // x3 全局指针
#define tp 4   // x4 线程指针
#define t0 5   // x5 临时寄存器
#define t1 6   // x6 临时寄存器
#define t2 7   // x7 临时寄存器
#define s0 8   // x8 保存寄存器/帧指针
#define fp 8   // x8 帧指针 (同 s0)
#define s1 9   // x9 保存寄存器
#define a0 10  // x10 函数参数/返回值
#define a1 11  // x11 函数参数/返回值
#define a2 12  // x12 函数参数
#define a3 13  // x13 函数参数
#define a4 14  // x14 函数参数
#define a5 15  // x15 函数参数
#define a6 16  // x16 函数参数
#define a7 17  // x17 函数参数
#define s2 18  // x18 保存寄存器
#define s3 19  // x19 保存寄存器
#define s4 20  // x20 保存寄存器
#define s5 21  // x21 保存寄存器
#define s6 22  // x22 保存寄存器
#define s7 23  // x23 保存寄存器
#define s8 24  // x24 保存寄存器
#define s9 25  // x25 保存寄存器
#define s10 26 // x26 保存寄存器
#define s11 27 // x27 保存寄存器
#define t3 28  // x28 临时寄存器
#define t4 29  // x29 临时寄存器
#define t5 30  // x30 临时寄存器
#define t6 31  // x31 临时寄存器

#define ft0 32  // f0 临时浮点寄存器
#define ft1 33  // f1 临时浮点寄存器
#define ft2 34  // f2 临时浮点寄存器
#define ft3 35  // f3 临时浮点寄存器
#define ft4 36  // f4 临时浮点寄存器
#define ft5 37  // f5 临时浮点寄存器
#define ft6 38  // f6 临时浮点寄存器
#define ft7 39  // f7 临时浮点寄存器
#define fs0 40  // f8 保存浮点寄存器
#define fs1 41  // f9 保存浮点寄存器
#define fa0 42  // f10 函数参数/返回值浮点寄存器
#define fa1 43  // f11 函数参数/返回值浮点寄存器
#define fa2 44  // f12 函数参数/返回值浮点寄存器
#define fa3 45  // f13 函数参数/返回值浮点寄存器
#define fa4 46  // f14 函数参数/返回值浮点寄存器
#define fa5 47  // f15 函数参数/返回值浮点寄存器
#define fa6 48  // f16 函数参数/返回值浮点寄存器
#define fa7 49  // f17 函数参数/返回值浮点寄存器
#define fs2 50  // f18 保存浮点寄存器
#define fs3 51  // f19 保存浮点寄存器
#define fs4 52  // f20 保存浮点寄存器
#define fs5 53  // f21 保存浮点寄存器
#define fs6 54  // f22 保存浮点寄存器
#define fs7 55  // f23 保存浮点寄存器
#define fs8 56  // f24 保存浮点寄存器
#define fs9 57  // f25 保存浮点寄存器
#define fs10 58 // f26 保存浮点寄存器
#define fs11 59 // f27 保存浮点寄存器
#define ft8 60  // f28 临时浮点寄存器
#define ft9 61  // f29 临时浮点寄存器
#define ft10 62 // f30 临时浮点寄存器
#define ft11 63 // f31 临时浮点寄存器

bool rv_is_r_reg(int no);
bool rv_is_f_reg(int no);
bool is_legal_i_s_imme_rv(int a);

enum class RVENUM
{
    rv_binary_begin,
    rv_add,
    rv_sub,
    rv_mul,
    rv_div,
    rv_rem,
    rv_and,
    rv_or,
    rv_xor,
    rv_sll,
    rv_srl,
    rv_sra,
    rv_slt,
    rv_sgt,
    rv_sltu,

    rv_fadd,
    rv_fsub,
    rv_fmul,
    rv_fdiv,
    rv_fmin,
    rv_fmax,
    rv_fsqrt,
    rv_binary_end,

    rv_unary_begin,
    rv_seqz,
    rv_snez,
    rv_mv,
    rv_neg,
    rv_unary_end,

    rv_jump_begin,
    rv_b,
    rv_j,
    rv_jump_end,

    rv_ret,

    rv_cmp_begin,
    rv_fcmp, // for float cmp only
    rv_cmp_end,

    rv_load_begin,
    rv_l,
    rv_load_end,

    rv_store_begin,
    rv_s,
    rv_store_end,

    rv_call,

    rv_fcvt_w_s,
    rv_fcvt_s_w,
};
enum class RVextend
{
    r,  // register
    a,  // addr(la)
    i,  // imme
    u,  // unsign
    b,  // Byte
    h,  // high ,or half word
    w,  // word
    su, // 有符号和无符号混合
};
enum class RVfloat
{
    w, // word
    s, // single precision
    d, // double precision
    x, // register
};
enum class RVcond
{
    nill,
    eqz,
    eq,
    ne,
    lt,
    le,
    ge,
    gt,
    ltu, // 无符号小于
    geu, // 无符号大于等于
};

/*
指令类别	指令	描述
整数计算指令（I-类型）	ADD	加法
    SUB	减法
    AND	按位与
    OR	按位或
    XOR	按位异或
    SLL	逻辑左移
    SRL	逻辑右移
    SRA	算术右移
    SLT	有符号小于
    SLTU	无符号小于
加载指令（L-类型）	LB	加载字节
    LH	加载半字
    LW	加载字
    LBU	加载无符号字节
    LHU	加载无符号半字
存储指令（S-类型）	SB	存储字节
    SH	存储半字
    SW	存储字
分支指令（B-类型）	BEQ	相等分支
    BNE	不等分支
    BLT	小于分支
    BGE	大于等于分支
    BLTU	无符号小于分支
    BGEU	无符号大于等于分支
跳转指令（J-类型）	JAL	跳转并链接
    JALR	寄存器跳转并链接
立即数指令（I-类型）	ADDI	加法立即数
    ANDI	按位与立即数
    ORI	按位或立即数
    XORI	按位异或立即数
    SLTI	有符号小于立即数
    SLTIU	无符号小于立即数
    SLLI	逻辑左移立即数
    SRLI	逻辑右移立即数
    SRAI	算术右移立即数
特殊指令	LUI	加载上半立即数
    AUIPC	加载上半立即数并加上PC
    FENCE	内存屏障
    ECALL	环境调用
    EBREAK	环境断点
RV32M 整数乘法与除法扩展指令集
指令类别	指令	描述
整数乘法和除法指令	MUL	乘法
    MULH	高位乘法
    MULHSU	高位乘法（有符号和无符号混合）
    MULHU	高位乘法（无符号）
    DIV	除法
    DIVU	无符号除法
    REM	取余
    REMU	无符号取余
RV32F 浮点扩展指令集（单精度）
指令类别	指令	描述
浮点加载和存储指令	FLW	浮点加载字
    FSW	浮点存储字
浮点算术指令	FADD.S	浮点加法（单精度）
    FSUB.S	浮点减法（单精度）
    FMUL.S	浮点乘法（单精度）
    FDIV.S	浮点除法（单精度）
    FMIN.S	浮点最小值（单精度）
    FMAX.S	浮点最大值（单精度）
    FSQRT.S	浮点平方根（单精度）
浮点比较指令	FEQ.S	浮点相等比较（单精度）
    FLT.S	浮点小于比较（单精度）
    FLE.S	浮点小于等于比较（单精度）
浮点类型转换指令	FCVT.W.S	浮点转有符号整数（单精度）
    FCVT.WU.S	浮点转无符号整数（单精度）
    FCVT.S.W	有符号整数转浮点（单精度）
    FCVT.S.WU	无符号整数转浮点（单精度）
浮点移动指令	FMV.X.W	浮点寄存器到整数寄存器移动（单精度）
    FMV.W.X	整数寄存器到浮点寄存器移动（单精度）
浮点状态指令	FCLASS.S	浮点分类（单精度）
    FSGNJ.S	浮点符号复制（单精度）
    FSGNJN.S	浮点符号取反（单精度）
    FSGNJX.S	浮点符号异或（单精度）
*/