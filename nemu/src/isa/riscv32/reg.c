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

#include "local-include/reg.h"
#include <isa.h>
//spike: (disasm/regnames.ecc)
/*
const char* xpr_name[] = {
  "zero", "ra", "sp",  "gp",  "tp", "t0",  "t1",  "t2",
  "s0",   "s1", "a0",  "a1",  "a2", "a3",  "a4",  "a5",
  "a6",   "a7", "s2",  "s3",  "s4", "s5",  "s6",  "s7",
  "s8",   "s9", "s10", "s11", "t3", "t4",  "t5",  "t6"
};
*/
const char *regs[] = {"$0", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
                      "s0", "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                      "a6", "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
                      "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
//


// DONE 打印寄存器
void isa_reg_display() {
  // static const int rows = 8;   // 行
  static const int colume = 4; // 列
  for (int i = 0; i < (sizeof regs / sizeof regs[0]); i++) {
    bool flag = false;
    word_t result = isa_reg_str2val(regs[i], &flag);
    if (flag) {
      printf("%s: %08x\t", regs[i], result);
    } else {
      printf("%s:   ERROR \t", regs[i]);
    }
    if (!((i + 1) % colume)) {
      printf("\r\n");
    }
  }
  // Log(str(MUXDEF(CONFIG_RVE, 16, 32))); 宏展开->32
  // Log(str(MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo,
  // riscv32_ISADecodeInfo)));
}
// DONE? TODO 全是0应该没问题吧
word_t isa_reg_str2val(const char *s, bool *success) {
  int num = -1;
  for (int i = 0; i < ARRLEN(regs); i++) {
    if (!strcmp(regs[i], s))
      num = i;
  }
  if (num == -1) {
    success && (*success = false);
    return -1;
  }
  success && (*success = true);
  Log("%s: %d", s, cpu.gpr[num]);
  return cpu.gpr[num];
  return -1;
}
char* get_regname_by_num(int num, char *name) {
  if (num < 0 || num >=sizeof regs / sizeof regs[0]) {
    name = NULL;
    return NULL;
  }
  strcpy(name, regs[num]);
  return name;
}
