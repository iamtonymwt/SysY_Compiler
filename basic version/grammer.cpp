#include<iostream>
#include<cstring>
#include <string>
#include <fstream>
#include "grammer.h"
#include "lexical.h"
#include "midcode.h"
#include <vector>
#define INTOFFSET 4

using namespace std;

extern int symbol;
extern string neotoken;
//extern ofstream outputFile;
extern int symbolListI;
extern int symbolListC;
extern vector<CompSymbol> symbolList;
enum resType {
    MAINTK, CONSTTK, INTTK, BREAKTK, CONTINUETK, IFTK, ELSETK, WHILETK, GETINTTK,
    PRINTFTK, RETURNTK,VOIDTK, // match with reword order
    IDENFR, INTCON, STRCON, NOT, AND, OR, PLUS, MINU, MULT, DIV, MOD, LSS, LEQ,
    GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE,
    ANNOTATION
};

vector<midCode> midCodeTable;
vector<pair<string, string>> allStringList;    //全局字符串常量 stringName, curString
extern vector<Variable> vTable;    //全局变量表
extern vector<Function> funcvMap;   //函数参数表
int level = 0;
vector<pair<string, string>> whileLabel; // loop_begin loop_end

extern int vAddress;
string curFunction = "";

//区分stmt的几种情况
bool IsExp() {
    for (int i = symbolListI; i < symbolListC; i++) {
        if (symbolList[i].symbol == ASSIGN) {
            return false;
        }
        else if (symbolList[i].symbol == SEMICN) {
            return true;
        }
    }
    return false;
}

int UnitBranch() {
    int i = symbolListI;
    if (symbolList[i].symbol == MAINTK) {
        return 1;
    }
    else if (symbolList[i+1].symbol == LPARENT) {
        return 0;
    }
    else {
        return -1;
    }
}

//编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef
//声明 Decl → ConstDecl | VarDecl // 覆盖两种声明
bool CompUnit() {
    bool exitFlag = false;
    while (true) {
        if (symbol == CONSTTK) {
            ConstDecl();
        }
        else {
            switch (UnitBranch()) {
                case 1:
                    MainFuncDef();
                    exitFlag = true;
                    break;
                case 0:
                    FuncDef();
                    break;
                case -1:
                    VarDecl();
                    break;
                default:
                    return false;
            }
        }
        if (exitFlag) {
            break;
        }
    }
//    outputFile << "<CompUnit>" << endl;
    return true;
}

//常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
bool ConstDecl(){
    if (symbol != CONSTTK) {
        return false;
    }
//    print();
    getsymP();
    getsymNP();
    ConstDef();
    while (symbol == COMMA) {
//        print();
        getsymNP();
        ConstDef();
    }
//    print();
    getsymNP();
//    outputFile << "<ConstDecl>" << endl;
    return true;
}

//基本类型 BType → 'int' // 存在即可
//常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
bool ConstDef() {
//    print();
    int dim = 0;
    int d[2];
    string ident = neotoken;
    getsymP();
    while (symbol == LBRACK) {
        dim += 1;
        getsymNP();
        d[dim-1] = stoi(ConstExp());
//        print();
        getsymP();
    }
    addOldV(ident, dim, d);
    int variableIndex = vTable.size()-1;
    getsymNP();
    ConstInitVal(variableIndex);
    if (dim == 0) {
        midCodeTable.emplace_back(CONST, vTable[variableIndex].fName);
    }
    else {
        midCodeTable.emplace_back(CONSTARRAY, vTable[variableIndex].fName);
    }
    splitAssign(variableIndex, true);
//    outputFile << "<ConstDef>" << endl;
    return true;
}

//常量初值 ConstInitVal → ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
bool ConstInitVal(int variableIndex) {
    if (symbol != LBRACE) {
        vTable[variableIndex].value.push_back(ConstExp());
    }
    else {
//        print();
        getsymNP();
        if (symbol != RBRACE) {
            ConstInitVal(variableIndex);
            while (symbol == COMMA) {
//                print();
                getsymNP();
                ConstInitVal(variableIndex);
            }
        }
//        print();
        getsymNP();
    }
//    outputFile << "<ConstInitVal>" << endl;
    return true;
}

