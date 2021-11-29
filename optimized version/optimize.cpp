#include <iostream>
#include <fstream>
#include "mipscode.h"
#include "vTable.h"
#include "vector"
#include "midcode.h"
#include "map"

extern vector<midCode> midCodeTable;

//窥孔优化
void digHole() {
    vector<midCode> neoMidCodeTable;
    for (int i = 0; i < midCodeTable.size(); i++) {

        //default
        neoMidCodeTable.push_back(midCodeTable[i]);
    }
    midCodeTable = neoMidCodeTable;
}


//主优化函数
void optimize() {
    digHole();
}