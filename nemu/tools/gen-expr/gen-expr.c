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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format = "#include <stdio.h>\n"
                           "int main() { "
                           "  unsigned result = %s; "
                           "  printf(\"%%u\", result); "
                           "  return 0; "
                           "}";
static int buflen = 0;
static unsigned int gen(const char *s);
static int check_buf_len(int add) { return add + buflen < 65536; }
static unsigned int choose(int n) { return rand() % n; }
static unsigned int gen_num() {

  int result = rand() % 100000;
  int tmp = result;
  int len = 1;
  while (tmp / 10 != 0) {
    len++;
    tmp /= 10;
  }
  if (!check_buf_len(len + 1)) {
    return 0;
  } else {
    sprintf(buf + buflen, "%d", result);
    buflen += len;
    buf[buflen] = '\0';
    return len;
  }
}
static unsigned int gen_rand_op() {
  switch (choose(4)) {
  case 0:
    return gen("+");
  case 1:
    return gen("-");
  case 2:
    return gen("*");
  default:
    return gen("/");
  }
}
static unsigned int gen_rand_space() {
  // return 0;
  int len = choose(3);
  if (!check_buf_len(len + 1)) {
    return 0;
  } else {
    for (int i = 0; i < len; i++) {
      buf[buflen + i] = ' ';
    }
    buflen += len;
    buf[buflen] = '\0';
    return len;
  }
}
static unsigned int gen(const char *s) {
  int len = strlen(s);
  if (!check_buf_len(len + 1)) {
    return 0;
  } else {
    strcpy(buf + buflen, s);
    buflen += len;
  }
  return len;
}
static unsigned int gen_rand_expr() {
  int len = 0;
  int old_buflen = buflen;
  switch (choose(3)) {
  case 0:
    // std::cout << "0" << std::endl;
    len += gen_rand_space();
    len += gen_num();
    break;
  case 1:
    // std::cout << "1" << std::endl;
    len += gen_rand_space();
    len += 1;
    gen("(");
    len += gen_rand_space();
    len += gen_rand_expr();
    len += gen_rand_space();
    len += 1;
    gen(")");
    len += gen_rand_space();
    break;
  default:
    // std::cout << "2" << std::endl;
    len += gen_rand_space();
    len += gen_rand_expr();
    len += 1;
    len += gen_rand_space();
    gen_rand_op();
    len += gen_rand_expr();
    len += gen_rand_space();
    break;
  }
  buflen = old_buflen;
  if (!check_buf_len(len + 1)) {
    buf[buflen] = '\0';
    return 0;
  } else {
    buflen += len;
    buf[buflen] = '\0';
    return len;
  }
}
int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i++) {
    buflen = 0;
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -O2 -Wall -Werror -o /tmp/.expr ");
    // printf("%d", ret);
    if (ret != 0) {
      i--;
      continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);
    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);
    unlink("/tmp/.expr");

    printf("%u %s\n", result, buf);
  }
  return 0;
}