//变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
bool VarDecl() {
//    print();
    getsymNP();
    VarDef();
    while (symbol == COMMA) {
//        print();
        getsymNP();
        VarDef();
    }
//    print();
    getsymNP();
//    outputFile << "<VarDecl>" << endl;
    return true;
}

//变量定义 VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
bool VarDef() {
//    print();
    int dim = 0;
    int d[2];
    string ident = neotoken;

    getsymNP();
    while (symbol == LBRACK) {
//        print();
        dim += 1;
        getsymNP();
        d[dim-1] = stoi(ConstExp());
//        print();
        getsymNP();
    }
    addOldV(ident, dim, d);
    int variableIndex = vTable.size()-1;
    if (dim == 0) {
        midCodeTable.emplace_back(VAR, vTable[variableIndex].fName);
    }
    else {
        midCodeTable.emplace_back(ARRAY, vTable[variableIndex].fName);
    }
    if (symbol == ASSIGN) {
//        print();
        getsymNP();
        InitVal(variableIndex);
        splitAssign(variableIndex, false);
    }
//    outputFile << "<VarDef>" << endl;
    return true;
}

//变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
bool InitVal(int variableIndex) {
    if (symbol != LBRACE) {
        string exp = Exp();
        vTable[variableIndex].value.push_back(exp);
    }
    else {
//        print();
        getsymNP();
        if (symbol != RBRACE) {
            InitVal(variableIndex);
            while (symbol == COMMA) {
//                print();
                getsymNP();
                InitVal(variableIndex);
            }
        }
//        print();
        getsymNP();
    }
//    outputFile << "<InitVal>" << endl;
    return true;
}

//函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
bool FuncDef() {
    bool isFuncFollow = true;
    vAddress = 0;
    level += 1;

    Function f;
    f.type = FuncType();
//    print();
    string ident = neotoken;
    midCodeTable.emplace_back(GOTO, ident+"_end");
    f.ident = ident;
    curFunction = ident;
    vector<Variable> params;
    f.params = params;
    funcvMap.push_back(f);
    midCodeTable.emplace_back(FUNC, f.type == 1 ? "int" : "void", ident);
    getsymP();
    getsymNP();
    if (symbol != RPARENT) {
        FuncFParams(ident);
    }
//    print();
    getsymNP();
    Block(isFuncFollow);
//    outputFile << "<FuncDef>" << endl;

    level -= 1;
    if (midCodeTable[midCodeTable.size()-1].op != RET) {
        midCodeTable.emplace_back(RET);
    }
    midCodeTable.emplace_back(LABEL, ident+"_end");
    curFunction = "";
    funcvMap[funcvMap.size()-1].length = vAddress + 8;
    return true;
}

//主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block
bool MainFuncDef() {
    midCodeTable.emplace_back(FUNC, "int", "main");
    bool isFuncFollow = true;
    level += 1;
    vAddress = 0;
    curFunction = "main";

//    print();
    for (int i = 0; i < 3; i++) {
        getsymP();
    }
    getsymNP();
    Block(isFuncFollow);
//    outputFile << "<MainFuncDef>" << endl;

    midCodeTable.emplace_back(EXIT);
    level -= 1;
    curFunction = "";

    vector<Variable> params;
    Function function;
    function.ident = "main";
    function.type = 1;
    function.length = vAddress + 8;
    function.params = params;
    funcvMap.push_back(function);
    return true;
}

//函数类型 FuncType → 'void' | 'int'
bool FuncType() {
//    print();
    bool isInt = (symbol == INTTK);
    getsymNP();
//    outputFile << "<FuncType>" << endl;
    return isInt;
}

//函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
bool FuncFParams(string ident) {
    FuncFParam(ident);
    while (symbol == COMMA) {
//        print();
        getsymNP();
        FuncFParam(ident);
    }
//    outputFile << "<FuncFParams>" << endl;
    return true;
}

