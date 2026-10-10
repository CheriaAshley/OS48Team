# Lab1 AI 辅助实验提示词记录

## 练习1：成员1 AI 辅助实验记录

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

## 练习2：使用 GDB 验证启动流程（成员2）

以下提示词按 Lab0.5 的标准四段式，将本次实际对话中的任务、约束和验证要求整理成便于复用的形式，不是每条聊天消息的逐字转录。成员在 VS Code Remote WSL 终端实际执行命令，AI 根据成员提供的输出以及保存的 GDB 日志进行分析。

### 提示词5：确认练习2的目标与 QEMU/GDB 调试方式

**[PROMPT] 任务说明**

严格按照实验指导书，在队友已完成练习1的基础上，说明练习2的目标，并一步一步指导我在 VS Code 远程连接 WSL 的终端中完成调试。先核对环境和连接状态，再开始跟踪启动流程。

**[RELY] 上下文信息**

小组仓库为 `/mnt/e/os_team/OS48Team`，工作分支为 `lab1-exercise2`，代码目录为 `code`。练习2要求跟踪 QEMU 模拟的 RISC-V 从复位到进入内核第一条指令的过程，内核入口地址为 `0x80200000`。

本次使用 QEMU `4.1.1`、SiFive GCC `10.2.0` 和 SiFive GDB-Metal `10.1`。项目原始 Makefile 使用 `-device loader` 装入镜像，`make debug` 添加 `-s -S`，`make gdb` 连接 `localhost:1234`。

连接过程中曾出现：

```text
Ignoring packet error, continuing...
Remote replied unexpectedly to 'vMustReplyEmpty': timeout
```

我曾尝试在浏览器打开 `localhost:1234`，需要说明该端口的用途以及 `make debug` 没有继续输出是否正常。

**[GUARANTEE] 接口清单**

1. 只使用已有的 `make debug`、`make gdb` 和 GDB 命令进行本练习，不修改共享内核源码和 Makefile。
2. 不将启动命令没有输出、出现 `(gdb)` 提示符或编译成功单独作为连接成功的证据。
3. 不通过另一个 GDB 会话抢占成员当前正在使用的调试连接。
4. 区分 WSL 终端操作、浏览器访问和 GDB 远程协议；不能根据报错虚构已证实的唯一原因。

**[SPECIFICATION] 行为规范**

1. 给出两个 WSL 终端的启动顺序，说明 QEMU 的 `-s` 和 `-S` 的作用。
2. 解释 TCP 1234 端口提供 GDB 调试服务，不能当作 HTTP 网页访问。
3. 对连接异常先结束原会话，再重新启动 QEMU 和 GDB。
4. 以成功连接后的初始 PC 和反汇编结果作为后续调试依据。
5. 每次给出少量命令，解释验证目的，等待实际结果后继续。

**AI 回答摘要**

AI 说明 VS Code Remote WSL 的集成终端可以直接运行 Linux 中安装的实验工具。`make debug` 会使 QEMU 暂停等待 GDB，因此安静等待是正常现象；1234 端口使用 GDB 远程协议，不能通过浏览器验证。

针对连接异常，AI 指导关闭浏览器访问、结束原有 QEMU/GDB 会话并重新连接，随后根据实际执行位置判断连接是否恢复。

**验证结果**

重新启动后，终端显示：

```text
Remote debugging using localhost:1234
0x0000000000001000 in ?? ()
```

随后 `info registers pc` 和 `x/5i $pc` 成功显示 `pc = 0x1000` 及五条复位指令，说明调试连接已经能够读取目标状态。没有对原连接错误进行协议抓包，因此只记录恢复操作与结果，不认定浏览器访问是唯一原因。

---

### 提示词6：逐条跟踪复位指令并验证固件与内核入口

**[PROMPT] 任务说明**

我已连接 GDB 并停在 `0x1000`。请逐步指导我观察最初几条指令的功能，再验证跳转到 OpenSBI 和内核入口的过程。我输入完后，你读取日志确认结果，并解释寄存器变化。

**[RELY] 上下文信息**

初始反汇编如下：

```asm
0x1000: auipc t0,0x0
0x1004: addi  a1,t0,32
0x1008: csrr  a0,mhartid
0x100c: ld    t0,24(t0)
0x1010: jr    t0
```

内核符号来自 `bin/kernel`，内核入口为 `0x80200000`。调试输出写入 `../report/lab1-exercise2-gdb.log`，使用的 GDB 10.1 支持 `set logging on` 和 `set logging off`。

**[GUARANTEE] 接口清单**

1. 使用 `si`、`info registers`、`x/i`、`x/5i`、`x/gx`、`break`、`continue` 和日志命令观察已有程序。
2. 不修改寄存器、内存或入口代码来人为制造预期结果。
3. AI 能读取保存的日志，不宣称直接看到 VS Code 的实时终端画面。
4. 区分复位代码、OpenSBI 固件和内核入口，区分镜像装入与 CPU 执行。
5. 未执行的命令和未观察到的内核输出，不得写成验证通过。

