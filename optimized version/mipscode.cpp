#include <iostream>
#include <fstream>
#include "mipscode.h"
#include "vTable.h"
#include "vector"
#include "midcode.h"
#include "optimize.h"
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
 *  $t0, $t1, $t2 运算用
 *  $t3, $t4, $t5 ins_Var 中间变量使用
 *  $a0, $v0 与 syscall使用
 *  $s0 函数返回值
 *  hi, lo
 */

//中间代码的ident是不是寄存器
bool isReg(string ident) {
    if (ident[0] == '$') {
        return true;
    }
    else {
        return false;
    }
}

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

//完成 num -> $dst / var -> $dst / $t1, $t2
string getVar(string varIdent, int lorR) {
    if (isNumber(varIdent)) {
        if (lorR == 1) {
            mipsCodeTable.emplace_back(mips_li, "$t1", "", "", stoi(varIdent));
            return "$t1";
        }
        else {
            mipsCodeTable.emplace_back(mips_li, "$t2", "", "", stoi(varIdent));
            return "$t2";
        }
    }
    else if (isReg(varIdent)) {
        return varIdent;
    }
    else {
        string funcBelong = ident2var.find(varIdent)->second.func;
        int addr = ident2var.find(varIdent)->second.address;
        //global
        if (funcBelong.empty()) {
            if (lorR == 1) {
                mipsCodeTable.emplace_back(mips_lw, "$t1", "$gp", "", addr);
                return "$t1";
            }
            else {
                mipsCodeTable.emplace_back(mips_lw, "$t2", "$gp", "", addr);
                return "$t2";
            }
        }
        //local
        else {
            if (lorR == 1) {
                mipsCodeTable.emplace_back(mips_lw, "$t1", "$fp", "", addr + 8);
                return "$t1";
            }
            else {
                mipsCodeTable.emplace_back(mips_lw, "$t2", "$fp", "", addr + 8);
                return "$t2";
            }
        }
    }
}
//完成 $src -> var
void pushVar(string src, string varIdent) {
    if (isReg(varIdent)) {
        mipsCodeTable.emplace_back(mips_move, varIdent, src);
        return;
    }
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
                    string string2 = getVar(midcode.y, 2);
                    mipsCodeTable.emplace_back(mips_addi, "$t0", string2, "", stoi(midcode.x));
                    pushVar("$t0", midcode.z);
                }
                else if (isNumber(midcode.y)) {
                    string string1 = getVar(midcode.x, 1);
                    mipsCodeTable.emplace_back(mips_addi, "$t0", string1, "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    string string1 = getVar(midcode.x, 1);
                    string string2 = getVar(midcode.y, 2);
                    mipsCodeTable.emplace_back(mips_add, "$t0", string1, string2);
                    pushVar("$t0", midcode.z);
                }
                break;
            }
            case MINUOP: {
                if (isNumber(midcode.y)) {
                    string string1 = getVar(midcode.x, 1);
                    mipsCodeTable.emplace_back(mips_subi, "$t0", string1, "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    string string1 = getVar(midcode.x, 1);
                    string string2 = getVar(midcode.y, 2);
                    mipsCodeTable.emplace_back(mips_sub, "$t0", string1, string2);
                    pushVar("$t0", midcode.z);
                }
                break;
            }
            case MULTOP: {
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_mult, string1, string2);
                mipsCodeTable.emplace_back(mips_mflo, "$t0");
                pushVar("$t0", midcode.z);
                break;
            }
            case DIVOP: {
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_div, string1, string2);
                mipsCodeTable.emplace_back(mips_mflo, "$t0");
                pushVar("$t0", midcode.z);
                break;
            }
            case MODOP: {
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_div, string1, string2);
                mipsCodeTable.emplace_back(mips_mfhi, "$t0");
                pushVar("$t0", midcode.z);
                break;
            }
            case ANDOP: {
                if (isNumber(midcode.x)) {
                    string string2 = getVar(midcode.y, 2);
                    mipsCodeTable.emplace_back(mips_andi, "$t0", string2, "", stoi(midcode.x));
                    pushVar("$t0", midcode.z);
                }
                else if (isNumber(midcode.y)) {
                    string string1 = getVar(midcode.x, 1);
                    mipsCodeTable.emplace_back(mips_andi, "$t0", string1, "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    string string1 = getVar(midcode.x, 1);
                    string string2 = getVar(midcode.y, 2);
                    mipsCodeTable.emplace_back(mips_and, "$t0", string1, string2);
                    pushVar("$t0", midcode.z);
                }
                break;
            }
            case OROP: {
                if (isNumber(midcode.x)) {
                    string string2 = getVar(midcode.y, 2);
                    mipsCodeTable.emplace_back(mips_ori, "$t0", string2, "", stoi(midcode.x));
                    pushVar("$t0", midcode.z);
                }
                else if (isNumber(midcode.y)) {
                    string string1 = getVar(midcode.x, 1);
                    mipsCodeTable.emplace_back(mips_ori, "$t0", string1, "", stoi(midcode.y));
                    pushVar("$t0", midcode.z);
                }
                else {
                    string string1 = getVar(midcode.x, 1);
                    string string2 = getVar(midcode.y, 2);
                    mipsCodeTable.emplace_back(mips_or, "$t0", string1, string2);
                    pushVar("$t0", midcode.z);
                }
                break;
            }
            case LSSOP: {
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_slt, "$t0", string1, string2);
                pushVar("$t0", midcode.z);
                break;
            }
            case LEQOP: {
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_slt, "$t0", string1, string2);
                string ins = getInsLabel();
                mipsCodeTable.emplace_back(mips_bne, string1, string2, ins);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, ins);
                pushVar("$t0", midcode.z);
                break;
            }
            case GREOP: {
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_slt, "$t0", string2, string1);
                pushVar("$t0", midcode.z);
                break;
            }
            case GEQOP: {
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_slt, "$t0", string2, string1);
                string ins = getInsLabel();
                mipsCodeTable.emplace_back(mips_bne, string1, string2, ins);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, ins);
                pushVar("$t0", midcode.z);
                break;
            }
            case EQLOP: {
                string label = getInsLabel();
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 0);
                mipsCodeTable.emplace_back(mips_bne, string1, string2, label);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, label);
                pushVar("$t0", midcode.z);
                break;
            }
            case NEQOP: {
                string label = getInsLabel();
                string string1 = getVar(midcode.x, 1);
                string string2 = getVar(midcode.y, 2);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 0);
                mipsCodeTable.emplace_back(mips_beq, string1, string2, label);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, label);
                pushVar("$t0", midcode.z);
                break;
            }
            case NOTOP: {
                string label1 = getInsLabel();
                string label2 = getInsLabel();
                string string1 = getVar(midcode.x, 1);
                mipsCodeTable.emplace_back(mips_beq, string1, "$zero", label1);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 0);
                mipsCodeTable.emplace_back(mips_j, label2);
                mipsCodeTable.emplace_back(mips_label, label1);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", 1);
                mipsCodeTable.emplace_back(mips_label, label2);
                pushVar("$t0", midcode.z);
                break;
            }
            case ASSIGNOP: {
                string string1 = getVar(midcode.x, 1);
                mipsCodeTable.emplace_back(mips_move, "$t0", string1);
                pushVar("$t0", midcode.z);
                break;
            }
            case GOTO: {
                mipsCodeTable.emplace_back(mips_j, midcode.z);
                break;
            }
            case BZ: {
                string string1 = getVar(midcode.x, 1);
                mipsCodeTable.emplace_back(mips_beq, string1, "$zero", midcode.z);
                break;
            }
            case BNZ: {
                string string1 = getVar(midcode.x, 1);
                mipsCodeTable.emplace_back(mips_bne, string1, "$zero", midcode.z);
                break;
            }
            case LABEL: {
                mipsCodeTable.emplace_back(mips_label, midcode.z);
                break;
            }
            case PUSH: {
                string string1 = getVar(midcode.z, 1);
                mipsCodeTable.emplace_back(mips_sw, string1, "$s1", "", 0);
                mipsCodeTable.emplace_back(mips_addi, "$s1", "$s1", "", 4);
                break;
            }
            case PUSHADDR: {
                //offset
                string offset = midcode.x; //[]
                string string1 = getVar(offset, 1);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", INTOFFSET);
                mipsCodeTable.emplace_back(mips_mult, "$t0", string1);
                mipsCodeTable.emplace_back(mips_mflo, "$t0");//[]*4
                //top
                string funcBelong = ident2var.find(midcode.z)->second.func;
                int addr = ident2var.find(midcode.z)->second.address;
                if (funcBelong.empty()) {
                    mipsCodeTable.emplace_back(mips_li, "$t1", "", "", addr);
                    mipsCodeTable.emplace_back(mips_add, "$t1", "$t1", "$gp");
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t2", "$t0", "$t1");//绝对地址
                }
                else {
                    mipsCodeTable.emplace_back(mips_li, "$t1", "", "", addr + 8);
                    mipsCodeTable.emplace_back(mips_add, "$t1", "$fp", "$t1");
                    //绝对地址
                    if ( ident2var.find(midcode.z)->second.dim[0] == 0) {
                        mipsCodeTable.emplace_back(mips_lw, "$t1", "$t1", "", 0);
                    }
                    mipsCodeTable.emplace_back(mips_add, "$t2", "$t0", "$t1");//绝对地址
                }

                mipsCodeTable.emplace_back(mips_sw, "$t2", "$s1", "", 0);
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
                    string string1 = getVar(midcode.z, 1);
                    mipsCodeTable.emplace_back(mips_move, "$s0", string1);
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
                string string1 = getVar(midcode.z, 1);
                mipsCodeTable.emplace_back(mips_move, "$a0", string1);
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
                string string1 = getVar(offset, 1);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", INTOFFSET);
                mipsCodeTable.emplace_back(mips_mult, "$t0", string1);
                mipsCodeTable.emplace_back(mips_mflo, "$t0");//[]*4
                //top
                string funcBelong = ident2var.find(midcode.x)->second.func;
                int addr = ident2var.find(midcode.x)->second.address;
                if (funcBelong.empty()) {
                    mipsCodeTable.emplace_back(mips_li, "$t1", "", "", addr);
                    mipsCodeTable.emplace_back(mips_add, "$t1", "$t1", "$gp");
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t2", "$t0", "$t1");//绝对地址
                }
                else {
                    mipsCodeTable.emplace_back(mips_li, "$t1", "", "", addr + 8);
                    mipsCodeTable.emplace_back(mips_add, "$t1", "$fp", "$t1");
                    //绝对地址
                    if ( ident2var.find(midcode.x)->second.dim[0] == 0) {
                        mipsCodeTable.emplace_back(mips_lw, "$t1", "$t1", "", 0);
                    }
                    mipsCodeTable.emplace_back(mips_add, "$t2", "$t1", "$t0");//绝对地址
                }
                mipsCodeTable.emplace_back(mips_lw, "$t1", "$t2", "", 0);
                pushVar("$t1", midcode.z);
                break;
            }
            case PUTARRAY: {
                //offset
                string offset = midcode.x; //[]
                string string1 = getVar(offset, 1);
                mipsCodeTable.emplace_back(mips_li, "$t0", "", "", INTOFFSET);
                mipsCodeTable.emplace_back(mips_mult, string1, "$t0");
                mipsCodeTable.emplace_back(mips_mflo, "$t0");//[]*4
                //top
                string funcBelong = ident2var.find(midcode.z)->second.func;
                int addr = ident2var.find(midcode.z)->second.address;
                if (funcBelong.empty()) {
                    mipsCodeTable.emplace_back(mips_li, "$t1", "", "", addr);
                    mipsCodeTable.emplace_back(mips_add, "$t1", "$t1", "$gp");
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t2", "$t0", "$t1");
                }
                else {
                    mipsCodeTable.emplace_back(mips_li, "$t1", "", "", addr + 8);
                    mipsCodeTable.emplace_back(mips_add, "$t1", "$fp", "$t1");
                    if ( ident2var.find(midcode.z)->second.dim[0] == 0) {
                        mipsCodeTable.emplace_back(mips_lw, "$t1", "$t1", "", 0);
                    }
                    //绝对地址
                    mipsCodeTable.emplace_back(mips_add, "$t2", "$t1", "$t0");
                }

                string1 = getVar(midcode.y, 1);
                mipsCodeTable.emplace_back(mips_sw, string1, "$t2", "", 0);
                break;
            }
            case EXIT: {
                mipsCodeTable.emplace_back(mips_li, "$v0", "", "", 10);
                mipsCodeTable.emplace_back(mips_syscall);
                break;
            }
            case SAVE: {
                pushVar(midcode.z, midcode.x);
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