//函数形参 FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }]
bool FuncFParam(string ident) {
//    print();
    getsymP();
    int dim = 0;
    int d[2];
    string varident = neotoken;
    getsymNP();
    if (symbol == LBRACK) {
//        print();
        dim += 1;
        d[dim-1] = 0;
        getsymP();
        getsymNP();
        while (symbol == LBRACK) {
//            print();
            dim += 1;
            getsymNP();
            d[dim-1] = stoi(ConstExp());
//            print();
            getsymNP();
        }
    }
    addOldV(varident, dim, d);
    Variable &v = vTable[vTable.size()-1];
    funcvMap[funcvMap.size()-1].params.push_back(v);
    midCodeTable.emplace_back(PARAM, v.fName);
//    outputFile << "<FuncFParam>" << endl;
//    if (dim == 0) {
//        midCodeTable.emplace_back(VAR, varident);
//    }
//    else {
//        midCodeTable.emplace_back(ARRAY, varident);
//    }
    return true;
}

//语句块项 BlockItem → Decl | Stmt
//语句块 Block → '{' { BlockItem } '}'
bool Block(bool isFuncFollow) {
//    print();
    if (!isFuncFollow) {
        level += 1;
    }

    getsymNP();
    while(true) {
        if (symbol == RBRACE) {
//            print();
            break;
        }
        if (ConstDecl()) {}
        else if (symbol == INTTK) {
            VarDecl();
        }
        else {
            Stmt();
        }
    }
    getsymNP();
//    outputFile << "<Block>" << endl;

    checkvTable();
    if(!isFuncFollow) {
        level -= 1;
    }
    return true;
}


