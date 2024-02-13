/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "../local-include/reg.h"
#include <cpu/difftest.h>
#include <isa.h>

/*
 * 实现isa_difftest_checkregs()函数,
 * 把通用寄存器和PC与从DUT中读出的寄存器的值进行比较. 若对比结果一致,
 * 函数返回true; 如果发现值不一样, 函数返回false,
 * 框架代码会自动停止客户程序的运行. 特别地,
 * isa_difftest_checkregs()对比结果不一致时,
 * 第二个参数pc应指向导致对比结果不一致的指令, 可用于打印提示信息.
 */
// 但是第二个参数不是指针...
// 哦， void difftest_step(vaddr_t pc, vaddr_t npc)函数已经保证了pc，，，

bool isa_difftest_checkregs(CPU_state *ref_r,
                            vaddr_t pc) { // 用于检查寄存器的值是否正确
  // printf("%d\n",cpu.pc);
  // cpu.gpr[32],cpu.pc
  // assert(cpu.pc==pc);
  // isa_reg_display();
  bool flag = true;
  char name[4] = {0};
  // printf("%p",regs);
  for (int i = 0; i < 32; i++) {
    if (ref_r->gpr[i] != cpu.gpr[i]) {
      flag = false;
      printf("reg[%d]:%s is different\n\tref(spike):0x%x\n\tdut( nemu):0x%x\n", i,
             get_regname_by_num(i, name), ref_r->gpr[i], cpu.gpr[i]);
    }
  }
  return flag;
}

void isa_difftest_attach() {}
