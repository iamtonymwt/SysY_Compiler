#include <string>
using namespace std;
#include "vTable.h"
#include <vector>

#define INTOFFSET 4
extern int level;

vector<Variable> vTable;    //全局变量表
vector<Function> funcvMap;   //函数参数表

int vNumber = 0; //全局变量序号
int vAddress = 0;   //相对函数头地址 乘了4
extern string curFunction;

//生成一个多次变量，存至vTable
//return: fName
string addNewV() {
    vNumber ++;
    Variable a;
    a.num = vNumber;
    a.dimCount = 0;
    a.rName = "variable_" + to_string(vNumber);
    a.fName = a.rName;
    a.level = level;
    a.valid = true;
    a.address = vAddress;
    a.func = curFunction;
    vTable.push_back(a);
    vAddress += INTOFFSET;
    return a.fName;
}

//生成一个中间变量（只是用两次），存放至vTable
//return: fName
string addInsV() {
    vNumber ++;
    Variable a;
    a.num = vNumber;
    a.dimCount = 0;
    a.rName = "ins_" + to_string(vNumber);
    a.fName = a.rName;
    a.level = level;
    a.valid = true;
    a.address = vAddress;
    a.func = curFunction;
    vTable.push_back(a);
    vAddress += INTOFFSET;
    return a.fName;
}

//将变量存进vTable
//Params: rName, dim, d[]
//return: 新的fName
string addOldV(string rName, int dim, int *d) {
    vNumber ++;
    Variable a;
    a.num = vNumber;
    a.rName = rName;
    a.fName = "variable_" + to_string(vNumber);
    a.dimCount = dim;
    if (dim != 0) {
        for(int i=0;i<dim;i++) {
            a.dim[i] = d[i];
        }
    }
    a.level = level;
    a.valid = true;
    a.address = vAddress;
    a.func = curFunction;
    vTable.push_back(a);
    vAddress += countOffset(dim, d);
    return a.fName;
}

//查找ident的对应fName
//Param : rName
//return: fName
string getOldVName(string rName) {
    for (int i=vTable.size()-1; i>0; i--) {
        if ((vTable[i].rName == rName) && (vTable[i].valid)) {
            return vTable[i].fName;
        }
    }
}

void checkvTable() {
    for (int i=vTable.size()-1; i>0; i--) {
        if (vTable[i].level == level) {
            vTable[i].valid = false;
        }
    }
}

int countOffset(int dim, int *d) {
   int offset = 1;
   for (int i = 0; i< dim; i++) {
       offset *= d[i];
   }

   offset *= INTOFFSET;
   if (offset == 0) {   //数组参数开个int空间
       return 4;
   }
   return offset;
}