/*语句 Stmt →
/ LVal '=' Exp ';'
| LVal '=' 'getint''('')'';'
| [Exp] ';' //有⽆Exp两种情况

| Block
| 'if' '(' Cond ')' Stmt [ 'else' Stmt ] // 1.有else 2.⽆else
| 'while' '(' Cond ')' Stmt
| 'break' ';' | 'continue' ';'
| 'return' [Exp] ';' // 1.有Exp 2.⽆Exp
| 'printf''('FormatString{','Exp}')'';' // 1.有Exp 2.⽆Exp
*/
bool Stmt() {
    switch (symbol) {
        case LBRACE:{
            Block(false);
            break;
        }

        //'if' '(' Cond ')' Stmt [ 'else' Stmt ] // 1.有else 2.⽆else
        case IFTK: {
//            print();
            string else_label = getLabel();
            string if_end = getLabel();
            getsymP();
            getsymNP();
            string op = Cond();
//            print();
            getsymNP();
            midCodeTable.emplace_back(BZ, else_label, op);
            Stmt();
            midCodeTable.emplace_back(GOTO, if_end);
            midCodeTable.emplace_back(LABEL, else_label);
            if (symbol == ELSETK) {
//                print();
                getsymNP();
                Stmt();
            }
            midCodeTable.emplace_back(LABEL, if_end);
            break;
        }

        //'while' '(' Cond ')' Stmt
        case WHILETK:{
//            print();
            string loop_begin = getLabel();
            string loop_end = getLabel();
            midCodeTable.emplace_back(LABEL, loop_begin);
            whileLabel.emplace_back(make_pair(loop_begin, loop_end));
            getsymP();
            getsymNP();
            string op = Cond();
            midCodeTable.emplace_back(BZ, loop_end, op);
//            print();
            getsymNP();
            Stmt();
            whileLabel.pop_back();
            midCodeTable.emplace_back(GOTO, loop_begin);
            midCodeTable.emplace_back(LABEL, loop_end);
            break;
        }

        case BREAKTK: {
//            print();
            getsymP();
            getsymNP();
            midCodeTable.emplace_back(GOTO, whileLabel[whileLabel.size()-1].second);
            break;
        }

        case CONTINUETK: {
//            print();
            getsymP();
            getsymNP();
            midCodeTable.emplace_back(GOTO, whileLabel[whileLabel.size()-1].first);
            break;
        }

        //'return' [Exp] ';' // 1.有Exp 2.⽆Exp
        case RETURNTK:{
//            print();
            getsymNP();
            if (symbol == SEMICN) {
//                print();
                getsymNP();
                midCodeTable.emplace_back(RET);
                break;
            }
            string ret = Exp();
//            print();
            getsymNP();
            midCodeTable.emplace_back(RET, ret);
            break;

        }

        //' printf''('FormatString{','Exp}')'';' // 1.有Exp 2.⽆Exp
        case PRINTFTK: {
//            print();
            vector<int> consequence;
            vector<pair<string, string>> stringList;
            getsymP();
            getsymP();
            string formatstring = neotoken;
            string curString;
            //把字符串分开并记录顺序
            for (int i=0; i<formatstring.size(); i++) {
                if ((formatstring[i] == '%') && (formatstring[i+1] == 'd')) {
                    if (!curString.empty()) {
                        string stringName = getString();
                        allStringList.emplace_back(stringName, curString);
                        stringList.emplace_back(stringName, curString);
                        consequence.push_back(0);
                    }
                    consequence.push_back(1);
                    i ++;
                    curString = "";
                }
                else {
                    curString += formatstring[i];
                }
            }
            if (!curString.empty()) {
                string stringName = getString();
                allStringList.emplace_back(stringName, curString);
                stringList.emplace_back(stringName, curString);
                consequence.push_back(0);
            }
            getsymNP();
            vector<string> varList;
            while (symbol == COMMA) {
//                print();
                getsymNP();
                varList.push_back(Exp());
            }
//            print();
            getsymP();
            getsymNP();

            for (int i = 0; i < consequence.size(); i++) {
                if (consequence[i] == 1) {
                    midCodeTable.emplace_back(PRINTD, varList[0]);
                    varList.erase(varList.begin());
                }
                else if (consequence[i] == 0) {
                    midCodeTable.emplace_back(PRINTS, stringList[0].first, stringList[0].second);
                    stringList.erase(stringList.begin());
                }
            }
            break;
        }

        case SEMICN:
//            print();
            getsymNP();
            break;

//    | LVal '=' Exp ';'
//    | LVal '=' 'getint''('')'';'
//    | [Exp] ';' //有⽆Exp两种情况
        case IDENFR: {
            if (IsExp()) {
                Exp();
//                print();
                getsymNP();
            }
            else {
                string lval;
                lval = LVal(false);
                string offset = isContainOffset(lval);
//                print();
                getsymNP();
                if (symbol == GETINTTK) {
                    if (offset.size() == 0) {
                        midCodeTable.emplace_back(SCAN, lval);
                    }
                    else {
                        string newV = addNewV();
                        midCodeTable.emplace_back(SCAN, newV);
                        midCodeTable.emplace_back(PUTARRAY, getVarWithoutOffset(lval), offset, newV);
                    }
//                    print();
                    for (int i = 0; i < 3; i++) {
                        getsymP();
                    }
                    getsymNP();
                }
                else {
                    string rval = Exp();
                    if (offset.size() == 0) {
                        midCodeTable.emplace_back(ASSIGNOP, lval, rval);
                    }
                    else {
//                        string newV = addNewV();
//                        midCodeTable.emplace_back(ASSIGNOP, newV, rval);
                        midCodeTable.emplace_back(PUTARRAY, getVarWithoutOffset(lval), offset, rval);
                    }
//                    print();
                    getsymNP();
                }
            }
            break;
        }

        default:
            Exp();
//            print();
            getsymNP();
    }
//    outputFile << "<Stmt>" << endl;
    return true;
}

//表达式 Exp → AddExp 注：SysY 表达式是int 型表达式
string Exp() {
    string neoVar = AddExp();
//    outputFile << "<Exp>" << endl;
    return neoVar;
}

//条件表达式 Cond → LOrExp
string Cond() {
    string neoVar = LOrExp();
//    outputFile << "<Cond>" << endl;
    return neoVar;
}

