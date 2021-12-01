#include <iostream>
#include <fstream>
#include "mipscode.h"
#include "vTable.h"
#include "vector"
#include "midcode.h"
#include "map"
#include "optimize.h"

extern vector<midCode> midCodeTable;

//基本快
vector<int> block_beginNumber;
vector<int> block_endNumber;
vector<Block> blocks;

//中间变量
string insReg[3] = {"$t3", "$t4", "$t5"};
bool insRegUse[3] = {false, false, false};
string insRegVar[3] = {"", "", ""};

bool isInsVar(string ident) {
    if (ident[0] == 'i') {
        return true;
    }
    return false;
}

//窥孔优化
void digHole() {
    vector<midCode> neoMidCodeTable;
    for (int i = 0; i < midCodeTable.size(); i++) {

        //ins_10 = ins_9 % 10
        //variable_3 = ins_10
        if (i < midCodeTable.size()-1) {
            midCode midcode1 = midCodeTable[i];
            midCode midcode2 = midCodeTable[i+1];
            if ((0 <= midcode1.op) && (midcode1.op <= 4) && (midcode2.op == ASSIGNOP) &&
                isInsVar(midcode1.z) && (midcode1.z == midcode2.x)) {
                neoMidCodeTable.emplace_back(midcode1.op, midcode2.z, midcode1.x, midcode1.y);
                i += 1;
                continue;
            }
        }

        //default
        neoMidCodeTable.push_back(midCodeTable[i]);
    }
    midCodeTable = neoMidCodeTable;
}

void genBlock() {
    //begin_number
    block_beginNumber.emplace_back(0);
    for (int i = 0; i < midCodeTable.size(); i++) {
        switch (midCodeTable[i].op) {
            case GOTO:
            case BZ:
            case BNZ:
            case CALL:
            case RET:
                {
                    block_beginNumber.emplace_back(i+1);
                    break;
                }
            case LABEL:
            case FUNC:
                {
                    if (i != block_beginNumber[block_beginNumber.size()-1]) {
                        block_beginNumber.emplace_back(i);
                    }
                    break;
                }
            default:
                break;
        }
    }
    //end_number
    for (int i = 1; i < block_beginNumber.size(); i++) {
        block_endNumber.push_back(block_beginNumber[i]-1);
    }
    block_endNumber.push_back(midCodeTable.size()-1);

    //genblock
    for (int i = 0; i < block_beginNumber.size(); i++) {
        blocks.emplace_back(i, block_beginNumber[i], block_endNumber[i]);
        for (int j = block_beginNumber[i]; j <= block_endNumber[i]; j++) {
            blocks[blocks.size()-1].midCodeVector.push_back(midCodeTable[j]);
        }
    }

//    for(int i = 0; i < block_beginNumber.size(); i++) {
//        cout << block_beginNumber[i] << "," << block_endNumber[i] << endl;
//    }
}

//查找空中间变量寄存器
//返回序号，-1无
int findAvaInsReg() {
    for (int i=0; i<3; i++){
        if (insRegUse[i] == false) {
            return i;
        }
    }
    return -1;
}
//第一次出现的ins_xx 分配Reg
string putIns(string ident){
    if (!isInsVar(ident)) {
        return ident;
    }
    int index = findAvaInsReg();
    if (index != -1){
        insRegUse[index] = true;
        insRegVar[index] = ident;
        return insReg[index];
    }
    else {
        return ident;
    }
}
//查看ins_xx 是否已经分配了Reg 回收
string getIns(string ident){
    if (!isInsVar(ident)) {
        return ident;
    }
    int index = -1;
    for (int i = 0; i < 3; i++) {
        if (insRegVar[i] == ident) {
            index = i;
        }
    }
    if (index != -1) {
        insRegUse[index] = false;
        insRegVar[index] = "";
        return insReg[index];
    }
    else {
        return ident;
    }
}
//$t3, $t4, $t5中间变量使用
void changeIns2Reg() {
    for (int i = 0; i < blocks.size(); i++) {
        Block& curBlock = blocks[i];
        for (int j = 0; j < curBlock.midCodeVector.size(); j++) {
            switch (curBlock.midCodeVector[j].op) {
                //put z get x y
                case PLUSOP:
                case MINUOP:
                case MULTOP:
                case DIVOP:
                case MODOP:
                case ANDOP:
                case OROP:
                case LSSOP:
                case LEQOP:
                case GREOP:
                case GEQOP:
                case EQLOP:
                case NEQOP:
                case GETARRAY:
                case PUTARRAY:
                {
                    curBlock.midCodeVector[j].z = putIns(curBlock.midCodeVector[j].z);
                    curBlock.midCodeVector[j].x = getIns(curBlock.midCodeVector[j].x);
                    curBlock.midCodeVector[j].y = getIns(curBlock.midCodeVector[j].y);
                    break;
                }
                //put z get x
                case NOTOP:
                case ASSIGNOP:{
                    curBlock.midCodeVector[j].z = putIns(curBlock.midCodeVector[j].z);
                    curBlock.midCodeVector[j].x = getIns(curBlock.midCodeVector[j].x);
                    break;
                }
                //get x
                case BZ:
                case BNZ:{
                    curBlock.midCodeVector[j].x = getIns(curBlock.midCodeVector[j].x);
                    break;
                }
                //get z
                case PUSH:
                case RET:
                case PRINTD:{
                    curBlock.midCodeVector[j].z = getIns(curBlock.midCodeVector[j].z);
                    break;
                }
                //get z x
                case PUSHADDR:{
                    curBlock.midCodeVector[j].z = getIns(curBlock.midCodeVector[j].z);
                    curBlock.midCodeVector[j].x = getIns(curBlock.midCodeVector[j].x);
                    break;
                }
                //put z
                case RETVALUE:
                case SCAN:{
                    curBlock.midCodeVector[j].z = putIns(curBlock.midCodeVector[j].z);
                    break;
                }
                default:
                    break;
            }
        }
        //寄存器回写
        midCode lastMidCode = curBlock.midCodeVector[curBlock.midCodeVector.size()-1];
        curBlock.midCodeVector.pop_back();
        for (int k = 0; k < 3; k++) {
            if (insRegUse[k]) {
                insRegUse[k] = false;
                curBlock.midCodeVector.emplace_back(SAVE, insReg[k], insRegVar[k]);
                insRegVar[k] = "";
            }
        }
        curBlock.midCodeVector.push_back(lastMidCode);
    }
}

//更新midCodeTable
void refreshMidCodeTable(){
    midCodeTable.clear();
    for (int i = 0; i < blocks.size(); i++) {
        for (int j = 0; j < blocks[i].midCodeVector.size(); j++){
            midCodeTable.push_back(blocks[i].midCodeVector[j]);
        }
    }
}


//主优化函数
void optimize() {
    digHole();
    genBlock();
    changeIns2Reg();
    refreshMidCodeTable();
}