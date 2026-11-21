# Lab4 AI 提示词记录

## 成员 A · 2413195-钟沁涵


**我的提示词**

- **任务目标**　我已经把老师的 Lab4 骨架放进 `code/`。请对照小组 Lab3 的代码，找出 Best Fit 和 `page_init()` 应该迁移到哪里。
- **依赖依据**　老师提供的 `lab4.zip`、小组已同步的 `lab3` 分支，以及新版 `default_pmm.c` 和 `pmm.c` 中的 `lab2` 标记。
- **预期结果**　说明需要复用哪些函数、怎么接入新版分配器，以及迁移后先编译哪些文件。
- **实现约束**　沿用 Lab4 的初始化流程、实验注释和老师提供的检查函数。

**AI 回答**

Lab4 的 `pmm_init()` 已经安排了页表检查和 `kmalloc_init()`。把旧版的 `page_init()` 和它依赖的两个常量放到 `pmm.c` 的 `lab2` 标记处即可。新版 `default_pmm.c` 引用了压缩包里没有的 `best_fit_pmm.h`，可以改用骨架已有的 `default_pmm.h`，把旧版 Best Fit 的分配和回收逻辑接到新版使用的 `default_pmm_manager`，保留新版的 `basic_check()` 和 `best_fit_check()`。完成后先单独编译 `default_pmm.c` 和 `pmm.c`。

**我的提示词**

- **任务目标**　我负责 `page_insert()` 和 `page_remove()`。请结合老师的测试，说明首次映射、同页更新、换页和重复删除时，代码应该怎样修改页表项和引用计数。
- **依赖依据**　使用骨架已有的 `get_pte()`、`pte_create()`、物理页引用计数接口和 `tlb_invalidate()`，对照 `check_pgdir()` 及 PMM 测试。
- **预期结果**　重复映射同一物理页时引用计数不变，换页时旧页和新页的计数正确，撤销或更新映射后旧的 TLB 翻译失效。
- **实现约束**　插入时允许建立缺失的中间页表，定位失败返回 `-E_NO_MEM`。删除时只查找已有路径，保留原有测试代码和实验标记。

**AI 回答**

插入时用 `get_pte(pgdir, la, 1)` 找表项。首次映射给目标页增加一次引用。旧表项已经指向目标页时，直接更新表项及权限，避免把只有一次引用的页提前释放。换页时先撤销旧映射，再给新页增加引用。删除时用 `get_pte(pgdir, la, 0)`，找不到有效映射就直接返回。找到后清空表项、刷新旧翻译，再减少原物理页的引用，计数为零才释放。老师的测试会传入权限 `0` 或仅带 `PTE_W` 的值，代码为这两种组合补上 `PTE_R`，以满足 Sv39 叶表项的编码要求。


**我的提示词**

- **任务目标**　我已经按步骤修改并推送代码。请根据终端截图判断成员 A 的代码编译到哪一步，以及是否上传成功。
- **依赖依据**　截图中的 `git diff --check`、两条目标文件的构建输出，以及 `git commit`、`git push origin lab4` 的结果。
- **预期结果**　明确编译、远端推送和小组运行测试各自的状态，方便填写实验报告。
- **实现约束**　只记录终端已经证实的结果，QEMU 和 `make grade` 的结论以小组联调后的实际输出为准。

**AI 回答**

`git diff --check` 无错误输出。`default_pmm.o is up to date` 表示已有的目标文件无需重新生成，`+ cc kern/mm/pmm.c` 后没有编译错误，说明新增页表代码通过了 RISC-V 单文件编译。推送日志中的 `7d91f6b..10de740 lab4 -> lab4` 表示远端 `lab4` 已更新到提交 `10de740`。目前可以在报告中填写编译和推送结果，并保留小组运行及评分截图的位置。


## 成员 B · 2413467-徐熙凯

以下按本次实际需求和实现约束整理，不作为逐字聊天记录。

**我的提示词**

- **任务目标**　先读 Lab4 指导书，克隆小组 lab4 分支，按分工检查队友完成情况，再实现我负责的进程创建与初始化。
- **依赖依据**　指导书模块二、分工图、当前 proc_struct/context/trapframe 定义，以及 kernel_thread_entry、forkrets、PMM、SLOB 和已有进程辅助函数。
- **预期结果**　完成 alloc_proc、proc_init、kernel_thread、do_fork、copy_thread、copy_mm，正确建立 idleproc 和可运行的 initproc。
- **实现约束**　保留 Lab4 骨架接口和实验注释，模块二标记填写 2413467，只修改自己的模块；本地验证，不上传 GitHub。

**AI 回答**

远端基线为 6d21c0b。A 已实现物理内存迁移及页映射，C 的调度、上下文切换和 Lab3 迁移尚未填写。B 在 proc.c 中分配并初始化 PCB，复用启动栈建立 idleproc，再构造临时 trapframe，通过 do_fork 分配内核栈、设置父进程、复制现场并登记 PID。kernel_thread_entry 使用 s0 中的函数和 s1 中的参数，故初始 trapframe 必须与该入口一致。context.ra 指向 forkret，后续由 C 的切换代码连接首次运行。

**我的提示词**

- **任务目标**　明确创建失败时如何回收，并检查首次执行现场与现有汇编接口是否匹配。
- **依赖依据**　setup_kstack、put_kstack、get_pid、hash_proc、wakeup_proc，以及本实验只创建内核线程的限制。
- **预期结果**　资源不足返回负错误码；失败不挂入进程表；成功返回 PID，保存正确父进程和就绪状态。
- **实现约束**　copy_mm 不实现用户地址空间复制；先构造完整现场，再在本地中断保护下登记、计数和唤醒。保留失败回收分支，不能用直接 panic 替代 do_fork 的资源不足返回。

**AI 回答**

alloc_proc 清零 PCB 并设置初始状态、PID 和内核页表地址。do_fork 先检查进程数上限，依次分配 PCB 和栈；栈分配失败释放 PCB，copy_mm 失败先释放栈再释放 PCB。copy_thread 将现场放入独立内核栈，设置子线程返回值 a0=0 和 context.ra/sp。kernel_thread 设置 SPP/SPIE 并清除 SIE。正常 copy_mm 断言 current->mm 为空，保留后续扩展接口。

**验证要求整理**

- **任务目标**　运行编译与评分，区分我的模块错误和队友尚未实现造成的限制。
- **依赖依据**　仓库原有 make grade、tools/inject.py 和 test_lab4.c。
- **预期结果**　记录实际输出；额外验证现场字段、PID 登记、失败回收和进程数上限。
- **实现约束**　不修改老师测试；临时测试桩及故障注入不进入仓库，不把隔离测试写成完整评分通过。

**AI 回答**

B 与 A 的目标文件编译通过。原始评分在链接阶段因 C 的 cpu_idle/schedule 缺失失败，未产生完整成绩。在临时副本中保留原 PMM 和 Process Create 断言，于 proc_init 后结束运行，隔离未完成调度路径；二者及 B 的附加检查均通过。首次实际调度到 init_main、完整 make grade 和最终截图仍待 C 完成后联调。详见 report.md 和 verification/lab4-b-verification.txt。
