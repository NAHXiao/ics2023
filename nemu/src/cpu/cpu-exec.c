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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>

// shuchu next i
#include <cpu/ifetch.h>
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10
#define BufNum 20
#define BufLen 256
static struct {
  char msg[BufNum][BufLen];
  unsigned int start;
  unsigned int end;
  unsigned int empty;
} IRingBuf = {
    .start = 0,
    .end = 0,
    .empty = 1,
    .msg = {{0}},
};
void print_Iringbuf();
void add_Iringbuf(const char *msg);

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0; // 执行指令数量,这些全局变量也是只能执行一次的原因
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

static Decode s_next;

void device_update();
int updateall_wp(int *NO, uint32_t *old_val, uint32_t *new_val);

#ifdef CONFIG_WATCHPOINT
static void check_watchpoint() {
  // DONE TODO TEST
  int ret = 0, NO = -1;
  uint32_t old_val = -1, new_val = -1;
  bool changed = false;
  while ((ret = updateall_wp(&NO, &old_val, &new_val)) != -1) {
    if (ret == 1) {
      changed = true;
      printf("Watchpoint %d: %u -> %u\n", NO, old_val, new_val);
    }
  }
  if (changed)
    nemu_state.state = NEMU_STOP;
}
#endif
static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) {
    log_write("this: %s\n", _this->logbuf);
    add_Iringbuf(_this->logbuf);
    log_write("next: %s\n", s_next.logbuf);
  }
#endif
  if (g_print_step) {
    IFDEF(CONFIG_ITRACE, printf("this: "));
    IFDEF(CONFIG_ITRACE, puts(_this->logbuf));
    IFDEF(CONFIG_ITRACE, printf("next: "));
    IFDEF(CONFIG_ITRACE, puts(s_next.logbuf));
  }
  IFDEF(CONFIG_WATCHPOINT, check_watchpoint());
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
}

static void ITrace(Decode *s);
static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc; // static next pc
  isa_exec_once(s);
  cpu.pc = s->dnpc; // dynamic next pc
  ITrace(s);
  { // next inst
    s_next.pc = s->dnpc;
    s_next.snpc = s->dnpc;
    s_next.dnpc = s->dnpc;
    s_next.isa.inst.val = inst_fetch(&s_next.snpc, 4);
    ITrace(&s_next);
  }
}
void ITrace(Decode *s) {
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i--) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0)
    space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;
#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
              MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc),
              (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
  //
}
// DONE
static void execute(uint64_t n) {
  Decode s;
  for (; n > 0; n--) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING)
      break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0)
    Log("simulation frequency = " NUMBERIC_FMT " inst/s",
        g_nr_guest_inst * 1000000 / g_timer);
  else
    Log("Finish running in less than 1 us and can not calculate the simulation "
        "frequency");
}

void assert_fail_msg() {
  ITrace(&s_next);
  add_Iringbuf(s_next.logbuf);
  print_Iringbuf();
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT); // 是否打印每步信息
  switch (nemu_state.state) {
  case NEMU_END:
  case NEMU_ABORT:
    printf("Program execution has ended. To restart the program, exit NEMU and "
           "run again.\n");
    return;
  default:
    nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
  case NEMU_RUNNING:
    nemu_state.state = NEMU_STOP;
    break;

  case NEMU_END:
  case NEMU_ABORT:
    // NOT ABORT && halt_ret!=0 => GOOD
    Log("nemu: %s at pc = " FMT_WORD,
        (nemu_state.state == NEMU_ABORT
             ? ANSI_FMT("ABORT", ANSI_FG_RED)
             : (nemu_state.halt_ret == 0
                    ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN)
                    : ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
        nemu_state.halt_pc);
    // fall through
  case NEMU_QUIT:
    statistic();
  }
}

void print_Iringbuf() {
  if (IRingBuf.empty)
    return;
  int T;
  if (IRingBuf.start == IRingBuf.end) {
    T = BufNum;
  } else {
    T = IRingBuf.end < IRingBuf.start ? IRingBuf.end + BufNum - IRingBuf.start
                                      : IRingBuf.end - IRingBuf.start;
  }
  uint32_t pc;
printf("#####INST TRACE#####\n");
  for (int i = 0; i < T; i++) {
      sscanf(IRingBuf.msg[(IRingBuf.start + i) % BufNum],"0x%x",&pc);
      if(pc==cpu.pc){
          printf("--> ");
      }else{
          printf("    ");
      }
      printf("%s\n", IRingBuf.msg[(IRingBuf.start + i) % BufNum]);
  }
printf("--------------------\n");
}
void add_Iringbuf(const char *msg) {
  if (strlen(msg) >= BufLen)
    assert(0);
  if (IRingBuf.empty || IRingBuf.start != IRingBuf.end) { // empty / +
    IRingBuf.empty = 0;
    strcpy(IRingBuf.msg[IRingBuf.end++ % BufNum], msg);
    IRingBuf.end %= BufNum;
  } else if (IRingBuf.start == IRingBuf.end) { // full
    strcpy(IRingBuf.msg[IRingBuf.end++ % BufNum], msg);
    IRingBuf.start++;
  }
}
