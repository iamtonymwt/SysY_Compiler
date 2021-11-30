#include <iostream>
#include <fstream>
#include "mipscode.h"
#include "vTable.h"
#include "vector"
#include "midcode.h"
#include "map"

#define INTOFFSET 4

extern ofstream mipsCodefile;
extern vector<midCode> midCodeTable;
extern vector<Variable> vTable;
extern vector<Function> funcvMap;
extern vector<pair<string, string>> allStringList;
vector<mipsCode> mipsCodeTable;
map<string, Variable> ident2var;  // 变量名 函数名（是否全局） 地址
extern int vAddress;

int paraCount = 0; //函数参数个数


/*
 *  $gp不变，存储全局变量,从低到高
 *  $sp函数尾,$fp函数头 运行栈（从低到高）：栈长、返回绝对地址、参数、局部变量
 *  $s1函数参数栈顶
 *  $ra函数ret
 *  $t0, $t1, $t2, $t3, $t4, $t5, $t6运算用
 *  $a0, $v0 与 syscall使用
 *  $s0 函数返回值
 *  hi, lo
 */

int insLabel = 0;//== != ! 需要小跳转
string getInsLabel() {
    insLabel ++;
    return "insLabel_" + to_string(insLabel);
}

void genIdent2addr() {
    for (int i = 0; i < vTable.size(); i++) {
        ident2var.insert(make_pair(vTable[i].fName, vTable[i]));
    }
}

//完成 num -> $dst / var -> $dst
void getVar(string varIdent, string dst) {
    if (isNumber(varIdent)) {
        mipsCodeTable.emplace_back(mips_li, dst, "", "", stoi(varIdent));
    }
    else {
        string funcBelong = ident2var.find(varIdent)->second.func;
        int addr = ident2var.find(varIdent)->second.address;
        //global
        if (funcBelong.empty()) {
            mipsCodeTable.emplace_back(mips_lw, dst, "$gp", "", addr);
        }
        //local
        else {
            mipsCodeTable.emplace_back(mips_lw, dst, "$fp", "", addr + 8);
        }
    }
}
//完成 $src -> var
void pushVar(string src, string varIdent) {
    string funcBelong = ident2var.find(varIdent)->second.func;
    int addr = ident2var.find(varIdent)->second.address;
    //global
    if (funcBelong.empty()) {
        mipsCodeTable.emplace_back(mips_sw, src, "$gp", "", addr);
    }
    //local
    else {
        mipsCodeTable.emplace_back(mips_sw, src, "$fp", "", addr + 8);
    }
}
//初始化$sp,$fp,$s1
void funcStackInit() {
    for (int i = funcvMap.size()-1; i >= 0; i--) {
        if (funcvMap[i].ident == "main") {
            mipsCodeTable.emplace_back(mips_subi, "$fp", "$sp", "", funcvMap[i].length);
            mipsCodeTable.emplace_back(mips_li, "$t0", "", "", funcvMap[i].length);
            mipsCodeTable.emplace_back(mips_sw, "$t0", "$fp", "", 0);
            break;
        }
    }
    mipsCodeTable.emplace_back(mips_li, "$s1", "", "", 0x10040000);
}

