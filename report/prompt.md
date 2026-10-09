# Lab1 成员1 AI 辅助实验提示词记录

### 提示词1：分析 RISC-V 内核汇编入口

**[PROMPT] 任务说明**

分析 Lab1 最小可执行内核的汇编入口，解释其如何完成启动栈初始化并进入 C 语言内核函数。分析结果用于练习1的实验报告。

**[RELY] 上下文信息**

实验环境为 QEMU 模拟的 RISC-V 64 位计算机，使用 OpenSBI 启动内核，内核入口地址为 `0x80200000`。

`kern/init/entry.S` 的关键代码为：

```asm
.section .text,"ax",%progbits
.globl kern_entry
kern_entry:
    la sp, bootstacktop
    tail kern_init

.section .data
    .align PGSHIFT
    .global bootstack
bootstack:
    .space KSTACKSIZE
    .global bootstacktop
bootstacktop:
```

**[GUARANTEE] 接口清单**

本任务只分析现有源码，不修改 `entry.S`、链接脚本或其他项目文件。必须结合实际代码解释标签与寄存器，不得假设不存在的函数或启动步骤。

**[SPECIFICATION] 行为规范**

1. 说明 `la` 伪指令、`sp` 寄存器及 `bootstacktop` 标签的含义。
2. 解释 `la sp, bootstacktop` 的实际操作及初始化栈的必要性。
3. 说明 `tail kern_init` 的跳转方式及其与普通函数调用的区别。
4. 解释 `.align PGSHIFT` 和 `.space KSTACKSIZE` 的作用。
5. 按照“汇编入口 → 栈初始化 → C 函数入口”的顺序组织说明。
6. 给出可以通过反汇编验证的关键结论。

**AI 回答摘要**

AI 指出，`la sp, bootstacktop` 将启动栈顶部地址加载到栈指针寄存器 `sp` 中，使后续 C 函数能够使用有效的栈空间。`tail kern_init` 将执行流程直接转移至 C 语言函数 `kern_init()`，不需要返回原汇编入口。

此外，`bootstack` 和 `bootstacktop` 分别标记预留栈空间的起始位置和顶部位置，栈初始化是后续内核代码正常执行的前提。

**验证结果**

使用 `riscv64-unknown-elf-objdump -d bin/kernel` 查看反汇编结果，观察到 `la` 被转换为实际地址计算指令，`tail kern_init` 被优化为跳转到 `kern_init` 的指令，验证了源码分析结果。

---

### 提示词2：分析 C 语言内核初始化流程

**[PROMPT] 任务说明**

分析 `kern/init/init.c` 中 `kern_init()` 的执行过程，说明汇编入口跳转到 C 语言函数后，内核依次完成哪些初始化操作，以及各操作之间的依赖关系。

**[RELY] 上下文信息**

内核已通过 `entry.S` 设置启动栈，并使用 `tail kern_init` 进入 C 语言初始化函数。

关键代码如下：

```c
int kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);

    const char *message = "(THU.CST) os is loading ...\n";
    cprintf("%s\n\n", message);

    while (1)
        ;
}
```

项目包含自行实现的基础库和 SBI 控制台接口，不能假设内核已经运行在 Linux 等宿主操作系统之上。

**[GUARANTEE] 接口清单**

仅分析 `kern_init()`、`memset()`、`cprintf()` 及其相关的内核启动机制，不修改函数内容，不添加与本实验无关的功能。

**[SPECIFICATION] 行为规范**

1. 解释 `edata`、`end` 在本实验中用于表示的内存边界。
2. 说明 `memset(edata, 0, end - edata)` 的作用及清零 BSS 段的必要性。
3. 解释内核输出与 SBI 控制台服务之间的关系。
4. 说明 `while(1)` 的目的，以及为什么当前内核不直接返回。
5. 明确区分实验已经实现的初始化操作和后续实验才会涉及的操作系统功能。
6. 输出按实际执行顺序组织的分析结果。

**AI 回答摘要**

AI 将 `kern_init()` 的执行过程分为 BSS 段清零、启动信息输出和无限循环三个阶段。

首先，`memset()` 对指定内存区域清零，使未初始化的全局变量和静态变量具有正确的初始状态。随后，`cprintf()` 通过内核控制台接口及底层 SBI 服务输出启动信息。最后，内核进入无限循环，避免初始化函数返回到无效执行位置。

**验证结果**

通过反汇编观察到 `kern_init()` 调用了 `memset()` 和 `cprintf()`，并在末尾生成循环跳转指令，与源码描述的执行流程一致。

---

### 提示词3：验证内核入口地址与镜像加载结果

**[PROMPT] 任务说明**

在不修改内核源码的前提下，验证 Lab1 生成的 ELF 文件是否符合 RISC-V 最小内核的启动要求，重点检查目标架构、入口地址和入口机器指令。

