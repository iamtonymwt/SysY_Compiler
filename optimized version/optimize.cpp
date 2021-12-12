#include <iostream>
#include <fstream>
#include "mipscode.h"
#include "vTable.h"
#include "vector"
#include "midcode.h"
#include "map"
#include "optimize.h"

extern vector<midCode> midCodeTable;
extern vector<Variable> vTable;
set<string> globalVar;

//基本快
vector<int> block_beginNumber;
vector<int> block_endNumber;
vector<Block> blocks;
map<string, vector<int>> func2blocks;

//中间变量
string insReg[3] = {"$t3", "$t4", "$t5"};
bool insRegUse[3] = {false, false, false};
string insRegVar[3] = {"", "", ""};

//全局寄存器
string globalReg[11] = {"$t6","$t7","$t8","$t9","$s1","$s2","$s3","$s4","$s5","$s6","$s7"};
//bool globalRegUse[11] = {false, false, false, false, false, false, false, false, false, false, false};
string globalRegVar[11] = {"","","","","","","","","","",""};
bool globalRegDirty[11] = {false, false, false, false, false, false, false, false, false, false, false};
map<string, int> varUseTimes;
vector<string> funcParams;


//找出全局变量
void initGlobalVar() {
    for (int i = 0; i < vTable.size(); i++) {
        if (vTable[i].func.empty()) {
            globalVar.insert(vTable[i].fName);
        }
    }
}

bool isInsVar(string ident) {
    if (ident[0] == 'i') {
        return true;
    }
    return false;
}
bool isGlobalVar(string ident) {
    if (ident[0] == 'v') {
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

        //BZ label_3($t3 == 0)
        //variable_5 = 1
        //GOTO label_4
        //LABEL: label_3
        //variable_5 = 0
        //LABEL: label_4
        //BZ label_1(variable_5 == 0)
//        if (i < midCodeTable.size()-6) {
//            if ((midCodeTable[i].op == BZ)
//                && (midCodeTable[i+1].op == ASSIGNOP)
//                && (midCodeTable[i+2].op == GOTO)
//                && (midCodeTable[i+3].op == LABEL)
//                && (midCodeTable[i+4].op == ASSIGNOP)
//                && (midCodeTable[i+5].op == LABEL)
//                && (midCodeTable[i+6].op == BZ)
//                && (midCodeTable[i+1].z == midCodeTable[i+4].z)
//                && (midCodeTable[i+4].z == midCodeTable[i+6].x)){
//                neoMidCodeTable.emplace_back(BZ, midCodeTable[i+6].z, midCodeTable[i].x);
//                i += 6;
//                continue;
//            }
//        }


        //default
        neoMidCodeTable.push_back(midCodeTable[i]);
    }
    midCodeTable = neoMidCodeTable;
}

