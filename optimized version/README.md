## 编译实验

优化版本

已优化内容：

* 常数合并（未解决变量）

* else少跳一次

* 各种i

* 开头直接j main

* 中间变量改名

* 运算-赋值 窥孔

* 减少运算寄存器使用

* 基本快划分,无流

* 临时寄存区分配

* 块的流

* 块的def use(list不考虑)

* 块的in out(list不考虑)

* 活跃变量分析(list不考虑)

* 死代码删除(list不考虑)

  在out的var的连续def没有删
  
* 把中间代码当成注释

* 全局寄存器分配（注释掉了）

* 优化了neq 和 not

* 函数传参（取消了函数栈、前三个参数寄存器）

* grammar 把短路空跳转删除了

* 全局寄存器分配（包括静态量）计数法（仍可优化）testfile5负优化了。。。

* mips窥孔，删除无用代码

* mul指令比mult+mflo操作要少一个other

* subi $t0, $t1, 100用addi $t0,$t1, -100替换；

* 循环while

* 全局寄存器分配
   		函数开头不用全load （全局）
      		PARAM LOAD
      		循环不全save 循环底看func， continue全存
      		函数call前save（dirty && out||global）
      		函数call完load（out）
      		寄存器数量	
      		$v1,$k0,$k1
   
* 基本快内（未考虑ins）
    		常量合并
    		复制传播



正确性检查：**通过**