void genMipsCode() {
    mipsCodeTable.clear();
    genIdent2addr();
    if (!allStringList.empty()) {
        mipsCodeTable.emplace_back(mips_dataSeg);
        for (int i = 0; i < allStringList.size(); i++) {
            mipsCodeTable.emplace_back(mips_asciizSeg, allStringList[i].first, allStringList[i].second);
        }
    }
    mipsCodeTable.emplace_back(mips_textSeg);
    funcStackInit();
    for (int index = 0; index < midCodeTable.size(); index ++) {
        midCode midcode = midCodeTable[index];
        switch (midcode.op) {
            case PLUSOP: {
                if (isNumber(midcode.x)) {
                    getVar(midcode.y, "$t2");
                    mipsCodeTable.emplace_back(mips_addi, "$t0", "$t2", "", stoi(midcode.x));
                    pushVar("$t0", midcode.z);
                }
                else if (isNumber(midcode.y)) {
                    getVar(midcode.x, "$t1");
                    mipsCodeTable.emplace_back(mips_addi, "$t0", "$t1", "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    getVar(midcode.x, "$t1");
                    getVar(midcode.y, "$t2");
                    mipsCodeTable.emplace_back(mips_add, "$t0", "$t1", "$t2");
                    pushVar("$t0", midcode.z);
                }

                break;
            }
            case MINUOP: {
                if (isNumber(midcode.y)) {
                    getVar(midcode.x, "$t1");
                    mipsCodeTable.emplace_back(mips_subi, "$t0", "$t1", "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    getVar(midcode.x, "$t1");
                    getVar(midcode.y, "$t2");
                    mipsCodeTable.emplace_back(mips_sub, "$t0", "$t1", "$t2");
                    pushVar("$t0", midcode.z);
                }
                break;
            }
            case MULTOP: {
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_mult, "$t1", "$t2");
                mipsCodeTable.emplace_back(mips_mflo, "$t0");
                pushVar("$t0", midcode.z);
                break;
            }
            case DIVOP: {
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_div, "$t1", "$t2");
                mipsCodeTable.emplace_back(mips_mflo, "$t0");
                pushVar("$t0", midcode.z);
                break;
            }
            case MODOP: {
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_div, "$t1", "$t2");
                mipsCodeTable.emplace_back(mips_mfhi, "$t0");
                pushVar("$t0", midcode.z);
                break;
            }
            case ANDOP: {
                if (isNumber(midcode.x)) {
                    getVar(midcode.y, "$t2");
                    mipsCodeTable.emplace_back(mips_andi, "$t0", "$t2", "", stoi(midcode.x));
                    pushVar("$t0", midcode.z);
                }
                else if (isNumber(midcode.y)) {
                    getVar(midcode.x, "$t1");
                    mipsCodeTable.emplace_back(mips_andi, "$t0", "$t1", "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    getVar(midcode.x, "$t1");
                    getVar(midcode.y, "$t2");
                    mipsCodeTable.emplace_back(mips_and, "$t0", "$t1", "$t2");
                    pushVar("$t0", midcode.z);
                }
                break;
            }
            case OROP: {
                if (isNumber(midcode.x)) {
                    getVar(midcode.y, "$t2");
                    mipsCodeTable.emplace_back(mips_ori, "$t0", "$t2", "", stoi(midcode.x));
                    pushVar("$t0", midcode.z);
                }
                else if (isNumber(midcode.y)) {
                    getVar(midcode.x, "$t1");
                    mipsCodeTable.emplace_back(mips_ori, "$t0", "$t1", "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    getVar(midcode.x, "$t1");
                    getVar(midcode.y, "$t2");
                    mipsCodeTable.emplace_back(mips_or, "$t0", "$t1", "$t2");
                    pushVar("$t0", midcode.z);
                }
                break;
            }
            case LSSOP: {
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_slt, "$t0", "$t1", "$t2");
                pushVar("$t0", midcode.z);
                break;
            }
            case LEQOP: {
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_slt, "$t0", "$t1", "$t2");
                string ins = getInsLabel();
                mipsCodeTable.emplace_back(mips_bne, "$t1", "$t2", ins);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, ins);
                pushVar("$t0", midcode.z);
                break;
            }
            case GREOP: {
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_slt, "$t0", "$t2", "$t1");
                pushVar("$t0", midcode.z);
                break;
            }
            case GEQOP: {
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_slt, "$t0", "$t2", "$t1");
                string ins = getInsLabel();
                mipsCodeTable.emplace_back(mips_bne, "$t1", "$t2", ins);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, ins);
                pushVar("$t0", midcode.z);
                break;
            }
            case EQLOP: {
                string label = getInsLabel();
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 0);
                mipsCodeTable.emplace_back(mips_bne, "$t1", "$t2", label);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, label);
                pushVar("$t0", midcode.z);
                break;
            }
            case NEQOP: {
                string label = getInsLabel();
                getVar(midcode.x, "$t1");
                getVar(midcode.y, "$t2");
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 0);
                mipsCodeTable.emplace_back(mips_beq, "$t1", "$t2", label);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, label);
                pushVar("$t0", midcode.z);
                break;
            }
            case NOTOP: {
                string label1 = getInsLabel();
                string label2 = getInsLabel();
                getVar(midcode.x, "$t1");
                mipsCodeTable.emplace_back(mips_beq, "$t1", "$zero", label1);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 0);
                mipsCodeTable.emplace_back(mips_j, label2);
                mipsCodeTable.emplace_back(mips_label, label1);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, label2);
                pushVar("$t0", midcode.z);
                break;
            }
            case ASSIGNOP: {
                getVar(midcode.x, "$t1");
                mipsCodeTable.emplace_back(mips_move, "$t0", "$t1");
                pushVar("$t0", midcode.z);
                break;
            }
            case GOTO: {
                mipsCodeTable.emplace_back(mips_j, midcode.z);
                break;
            }
            case BZ: {
                getVar(midcode.x, "$t1");
                mipsCodeTable.emplace_back(mips_beq, "$t1", "$zero", midcode.z);
                break;
            }
            case BNZ: {
                getVar(midcode.x, "$t1");
                mipsCodeTable.emplace_back(mips_bne, "$t1", "$zero", midcode.z);
                break;
            }
            case LABEL: {
                mipsCodeTable.emplace_back(mips_label, midcode.z);
                break;
            }
            case PUSH: {
                getVar(midcode.z, "$t0");
                mipsCodeTable.emplace_back(mips_sw, "$t0", "$s1", "", 0);
                mipsCodeTable.emplace_back(mips_addi, "$s1", "$s1", "", 4);
                break;
            }
            case PUSHADDR: {
                //offset
                string offset = midcode.x; //[]
                getVar(offset, "$t0");
                mipsCodeTable.emplace_back(mips_li, "$t1", "", "", INTOFFSET);
                mipsCodeTable.emplace_back(mips_mult, "$t0", "$t1");
                mipsCodeTable.emplace_back(mips_mflo, "$t2");//[]*4
                //top
                string funcBelong = ident2var.find(midcode.z)->second.func;
                int addr = ident2var.find(midcode.z)->second.address;
                if (funcBelong.empty()) {
                    mipsCodeTable.emplace_back(mips_li, "$t3", "", "", addr);
                    mipsCodeTable.emplace_back(mips_add, "$t4", "$t3", "$gp");
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t5", "$t2", "$t4");//绝对地址
                }
                else {
                    mipsCodeTable.emplace_back(mips_li, "$t3", "", "", addr + 8);
                    mipsCodeTable.emplace_back(mips_add, "$t4", "$fp", "$t3");
                    //绝对地址
                    if ( ident2var.find(midcode.z)->second.dim[0] == 0) {
                        mipsCodeTable.emplace_back(mips_lw, "$t5", "$t4", "", 0);
                        mipsCodeTable.emplace_back(mips_move, "$t4", "$t5", "", 0);
                    }
                    mipsCodeTable.emplace_back(mips_add, "$t5", "$t4", "$t2");//绝对地址
                }

                mipsCodeTable.emplace_back(mips_sw, "$t5", "$s1", "", 0);
                mipsCodeTable.emplace_back(mips_addi, "$s1", "$s1", "", 4);
                break;
            }
            case CALL: {
                int nextFuncLength  = 0;
                for (int i = 0; i < funcvMap.size(); i++) {
                    if (funcvMap[i].ident == midcode.z) {
                        nextFuncLength = funcvMap[i].length;
                    }
                }
                mipsCodeTable.emplace_back(mips_sw, "$ra", "$fp", "", 4);
                mipsCodeTable.emplace_back(mips_move, "$sp", "$fp");
                mipsCodeTable.emplace_back(mips_subi, "$fp", "$fp", "", nextFuncLength);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", nextFuncLength);
                mipsCodeTable.emplace_back(mips_sw, "$t0", "$fp", "", 0);
                mipsCodeTable.emplace_back(mips_jal, midcode.z);
                mipsCodeTable.emplace_back(mips_lw, "$t0", "$sp", "", 0);
                mipsCodeTable.emplace_back(mips_move, "$fp", "$sp");
                mipsCodeTable.emplace_back(mips_add, "$sp", "$sp", "$t0");
                mipsCodeTable.emplace_back(mips_lw, "$ra", "$fp", "", 4);
                break;
            }
            case RET: {
                //main 不能去返回
                if (midCodeTable[index+1].op == EXIT) {
                    break;
                }
                string MAINRETURN = getInsLabel();
                mipsCodeTable.emplace_back(mips_beq, "$ra", "$zero", MAINRETURN);

                if (!midcode.z.empty()) {
                    getVar(midcode.z, "$s0");
                }
                mipsCodeTable.emplace_back(mips_jr, "$ra");
                mipsCodeTable.emplace_back(mips_label, MAINRETURN);
                break;
            }
            case RETVALUE: {
                pushVar("$s0", midcode.z);
                break;
            }
            case SCAN: {
                mipsCodeTable.emplace_back(mips_li, "$v0", "", "", 5);
                mipsCodeTable.emplace_back(mips_syscall);
                pushVar("$v0", midcode.z);
                break;
            }
            case PRINTD: {
                getVar(midcode.z, "$a0");
                mipsCodeTable.emplace_back(mips_li, "$v0", "", "", 1);
                mipsCodeTable.emplace_back(mips_syscall);
                break;
            }
            case PRINTS: {
                mipsCodeTable.emplace_back(mips_la, "$a0", midcode.z);
                mipsCodeTable.emplace_back(mips_li, "$v0", "", "", 4);
                mipsCodeTable.emplace_back(mips_syscall);
                break;
            }