**[SPECIFICATION] 行为规范**

1. 开启日志，记录初始 PC 和最初五条指令。
2. 每次单步后查看相关寄存器，说明 `t0`、`a1`、`a0` 的变化。
3. 通过 `x/gx 0x1018` 检查跳转地址，再验证 `ld` 和 `jr` 的实际效果。
4. 到达 OpenSBI 后，设置 `break *0x80200000`，通过 `continue` 等待入口断点，避免逐条跟踪大量固件代码。
5. 命中内核入口后查看 PC 和机器指令，说明源码伪指令与反汇编的关系。
6. 对照截图和日志回答最初指令的位置与主要功能；结束时关闭日志。

**AI 回答摘要**

AI 按指令顺序给出少量操作，先解释预期值，再根据实际日志确认。单步执行依次得到 `t0 = 0x1000`、`a1 = 0x1020`、`a0 = 0`，随后从 `0x1018` 读取 `0x80000000` 并跳转到 OpenSBI。

AI 建议在 `0x80200000` 设置断点，通过继续执行验证固件向内核的控制权交接，而不是单步遍历整个 OpenSBI。报告整理时进一步核对 QEMU 4.1.1 平台源码，确认 `a1` 对应本版本的设备树地址。

**验证结果**

实际日志和截图记录了：

```text
初始 PC：                   0x1000
读取 0x1018 得到的入口地址： 0x0000000080000000
执行 jr t0 后的 PC：        0x80000000
最终内核入口 PC：           0x80200000 <kern_entry>
```

GDB 命中 `kern/init/entry.S:7` 的 `la sp, bootstacktop`，反汇编显示入口机器指令为 `auipc sp,0x3`、`mv sp,sp`，随后是跳转到 `kern_init` 的指令。此时停在内核第一条机器指令执行之前；本次验证不包含后续 BSS 清零和控制台输出。

完整证据见 [GDB 调试日志](./lab1-exercise2-gdb.log) 和 [实验报告的练习2部分](./report.md#练习2使用-gdb-验证启动流程)。

---

### 提示词7：根据实际截图与日志整理练习2报告

**[PROMPT] 任务说明**

我已经完成练习2，并把截图按时间顺序放入 `images` 文件夹。请参考队友已有报告的形式，只补写练习2，完成图片重命名、`prompt.md` 的 AI 协作记录以及 `report.md` 中的练习2部分。

**[RELY] 上下文信息**

队友已经在 `report/report.md` 中完成练习1，在 `report/prompt.md` 中记录四段式提示词，并将图片以 `lab1_*.png` 的形式存放于 `report/images`。

本次有六张时间命名截图，分别记录复位入口、保存基地址、准备启动参数、跳转到 OpenSBI、命中内核断点和最终 PC。对应的 GDB 输出保存在 `report/lab1-exercise2-gdb.log`。

**[GUARANTEE] 接口清单**

1. 只补写练习2及其对应的验证、提示词和图片引用，保留队友的练习1内容。
2. 图片仅按内容重命名并归档，不编辑截图像素或改写调试日志。
3. 所有地址、指令与结果以实际截图和日志为依据，不生成虚构截图、输出或测试通过记录。
4. 未提供的学号和姓名保留占位，不代填身份信息。
5. 不修改内核源码，不自动提交或推送仓库。

**[SPECIFICATION] 行为规范**

1. 逐张检查截图内容，将六张图片整理到 `report/images`，使用 `lab1_exercise2_*.png` 命名。
2. 按“实验任务 → 调试环境 → 调试过程 → 问题回答 → 问题与协作记录”组织练习2报告。
3. 写明 `0x1000 → 0x80000000 → 0x80200000` 的实际启动路径，并说明最初五条指令的功能。
4. 按 `[PROMPT]`、`[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 整理实际协作过程，补充回答摘要与验证结果。
5. 说明重复入口断点不影响结果，并区分已验证的内核入口与尚未执行的后续初始化。
6. 检查 Markdown 图片链接、日志链接、改动范围以及图片内容是否在移动后保持一致。

**AI 回答摘要**

AI 对照六张截图、GDB 日志和队友的写作格式，将报告按实际操作顺序组织，保留命令、关键输出与分析，分别回答指导书的两个问题。提示词记录覆盖调试连接问题、单步验证和报告整理过程。

**验证结果**

六张图片按内容重命名并归档，报告引用能够定位到对应文件；截图和日志中的入口地址及指令相互一致。报告保留了练习1的原有内容，新增练习2及其验证说明。具体文件变更以当前分支的工作区差异为准。

---