**[RELY] 上下文信息**

实验使用 `riscv64-unknown-elf-gcc` 交叉编译，生成 `bin/kernel`，并通过 `objcopy` 生成 `bin/ucore.img`。

指导书要求内核入口地址为 `0x80200000`。已有的汇编入口为 `kern_entry`，随后通过 `tail kern_init` 进入 C 语言初始化函数。

**[GUARANTEE] 接口清单**

只允许使用只读检查命令，不修改链接脚本、Makefile 或源代码。不得仅根据编译成功推断内核一定能够正常运行。

**[SPECIFICATION] 行为规范**

1. 给出检查 ELF 文件格式和入口地址的命令。
2. 给出检查 `kern_entry` 反汇编结果的命令。
3. 说明预期观察到的地址与指令关系。
4. 区分磁盘 ELF 文件的入口配置和 QEMU 实际执行状态。
5. 根据终端实际输出判断检查是否通过，不得虚构验证结果。

**AI 回答摘要**

AI 建议使用：

```bash
riscv64-unknown-elf-readelf -h bin/kernel
riscv64-unknown-elf-objdump -d bin/kernel | head -40
```

其中，`readelf` 用于检查 ELF 文件头部和入口地址，`objdump` 用于查看编译后的机器指令，从而验证汇编入口和 C 函数之间的跳转关系。

AI 同时指出，静态反汇编结果只能证明编译链接后的代码正确，不能单独证明 CPU 已经执行到了这些指令。

**验证结果**

实际检查显示：

```text
Class: ELF64
Machine: RISC-V
Entry point address: 0x80200000
```

反汇编显示：

```asm
80200000: auipc sp,0x3
80200004: mv    sp,sp
80200008: j     8020000a <kern_init>
```

结果表明 ELF 架构与入口地址符合实验要求，汇编入口代码也已正确生成。

---

### 提示词4：排查 QEMU 与 OpenSBI 的内核启动问题

**[PROMPT] 任务说明**

排查 Lab1 中 OpenSBI 能够启动，但内核未输出预期字符串的问题。目标是在不修改小组共享源码的条件下定位原因，并找到能够验证最小内核运行的启动方式。

**[RELY] 上下文信息**

使用 `make qemu` 启动时，OpenSBI v1.8 正常显示固件信息，但没有输出：

```text
(THU.CST) os is loading ...
```

已有检查结果如下：

- ELF 入口地址为 `0x80200000`。
- 内核镜像中包含正确的汇编入口指令。
- QEMU 原始启动命令使用 `-bios default` 和 `-device loader,file=bin/ucore.img,addr=0x80200000`。
- OpenSBI 显示的下一阶段入口地址为 `0x0`。
- GDB 能连接 QEMU，但没有命中 `0x80200000` 的内核入口断点。
- 使用 GDB 读取 `0x80200000` 的内存时，能观察到预期的内核机器指令。

**[GUARANTEE] 接口清单**

1. 不直接修改 `Makefile`、`entry.S` 或 `init.c`。
2. 不删除小组代码，不影响其他成员的实验内容。
3. 优先使用可逆的命令行对照测试。
4. 不得将 OpenSBI 正常启动等同于内核成功执行。
5. 只有观察到内核实际输出后，才能确认启动验证成功。

**[SPECIFICATION] 行为规范**

1. 根据已知证据区分编译、镜像加载、固件交接和内核执行四个阶段。
2. 优先检查 OpenSBI 的下一阶段入口地址是否正确。
3. 对比 `-device loader` 与 `-kernel` 两种启动方式。
4. 每次只提出一个主要检查步骤，并解释该步骤的验证目的。
5. 不根据单个寄存器值直接认定故障原因。
6. 给出实际验证成功的命令、输出及结论。
7. 明确说明诊断启动成功不代表原始 `make qemu` 已经修复。

**AI 回答摘要**

AI 首先指导检查 ELF 入口地址、QEMU 加载参数和 GDB 断点执行情况。检查表明，内核机器指令已经加载到预期地址，但原始启动过程中 CPU 未命中内核入口断点。

随后，AI 建议使用 QEMU 的 `-kernel` 参数提供 ELF 内核作为对照，以检查固件能否正确获得下一阶段入口地址。

对照测试命令如下：

```bash
qemu-system-riscv64 -machine virt -nographic -bios default -kernel bin/kernel
```

**验证结果**

采用对照启动命令后，OpenSBI 输出：

```text
Domain0 Next Address : 0x0000000080200000
```

并成功显示：

```text
(THU.CST) os is loading ...
```

表明在该启动配置下，OpenSBI 能够正确向内核交接控制权，内核正常执行初始化与输出操作。

此次问题主要与原始启动方式下的固件交接配置有关。实验并未直接修改原始 Makefile，因此原始 `make qemu` 未成功输出的问题仍需后续统一处理。

