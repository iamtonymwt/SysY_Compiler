#pragma once
#ifndef GRAMMER_GRAMMER_H
#define GRAMMER_GRAMMER_H

#include <string>
#include "vTable.h"
using namespace std;

bool IsExp();
int UnitBranch();


//编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef
//声明 Decl → ConstDecl | VarDecl // 覆盖两种声明
bool CompUnit();

//常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
bool ConstDecl();

//基本类型 BType → 'int' // 存在即可
//常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
bool ConstDef();

//常量初值 ConstInitVal → ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
bool ConstInitVal(int variableIndex);

//变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
bool VarDecl();

//变量定义 VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
bool VarDef();

//变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
bool InitVal(int variableIndex);

//函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
bool FuncDef();

//主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block
bool MainFuncDef();

//函数类型 FuncType → 'void' | 'int'
bool FuncType();

//函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
bool FuncFParams(string ident);

//函数形参 FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }]
bool FuncFParam(string ident);

//语句块 Block → '{' { BlockItem } '}'
bool Block(bool isFuncFollow);

//语句块项 BlockItem → Decl | Stmt
/*语句 Stmt → LVal '=' Exp ';' // 每种类型的语句都要覆盖
| [Exp] ';' //有⽆Exp两种情况
| Block
| 'if' '(' Cond ')' Stmt [ 'else' Stmt ] // 1.有else 2.⽆else
| 'while' '(' Cond ')' Stmt
| 'break' ';' | 'continue' ';'
| 'return' [Exp] ';' // 1.有Exp 2.⽆Exp
| LVal '=' 'getint''('')'';'
| 'printf''('FormatString{','Exp}')'';' // 1.有Exp 2.⽆Exp
*/
bool Stmt();

//表达式 Exp → AddExp 注：SysY 表达式是int 型表达式
string Exp();

//条件表达式 Cond → LOrExp
string Cond();

//左值表达式 LVal → Ident {'[' Exp ']'} //1.普通变量 2.⼀维数组 3.⼆维数组
string LVal(bool isGet);

//基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // 三种情况均需覆盖
string PrimaryExp();

//数值 Number → IntConst // 存在即可
string Number();

//⼀元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp // 存在即可
string UnaryExp();

//单⽬运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 // 三种均需覆盖
string UnaryOp();

//函数实参表 FuncRParams → Exp { ',' Exp }
bool FuncRParams(string ident);

//乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
string MulExp();

//加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
string AddExp();

//关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
string RelExp();

//相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
string EqExp();

//逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp // 1.EqExp 2.&& 均需覆盖
bool LAndExp(string andEndLabel);

//逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp // 1.LAndExp 2.|| 均需覆盖
string LOrExp();

//常量表达式 ConstExp → AddExp 注：使⽤的Ident 必须是常量 // 存在即可
string ConstExp();

//main function
bool Gprocess();

#endif //GRAMMER_GRAMMER_H
