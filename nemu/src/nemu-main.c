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

#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[]) {
  printf("old args\n");
  for (int i = 0; i < argc; i++) {
    printf("%s\n", argv[i]);
  }
  char *override_args = getenv("APARG");
  if (override_args) {
    size_t argssize = 0;
    for (int i = 0; i < argc; i++) {
      argssize += strlen(argv[i]);
    }
    argssize += strlen(override_args);
    int args_max_count = argc + strlen(override_args);

    char *args = (char *)malloc(sizeof(char) * argssize);
    char **newargv = (char **)malloc(sizeof(char *) * args_max_count);
    strcpy(args, argv[0]);
    strcat(args, " ");
    strcat(args, override_args);
    for (int i = 1; i < argc; i++) {
      strcat(args, " ");
      strcat(args, argv[i]);
    }
    char *p = args;
    char *start = NULL;
    int count = 0;
    while (*p) {
      while (*p && *p == ' ')
        p++;
      if (*p) {
        count++;
        start = p;
        while (*p && *p != ' ')
          p++;
        *p = 0;
        p++;
        newargv[count - 1] = start;
      }
    }
    argc = count;
    argv = newargv;
    printf("argc , argv is overrided ,argc: %d\n", argc);
    for (int i = 0; i < argc; i++) {
      printf("argv[%d]=%s\n", i, argv[i]);
    }
  }
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
