# 构建 AST

AST 的结构一般以文法为基础：

例如我有一个文法

```
FuncDef      -> FuncType Ident '(' [FuncFParams] ')' Block
FuncType     -> 'void' | 'int'  
FuncFParams  -> FuncFParam { ',' FuncFParam }
FuncFParam   -> BType Ident ['[' ']' { '[' Exp ']' }]
```

我可以构造出这样的 AST：

```rust
struct Func {
    name: Ident,
    params: Vector,
    ret_ty: FuncTypeDef,
    body: BlockStmt,
}

struct FuncParam {
    name: Ident,
    dims: Dim,
    ty: TypeDef,
}

struct Dim {
    dims: Vector,
    span: Span,
}
```

有几点值得注意：
- AST 的结构和文法高度相似
- 但是 AST 不必拘泥于文法，例如我没有定义 `FuncParams` 这个类，而是用了 `Vector`。
- 同样，文法里面没有 `IfStmt`，但是在 AST 上我要自己定义一个这样的类。

```c
struct IfStmt {
    cond: Expr,
    then_block: BlockStmt,
    else_block: BlockStmt,
}
```

注意每个类还要加上一个字段 `Pos` 表示当前语法范畴在源文件里的位置，用于报错。

另外要善于利用 union、variants 类型（指 C、C++，Java 只能用 enum 模拟 C、C++ 里面 union 的效果）：

```c
union Stmt {
    AssignStmt assignStmt, 
    Expr expr,
    Block blockStmt,
    IfStmt ifStmt,
    WhileStmt whileStmt,
    BreakStmt breakStmt,
    ContinueStmt continueStmt,
    ReturnStmt returnStmt,
    Empty emptyStmt,
}
```

具体看我的代码（in Rust）：https://github.com/roife/racoon/blob/master/src/compiler/syntax/ast.rs，使用的文法是 https://github.com/BUAA-SE-Compiling/miniSysY-tutorial/blob/master/miniSysY.md。

注解：pub 是 public 的意思，vec 是 vector 的意思，enum 类似于 std::variants

作者：wjy

roife:
https://www.verybin.com/?a0c0fdd7ee325882#vaHaYTDdRFscEPI8tjeLwqLZquz4AUb2YsSbAR6irk4=

roife:
具体看我的代码（Rust）：https://github.com/roife/racoon/blob/master/src/compiler/syntax/ast.rs 文法是 https://github.com/BUAA-SE-Compiling/miniSysY-tutorial/blob/master/miniSysY.md。

pub 是 public 的意思，vec 是 vector 的意思，enum 类似于 std::variants，就是多个值里面选择一个