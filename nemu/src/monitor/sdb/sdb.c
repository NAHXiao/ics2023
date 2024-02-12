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

#include "sdb.h"
#include <cpu/cpu.h>
#include <isa.h>
// TODO ? 感觉这样不对
#include <memory/paddr.h>

#include <readline/history.h>
#include <readline/readline.h>
#include <string.h>
#include <threads.h>

static int is_batch_mode = false;
// static int is_batch_mode = true;

void init_regex();
void init_wp_pool();
struct watchpoint;
typedef struct watchpoint WP;
WP *get_wp_head();
// void print_wp();
void print_wp(int NO);
WP *new_wp(char *args, uint32_t old_val);
void free_wp(int NO);
/* We use the `readline' library to provide more flexibility to read from stdin.
 */
// DONE 释放上一个line_read,读入newline,计入历史
static char *rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) { return -1; }

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_ph(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);
static int cmd_test(char *args);
static int cmd_bt(char *args);

// DONE 命令列表
static struct {
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "si [n] : 单步执行n条指令 n缺省1", cmd_si},
    {"info", "info <r|w> r打印寄存器,w打印监视点", cmd_info},
    {"x", "x N EXPR 以expr(EXPR)为起始地址打印Nx4字节内存", cmd_x},
    {"p", "p EXPR 表达式求值", cmd_p},
    {"ph", "ph EXPR 表达式求值(HEX输出)", cmd_ph},
    {"w", "w EXPR 监视EXPR,发生变化时暂停程序", cmd_w},
    {"d", "d N 删除序号为N的监视点", cmd_d},
    {"test", "测试expr", cmd_test},
    {"bt", "测试expr", cmd_bt},

    /* TODO->DONE : Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  } else {
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}
// TODO->DONE si [n] : 单步执行n条指令 n缺省1
static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  if (strtok(NULL, " ")) {
    printf("Too many arguments\n");
    return 0;
  }
  if (!arg) {
    cpu_exec(1);
  } else {
    int n = atoi(arg);
    while (n--) {
      cpu_exec(1);
    }
  }
  return 0;
}
// TODO info <r|w> r打印寄存器,w打印监视点
static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (strtok(NULL, " ")) {
    printf("Too many arguments\n");
    return 0;
  } else if (!arg) {
    printf("Usage: info <r|w>\n");
    return 0;
  }
  if (!strcmp(arg, "w")) {
    print_wp(-1);
    return 0;
    // r
  } else if (!strcmp(arg, "r")) {
    isa_reg_display();
    return 0;
  } else {
    // cmd_help("info");
    printf("Usage: info <r|w>\n");
    return 0;
  }
}

// TODO x N EXPR 以expr(EXPR)为起始地址打印Nx4字节内存
// risc32/64从0x80000000开始
static int cmd_x(char *args) {
  // TODO 假定EXPR为16进制数
  int N = -1;
  char *Nstr = strtok(NULL, " ");
  paddr_t addr = -1;
  char *EXPRstr = strtok(NULL, "");
  if (!Nstr || !EXPRstr) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  sscanf(Nstr, "%d", &N);
  // char * arg
  // sscanf(EXPRstr, "0x%x", &addr);
  bool success = true;
  addr = expr(EXPRstr, &success);
  if (!success) {
    printf("Invalid EXPR\n");
    return 0;
  }
  if (N == -1 || addr == -1) {
    printf("Invalid Arguments\n");
    return 0;
  } else {
    for (int i = 0; i < N; i++) {
      printf("0x%08x: 0x%08x\n", addr, paddr_read(addr, 4));
      addr += 4;
    }
  }
  return 0;
}
// TODO p EXPR 表达式求值
static int cmd_p(char *args) {
  char *arg = strtok(NULL, "");
  if (arg) {
    int size = strlen(arg)+1;
    char *argbak = (char *)malloc(sizeof(char) * size);
    // strcpy(argbak, arg);
    strncpy(argbak, arg, sizeof(char) * size );
    bool success = true;
    uint32_t result = expr(argbak, &success);
    if (success) {
      Log("result : %u", result);
      printf("%u\n", result);
    } else {
      printf("Invalid EXPR\n");
    }
    free(argbak);
  }
  // Log("TODO");
  return 0;
}
static int cmd_ph(char *args) {
  char *arg = strtok(NULL, "");
  if (arg) {
    int size = strlen(arg)+1;
    char *argbak = (char *)malloc(sizeof(char) * size);
    strncpy(argbak, arg, sizeof(char) * size);
    // strcpy(argbak, arg);
    bool success = true;
    uint32_t result = expr(argbak, &success);
    if (success) {
      Log("(hex)result : 0x%08x", result);
      printf("hex: 0x%08x\n", result);
    } else {
      printf("Invalid EXPR\n");
    }
    free(argbak);
  }
  // Log("TODO");
  return 0;
}
// TODO w EXPR 监视EXPR,发生变化时暂停程序
static int cmd_w(char *args) {
#ifdef CONFIG_WATCHPOINT
  char *arg = strtok(NULL, "");
  if (arg) {
    bool success = true;
    uint32_t result = expr(arg, &success);
    if (success) {
      Log("result : %u", result);
      new_wp(arg, result);
      printf("Add watchpoint %s : %u\n", arg, result);
    }
  } else {
    printf("Usage : w EXPR\n");
  }
#else
  printf("Please open \"Enable WATCHPOINT\" to use watchpoint\n");
#endif
  return 0;
}
// TODO d N 删除序号为N的监视点
// static char *argg;
static int cmd_d(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg) {
    int NO = atoi(arg);
    free_wp(NO);
  }
  return 0;
}
static int cmd_test(char *args) {
  FILE *fp =
      fopen("/home/wangsf/workspace/pa/ics2023/nemu/tools/gen-expr/input", "r");
  if (!fp)
    assert(0);
  char buf[1024];
  int line = 1;
  while (fgets(buf, 1024, fp)) {
    uint32_t result = atoi(buf);
    // sprintf(&result, "%d", buf);
    for (int i = 0; i < 1024 && buf[i] != ' '; i++) {
      buf[i] = ' ';
    }
    bool success = true;
    uint32_t ret = expr(buf, &success);
    printf("line %d: %s", line, buf);
    printf("expected result : %u , result by expr fun : %u\n", result, ret);
    assert(result == ret);
    assert(success);
    line++;
  }
  return 0;
}
// TODO 表达式求值
// static int expr2value(char *exprs) { return 0; }

void sdb_set_batch_mode() { is_batch_mode = true; }

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  // 交互模式一般不会NULL,收到EOF退出
  for (char *str; (str = rl_gets()) != NULL;) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " "); // help i w ...

    // Log("Catch cmd : %s", cmd);
    if (cmd == NULL) {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1; //$1...
    if (args >= str_end) {
      args = NULL;
    }
// TODO
#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD) {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
  int ret = load_func_systab();
  if (ret) {
    printf("Loaded %d Func symbols!\n", ret);
  }
}
extern char* elf_file;
static int cmd_bt(char *args) {
  if(!elf_file){
    printf("WARNNING: 未从elf文件加载符号\n");
  }
  print_func_bt();
  return 0;
}
