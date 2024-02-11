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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <memory/vaddr.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#define TOKEN_NUM 1280
//+ - * / == != () [] && || ! < > <= >=
enum {
  TK_NOTYPE = 256, // 256 避免与+-*/等冲突
  TK_EQ,           // ==


  TK_BITAND,//&
  TK_BITOR,//|
  TK_BITNOT,//~
  TK_AND, // &&
  TK_OR,  //||

  TK_GE, //>=
  TK_NE, // !=
  TK_LE, //<=

  TK_NUM_TEN, // 十进制
  TK_NUM_HEX, // 0x十六进制

  TK_REG, //$.*寄存器

  TK_DEREF, // *解引用
  TK_MINUS, // -负号

  /* TODO: Add more token types */

};
//all ( expect -)
static const int OpAddr[] = {'+',   '-',      '*',      '/',    TK_EQ,
                             TK_NE, TK_DEREF, TK_MINUS, TK_AND, TK_OR,
                            TK_BITAND,TK_BITOR,TK_BITNOT,
                             TK_GE, TK_LE,    '>',      '<'};
// single 
static const int SingleOpArr[] = {TK_MINUS, TK_DEREF,TK_BITNOT};
static const int NumArr[] = {TK_NUM_TEN, TK_NUM_HEX};
static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /* DONE: Add more rules.
     * Pay attention to the precedence level of different rules.//DONE 优先级
     */

    {" +", TK_NOTYPE}, // spaces
    {"\n", TK_NOTYPE}, // spaces (solve the problome(?) when test by using file or the `test` command )
    {"\\+", '+'},      // plus // c字符串\转义,因此"\\"表示正则的'\'
    {"-", '-'},        //
    {"\\*", '*'},      //
    {"/", '/'},        //
    {"0x[0-9a-fA-F]+", TK_NUM_HEX},
    {"[0-9]+", TK_NUM_TEN}, // TK_NUM_TEN
    {"\\$\\$?[a-zA-Z0-9]+", TK_REG},
    {"\\(", '('},      //
    {"\\)", ')'},      //
                       //  {"!", '!'},        //
    {"==", TK_EQ},     // equal
    {"!=", TK_NE},     //
                       
    {"&&", TK_AND},    //
    {"\\|\\|", TK_OR}, //
                       
    {"&", TK_BITAND},    //
    {"\\|", TK_BITOR}, //
    {"~", TK_BITNOT}, //
                      
    {">=", TK_GE},     //
    {"<=", TK_LE},     //
                       
    {">", '>'},        //
    {"<", '<'},        //
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32]; // TODO >32处理
} Token;

static Token tokens[TOKEN_NUM] __attribute__((used)) = {}; // 已被识别
static int nr_token __attribute__((used)) = 0;             // 已被识别长度

