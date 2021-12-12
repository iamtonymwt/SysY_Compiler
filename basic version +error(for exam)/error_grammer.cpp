#include<iostream>
#include<cstring>
#include <string>
#include <fstream>
#include "error_grammer.h"
#include "lexical.h"
#include "error.h"
#include <vector>
using namespace std;

bool hasError = false;

//是否在while中
int isInWhile = 0;

extern int symbol;
//extern ofstream outputFile;
extern int symbolListI;
extern int symbolListC;
extern string neotoken;
extern int curLine;
extern int lastLine;

extern vector<CompSymbol> symbolList;

enum resType {
    MAINTK, CONSTTK, INTTK, BREAKTK, CONTINUETK, IFTK, ELSETK, WHILETK, GETINTTK,
    PRINTFTK, RETURNTK,VOIDTK, // match with reword order
    IDENFR, INTCON, STRCON, NOT, AND, OR, PLUS, MINU, MULT, DIV, MOD, LSS, LEQ,
    GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE,
    ANNOTATION
};

// 符号表相关
vector<var> varTable;
vector<func> funcTable;
//int varTableIndex = 0;
//int funcTableIndex = 0;
int error_level=0;
void error_deleteVarTable(){
    for (int i = varTable.size()-1; i >= 0 ; i--) {
        if (varTable[i].level == error_level) {
            varTable.pop_back();
        }
        else {
            break;
        }
    }
}

//区分stmt的几种情况
bool error_isInFirstExp() {
//    if (curLine != lastLine) {
//        return false;
//    }
    if ((symbol == PLUS) || (symbol == MINU) || (symbol == NOT)
        || (symbol == LPARENT) || (symbol == IDENFR) || (symbol == INTCON)) {
        return true;
    }
    return false;
}

bool error_IsExp() {
    int line0 = symbolList[symbolListI-1].line;
    for (int i = symbolListI; i < symbolListC; i++) {
        if (symbolList[i].symbol == ASSIGN) {
            return false;
        }
        else if ((symbolList[i].symbol == SEMICN) || (symbolList[i].line != line0)) {
            return true;
        }
    }
    return false;
}

