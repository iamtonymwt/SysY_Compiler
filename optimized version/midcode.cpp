#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include "midcode.h"
#include "vTable.h"
using namespace std;

extern vector<midCode> midCodeTable;
extern vector<Variable> vTable;

int label = 0; //label编号计数器
//int variable = 0;//临时变量计数器
int stringCount = 0; //字符串常量计数器

//新建一个label
string getLabel() {
    label ++;
    return "label_" + to_string(label);
}

//新建一个字符串常量
string getString() {
    stringCount ++;
    return "String_" + to_string(stringCount);
}

//在已经有初值的情况下可以完成对于数组的拆分赋值
void splitAssign(int variableIndex, bool isConstV) {
    Variable &v = vTable[variableIndex];
    vector<string> value = v.value;
    //此处将var的初值清空
    if (!isConstV) {
        v.value.clear();
    }
    if (value.empty()) {
        return;
    }
    if (v.dimCount == 0) {
        midCodeTable.emplace_back(ASSIGNOP, v.fName, value[0]);
        return;
    }
    else if (v.dimCount == 1) {
        int m = v.dim[0];
        for (int i = 0; i < m; i++) {
            midCodeTable.emplace_back(PUTARRAY, v.fName, to_string(i), value[i]);
        }
        return;
    }
    else if (v.dimCount == 2) {
        int m = v.dim[0];
        int n = v.dim[1];
        for (int i = 0; i < m*n; i++) {
            midCodeTable.emplace_back(PUTARRAY, v.fName, to_string(i), value[i]);
        }
        return;
    }
}

bool isNumber(string item) {
    if((('0' <= char(item[0])) && (char(item[0]) <= '9'))) {
        return true;
    }
    if((char(item[0]) == '+') || (char(item[0]) == '-')) {
        return true;
    }
    return false;
}

// return "": no offset
// return "offset": offset
string isContainOffset(string item) {
    bool containOffset = false;
    string offset = "";
    int i = 0;
    while (i < item.size()) {
        if (char(item[i]) == '[') {
            containOffset = true;
            break;
        }
        i ++;
    }
    if (!containOffset) {
        return "";
    }
    else {
        for (int j = i+1; j < item.size()-1; j++) {
            offset += item[j];
        }
        return offset;
    }
}

//return ident
string getVarWithoutOffset(string item) {
    if (isContainOffset(item).size() == 0) {
        return item;
    }
    else {
        string ident;
        for (int i = 0; i < item.size(); i++) {
            if (item[i] != '[') {
                ident += item[i];
            }
            else {
                break;
            }
        }
        return ident;
    }
}

string merge(operation o, string left, string right = "") {
    bool isNumLeft = isNumber(left);
    bool isNumRight = isNumber(right);
    if (isNumLeft && isNumRight) {
        int l = stoi(left);
        int r = stoi(right);
        int merge;
        switch (o) {
            case PLUSOP:
                merge = l + r;
                break;
            case MINUOP:
                merge = l - r;
                break;
            case MULTOP:
                merge = l * r;
                break;
            case DIVOP:
                merge = l / r;
                break;
            case MODOP:
                merge = l % r;
                break;
            case ANDOP:
                merge = l && r;
                break;
            case OROP:
                merge = l || r;
                break;
            case LSSOP:
                merge = l < r;
                break;
            case LEQOP:
                merge = l <= r;
                break;
            case GREOP:
                merge = l > r;
                break;
            case GEQOP:
                merge = l >= r;
                break;
            case EQLOP:
                merge = l == r;
                break;
            case NEQOP:
                merge = l != r;
                break;
            case NOTOP:
                merge = !l;
        }
        return to_string(merge);
    }
    else  {
        string merge = addInsV();
        midCodeTable.emplace_back(o, merge, left, right);
        return merge;
    }
}

