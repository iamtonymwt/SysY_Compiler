#include <iostream>
#include <fstream>
#include "mipscode.h"
#include "vTable.h"
#include "vector"
#include "midcode.h"
#include "map"

extern vector<midCode> midCodeTable;

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


//主优化函数
void optimize() {
    digHole();
}