/////////////////////////////////////////////////////////////////////
//check初始化值/indexs是不是全是数字
bool isAllNumber(vector<string> list) {
    for (int i = 0; i < list.size(); i ++) {
        if (!isNumber(list[i])) {
            return false;
        }
    }
    return true;
}
//对于right 为ident / number / ident[offset]
string LValGetValue(string ident, vector<string> indexs) {
    //找到对应变量
    Variable variable;
    for (int i = vTable.size()-1; i >= 0; i--) {
        if ((vTable[i].rName == ident) && (vTable[i].valid)) {
            variable = vTable[i];
            break;
        }
    }
    //精确到了值
    if (variable.dimCount == indexs.size()) {
        //能拿出来一个数
        if ((!variable.value.empty())&&(isAllNumber(indexs))&&(isAllNumber(variable.value))) {
            switch (indexs.size()) {
                case 0:{
                    return variable.value[0];
                    break;
                }
                case 1:{
                    return variable.value[stoi(indexs[0])];
                    break;
                }
                case 2:{
                    int offset = stoi(indexs[0]) * variable.dim[1] + stoi(indexs[1]);
                    return variable.value[offset];
                    break;
                }
            }
        }
        //只能返回一个变量
        else {
            switch (indexs.size()) {
                case 0:
                    return variable.fName;
                    break;
                case 1: {
                    string neoV = addNewV();
                    midCodeTable.emplace_back(GETARRAY, neoV, variable.fName, indexs[0]);
                    return neoV;
                    break;
                }
                case 2: {
                    string neoV = merge(MULTOP, indexs[0], to_string(variable.dim[1]));
                    string neoVV = merge(PLUSOP, neoV, indexs[1]);
                    string neoVVV = addNewV();
                    midCodeTable.emplace_back(GETARRAY, neoVVV, variable.fName, neoVV);
                    return neoVVV;
                    break;
                }
            }
        }
    }
    //要了个数组（函数实参）
    else if (variable.dimCount > indexs.size()){
        if (indexs.size() == 0) {
            return variable.fName + "[" + "0" + "]";
        }
        else if ((indexs.size() == 1) && (variable.dimCount == 2)) {
            string neoV = merge(MULTOP, indexs[0], to_string(variable.dim[1]));
            return variable.fName + "[" + neoV + "]";
        }
    }
}
//对于left 为 ident[offset]
string LValGetLeft(string ident, vector<string> indexs) {
    for (int i = vTable.size() - 1; i >= 0; i--) {
        if ((vTable[i].rName == ident) && (vTable[i].valid)) {
            if (indexs.empty()) {
                return vTable[i].fName;
            }
            else if (indexs.size() == 1) {
                return vTable[i].fName + "[" + indexs[0] + "]";
            }
            else if (indexs.size() == 2) {
                string neoV = merge(MULTOP, indexs[0], to_string(vTable[i].dim[1]));
                string neoVV = merge(PLUSOP, neoV, indexs[1]);
                return vTable[i].fName + "[" + neoVV + "]";
            }
        }
    }
}
//左值表达式 LVal → Ident {'[' Exp ']'}
//对于left 为 <ident, offset>
//对于right 为<variable, NONE> / <number, NONE> / <ident, offset>
string LVal(bool isGet) {
//  print();
    string re;
    string ident = neotoken;
    vector<string> indexs;
    getsymNP();
    while (true) {
        if (symbol == LBRACK) {
//          print();
            getsymNP();
            string index = Exp();
            indexs.push_back(index);
//            print();
            getsymNP();
        } else {
            break;
        }
    }
    if (isGet) {
        re = LValGetValue(ident, indexs);
    } else {
        re = LValGetLeft(ident, indexs);
    }
    return re;
//  outputFile << "<LVal>" << endl;
}
/////////////////////////////////////////////////////////////////////

//基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // 三种情况均需覆盖
string PrimaryExp() {
    string neoVar;
    if (symbol == LPARENT) {
//        print();
        getsymNP();
        neoVar = Exp();
//        print();
        getsymNP();
    }
    else if (symbol != INTCON) {
        neoVar = LVal(true);
    }
    else if (symbol == INTCON) {
        neoVar = Number();
    }
//    outputFile << "<PrimaryExp>" << endl;
    return neoVar;
}