//生成基本块
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
//流图
int label2block(string label) {
    for (int i = 0; i < blocks.size(); i++) {
        if ((blocks[i].midCodeVector[0].op == LABEL) &&
                (blocks[i].midCodeVector[0].z == label)) {
            return blocks[i].number;
        }
    }
}
void linkBlocks() {
    string curFuncName = "";
    Block exitBlock = blocks[blocks.size()-1];//exit block
    for (int i = 0; i < blocks.size()-1; i++) {
        if (blocks[i].midCodeVector[0].op == FUNC) {
            curFuncName = blocks[i].midCodeVector[0].x;
            vector<int> insBlocks;
            func2blocks.emplace(make_pair(curFuncName, insBlocks));
        }
        if (!curFuncName.empty()) {
            func2blocks.find(curFuncName)->second.push_back(blocks[i].number);
            midCode lastMidCode = blocks[i].midCodeVector[blocks[i].midCodeVector.size() - 1];
            switch (lastMidCode.op) {
                case GOTO: {
                    blocks[i].setNextBlock(label2block(lastMidCode.z));
                    break;
                }
                case BZ:
                case BNZ: {
                    blocks[i].setNextBlock(label2block(lastMidCode.z));
                    blocks[i].setNextBlock(blocks[i + 1].number);
                    break;
                }
                case CALL: {
                    blocks[i].setNextBlock(blocks[i + 1].number);
                    break;
                }
                case RET: {
                    blocks[i].setNextBlock(exitBlock.number);
                    break;
                }
                default:
                    blocks[i].setNextBlock(blocks[i + 1].number);
                    break;
            }
        }
    }
}
//块的use def
void add2use(Block& block, string ident) {
    if (isNumber(ident)) {
       return;
    }
    if (block.def.find(ident) == block.def.end()) {
        block.use.insert(ident);
    }
}
void add2def(Block& block, string ident) {
    if (isNumber(ident)) {
        return;
    }
    if (block.use.find(ident) == block.use.end()) {
        block.def.insert(ident);
    }
}
void calUseDef() {
    auto iter = blocks.begin();
    while (iter != blocks.end()) {
        iter->use.clear();
        iter->def.clear();
        iter ++;
    }

    for (int i = 0; i < blocks.size(); i++) {
        Block& curBlock = blocks[i];
        for (int j = 0; j < curBlock.midCodeVector.size(); j++) {
            midCode curMidCode = curBlock.midCodeVector[j];
            switch (curMidCode.op) {
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
                {
                    add2use(curBlock, curMidCode.x);
                    add2use(curBlock,curMidCode.y);
                    add2def(curBlock,curMidCode.z);
                    break;
                }
                case GETARRAY:
                {
                    add2use(curBlock,curMidCode.y);
                    add2def(curBlock,curMidCode.z);
                    break;
                }
                case PUTARRAY:
                {
                    add2use(curBlock, curMidCode.x);
                    add2use(curBlock,curMidCode.y);
                    break;
                }
                    //put z get x
                case NOTOP:
                case ASSIGNOP:{
                    add2use(curBlock, curMidCode.x);
                    add2def(curBlock,curMidCode.z);
                    break;
                }
                    //get x
                case BZ:
                case BNZ:{
                    add2use(curBlock, curMidCode.x);
                    break;
                }
                    //get z
                case PUSH:
                case RET:
                case PRINTD:{
                    add2use(curBlock, curMidCode.z);
                    break;
                }
                    //get z x
                case PUSHADDR:{
//                    add2use(curBlock, curMidCode.z);
                    add2use(curBlock,curMidCode.x);
                    break;
                }
                    //put z
                case RETVALUE:
                case SCAN:{
                    add2def(curBlock,curMidCode.z);
                    break;
                }
                default:
                    break;
            }
        }
    }
}
//块的in out
bool calBlockInOut(Block& block) {
    bool updated = false;
    if (block.nextBlock1 != -1) {
        set<string> nextIn1 = blocks[block.nextBlock1].in;
        auto iter = nextIn1.begin();
        while (iter != nextIn1.end()) {
            if (block.out.find(*iter) == block.out.end()){
                block.out.insert(*iter);
                updated = true;
            }
            iter ++;
        }
    }
    if (block.nextBlock2 != -1) {
        set<string> nextIn2 = blocks[block.nextBlock2].in;
        auto iter = nextIn2.begin();
        while (iter != nextIn2.end()) {
            if (block.out.find(*iter) == block.out.end()){
                block.out.insert(*iter);
                updated = true;
            }
            iter ++;
        }
    }

    set<string> newIn;
    auto iter = block.out.begin();
    while (iter != block.out.end()) {
        newIn.insert(*iter);
        iter ++;
    }
    iter = block.use.begin();
    while (iter != block.use.end()) {
        newIn.insert(*iter);
        iter ++;
    }
    iter = block.def.begin();
    while (iter != block.def.end()) {
        newIn.erase(*iter);
        iter ++;
    }
    if (newIn.size() != block.in.size()) {
        updated = true;
    }
    block.in = newIn;
    return updated;
}
void calInOut() {
    auto i = blocks.begin();
    while (i != blocks.end()) {
        i->out.clear();
        i->in.clear();
        i ++;
    }

    auto iter = func2blocks.begin();
    while(iter != func2blocks.end()) {
        vector<int> indexs = iter->second;
        while(true) {
            bool updated = false;
            for (int i = indexs.size()-1; i >= 0; i--) {
                Block& block = blocks[indexs[i]];
                updated = updated || calBlockInOut(block);
            }
            if (!updated) {
                break;
            }
        }
        iter ++;
    }
}

