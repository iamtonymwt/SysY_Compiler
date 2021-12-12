#include<iostream>
#include<cstring>
#include <string>
#include <fstream>
#include "error_grammer.h"
#include "lexical.h"
#include <vector>
using namespace std;

extern bool hasError;

//是否在while循环中
extern int isInWhile;

// token相关
enum resType {
    MAINTK, CONSTTK, INTTK, BREAKTK, CONTINUETK, IFTK, ELSETK, WHILETK, GETINTTK,
    PRINTFTK, RETURNTK,VOIDTK, // match with reword order
    IDENFR, INTCON, STRCON, NOT, AND, OR, PLUS, MINU, MULT, DIV, MOD, LSS, LEQ,
    GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE,
    ANNOTATION
};
extern int symbol;
extern ofstream errorFile;
extern int symbolListI;
extern int symbolListC;
extern string neotoken;
extern int curLine;
extern int lastLine;
extern vector<CompSymbol> symbolList;

// 符号表相关
extern vector<var> varTable;
extern vector<func> funcTable;
//int varTableIndex = 0;
//int funcTableIndex = 0;
extern int error_level;

//<NormalChar> → ⼗进制编码为32,33,40-126的ASCII字符，'\'（编码92）出现当且仅当为'\n'
struct ErrorA: public exception{};
//此时formatstring应该已经被读入
bool isValidChar(char a){
    return (a == 32 || a == 33 || ((a >= 40)&&(a <=126)));
}
void lookErrorA() {
    try {
        string formatString = neotoken;
        for (int i = 0; i < formatString.size(); i++) {
            if ((formatString[i] == 37)) {
                if (formatString[i+1] == 'd') {
                    continue;
                }
                else {
                    throw ErrorA();
                }
            }
            else if (!isValidChar(formatString[i])) {
                throw ErrorA();
            }
            if ((formatString[i] == 92) && (formatString[i+1] != 'n')){
                throw ErrorA();
            }
        }
    }
    catch (ErrorA& errorA) {
        hasError = true;
        errorFile << curLine << " " << "a" << endl;
    }
}

struct ErrorB: public exception{};
//此时ident应该已经被读入
void lookErrorBv() {
    try {
        for (int i = 0; i < varTable.size(); i++) {
            if ((varTable[i].ident == neotoken) && (varTable[i].level == error_level)) {
                throw(ErrorB());
            }
        }
    }
    catch (ErrorB& errorB) {
        hasError = true;
        errorFile << curLine << " " << "b" << endl;
    }
}
//此时ident应该已经被读入
void lookErrorBf() {
    try {
        for (int i = 0; i < funcTable.size(); i++) {
            if (funcTable[i].ident == neotoken) {
                throw(ErrorB());
            }
        }
    }
    catch (ErrorB& errorB) {
        hasError = true;
        errorFile << curLine << " " << "b" << endl;
    }
}

struct ErrorC: public exception{};
//此时ident应该已经被读入
void lookErrorCv(){
    try {
        bool flag = false;
        for (int i = 0; i < varTable.size(); i++) {
            if (varTable[i].ident == neotoken) {
                flag = true;
            }
        }
        if (!flag) {
            throw ErrorC();
        }
    }
    catch (ErrorC& errorC) {
        hasError = true;
        errorFile << curLine << " " << "c" << endl;
    }
}
//此时ident应该已经被读入
void lookErrorCf(){
    try {
        bool flag = false;
        for (int i = 0; i < funcTable.size(); i++) {
            if (funcTable[i].ident == neotoken) {
                flag = true;
            }
        }
        if (!flag) {
            throw ErrorC();
        }
    }
    catch (ErrorC& errorC) {
        hasError = true;
        errorFile << curLine << " " << "c" << endl;
    }
}

struct ErrorD: public exception{};
void lookErrorD(string funcIdent, int rCount, int funcNameLine){
    try {
        for (int i = 0; i < funcTable.size(); i++) {
            if (funcTable[i].ident == funcIdent) {
                int fCount = funcTable[i].params.size();
                if (fCount != rCount) {
                    throw ErrorD();
                }
                break;
            }
        }
    }
    catch (ErrorD& errorD) {
        hasError = true;
        errorFile << funcNameLine << " " << "d" << endl; //理论上此处是ident line
    }
}

