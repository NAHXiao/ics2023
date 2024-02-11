#include <am.h>
#include <nemu.h>

extern char _heap_start;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END); // 用于指示堆区的起始和末尾
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

void putch(char ch) { outb(SERIAL_PORT, ch); } // 用于输出一个字符

void halt(int code) { // 用于结束程序的运行
  // (#abstract-machine/am/src/platform/nemu/include/nemu.h)
  // 汇编nemu_trap(二进制)
  nemu_trap(code);
  /*nemu_trap()宏还会把一个标识结束的结束码移动到通用寄存器中, 这样,
   * 这段汇编代码的功能就和nemu/src/isa/$ISA/instr/special.h
   * 中的执行辅助函数def_EHelper(nemu_trap)对应起来了:
   * 通用寄存器中的值将会作为参数传给rtl_hostcall,
   * rtl_hostcall将会根据传入的id(此处为HOSTCALL_EXIT)来调用set_nemu_state(),
   * 将halt()中的结束码设置到NEMU的monitor中,
   * monitor将会根据结束码来报告程序结束的原因.*/

  // should not reach here
  while (1)
    ;
}

void _trm_init() { // 用于进行TRM相关的初始化工作
  int ret = main(mainargs);
  halt(ret);
}