void outputMidCode(ofstream& midCodefile) {
    for (int i = 0; i < midCodeTable.size(); i++) {
        midCode mc = midCodeTable[i];
        switch (mc.op) {
            case PLUSOP:
                midCodefile << mc.z << " = " << mc.x << " + " << mc.y << endl;
                break;
            case MINUOP:
                midCodefile << mc.z << " = " << mc.x << " - " << mc.y << endl;
                break;
            case MULTOP:
                midCodefile << mc.z << " = " << mc.x << " * " << mc.y << endl;
                break;
            case DIVOP:
                midCodefile << mc.z << " = " << mc.x << " / " << mc.y << endl;
                break;
            case MODOP:
                midCodefile << mc.z << " = " << mc.x << " % " << mc.y << endl;
                break;
            case ANDOP:
                midCodefile << mc.z << " = " << mc.x << " && " << mc.y << endl;
                break;
            case OROP:
                midCodefile << mc.z << " = " << mc.x << " || " << mc.y << endl;
                break;
            case LSSOP:
                midCodefile << mc.z << " = (" << mc.x << " < " << mc.y << ")\n";
                break;
            case LEQOP:
                midCodefile << mc.z << " = (" << mc.x << " <= " << mc.y << ")\n";
                break;
            case GREOP:
                midCodefile << mc.z << " = (" << mc.x << " > " << mc.y << ")\n";
                break;
            case GEQOP:
                midCodefile << mc.z << " = (" << mc.x << " >= " << mc.y << ")\n";
                break;
            case EQLOP:
                midCodefile << mc.z << " = (" << mc.x << " == " << mc.y << ")\n";
                break;
            case NEQOP:
                midCodefile << mc.z << " = (" << mc.x << " != " << mc.y << ")\n";
                break;
            case NOTOP:
                midCodefile << mc.z << " = !" << mc.x << endl;
                break;
            case ASSIGNOP:
                midCodefile << mc.z << " = " << mc.x << endl;
                break;
            case GOTO:
                midCodefile << "GOTO " << mc.z << endl;
                break;
            case BZ:
                midCodefile << "BZ " << mc.z << "(" << mc.x << " == 0)" << endl;
                break;
            case BNZ:
                midCodefile << "BNZ " << mc.z << "(" << mc.x << " == 1)" << endl;
                break;
            case LABEL:
                midCodefile << "LABEL" << ": " << mc.z << endl;
                break;
            case PUSH:
                midCodefile << "PUSH " << mc.z << endl;
                break;
            case PUSHADDR:
                midCodefile << "PUSHADDR " << mc.z << "[" << mc.x << "]" << endl;
                break;
            case CALL:
                midCodefile << "CALL " << mc.z << endl;
                break;
            case RET:
                midCodefile << "RET " << mc.z << endl;
                break;
            case RETVALUE:
                midCodefile << "RETVALUE " << mc.z << " = " << mc.x << endl;
                break;
            case SCAN:
                midCodefile << "SCAN " << mc.z << endl;
                break;
            case PRINTD:
                midCodefile << "PRINTD " << mc.z << endl;
                break;
            case PRINTS:
                midCodefile << "PRINTS " << mc.z << " \"" << mc.x << "\" "<< endl;
                break;
            case CONST:
                midCodefile << "CONST INT " << mc.z << endl;
                break;
            case CONSTARRAY:
                midCodefile << "CONST INT ARRAY " << mc.z << endl;
                break;
            case ARRAY:
                midCodefile << "INT ARRAY " << mc.z << endl;
                break;
            case VAR:
                midCodefile << "INT " << mc.z << endl;
                break;
            case FUNC:
                midCodefile << "FUNC " << mc.z << " " << mc.x << "()" << endl;
                break;
            case PARAM:
                midCodefile << "PARAM " << mc.z << endl;
                break;
            case GETARRAY:
                midCodefile << mc.z << " = " << mc.x << "[" << mc.y << "]" << endl;
                break;
            case PUTARRAY:
                midCodefile << mc.z << "[" << mc.x << "]" << " = " << mc.y << endl;
                break;
            case EXIT:
                midCodefile << "EXIT\n";
                break;

            case SAVE:
                midCodefile << mc.z << " save to " << mc.x << " in memory" << endl;
                break;
            case LOAD:
                midCodefile << "LOAD " << mc.x << " from memory to " << mc.z << endl;
                break;

            default:
                break;
        }
    }
}