//            case CONST: {
//                break;
//            }
//            case CONSTARRAY: {
//                break;
//            }
//            case ARRAY: {
//                break;
//            }
//            case VAR: {
//                break;
//            }
            case FUNC: {
                if (midcode.x == funcvMap[0].ident) {
                    mipsCodeTable.emplace_back(mips_j, "main");
                }
                mipsCodeTable.emplace_back(mips_label, midcode.x);//函数名label
                break;
            }
            case PARAM: {
                paraCount += 1;
                if (midCodeTable[index + 1].op != PARAM) {
                    for (int i = paraCount; i > 0; i--) {
                        mipsCodeTable.emplace_back(mips_subi, "$s1", "$s1", "", 4);
                        mipsCodeTable.emplace_back(mips_lw, "$t0", "$s1", "", 0);
                        mipsCodeTable.emplace_back(mips_sw, "$t0", "$fp", "", 4*i + 4);
                    }
                    paraCount = 0;
                }
                break;
            }
            case GETARRAY: {
                //offset
                string offset = midcode.y; //[]
                getVar(offset, "$t0");
                mipsCodeTable.emplace_back(mips_li, "$t1", "", "", INTOFFSET);
                mipsCodeTable.emplace_back(mips_mult, "$t0", "$t1");
                mipsCodeTable.emplace_back(mips_mflo, "$t2");//[]*4
                //top
                string funcBelong = ident2var.find(midcode.x)->second.func;
                int addr = ident2var.find(midcode.x)->second.address;
                if (funcBelong.empty()) {
                    mipsCodeTable.emplace_back(mips_li, "$t3", "", "", addr);
                    mipsCodeTable.emplace_back(mips_add, "$t4", "$t3", "$gp");
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t5", "$t2", "$t4");//绝对地址
                }
                else {
                    mipsCodeTable.emplace_back(mips_li, "$t3", "", "", addr + 8);
                    mipsCodeTable.emplace_back(mips_add, "$t4", "$fp", "$t3");
                    //绝对地址
                    if ( ident2var.find(midcode.x)->second.dim[0] == 0) {
                        mipsCodeTable.emplace_back(mips_lw, "$t5", "$t4", "", 0);
                        mipsCodeTable.emplace_back(mips_move, "$t4", "$t5", "", 0);
                    }
                    mipsCodeTable.emplace_back(mips_add, "$t5", "$t4", "$t2");//绝对地址
                }
                mipsCodeTable.emplace_back(mips_lw, "$t6", "$t5", "", 0);
                pushVar("$t6", midcode.z);
                break;
            }
            case PUTARRAY: {
                //offset
                string offset = midcode.x; //[]
                getVar(offset, "$t0");
                mipsCodeTable.emplace_back(mips_li, "$t1", "", "", INTOFFSET);
                mipsCodeTable.emplace_back(mips_mult, "$t0", "$t1");
                mipsCodeTable.emplace_back(mips_mflo, "$t2");//[]*4
                //top
                string funcBelong = ident2var.find(midcode.z)->second.func;
                int addr = ident2var.find(midcode.z)->second.address;
                if (funcBelong.empty()) {
                    mipsCodeTable.emplace_back(mips_li, "$t3", "", "", addr);
                    mipsCodeTable.emplace_back(mips_add, "$t4", "$t3", "$gp");
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t5", "$t2", "$t4");
                }
                else {
                    mipsCodeTable.emplace_back(mips_li, "$t3", "", "", addr + 8);
                    mipsCodeTable.emplace_back(mips_add, "$t4", "$fp", "$t3");
                    if ( ident2var.find(midcode.z)->second.dim[0] == 0) {
                        mipsCodeTable.emplace_back(mips_lw, "$t5", "$t4", "", 0);
                        mipsCodeTable.emplace_back(mips_move, "$t4", "$t5", "", 0);
                    }
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t5", "$t4", "$t2");
                }

                getVar(midcode.y, "$t6");
                mipsCodeTable.emplace_back(mips_sw, "$t6", "$t5", "", 0);
                break;
            }
            case EXIT: {
                mipsCodeTable.emplace_back(mips_li, "$v0", "", "", 10);
                mipsCodeTable.emplace_back(mips_syscall);
                break;
            }
            default:break;
        }
    }
}