//死代码删除
bool delDeadCodeBlock(Block& block) {
    bool updated = false;
    set<string> cantDelSet;
    for (auto iter = block.midCodeVector.end()-1; iter >= block.midCodeVector.begin();) {
        switch ((*iter).op) {
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
            {
                if ((cantDelSet.find((*iter).z) == cantDelSet.end()) &&
                    (block.out.find((*iter).z) == block.out.end()) &&
                    (globalVar.find((*iter).z) == globalVar.end())) {
                    iter = block.midCodeVector.erase(iter);
                    iter --;
                    updated = true;
                }
                else {
                    cantDelSet.erase((*iter).z);
                    cantDelSet.insert((*iter).x);
                    cantDelSet.insert((*iter).y);
                    iter --;
                }
                break;
            }
            case GETARRAY:{
                if ((cantDelSet.find((*iter).z) == cantDelSet.end()) &&
                    (block.out.find((*iter).z) == block.out.end()) &&
                    (globalVar.find((*iter).z) == globalVar.end())) {
                    iter = block.midCodeVector.erase(iter);
                    iter --;
                    updated = true;
                }
                else {
                    cantDelSet.erase((*iter).z);
                    cantDelSet.insert((*iter).y);
                    iter --;
                }
                break;
            }
            case PUTARRAY:{
                cantDelSet.insert((*iter).x);
                cantDelSet.insert((*iter).y);
                iter --;
                break;
            }
                //put z get x
            case NOTOP:
            case ASSIGNOP:{
                if ((cantDelSet.find((*iter).z) == cantDelSet.end()) &&
                    (block.out.find((*iter).z) == block.out.end()) &&
                    (globalVar.find((*iter).z) == globalVar.end())) {
                    iter = block.midCodeVector.erase(iter);
                    iter --;
                    updated = true;
                }
                else {
                    cantDelSet.erase((*iter).z);
                    cantDelSet.insert((*iter).x);
                    iter --;
                }
                break;
            }
                //get x
            case BZ:
            case BNZ:{
                cantDelSet.insert((*iter).x);
                iter --;
                break;
            }
                //get z
            case PUSH:
            case RET:
            case PRINTD:{
                cantDelSet.insert((*iter).z);
                iter --;
                break;
            }
                //get z x
            case PUSHADDR:{
//                cantDelSet.insert((*iter).z);
                cantDelSet.insert((*iter).x);
                iter --;
                break;
            }
                //put z
            case RETVALUE:
            {
                if ((cantDelSet.find((*iter).z) == cantDelSet.end()) &&
                    (block.out.find((*iter).z) == block.out.end()) &&
                    (globalVar.find((*iter).z) == globalVar.end())) {
                    iter = block.midCodeVector.erase(iter);
                    iter --;
                    updated = true;
                }
                else {
                    cantDelSet.erase((*iter).z);
                    iter --;
                }
                break;
            }
            case SCAN: {
                cantDelSet.erase((*iter).z);
                iter --;
                break;
            }
            default:
                iter --;
                break;
        }
    }
    return updated;
}
void delDeadCode() {
    bool updated;
    do {
        updated = false;
        auto iter = blocks.begin();
        if (blocks.begin()->midCodeVector[0].op != FUNC) {//全局变量块
            iter ++;
        }
        while (iter != blocks.end()) {
            updated = updated || delDeadCodeBlock(*iter);
            iter ++;
        }
        calUseDef();
        calInOut();
    }
    while (updated);
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
        if (curBlock.midCodeVector.empty()) {
            continue;
        }
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

//func：一个变量出现一次
void varAddTimes(string varIdent) {
    if (isNumber(varIdent) ||
        isReg(varIdent) ||
        isInsVar(varIdent)){
        return;
    }
    //前三个参数不管的
    for (int i = 0; i < funcParams.size(); i++) {
        if (i > 2) {
            break;
        }
        if(funcParams[i] == varIdent) {
            return;
        }
    }

    if (varUseTimes.find(varIdent) == varUseTimes.end()) {
        varUseTimes.insert(make_pair(varIdent, 1));
    }
    else {
        varUseTimes.find(varIdent)->second += 1;
    }
}
//func：变量使用次数
void calFuncVarTimes(int beginBlock, int endBLock) {
    varUseTimes.clear();
    funcParams.clear();
    //参数
    int k = 1;
    while (blocks[beginBlock].midCodeVector[k].op == PARAM) {
        funcParams.push_back(blocks[beginBlock].midCodeVector[k].z);
        k++;
    }
    for (int i = beginBlock; i <= endBLock; i++) {
        Block& curBlock = blocks[i];
        for (int j = 0; j < curBlock.midCodeVector.size(); j++) {
            midCode curMidCode = curBlock.midCodeVector[j];
            switch (curMidCode.op) {
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
                {
                    varAddTimes(curMidCode.x);
                    varAddTimes(curMidCode.y);
                    varAddTimes(curMidCode.z);
                    break;
                }
                case GETARRAY:
                {
                    varAddTimes(curMidCode.y);
                    varAddTimes(curMidCode.z);
                    break;
                }
                case PUTARRAY:
                {
                    varAddTimes(curMidCode.x);
                    varAddTimes(curMidCode.y);
                    break;
                }
                    //put z get x
                case NOTOP:
                case ASSIGNOP:{
                    varAddTimes(curMidCode.x);
                    varAddTimes(curMidCode.z);
                    break;
                }
                    //get x
                case BZ:
                case BNZ:{
                    varAddTimes(curMidCode.x);
                    break;
                }
                    //get z
                case PUSH:
                case RET:
                case PRINTD:{
                    varAddTimes(curMidCode.z);
                    break;
                }
                    //get z x
                case PUSHADDR:{
//                    add2use(curBlock, curMidCode.z);
                    varAddTimes(curMidCode.x);
                    break;
                }
                    //put z
                case RETVALUE:
                case SCAN:{
                    varAddTimes(curMidCode.z);
                    break;
                }
                default:
                    break;
            }
        }
    }
}
//func：分配全局寄存器
void disGlobalReg() {
    int i = 0;
    while ((i < 11) && (varUseTimes.size() != 0)) {
        auto maxIter = varUseTimes.begin();
        auto iter = varUseTimes.begin();
        while (iter != varUseTimes.end()) {
            if (iter->second > maxIter->second) {
                maxIter = iter;
            }
            iter ++;
        }
        globalRegVar[i] = maxIter->first;
        varUseTimes.erase(maxIter);
        i += 1;
    }
}
//func: 找对应的GReg
string getGReg(bool isDef, string varIdent) {
    for (int i = 0; i < 11; i++) {
        if (globalRegVar[i] == varIdent) {
            if (isDef) {
                globalRegDirty[i] = true;
            }
            return globalReg[i];
        }
    }
    return varIdent;
}
//func：改写midcode
void changeGlobalReg(int beginBlock, int endBLock) {
    for (int i = beginBlock; i <= endBLock; i++) {
        Block& curBlock = blocks[i];
        auto iter = curBlock.midCodeVector.begin();
        while(iter != curBlock.midCodeVector.end()) {
            switch (iter->op) {
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
                {
                    iter->z = getGReg(true, iter->z);
                    iter->x = getGReg(false, iter->x);
                    iter->y = getGReg(false, iter->y);
                    break;
                }
                case PUTARRAY:{
                    iter->x = getGReg(false, iter->x);
                    iter->y = getGReg(false, iter->y);
                    break;
                }
                case GETARRAY:{
                    iter->z = getGReg(true, iter->z);
                    iter->y = getGReg(false, iter->y);
                    break;
                }
                    //put z get x
                case NOTOP:
                case ASSIGNOP:{
                    iter->z = getGReg(true, iter->z);
                    iter->x = getGReg(false, iter->x);
                    break;
                }
                    //get x
                case BZ:
                case BNZ:{
                    iter->x = getGReg(false, iter->x);
                    break;
                }
                    //get z
                case PUSH:
                case PRINTD:{
                    iter->z = getGReg(false, iter->z);
                    break;
                }
                    //get z x
                case PUSHADDR:{
                    iter->x = getGReg(false, iter->x);
                    break;
                }
                    //put z
                case RETVALUE:
                case SCAN:{
                    iter->z = getGReg(true, iter->z);
                    break;
                }
                case FUNC:{
                    iter ++;
                    for (int k = 0; k < 11; k++) {
                        if (!globalRegVar[k].empty()) {
                            iter = curBlock.midCodeVector.insert(iter, midCode(LOAD, globalReg[k], globalRegVar[k]));
                            iter ++;
                        }
                    }
                    iter --;
                    break;
                }
                case CALL:{
                    for (int k = 0; k < 11; k++) {
                        if (globalRegDirty[k]) {
                            iter = curBlock.midCodeVector.insert(iter, midCode(SAVE, globalReg[k], globalRegVar[k]));
                            iter ++;
                        }
                    }
                    iter ++;
                    for (int k = 0; k < 11; k++) {
                        if (!globalRegVar[k].empty()) {
                            iter = curBlock.midCodeVector.insert(iter, midCode(LOAD, globalReg[k], globalRegVar[k]));
                            iter ++;
                        }
                    }
                    iter --;
                    break;
                }
                case RET:{
                    iter->z = getGReg(false, iter->z);
                    for (int k = 0; k < 11; k++) {
                        if (!globalRegVar[k].empty() && (globalVar.find(globalRegVar[k])!=globalVar.end())) {
                            iter = curBlock.midCodeVector.insert(iter, midCode(SAVE, globalReg[k], globalRegVar[k]));
                            iter ++;
                        }
                    }
                    break;
                }
                case GOTO: {
                    if ((curBlock.nextBlock1 != -1 && curBlock.nextBlock1 < curBlock.number) ||
                        (curBlock.nextBlock2 != -1 && curBlock.nextBlock2 < curBlock.number)) {
                        for (int k = 0; k < 11; k++) {
                            if (globalRegDirty[k]) {
                                iter = curBlock.midCodeVector.insert(iter,midCode(SAVE, globalReg[k], globalRegVar[k]));
                                iter++;
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            iter ++;
        }
    }
}
//引用计数全局寄存器分配
void changeGlobal2Reg() {
    //funcsBeginPlace
    vector<int> funcsBeginPlace;
    for(int i = 0; i < blocks.size(); i++) {
        if (blocks[i].midCodeVector[0].op == FUNC) {
            funcsBeginPlace.push_back(blocks[i].number);
        }
    }
    funcsBeginPlace.push_back(blocks.size());
    //分配寄存器
    for(int i = 0; i < funcsBeginPlace.size() - 1; i++) {
        int beginBlock = funcsBeginPlace[i];
        int endBLock = funcsBeginPlace[i+1] - 1;
        int k = 0;
        while (k < 11) {
            globalRegVar[k] = "";
            globalRegDirty[k] = false;
            k++;
        }
        calFuncVarTimes(beginBlock, endBLock);
        disGlobalReg();
        changeGlobalReg(beginBlock, endBLock);
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
    initGlobalVar();//全局变量
    digHole();  //各种窥孔
    genBlock(); //生成基本块
    linkBlocks(); //生成流图
    calUseDef(); //计算块的use和def
    calInOut(); //计算块的in和out
    delDeadCode(); //删除死代码
    changeIns2Reg(); //中间变量Reg分配
    refreshMidCodeTable(); //block.midcode -> midcodeTable
    changeGlobal2Reg(); //全局变量Reg分配
    refreshMidCodeTable(); //block.midcode -> midcodeTable
}


/*useles
 * //查找空全局变量寄存器
//返回序号，-1无
int findAvaGlobalReg() {
    for (int i=0; i<10; i++){
        if (!globalRegUse[i]) {
            return i;
        }
    }
    return -1;
}
//查看global_var 是否已经分配了Reg
//RVal
string putGlobal(string ident, vector<midCode>& load){
    //不是variable
    if (!isGlobalVar(ident)) {
        return ident;
    }
    int index = -1;
    for (int i = 0; i < 10; i++) {
        if (globalRegVar[i] == ident) {
            index = i;
        }
    }
    //分配了寄存器
    if (index != -1) {
        return globalReg[index];
    }
    //未分配寄存器
    index = findAvaGlobalReg();
    //可以分配
    if (index != -1){
        globalRegUse[index] = true;
        globalRegVar[index] = ident;
        load.emplace_back(LOAD, globalReg[index], ident);
        return globalReg[index];
    }
    //不可分配
    else {
        return ident;
    }
}
//查看global_var 是否已经分配了Reg
//LVal
string getGlobal(string ident){
    if (!isGlobalVar(ident)) {
        return ident;
    }
    int index = -1;
    for (int i = 0; i < 10; i++) {
        if (globalRegVar[i] == ident) {
            index = i;
        }
    }
    //已分配
    if (index != -1) {
        return globalReg[index];
    }
    //未分配
    index = findAvaGlobalReg();
    //可以分配
    if (index != -1){
        globalRegUse[index] = true;
        globalRegVar[index] = ident;
        return globalReg[index];
    }
    //不可分配
    else {
        return ident;
    }
}
//$t6,$t7,$t8,$t9,$s2,$s3,$s4,$s5,$s6,$s7 全局变量使用
void changeGlobal2Reg() {
    int i;
    if (blocks[0].midCodeVector[0].op == FUNC) {
        i = 0;
    }
    else {
        i = 1;
    }
    for (; i < blocks.size(); i++) {
        Block& curBlock = blocks[i];
        if (curBlock.midCodeVector.empty()) {
            continue;
        }
        if (curBlock.midCodeVector[0].op == FUNC) {
            for (int k = 0; k < 10; k++) {
                globalRegUse[k] = false;
                globalRegVar[k] = "";
            }
        }
        vector<midCode> loadMidCodes;
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
                {
                    curBlock.midCodeVector[j].x = putGlobal(curBlock.midCodeVector[j].x, loadMidCodes);
                    curBlock.midCodeVector[j].y = putGlobal(curBlock.midCodeVector[j].y, loadMidCodes);
                    curBlock.midCodeVector[j].z = getGlobal(curBlock.midCodeVector[j].z);
                    break;
                }
                case GETARRAY:
                {
                    curBlock.midCodeVector[j].y = putGlobal(curBlock.midCodeVector[j].y, loadMidCodes);
                    curBlock.midCodeVector[j].z = getGlobal(curBlock.midCodeVector[j].z);
                    break;
                }
                case PUTARRAY:
                {
                    curBlock.midCodeVector[j].x = putGlobal(curBlock.midCodeVector[j].x, loadMidCodes);
                    curBlock.midCodeVector[j].y = putGlobal(curBlock.midCodeVector[j].y, loadMidCodes);
                    break;
                }
                    //put z get x
                case NOTOP:
                case ASSIGNOP:{
                    curBlock.midCodeVector[j].x = putGlobal(curBlock.midCodeVector[j].x, loadMidCodes);
                    curBlock.midCodeVector[j].z = getGlobal(curBlock.midCodeVector[j].z);
                    break;
                }
                    //get x
                case BZ:
                case BNZ:{
                    curBlock.midCodeVector[j].x = putGlobal(curBlock.midCodeVector[j].x, loadMidCodes);
                    break;
                }
                    //get z
                case PUSH:
                case RET:
                case PRINTD:{
                    curBlock.midCodeVector[j].z = putGlobal(curBlock.midCodeVector[j].z, loadMidCodes);
                    break;
                }
                    //get x
                case PUSHADDR:{
                    curBlock.midCodeVector[j].x = putGlobal(curBlock.midCodeVector[j].x, loadMidCodes);
                    break;
                }
                    //put z
                case RETVALUE:
                case SCAN:{
                    curBlock.midCodeVector[j].z = getGlobal(curBlock.midCodeVector[j].z);
                    break;
                }
                case LABEL:{
                    if (curBlock.midCodeVector[j].x == "loop_begin") {
                        isInLoop += 1;
                    }
                    else if(curBlock.midCodeVector[j].x == "loop_end") {
                        isInLoop -= 1;
                    }
                }
                default:
                    break;
            }
        }


        //寄存器回写
        midCode lastMidCode = curBlock.midCodeVector[curBlock.midCodeVector.size()-1];
        curBlock.midCodeVector.pop_back();
        //函数调用写回，不回收
        if (lastMidCode.op == CALL) {
            for (int k = 0; k < 10; k++) {
                if (globalRegUse[k]) {
//                    globalRegUse[k] = false;
                    curBlock.midCodeVector.emplace_back(SAVE, globalReg[k], globalRegVar[k]);
//                    globalRegVar[k] = "";
                }
            }
            curBlock.midCodeVector.push_back(lastMidCode);
            //写回寄存器
            for (int k = 0; k < 10; k++) {
                if (globalRegUse[k]) {
//                    globalRegUse[k] = false;
                    curBlock.midCodeVector.emplace_back(LOAD, globalReg[k], globalRegVar[k]);
//                    globalRegVar[k] = "";
                }
            }
        }
        else if (lastMidCode.op == RET) {
            for (int k = 0; k < 10; k++) {
                if (globalRegUse[k]) {
//                    globalRegUse[k] = false;
                    curBlock.midCodeVector.emplace_back(SAVE, globalReg[k], globalRegVar[k]);
//                    globalRegVar[k] = "";
                }
            }
            curBlock.midCodeVector.push_back(lastMidCode);
        }
        //在loop内 回跳的时候回写，不释放寄存器
        else if (isInLoop != 0) {
            if (curBlock.nextBlock1 <= curBlock.number && curBlock.nextBlock1 != -1
                || curBlock.nextBlock2 <= curBlock.number && curBlock.nextBlock2 != -1) {
                for (int k = 0; k < 10; k++) {
                    if (!globalRegUse[k]) {
                        continue;
                    } else {
                        curBlock.midCodeVector.emplace_back(SAVE, globalReg[k], globalRegVar[k]);
                    }
                }
            }
            curBlock.midCodeVector.push_back(lastMidCode);
        }
        //根据out集来写回 在loop外
        else {
            for (int k = 0; k < 10; k++) {
                if (!globalRegUse[k]) {
                    continue;
                }
                if (curBlock.out.find(globalRegVar[k]) == curBlock.out.end()) {
                    curBlock.midCodeVector.emplace_back(SAVE, globalReg[k], globalRegVar[k]);
                    globalRegVar[k] = "";
                    globalRegUse[k] = false;
                }
            }
            curBlock.midCodeVector.push_back(lastMidCode);
        }


        //首端的load集
        if (curBlock.midCodeVector[0].op == FUNC) {
            auto iter = curBlock.midCodeVector.begin()+1;
            while(iter->op == PARAM) {
                iter ++;
            }
            curBlock.midCodeVector.insert(iter,loadMidCodes.begin(),loadMidCodes.end());

        }
        else if (curBlock.midCodeVector[0].op == LABEL) {
            curBlock.midCodeVector.insert(curBlock.midCodeVector.begin()+1,loadMidCodes.begin(),loadMidCodes.end());
        }
        else {
            curBlock.midCodeVector.insert(curBlock.midCodeVector.begin(),loadMidCodes.begin(),loadMidCodes.end());
        }
    }
}
 */