string showMidCode(midCode mc) {
    string showContent;
    switch (mc.op) {
        case PLUSOP:
            showContent = mc.z + " = " + mc.x + " + " + mc.y;
            break;
        case MINUOP:
            showContent = mc.z + " = " + mc.x + " - " + mc.y;
            break;
        case MULTOP:
            showContent = mc.z + " = " + mc.x + " * " + mc.y;
            break;
        case DIVOP:
            showContent = mc.z + " = " + mc.x + " / " + mc.y;
            break;
        case MODOP:
            showContent = mc.z + " = " + mc.x + " % " + mc.y;
            break;
        case ANDOP:
            showContent = mc.z + " = " + mc.x + " && " + mc.y;
            break;
        case OROP:
            showContent = mc.z + " = " + mc.x + " || " + mc.y;
            break;
        case LSSOP:
            showContent = mc.z + " = (" + mc.x + " < " + mc.y + ")";
            break;
        case LEQOP:
            showContent = mc.z + " = (" + mc.x + " <= " + mc.y + ")";
            break;
        case GREOP:
            showContent = mc.z + " = (" + mc.x + " > " + mc.y + ")";
            break;
        case GEQOP:
            showContent = mc.z + " = (" + mc.x + " >= " + mc.y + ")";
            break;
        case EQLOP:
            showContent = mc.z + " = (" + mc.x + " == " + mc.y + ")";
            break;
        case NEQOP:
            showContent = mc.z + " = (" + mc.x + " != " + mc.y + ")";
            break;
        case NOTOP:
            showContent = mc.z + " = !" + mc.x;
            break;
        case ASSIGNOP:
            showContent = mc.z + " = " + mc.x;
            break;
        case GOTO:
            showContent = "GOTO " + mc.z;
            break;
        case BZ:
            showContent = "BZ " + mc.z + "(" + mc.x + " == 0)";
            break;
        case BNZ:
            showContent = "BNZ " + mc.z + "(" + mc.x + " == 1)";
            break;
        case LABEL:
            showContent = "LABEL: " + mc.z;
            break;
        case PUSH:
            showContent = "PUSH " + mc.z;
            break;
        case PUSHADDR:
            showContent = "PUSHADDR " + mc.z + "[" + mc.x + "]";
            break;
        case CALL:
            showContent = "CALL " + mc.z;
            break;
        case RET:
            showContent = "RET " + mc.z;
            break;
        case RETVALUE:
            showContent = "RETVALUE " + mc.z + " = " + mc.x;
            break;
        case SCAN:
            showContent = "SCAN " + mc.z;
            break;
        case PRINTD:
            showContent = "PRINTD " + mc.z;
            break;
        case PRINTS:
            showContent = "PRINTS " + mc.z + " \"" + mc.x + "\" ";
            break;
        case CONST:
            showContent = "CONST INT " + mc.z;
            break;
        case CONSTARRAY:
            showContent = "CONST INT ARRAY " + mc.z;
            break;
        case ARRAY:
            showContent = "INT ARRAY " + mc.z;
            break;
        case VAR:
            showContent = "INT " + mc.z;
            break;
        case FUNC:
            showContent = "FUNC " + mc.z + " " + mc.x + "()";
            break;
        case PARAM:
            showContent = "PARAM " + mc.z;
            break;
        case GETARRAY:
            showContent = mc.z + " = " + mc.x + "[" + mc.y + "]";
            break;
        case PUTARRAY:
            showContent = mc.z + "[" + mc.x + "]" + " = " + mc.y;
            break;
        case EXIT:
            showContent = "EXIT";
            break;

        case SAVE:
            showContent = mc.z + " save to " + mc.x + " in memory";
            break;
        case LOAD:
            showContent = "LOAD " + mc.x + " from memory to " + mc.z;
            break;

        default:
            break;
    }
    return showContent;
}