int error_UnitBranch() {
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
bool error_CompUnit() {
    bool exitFlag = false;
    while (true) {
        if (symbol == CONSTTK) {
            error_ConstDecl();
        }
        else {
            switch (error_UnitBranch()) {
                case 1:
                    error_MainFuncDef();
                    exitFlag = true;
                    break;
                case 0:
                    error_FuncDef();
                    break;
                case -1:
                    error_VarDecl();
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
bool error_ConstDecl(){
    if (symbol != CONSTTK) {
        return false;
    }
    getsymP();
    getsymNP();
    error_ConstDef();
    while (symbol == COMMA) {
        getsymNP();
        error_ConstDef();
    }
    if (lookErrorI(symbol)) {
        getsymNP();
    }
//    outputFile << "<ConstDecl>" << endl;
    return true;
}

//基本类型 BType → 'int' // 存在即可
//常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
bool error_ConstDef() {
    //更新符号表
    var a;
    a.ident = neotoken;
    a.type = Int;
    a.level = error_level;
    a.isConst = true;
    lookErrorBv();
    varTable.push_back(a);
    var& a1 = varTable[varTable.size()-1];

    getsymP();
    while (symbol == LBRACK) {
        a1.dim += 1;     //符号表
        getsymNP();
        if (error_isInFirstExp()) {
            error_ConstExp();
        }
        if (lookErrorK(symbol)) {
            getsymNP();
        }
    }
    getsymNP();
    error_ConstInitVal();
//    outputFile << "<ConstDef>" << endl;
    return true;
}

//常量初值 ConstInitVal → ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
bool error_ConstInitVal() {
    if (symbol != LBRACE) {
        error_ConstExp();
    }
    else {
        getsymNP();
        if (symbol != RBRACE) {
            error_ConstInitVal();
            while (symbol == COMMA) {
                getsymNP();
                error_ConstInitVal();
            }
        }
        getsymNP();
    }
//    outputFile << "<ConstInitVal>" << endl;
    return true;
}

//变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
bool error_VarDecl() {
    getsymNP();
    error_VarDef();
    while (symbol == COMMA) {
        getsymNP();
        error_VarDef();
    }
    if (lookErrorI(symbol)) {
        getsymNP();
    }
//    outputFile << "<VarDecl>" << endl;
    return true;
}

//变量定义 VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
bool error_VarDef() {
    //更新符号表
    var a;
    a.ident = neotoken;
    a.type = Int;
    a.level = error_level;
    a.isConst = false;
    lookErrorBv();
    varTable.push_back(a);
    var& a1 = varTable[varTable.size()-1];

    getsymNP();
    while (symbol == LBRACK) {
        a1.dim += 1;     //符号表
        getsymNP();
        if (error_isInFirstExp()) {
            error_ConstExp();
        }
        if (lookErrorK(symbol)) {
            getsymNP();
        }
    }
    if (symbol == ASSIGN) {
        getsymNP();
        error_InitVal();
    }
//    outputFile << "<VarDef>" << endl;
    return true;
}

//变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
bool error_InitVal() {
    if (symbol != LBRACE) {
        error_Exp();
    }
    else {
        getsymNP();
        if (symbol != RBRACE) {
            error_InitVal();
            while (symbol == COMMA) {
                getsymNP();
                error_InitVal();
            }
        }
        getsymNP();
    }
//    outputFile << "<InitVal>" << endl;
    return true;
}

//函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
bool error_FuncDef() {
    error_level += 1;
    func a;
    error_FuncType(a);
    a.ident = neotoken;
    lookErrorBf();
    funcTable.push_back(a);
    func& a1 = funcTable[funcTable.size()-1];
    getsymP();
    getsymNP();
    if (symbol != RPARENT) {
        if (symbol == INTTK) {
            error_FuncFParams(a1);
            if (lookErrorJ(symbol)) {
                getsymNP();
            }
        }
        else {
            lookErrorJ(symbol);
        }
    }
    else {
        getsymNP();
    }
    error_Block(true);
//    outputFile << "<FuncDef>" << endl;
    //更新符号表
    error_deleteVarTable();
    error_level -= 1;
    return true;
}

//主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block
bool error_MainFuncDef() {
    for (int i = 0; i < 3; i++) {
        getsymP();
    }
    if (lookErrorJ(symbol)) {
        getsymNP();
    }
    func main;
//    main = funcTable[funcTable.size()-1];
    main.ident = "main";
    main.type = Int;
    funcTable.push_back(main);
    error_level += 1;
    error_Block(true);
//    outputFile << "<MainFuncDef>" << endl;
    error_deleteVarTable();
    error_level -= 1;
    return true;
}

//函数类型 FuncType → 'void' | 'int'
bool error_FuncType(func& a) {
    switch (symbol) {
        case INTTK:
            a.type = Int;
            break;
        case VOIDTK:
            a.type = Void;
            break;
    }
    getsymNP();
//    outputFile << "<FuncType>" << endl;
    return true;
}

//函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
bool error_FuncFParams(func& a) {
    error_FuncFParam(a);
    while (symbol == COMMA) {
        getsymNP();
        error_FuncFParam(a);
    }
//    outputFile << "<FuncFParams>" << endl;
    return true;
}

//函数形参 FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }]
bool error_FuncFParam(func& a) {
    var b;
    b.type = Int;
    b.level = error_level;
    b.isConst = false;
    getsymP();
    b.ident = neotoken;
    lookErrorBv();
    getsymNP();
    if (symbol == LBRACK) {
        b.dim += 1;
        getsymNP();
        if (lookErrorK(symbol)) {
            getsymNP();
        }
        while (symbol == LBRACK) {
            b.dim += 1;
            getsymNP();
            error_ConstExp();
            if (lookErrorK(symbol)) {
                getsymNP();
            }
        }
    }
    varTable.push_back(b);
    a.params.push_back(b);
//    outputFile << "<FuncFParam>" << endl;
    return true;
}

//语句块项 BlockItem → Decl | Stmt
//语句块 Block → '{' { BlockItem } '}'
bool error_Block(bool isFuncFollow) {
    bool isLatestReturn;
    if (!isFuncFollow) {
        error_level += 1;
    }
    getsymNP();
    while(true) {
        if (symbol == RBRACE) {
            break;
        }
        if (error_ConstDecl()) {}
        else if (symbol == INTTK) {
            error_VarDecl();
        }
        else {
            isLatestReturn = error_Stmt();
        }
    }
    lookErrorG(isFuncFollow, isLatestReturn);
    getsymNP();
//    outputFile << "<Block>" << endl;
    if (!isFuncFollow) {
        error_deleteVarTable();
        error_level -= 1;
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
 返回值返回的是是不是Return语句
*/
bool error_Stmt() {
    switch (symbol) {
        case LBRACE:
            error_Block(false);
            break;
        case IFTK:
            getsymP();
            getsymNP();
            error_Cond();
            if (lookErrorJ(symbol)) {
                getsymNP();
            }
            error_Stmt();
            if (symbol == ELSETK) {
                getsymNP();
                error_Stmt();
            }
            break;
        case WHILETK:
            getsymP();
            getsymNP();
            error_Cond();
            if (lookErrorJ(symbol)) {
                getsymNP();
            }
            isInWhile += 1;
            error_Stmt();
            isInWhile -= 1;
            break;
        case BREAKTK:
            lookErrorM();
            getsymP();
            if (lookErrorI(symbol)) {
                getsymNP();
            }
            break;
        case CONTINUETK:
            lookErrorM();
            getsymP();
            if (lookErrorI(symbol)) {
                getsymNP();
            }
            break;
        case RETURNTK: {
            int returntkLine = curLine;
            getsymNP();
            if (error_isInFirstExp()) {
                lookErrorF(returntkLine);//此时后面一定跟了一个Exp
                error_Exp();
            }
            if (lookErrorI(symbol)) {
                getsymNP();
            }
            return true;
        }
        case PRINTFTK: {
            int printftkLine = curLine;
            int expCount = 0;
            getsymP();
            getsymP();
            string formatString = neotoken;
            lookErrorA();   //A类错误查询
            getsymNP();
            while (symbol == COMMA) {
                expCount += 1;
                getsymNP();
                error_Exp();
            }
            lookErrorL(formatString, expCount, printftkLine);
            if (lookErrorJ(symbol)) {
                getsymNP();
            }
            if (lookErrorI(symbol)) {
                getsymNP();
            }
            break;
        }
        case SEMICN:
            getsymNP();
            break;
        case IDENFR:
            if (error_IsExp()) {
                error_Exp();
                if (lookErrorI(symbol)) {
                    getsymNP();
                }
            }
            else {
                error_LVal(true);
                getsymNP();
                if (symbol == GETINTTK) {
                    for (int i = 0; i < 2; i++) {
                        getsymP();
                    }
                    if (lookErrorJ(symbol)) {
                        getsymNP();
                    }
                    if (lookErrorI(symbol)) {
                        getsymNP();
                    }
                }
                else {
                    error_Exp();
                    if (lookErrorI(symbol)) {
                        getsymNP();
                    }
                }
            }
            break;
        default:
            error_Exp();
            if(lookErrorI(symbol)) {
                getsymNP();
            }
    }
//    outputFile << "<Stmt>" << endl;
    return false;
}

//表达式 Exp → AddExp 注：SysY 表达式是int 型表达式
int error_Exp() {
    int isInt;
    isInt = error_AddExp();
//    outputFile << "<Exp>" << endl;
    return isInt;
}

//条件表达式 Cond → LOrExp
bool error_Cond() {
    error_LOrExp();
//    outputFile << "<Cond>" << endl;
    return true;
}

/*左值表达式 LVal → Ident {'[' Exp ']'}
 *传入是否是赋值语句Assign
 *传出dim就是ident变量的原dim
*/
int error_LVal(bool isAssign) {
    int dim;
    lookErrorCv();
    lookErrorH(isAssign, neotoken);
    for (int i =0 ;i < varTable.size(); i++) {
        if (varTable[i].ident == neotoken) {
            dim = varTable[i].dim;
        }
    }
    getsymNP();
    while (symbol == LBRACK) {
        dim -= 1;
        getsymNP();
        error_Exp();
        if (lookErrorK(symbol)) {
            getsymNP();
        }
    }
//    outputFile << "<LVal>" << endl;
    return dim;
}

//基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // 三种情况均需覆盖
int error_PrimaryExp() {
    int isInt;
    if (symbol == LPARENT) {
        getsymNP();
        isInt = error_Exp();
        if (lookErrorJ(symbol)) {
            getsymNP();
        }
    }
    else if (symbol != INTCON) {
        isInt = error_LVal(false);
    }
    else if (symbol == INTCON) {
        error_Number();
        isInt = 0;
    }
//    outputFile << "<PrimaryExp>" << endl;
    return isInt;
}

//数值 Number → IntConst // 存在即可
bool error_Number(){
    if (symbol == INTCON) {
        getsymNP();
//        outputFile << "<Number>" << endl;
        return true;
    }
    return false;
}

//⼀元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp // 存在即可
int error_UnaryExp() {
    int isInt;
    if (symbol == PLUS || symbol == MINU || symbol == NOT) {
        error_UnaryOp();
        isInt = error_UnaryExp();
    }
    else if (symbol == LPARENT || symbol == INTCON) {
        isInt = error_PrimaryExp();
    }
    else if (symbol == IDENFR) {
        int funcNameLine = curLine;
        if (symbolList[symbolListI].symbol == LPARENT) {
            string funcIdent = neotoken;
            lookErrorCf();
            getsymP();
            getsymNP();
            for (int i = 0; i < funcTable.size(); i++) {    // return isInt
                if (funcTable[i].ident == funcIdent) {
                    isInt = (funcTable[i].type == Int) ? 0 : -1;
                    break;
                }
            }
            if (error_isInFirstExp()) {
                error_FuncRParams(funcIdent, funcNameLine);
                if (lookErrorJ(symbol)) {
                    getsymNP();
                }
            }
            else if (lookErrorJ(symbol)) {
                lookErrorD(funcIdent, 0, funcNameLine);
                getsymNP();
            }
        }
        else {
            isInt = error_PrimaryExp();
        }
    }
//    outputFile << "<UnaryExp>" << endl;
    return isInt;
}

//单⽬运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 // 三种均需覆盖
bool error_UnaryOp() {
    if (symbol == PLUS || symbol == MINU || symbol == NOT) {
        getsymNP();
//        outputFile << "<UnaryOp>" << endl;
        return true;
    }
    return true; //
}

//函数实参表 FuncRParams → Exp { ',' Exp }
bool error_FuncRParams(string funcIdent, int funcNameLine) {
    int rCount = 1;
    lookErrorE(funcIdent, error_Exp(), rCount, funcNameLine);
    while (symbol == COMMA) {
        rCount += 1;
        getsymNP();
        lookErrorE(funcIdent, error_Exp(), rCount, funcNameLine);
    }
    lookErrorD(funcIdent, rCount, funcNameLine);
//    outputFile << "<FuncRParams>" << endl;
    return true;
}

//乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
int error_MulExp() {
    int isInt;
    isInt = error_UnaryExp();
//    outputFile << "<MulExp>" << endl;
    while (symbol == MULT || symbol == DIV || symbol == MOD) {
        getsymNP();
        isInt = error_UnaryExp();
//        outputFile << "<MulExp>" << endl;
    }
    return isInt;
}

//加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
int error_AddExp() {
    int isInt;
    isInt = error_MulExp();
//    outputFile << "<AddExp>" << endl;
    while (symbol == PLUS || symbol == MINU) {
        getsymNP();
        isInt = error_MulExp();
//        outputFile << "<AddExp>" << endl;
    }
    return isInt;
}

//关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
bool error_RelExp() {
    error_AddExp();
//    outputFile << "<RelExp>" << endl;
    while (symbol == LSS || symbol == GRE || symbol == LEQ || symbol == GEQ) {
        getsymNP();
        error_AddExp();
//        outputFile << "<RelExp>" << endl;
    }
    return true;
}

//相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
bool error_EqExp() {
    error_RelExp();
//    outputFile << "<EqExp>" << endl;
    while (symbol == EQL || symbol == NEQ) {
        getsymNP();
        error_RelExp();
//        outputFile << "<EqExp>" << endl;
    }
    return true;
}

//逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp // 1.EqExp 2.&& 均需覆盖
bool error_LAndExp() {
    error_EqExp();
//    outputFile << "<LAndExp>" << endl;
    while (symbol == AND) {
        getsymNP();
        error_EqExp();
//        outputFile << "<LAndExp>" << endl;
    }
    return true;
}

//逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp // 1.LAndExp 2.|| 均需覆盖
bool error_LOrExp() {
    error_LAndExp();
//    outputFile << "<LOrExp>" << endl;
    while (symbol == OR) {
        getsymNP();
        error_LAndExp();
//        outputFile << "<LOrExp>" << endl;
    }
    return true;
}

//常量表达式 ConstExp → AddExp 注：使⽤的Ident 必须是常量 // 存在即可
bool error_ConstExp() {
    error_AddExp();
//    outputFile << "<ConstExp>" << endl;
    return true;
}

//main function
bool errorDetectProcess(){
    if (error_CompUnit()) {
        return true;
    }
    return false;
}



