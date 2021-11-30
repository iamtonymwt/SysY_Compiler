#include <iostream>
#include <fstream>
#include "mipscode.h"
#include "vTable.h"
#include "vector"
#include "midcode.h"
#include "map"
#include "optimize.h"

extern vector<midCode> midCodeTable;

vector<int> block_beginNumber;
vector<int> block_endNumber;
vector<Block> blocks;



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

void changeIns2Reg() {

}


//主优化函数
void optimize() {
    digHole();
    genBlock();
    changeIns2Reg();

}