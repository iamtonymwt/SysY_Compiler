#ifndef MIPS_MIPSCODE_H
#define MIPS_MIPSCODE_H
#pragma once

#include <string>
using namespace std;

enum mipsOperation {
    mips_add,    //add $s1, $s2, $s3
    mips_addi,   //addi $s1, $s2, -1
    mips_sub,    //sub $s1, $s2, $s3
    mips_subi,   //subi $s1, $s2, 1
    mips_and,    //and $s1, $s2, $s3
    mips_andi,   //andi $s1, $s2, 0x55AA
    mips_or,     //or $s1, $s2, $s3
    mips_ori,    //ori $s1, $s2, 0x55AA

    mips_mult,   //mult $s1, $s2
    mips_div,    //div $s1, $s2
    mips_mflo,   //商/乘积
    mips_mfhi,   //余数

    mips_slt,    //slt $s1, $s2, $s3
    mips_beq,    //beq $s1, $s2, label
    mips_bne,    //bne $s1, $s2, label
    mips_j,      //j Loop_End
    mips_jal,    //jal my_function_name
    mips_jr,     // jr $31

    mips_lw,     //lw $v1, 8($sp)
    mips_sw,     //sw $v1, 8($sp)
    mips_syscall,
    mips_li,     //li $t1, -1
    mips_la,     //la $a0, head
    mips_move,   //move $t0, $t1

    mips_dataSeg,  //.data
    mips_asciizSeg, //.asciiz
    mips_textSeg,  //.text
    mips_label,  //产生标号
};

class mipsCode {
public:
    mipsOperation op; // 操作
    string z;     // 结果
    string x;     // 左操作数
    string y;     // 右操作数
    int imm;     // 立即数
    mipsCode(mipsOperation o, string zz = "", string xx = "", string yy = "", int i = 0) : op(o), z(zz), x(xx), y(yy), imm(i) {}
};

void genMipsCode();
void outputMipsCode();

#endif //MIPS_MIPSCODE_H