//数值 Number → IntConst // 存在即可
string Number(){
    if (symbol == INTCON) {
//        print();
        string neoVar = neotoken;
        getsymNP();
//        outputFile << "<Number>" << endl;
        return neoVar;
    }
}

//⼀元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp // 存在即可
string UnaryExp() {
    string neoVar;
    if (symbol == PLUS || symbol == MINU || symbol == NOT) {
        string neoOp = UnaryOp();
        if (neoOp == "+") {
            neoVar = UnaryExp();
        }
        else if (neoOp == "-") {
            neoVar = merge(MINUOP, "0", UnaryExp());
        }
        else if (neoOp == "!") {
            neoVar = merge(NOTOP, UnaryExp(), "");
        }
    }
    else if (symbol == LPARENT || symbol == INTCON) {
        neoVar = PrimaryExp();
    }
    else if (symbol == IDENFR) {
        if (symbolList[symbolListI].symbol == LPARENT) {
//            print();
            string ident = neotoken;
            getsymP();
            getsymNP();
            if (symbol != RPARENT) {
                FuncRParams(ident);
            }
//            print();
            getsymNP();
            midCodeTable.emplace_back(CALL, ident);
            for (int i = funcvMap.size()-1; i >= 0 ; i--) {
                if (funcvMap[i].ident == ident) {
                    if (funcvMap[i].type == 1) {
                        neoVar = addNewV();
                        midCodeTable.emplace_back(RETVALUE, neoVar, "RET");
                    }
                }
            }
        }
        else {
            neoVar = PrimaryExp();
        }
    }
//    outputFile << "<UnaryExp>" << endl;
    return neoVar;
}

//单⽬运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 // 三种均需覆盖
string UnaryOp() {
    if (symbol == PLUS || symbol == MINU || symbol == NOT) {
        string neoOp;
        switch (symbol) {
            case PLUS:
                neoOp = "+";
                break;
            case MINU:
                neoOp = "-";
                break;
            case NOT:
                neoOp = "!";
                break;
        }
//        print();
        getsymNP();
//        outputFile << "<UnaryOp>" << endl;
        return neoOp;
    }
    return "";
}

// 将实参 传值/传指针
// Params: ident函数名, dim第几个, neoVar实参名
void funcParamPush(string ident, int dim, string neoVar) {
    Function function;
    for (int i = funcvMap.size()-1; i >= 0; i--) {
        if (funcvMap[i].ident == ident) {
            function = funcvMap[i];
        }
    }
    string offset = isContainOffset(neoVar);
    //传值
    if (offset.size() == 0) {
        midCodeTable.emplace_back(PUSH, neoVar, to_string(dim));
//        midCodeTable.emplace_back(ASSIGNOP, function.params[dim].fName, neoVar);
    }
    //传地址
    else {
        string ident = getVarWithoutOffset(neoVar);
        midCodeTable.emplace_back(PUSHADDR, ident, offset, to_string(dim));
    }
}
//函数实参表 FuncRParams → Exp { ',' Exp }
bool FuncRParams(string ident) {
    string neoVar;
    int dim = 0;
    neoVar = Exp();
    funcParamPush(ident, dim, neoVar);
    while (symbol == COMMA) {
//        print();
        dim += 1;
        getsymNP();
        neoVar = Exp();
        funcParamPush(ident, dim, neoVar);
    }
//    outputFile << "<FuncRParams>" << endl;
    return true;
}

//乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
string MulExp() {
    string neoVar = UnaryExp();
//    outputFile << "<MulExp>" << endl;
    while (symbol == MULT || symbol == DIV || symbol == MOD) {
//        print();
        if (symbol == MULT) {
            getsymNP();
            neoVar = merge(MULTOP, neoVar, UnaryExp());
        }
        else if (symbol == DIV) {
            getsymNP();
            neoVar = merge(DIVOP, neoVar, UnaryExp());
        }
        else if (symbol == MOD) {
            getsymNP();
            neoVar = merge(MODOP, neoVar, UnaryExp());
        }
//        outputFile << "<MulExp>" << endl;
    }
    return neoVar;
}

