#ifndef MIPS_MIDCODE_H
#define MIPS_MIDCODE_H
#include <string>
#include "vTable.h"
using namespace std;

enum operation {
    PLUSOP, //+ z x y
    MINUOP, //- z x y
    MULTOP, //* z x y
    DIVOP,  // / z x y
    MODOP,  // % z x y
    ANDOP,  // && z x y
    OROP,   // || z x y
    LSSOP,  //< z x y
    LEQOP,  //<= z x y
    GREOP,  //> z x y
    GEQOP,  //>= z x y
    EQLOP,  //== z x y
    NEQOP,  //!= z x y
    NOTOP,  //! z x

    ASSIGNOP,  //= z x

    //z:label
    //x:op
    GOTO,  //无条件跳转 z
    BZ,    //为0跳转 z x
    BNZ,   //为1跳转 z x
    LABEL, //标号 z

    PUSH,  //函数调用时值传递 z:ident x:dim
    PUSHADDR, //函数调用时地址传递 z:ident x:offset y:要传到第几个
    CALL,  //函数调用 z:func_ident
    RET,   //函数返回 z:var_ident
    RETVALUE, //函数返回值赋值 z:ident x:RET

    SCAN,  //读 z:ident
    PRINTD, //写 z:ident
    PRINTS, //写 z:string_ident x:string_content

    CONST, //常量 z
    CONSTARRAY, // 常量数组 z
    ARRAY, //数组声明 z
    VAR,   //变量 z

    FUNC,  //函数定义 z:type x:ident
    PARAM, //函数参数 z:ident
    GETARRAY,  //取数组的值  z:ident x:ident y:offset
    PUTARRAY,  //给数组赋值  z:ident x:offset y:ident
    EXIT,  //退出 main最后

    SW, //寄存器sw z:reg_ident x:ins_ident
};

class midCode {  //z = x op y
public:
    operation op; // 操作
    string z;     // 结果
    string x;     // 左操作数
    string y;     // 右操作数
    midCode(operation o, string zz = "", string xx = "", string yy = "") : op(o), z(zz), x(xx), y(yy) {}
};

void splitAssign(int variableIndex, bool isConstV);
string isContainOffset(string item);
string getVarWithoutOffset(string item);
bool isNumber(string item);
string merge(operation o, string left, string right);

void outputMidCode(ofstream& midCodefile);

string getString();
string getLabel();

#endif //MIPS_MIDCODE_H
