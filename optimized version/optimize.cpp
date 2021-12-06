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
}