//加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
string AddExp() {
    string neoVar = MulExp();
//    outputFile << "<AddExp>" << endl;
    while (symbol == PLUS || symbol == MINU) {
//        print();
        if (symbol == PLUS) {
            getsymNP();
            neoVar = merge(PLUSOP, neoVar, MulExp());
        }
        else if (symbol == MINU) {
            getsymNP();
            neoVar = merge(MINUOP, neoVar, MulExp());
        }
//        outputFile << "<AddExp>" << endl;
    }
    return neoVar;
}

//关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
string RelExp() {
    string neoVar = AddExp();
//    outputFile << "<RelExp>" << endl;
    while (symbol == LSS || symbol == GRE || symbol == LEQ || symbol == GEQ) {
//        print();
        if (symbol == LSS) {
            getsymNP();
            neoVar = merge(LSSOP, neoVar, AddExp());
        }
        else if (symbol == GRE) {
            getsymNP();
            neoVar = merge(GREOP, neoVar, AddExp());
        }
        else if (symbol == LEQ) {
            getsymNP();
            neoVar = merge(LEQOP, neoVar, AddExp());
        }
        else if (symbol == GEQ) {
            getsymNP();
            neoVar = merge(GEQOP, neoVar, AddExp());
        }
//        outputFile << "<RelExp>" << endl;
    }
    return neoVar;
}

//相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
string EqExp() {
    string neoVar = RelExp();
//    outputFile << "<EqExp>" << endl;
    while (symbol == EQL || symbol == NEQ) {
//        print();
        if (symbol == EQL) {
            getsymNP();
            neoVar = merge(EQLOP, neoVar, RelExp());
        }
        else if (symbol == NEQ) {
            getsymNP();
            neoVar = merge(NEQOP, neoVar, RelExp());
        }
//        outputFile << "<EqExp>" << endl;
    }
    return neoVar;
}

//逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp // 1.EqExp 2.&& 均需覆盖
string LAndExp(string andEndLabel) {
    string eqExp = EqExp();
//    outputFile << "<LAndExp>" << endl;
    midCodeTable.emplace_back(BZ, andEndLabel, eqExp);
    while (symbol == AND) {
//        print();
        getsymNP();
        eqExp = EqExp();
        midCodeTable.emplace_back(BZ, andEndLabel, eqExp);
//        outputFile << "<LAndExp>" << endl;
    }
    return "";
}

//逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp // 1.LAndExp 2.|| 均需覆盖
string LOrExp() {
    string neoVar = addNewV();
    string andEndLabel = getLabel();
    string orEndLabel = getLabel();
    LAndExp(andEndLabel);
    midCodeTable.emplace_back(ASSIGNOP, neoVar, "1");
    midCodeTable.emplace_back(GOTO, orEndLabel);
    midCodeTable.emplace_back(LABEL, andEndLabel);
    midCodeTable.emplace_back(ASSIGNOP, neoVar, "0");
//    outputFile << "<LOrExp>" << endl;
    while (symbol == OR) {
//        print();
        getsymNP();
        andEndLabel = getLabel();
        LAndExp(andEndLabel);
        midCodeTable.emplace_back(ASSIGNOP, neoVar, "1");
        midCodeTable.emplace_back(GOTO, orEndLabel);
        midCodeTable.emplace_back(LABEL, andEndLabel);
        midCodeTable.emplace_back(ASSIGNOP, neoVar, "0");
//        outputFile << "<LOrExp>" << endl;
    }
    midCodeTable.emplace_back(LABEL, orEndLabel);
    return neoVar;
}

//常量表达式 ConstExp → AddExp 注：使⽤的Ident 必须是常量 // 存在即可
string ConstExp() {
    string neoVar;
    neoVar = AddExp();
//    outputFile << "<ConstExp>" << endl;
    return neoVar;
}

//main function
bool Gprocess(){
    if (CompUnit()) {
        return true;
    }
    return false;
}



