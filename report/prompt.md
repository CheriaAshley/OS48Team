# Lab3 AI 提示词记录

## 成员一：2413195


**提示词：**

```text
[PROMPT]
分析 Lab3 中 TrapFrame 的数据结构，
说明 CPU 上下文在内存中如何组织。

[RELY]
依赖 kern/trap/trap.h 中的 struct pushregs、
struct trapframe，以及 trapentry.S 中已有的 SAVE_ALL。

[GUARANTEE]
需要说明各结构体成员的作用，
以及寄存器保存位置与结构体布局的对应关系。

[SPECIFICATION]
基于 RV64 架构，REGBYTES 为 8。
重点分析通用寄存器和四个 CSR 状态保存槽位。
不修改现有代码，只提供结构分析和实现思路。
```

**AI 回答：**

`struct pushregs` 用于组织 32 个通用寄存器的保存槽位，`struct trapframe` 在此基础上增加 `status`、`epc`、`badvaddr` 和 `cause`。在 RV64 下，每个槽位占用 8 字节，总共 36 个槽位，占用 288 字节。`SAVE_ALL` 根据对应偏移保存寄存器，后续恢复过程必须使用相同布局，避免寄存器数据错位。



**提示词：**

```text
[PROMPT]
分析 RESTORE_ALL 的实现思路，
重点说明寄存器恢复顺序及特殊寄存器的处理。

[RELY]
依赖已有 SAVE_ALL、TrapFrame 数据结构、
LOAD、REGBYTES 宏以及 RISC-V 的 sret 指令。

[GUARANTEE]
明确需要恢复的寄存器和 CSR 状态，
给出合理的恢复步骤及需要注意的问题。

[SPECIFICATION]
LOAD 偏移必须与 SAVE_ALL 的 STORE 偏移对应。
恢复 sstatus、sepc 和必要的通用寄存器。
x0 恒为零，无需恢复。
sp 用于定位 TrapFrame，不能提前修改。
只分析 RESTORE_ALL，不涉及其他成员的模块。
```

**AI 回答：**

首先从 TrapFrame 中读取 `sstatus` 和 `sepc`，使用 `csrw` 恢复对应的 CSR；然后按照保存偏移，利用 `LOAD` 恢复其他通用寄存器。`x0` 不需要恢复，`sp` 应保持有效直到其他数据读取完成。最后通过 `addi sp, sp, 36*REGBYTES` 恢复原栈指针，由后续 `sret` 完成 Trap 返回。


**提示词：**

```text
[PROMPT]
分析 trapentry.S 单独编译时出现的
unknown CSR 'sbadaddr' 错误，并说明修复方法。

[RELY]
依赖当前汇编器报错、SAVE_ALL 中的
csrr s3, sbadaddr 指令及 RISC-V CSR 定义。

[GUARANTEE]
定位报错原因，说明正确的 CSR 名称，
并判断是否需要修改 TrapFrame 结构体。

[SPECIFICATION]
尽量保持老师原有代码结构。
不改变 SAVE_ALL 的寄存器保存布局，
不修改其他成员负责的代码。
```

**AI 回答：**

`sbadaddr` 是旧版 RISC-V 使用的 CSR 名称，当前汇编器识别对应的 `stval`。将 `csrr s3, sbadaddr` 修改为 `csrr s3, stval`，即可解决该名称不兼容的问题。`trapframe` 中的 `badvaddr` 是普通结构体成员名称，不需要修改。修复后重新单独编译，汇编文件成功通过检查。


**提示词：**

```text
[PROMPT]
设计成员一 RESTORE_ALL 的独立编译检查方法，
避免其他尚未实现的模块影响验证。

[RELY]
依赖 Lab3 的 Makefile、RISC-V 交叉编译工具链
及 kern/trap/trapentry.S 源文件。

[GUARANTEE]
提供独立编译命令、检查方法，
并说明编译通过能够验证哪些内容。

[SPECIFICATION]
不修改 trap.c 中尚未完成的中断和异常处理函数。
优先使用现有 Makefile 的构建规则。
区分汇编检查与完整系统运行测试。
```