static bool make_token(char *e) { // false=>词法分析失败
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (nr_token > TOKEN_NUM)
        assert(0);
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
        case (TK_NOTYPE):
          break;
        default:
          if (substr_len <= 31) {
            tokens[nr_token].type = rules[i].token_type;
            for (int i = 0; i < substr_len; i++) {
              tokens[nr_token].str[i] = *(substr_start + i);
            }
            tokens[nr_token].str[substr_len] = '\0';
            // Log("Add token to tokens : %s", tokens[nr_token].str);
            nr_token++;
          } else {
            printf("token length is too long at position : %d\n%s\n%*.s^\n",
                   position, e, position, "");
            return false;
          }
          // TODO();
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      printf("char: %c\n", e[position]);
      printf("ascii: %d\n", e[position]);
      return false;
    }
  }

  return true;
}
static bool check_parentheses(int p, int q);
static uint32_t eval(int p, int q);

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  Log("length of tokens: %d", nr_token);

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '*' &&
        (i == 0 || tokens[i - 1].type == '+' || tokens[i - 1].type == '-' ||
         tokens[i - 1].type == '*' || tokens[i - 1].type == '/' ||
         tokens[i - 1].type == '(')) {
      tokens[i].type = TK_DEREF;
    }
  }

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '-' &&
        (i == 0 || tokens[i - 1].type == '+' || tokens[i - 1].type == '-' ||
         tokens[i - 1].type == '*' || tokens[i - 1].type == '/' ||
         tokens[i - 1].type == '(')) {
      tokens[i].type = TK_MINUS;
    }
  }
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == TK_REG) {
      if (!strcmp(tokens[i].str, "$pc")) {
        // printf("%u", cpu.pc);
        sprintf(tokens[i].str, "%u", cpu.pc);
      } else {
        bool success = true;
        sprintf(tokens[i].str, "%d",
                isa_reg_str2val(tokens[i].str + 1, &success));
        if (!success)
          assert(0);
      }
      tokens[i].type = TK_NUM_TEN;
    }
  }
  return eval(0, nr_token - 1); // TODO -1?
}
static bool is_op(int optype) {
  for (int i = 0; i < sizeof(OpAddr) / sizeof(int); i++) {
    if (optype == OpAddr[i]) {
      return true;
    }
  }
  return false;
}
static bool is_Sop(int optype) {
  for (int i = 0; i < sizeof(SingleOpArr) / sizeof(int); i++) {
    if (optype == SingleOpArr[i]) {
      return true;
    }
  }
  return false;
}
static bool is_num(int optype) {
  for (int i = 0; i < sizeof(NumArr) / sizeof(int); i++) {
    if (optype == NumArr[i]) {
      return true;
    }
  }
  return false;
}
//[+-][*/]
//
static bool op_cmp(int cmpd, int cmp) {
  int cmpd_preo = 0, cmp_preo = 0;
  for (int i = 0; i < 2; i++) {
    int *op_preo = i == 0 ? &cmpd_preo : &cmp_preo;
    int *op = i == 0 ? &cmpd : &cmp;
    switch (*op) {
    case '+':
    case '-':
      *op_preo = 3;
      break;
    case '*':
    case '/':
      *op_preo = 2;
      break;
    case TK_DEREF:
    case TK_BITNOT:
      *op_preo = 1;
      break;
    case TK_MINUS:
      *op_preo = 0;
      break;
    case '>':
    case '<':
    case TK_EQ:
    case TK_NE:
    case TK_BITAND:
    case TK_BITOR:
    case TK_AND:
    case TK_OR:
    case TK_GE:
    case TK_LE:
      *op_preo = 4;
      break;
    default:
      // return false;
      assert(0);
    }
  }
  // printf(" : %s\n", cmpd_preo <= cmp_preo ? "true" : "false");
  return cmpd_preo <= cmp_preo;
}
// // 溢出判断与printf一致,减法
// static int check_overflow_minus(uint32_t a, uint32_t b) {
//   if (a < b) {
//     return 1;
//   } else {
//     return 0;
//   }
// };
// static bool overflowed = false;
static int _eval(int p, int q);
uint32_t eval(int p, int q) { return _eval(p, q); }
int _eval(int p, int q) {
  if (p > q) {
    Log("Bad expression");
    assert(0);
    // return false;
    /* Bad expression */
  } else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (!is_num(tokens[p].type))
      assert(0);

    if (tokens[p].type == TK_NUM_HEX) {
      return strtol(tokens[p].str, NULL, 16);
    } else if (tokens[p].type == TK_NUM_TEN) {
      return atoi(tokens[p].str);
    } else {
      assert(0);
    }

  } else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  } else {
    int op = 0;
    int op_type = 1e9;
    int left_bracket_num = 0;
    for (int i = p; i <= q; i++) {
      /**
       * 判断括号匹配,并跳过括号中的操作符
       */
      if (tokens[i].type == '(') {
        left_bracket_num++;
        continue;
      }
      if (tokens[i].type == ')') {
        if (left_bracket_num > 0) {
          left_bracket_num--;
        } else {
          assert(0);
        }
        continue;
      }
      if (left_bracket_num > 0) {
        continue;
      }
      /**
       * Select op
       */
      if (is_op(tokens[i].type) &&
          (op_type == 1e9 || op_cmp(op_type, tokens[i].type))) {
        op_type = tokens[i].type;
        op = i;
      }
    }
    int val1 = 0, val2 = 0;
    if (!is_Sop(op_type)) {
      // uint32_t val1 = eval(p, op - 1);
      val1 = eval(p, op - 1);
      // if (overflowed) {
      //   int tmp = val - ((1LL << 32) - 1) - 1; //
      // }
      // uint32_t val2 = eval(op + 1, q);
      // if (overflowed) {
      //   int tmp = val - ((1LL << 32) - 1) - 1; //
      // }
    }
    val2 = eval(op + 1, q);

    switch (op_type) {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      if (val1 % val2) {
        return val1 / val2;
      } else {
        return val1 / val2;
        // Log("val1%val2!=0");
        // assert(0);
      }
    case TK_DEREF:
      return vaddr_read(val2, 4);
    case TK_BITNOT:
      return ~val2;
    case '>':
      return val1 > val2;
    case '<':
      return val1 < val2;
    case TK_EQ:
      return val1 == val2;
    case TK_NE:
      return val1 != val2;
    case TK_BITAND:
      return val1&val2;
    case TK_BITOR:
      return val1|val2;
    case TK_AND:
      return val1 && val2;
    case TK_OR:
      return val1 || val2;
    case TK_GE:
      return val1 >= val2;
    case TK_LE:
      return val1 <= val2;
    default:
      assert(0);
    }
  }
}
// 检查 (()+()+...)类型
static bool check_parentheses(int p, int q) {
  { // 检查括号匹配=>return false
    int start = p + 1, end = q - 1;
    // int stack[32] = {0};
    // int top = -1;
    int left_bracket_num = 0;
    for (int i = start; i <= end; i++) {
      if (tokens[i].type == '(') {
        left_bracket_num++;
      } else if (tokens[i].type == ')') {
        if (left_bracket_num > 0) {
          left_bracket_num--;
        } else {
          return false;
        }
      }
    }
    if (left_bracket_num > 0) {
      return false;
    }
  }
  return tokens[p].type == '(' && tokens[q].type == ')';
}
/*
 #include <cstdint>
#include <cstdio>
int main(int argc, char *argv[]) {
  printf("%u\n", 73158 / 58532 - ((((26918))) -
                                  ((68173 - ((95883)))) / 24545)); // 4294940378
  printf("%u\n", 73158 / 58532 - ((((26918))) -
                                  ((68173 - ((95883)))) / 24545)); // 4294940378
  printf("%u\n",
         73158 / 58532 - ((26918) - ((68173 - 95883)) / 24545)); // 4294940378
  printf("%u\n",
         73158 / 58532 - (26918 - (68173 - 95883) / 24545)); // 4294940378

  { // 每一步都以u解释
    printf("1: \n");
    printf("(68173 - 95883): %u\n", (68173 - 95883)); // 4294939586
    printf("%u\n", -27710);                           // 4294939586
    int tmp = 4294939586LL - ((1LL << 32) - 1) - 1;   //
    // int tmp2 = 4294939586LL - ((1LL << 32) - 1);
    // int tmp3 = 4294939586LL - (1LL << 32);
    // int tmp4 = 4294939586LL - (1LL << 32);
    printf("Forced convert tmp %d\n", tmp);
    // printf("Forced convert tmp %d", tmp2);
    // printf("Forced convert tmp %d", tmp3);
    // printf("Forced convert tmp %d", tmp4);
    printf("%u\n", 73158 / 58532 - (26918 - 4294939586 / 24545));  // 148065
    printf("%ld\n", 73158 / 58532 - (26918 - 4294939586 / 24545)); // 148065
  }
  { // 该步以%d解释
    printf("2: \n");
    printf("(68173 - 95883): %d\n", (68173 - 95883));         // -27710
    printf("%u\n", 73158 / 58532 - (26918 - -27710 / 24545)); // 4294940378
    printf("%u\n", 73158 / 58532 - (26918 + 27710 / 24545));  // 4294940378
  }
  printf("uint32max: %lld\n", (1LL << 32) - 1);
  // printf("4294939586148065 / 24545: %u\n",
  //        4294939586148065 / 24545);                 // 3183568748
  // printf("%u\n", 73158 / 58532 - (26918 - 174982)); // 148065
  // printf("%u\n", 73158 / 58532 - 4294819232);       // 148065
  // printf("%u\n", 1 - 4294819232);                   // 148065
  {
    uint32_t val = -27710;
    int tmp = val - ((1LL << 32) - 1) - 1; //
    printf("%d\n", tmp);
  }
  {
    uint32_t val = 4294939586;
    int val2 = val;
    printf("val2: %d\n", val2);
  }
  return 0;
}
*/
