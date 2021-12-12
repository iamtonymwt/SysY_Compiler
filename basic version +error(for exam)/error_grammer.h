#include <string>
#include <vector>
using namespace std;

void error_deleteVarTable();
bool error_isInFirstExp();
bool error_IsExp();
int error_UnitBranch();

enum vfType{
    Int, Void
};
struct var{
    string ident;
    int type;
    bool isConst;
    int dim = 0;        //此处dim和value有缺陷    //对应数组维数
    int level;
};
struct func{
    string ident;
    int type;
    int returnType; //-1无return，0return，1return int
    vector<var> params;
};

//编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef
//声明 Decl → ConstDecl | VarDecl // 覆盖两种声明
bool error_CompUnit();

//常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
bool error_ConstDecl();

//基本类型 BType → 'int' // 存在即可
//常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
bool error_ConstDef();

//常量初值 ConstInitVal → ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
bool error_ConstInitVal();

//变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
bool error_VarDecl();

//变量定义 VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
bool error_VarDef();

//变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
bool error_InitVal();

//函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
bool error_FuncDef();

//主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block
bool error_MainFuncDef();

//函数类型 FuncType → 'void' | 'int'
bool error_FuncType(func& a);

//函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
bool error_FuncFParams(func& a);

//函数形参 FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }]
bool error_FuncFParam(func& a);

//语句块 Block → '{' { BlockItem } '}'
bool error_Block(bool isFuncFollow);

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
bool error_Stmt();

//表达式 Exp → AddExp 注：SysY 表达式是int 型表达式
int error_Exp();

//条件表达式 Cond → LOrExp
bool error_Cond();

//左值表达式 LVal → Ident {'[' Exp ']'} //1.普通变量 2.⼀维数组 3.⼆维数组
int error_LVal(bool isAssign);

//基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // 三种情况均需覆盖
int error_PrimaryExp();

//数值 Number → IntConst // 存在即可
bool error_Number();

//⼀元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp // 存在即可
int error_UnaryExp();

//单⽬运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 // 三种均需覆盖
bool error_UnaryOp();

//函数实参表 FuncRParams → Exp { ',' Exp }
bool error_FuncRParams(string funcIdent, int funcNameLine);

//乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
int error_MulExp();

//加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
int error_AddExp();

//关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
bool error_RelExp();

//相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
bool error_EqExp();

//逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp // 1.EqExp 2.&& 均需覆盖
bool error_LAndExp();

//逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp // 1.LAndExp 2.|| 均需覆盖
bool error_LOrExp();

//常量表达式 ConstExp → AddExp 注：使⽤的Ident 必须是常量 // 存在即可
bool error_ConstExp();

//main function
bool errorDetectProcess();