**AI 回答：**

可在 `code` 目录执行 `make obj/kern/trap/trapentry.o`，单独编译 `trapentry.S`。该方法不依赖 `trap.c` 中尚未完成的处理函数。实际修复 CSR 名称问题后，编译正常结束，说明汇编文件通过了当前工具链的检查。但编译成功不能证明上下文恢复在运行时完全正确，仍需要等待其他模块完成后，通过 `make qemu` 和 `make grade` 进行集成验证。

---

## 成员二：2413467-徐熙凯

以下根据成员 B 的实际开发需求整理为四段式提示词，AI 回答为本次实现及验证过程的摘要。

**提示词：**

```text
[PROMPT]
检查 __alltraps 与 TrapFrame、trap()、__trapret 的关系，
明确成员 B 需要实现的入口参数传递和返回调用。

[RELY]
依赖已有的 SAVE_ALL、RESTORE_ALL、struct trapframe，
以及 void trap(struct trapframe *tf) 的签名和 RV64 ABI。

[GUARANTEE]
说明保存后 sp 指向哪里，第一个参数使用哪个寄存器，
以及 C 函数返回后怎样继续恢复现场。

[SPECIFICATION]
以当前源码为依据，不改变 A/C 的实现或 TrapFrame 布局。
区分普通函数返回与 sret，不假定所有 Trap 都返回用户态。
```

**AI 回答：**

`SAVE_ALL` 分配 288 字节并保存现场后，`sp` 指向当前 TrapFrame。需要将该指针放入第一参数寄存器 `a0`，再通过正常函数调用进入 `trap()`。C 函数返回后，栈指针应恢复到调用前的位置，继续进入 `__trapret`。普通函数返回使用 `ra`，最终 Trap 返回使用 `sepc` 和保存的状态，二者不能混淆。

**提示词：**

```text
[PROMPT]
实现成员 B 的 Trap 入口与返回调用，并接通已有的中断处理流程。

[RELY]
成员 A 已完成 RESTORE_ALL，成员 C 已完成 interrupt_handler()
和 exception_handler()；__trapret 和 sret 已经存在。

[GUARANTEE]
正确传入 TrapFrame 指针，调用 trap() 并衔接原返回流程。
将对应 YOUR CODE 替换为学号 2413467。

[SPECIFICATION]
保存现场之后才能改写参数寄存器。
调用前后 sp 保持同一 TrapFrame 基址，不重复调整栈空间。
不改动 SAVE_ALL、RESTORE_ALL、trap.c、结构体或原有测试。
本次不提交或上传 GitHub。
```

**AI 回答：**

在 `SAVE_ALL` 后补充 `mv a0, sp` 和 `jal trap`。前者传入帧指针，后者生成有效的函数返回地址；`trap()` 返回后顺序进入已有的 `__trapret`。原 `a0` 和 `ra` 在调用前已由 `SAVE_ALL` 保存，因此可以由 `RESTORE_ALL` 恢复。除了 B 的标记和调用代码，其他成员的实现保持不变。

**提示词：**

```text
[PROMPT]
完成 B 后检查实际运行是否成功，运行 make grade，
并根据真实测试结果填写成员 B 的报告。

[RELY]
使用仓库原有 Makefile、注入脚本及课程测试，
使用现有 WSL RISC-V 工具链和 QEMU 环境。

[GUARANTEE]
验证寄存器上下文、断点返回、非法指令返回、兜底处理、
持续时钟中断以及正常内核达到规定次数后的关机。
保存原始输出，说明每项检查覆盖的行为。

[SPECIFICATION]
不修改测试来取得分数，不虚构截图或失败经历。
区分评分内核与正常 make qemu 的运行结果。
仅填写 B 的内容，并注明联合测试对 A/C 的验证范围。
```

**AI 回答：**

先运行原有 `make grade`，核对三个评分项及总分，再重新构建正常内核执行 `make qemu`，检查计数输出和关机。接入前的真实问题是入口没有调用 C 处理函数，导致初始化后没有时钟统计。最终结果以 `report/verification/lab3-b-verification.txt` 的原始输出及实验报告测试章节为准。