void outputMipsCode(ofstream& mipsCodefile) {
    for (int i = 0; i < mipsCodeTable.size(); i++) {
        mipsCode mc = mipsCodeTable[i];
        switch (mc.op) {
            case mips_add:
                mipsCodefile << "add " << mc.z << "," << mc.x << "," << mc.y << "\n";
                break;
            case mips_addi:
                mipsCodefile << "addi " << mc.z << "," << mc.x << "," << mc.imm << "\n";
                break;
            case mips_sub:
                mipsCodefile << "sub " << mc.z << "," << mc.x << "," << mc.y << "\n";
                break;
            case mips_subi:
                mipsCodefile << "subi " << mc.z << "," << mc.x << "," << mc.imm << "\n";
                break;
            case mips_and:
                mipsCodefile << "and " << mc.z << "," << mc.x << "," << mc.y << "\n";
                break;
            case mips_andi:
                mipsCodefile << "andi " << mc.z << "," << mc.x << "," << mc.imm << "\n";
                break;
            case mips_or:
                mipsCodefile << "add " << mc.z << "," << mc.x << "," << mc.y << "\n";
                break;
            case mips_ori:
                mipsCodefile << "addi " << mc.z << "," << mc.x << "," << mc.imm << "\n";
                break;
            case mips_mult:
                mipsCodefile << "mult " << mc.z << "," << mc.x << "\n";
                break;
            case mips_div:
                mipsCodefile << "div " << mc.z << "," << mc.x << "\n";
                break;
            case mips_mflo:
                mipsCodefile << "mflo " << mc.z << "\n";
                break;
            case mips_mfhi:
                mipsCodefile << "mfhi " << mc.z << "\n";
                break;
            case mips_slt:
                mipsCodefile << "slt " << mc.z << "," << mc.x << "," << mc.y << "\n";
                break;
            case mips_beq:
                mipsCodefile << "beq " << mc.z << "," << mc.x << "," << mc.y << "\n";
                break;
            case mips_bne:
                mipsCodefile << "bne " << mc.z << "," << mc.x << "," << mc.y << "\n";
                break;
            case mips_j:
                mipsCodefile << "j " << mc.z << "\n";
                break;
            case mips_jal:
                mipsCodefile << "jal " << mc.z << "\n";
                break;
            case mips_jr:
                mipsCodefile << "jr " << mc.z << "\n";
                break;
            case mips_lw:
                mipsCodefile << "lw " << mc.z << "," << mc.imm << "(" << mc.x << ")\n";
                break;
            case mips_sw:
                mipsCodefile << "sw " << mc.z << "," << mc.imm << "(" << mc.x << ")\n";
                break;
            case mips_syscall:
                mipsCodefile << "syscall\n";
                break;
            case mips_li:
                mipsCodefile << "li " << mc.z << "," << mc.imm << "\n";
                break;
            case mips_la:
                mipsCodefile << "la " << mc.z << "," << mc.x << "\n";
                break;
            case mips_move:
                mipsCodefile << "move " << mc.z << "," << mc.x << "\n";
                break;
            case mips_dataSeg:
                mipsCodefile << ".data\n";
                break;
            case mips_asciizSeg:
                mipsCodefile << mc.z << ": .asciiz \"" << mc.x << "\"\n";
                break;
            case mips_textSeg:
                mipsCodefile << ".text\n";
                break;
            case mips_label:
                mipsCodefile << mc.z << ":\n";
                break;
            default:
                break;
        }
    }
}