struct ErrorE: public exception{};
//接受参数类型为int的kind信号 -1void 0a 1a[] 2a[][]
void lookErrorE(string funcIdent, int kind, int rCount, int funcNameLine) {
    try {
        for (int i = 0; i < funcTable.size(); i++) {
            if (funcTable[i].ident == funcIdent) {
                func function = funcTable[i];
                if (rCount > function.params.size()) {  //此时应该交给ErrorD处理才对
                    return;
                }
                if (function.params[rCount - 1].dim != kind) {
                    throw ErrorE();
                }
                break;
            }
        }
    }
    catch (ErrorE& errorE) {
        hasError = true;
        errorFile << funcNameLine << " " << "e" << endl; //理论上此处是ident line
    }
}

struct ErrorF: public exception {};
//此时默认return后面有返回值
void lookErrorF(int returntkLine){
    try {
        func function = funcTable[funcTable.size()-1];
        if (function.type != Int) {
            throw ErrorF();
        }
    }
    catch (ErrorF& errorF) {
        hasError = true;
        errorFile << returntkLine << " " << "f" << endl;//此处应该是}的行号
    }
}

struct ErrorG: public exception {};
void lookErrorG(bool isFuncFollow, bool isLatestReturn){
    try {
        func function = funcTable[funcTable.size()-1];
        if (isFuncFollow && (!isLatestReturn) && (function.type == Int)) {
            throw ErrorG();
        }
    }
    catch (ErrorG& errorG) {
        hasError = true;
        errorFile << curLine << " " << "g" << endl;
    }
}

struct ErrorH: public exception {};
void lookErrorH(bool isAssign, string ident) {
    try {
        for (int i = varTable.size() - 1; i >= 0; i--) {
            if (varTable[i].ident == ident) {
                var variable = varTable[i];
                if (isAssign && !variable.isConst) {
                    break;
                }
                if (isAssign && variable.isConst) {
                    throw ErrorH();
                }
                break;
            }
        }
    }
    catch (ErrorH& errorH) {
        hasError = true;
        errorFile << curLine << " " << "h" << endl;
    }
}

struct ErrorI: public exception {};
// return TRUE:是; FALSE:不是;
bool lookErrorI(int symbol) {
    try {
        if (symbol != SEMICN) {
            throw ErrorI();
        }
    }
    catch (ErrorI& errorI) {
        hasError = true;
        errorFile << lastLine << " " << "i" << endl;
        return false;
    }
    return true;
}

struct ErrorJ: public exception {};
// return TRUE:是) FALSE:不是)
bool lookErrorJ(int symbol) {
    try {
        if (symbol != RPARENT) {
            throw ErrorJ();
        }
    }
    catch (ErrorJ& errorJ) {
        hasError = true;
        errorFile << lastLine << " " << "j" << endl;
        return false;
    }
    return true;
}

struct ErrorK: public exception {};
// return TRUE:是] FALSE:不是]
bool lookErrorK(int symbol) {
    try {
        if (symbol != RBRACK) {
            throw ErrorK();
        }
    }
    catch (ErrorK& errorK) {
        hasError = true;
        errorFile << lastLine << " " << "k" << endl;
        return false;
    }
    return true;
}

struct ErrorL: public exception {};
void lookErrorL(string formatString, int expCount, int printftkLine) {
    try {
        int formatCount = 0;
        for (int i = 0; i < formatString.size(); i++) {
            if ((formatString[i] == '%') && (formatString[i+1] == 'd')) {
                formatCount += 1;
            }
        }
        if (formatCount != expCount) {
            throw ErrorL();
        }
    }
    catch (ErrorL& errorL) {
        hasError = true;
        errorFile << printftkLine << " " << "l" << endl;
    }
}

struct ErrorM: public exception {};
//此时默认检测到了break or continue
void lookErrorM() {
    try {
        if (isInWhile == 0) {
            throw ErrorM();
        }
    }
    catch (ErrorM& errorM) {
        hasError = true;
        errorFile << curLine << " " << "m" << endl;
    }
}
