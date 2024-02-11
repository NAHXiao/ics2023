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

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  uint32_t old_val; // TODO ? 选择什么类型?
  char expr[65536];
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
WP *new_wp(char *expr, uint32_t old_val);
WP *get_wp_head() { return head; }
// void free_wp(WP *wp);
void free_wp(int NO);
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i++) {
    wp_pool[i].NO = i;
    // wp_pool[i].expr = NULL;
    wp_pool[i].old_val = 0;
    for (int j = 0; j < 65536; j++) {
      wp_pool[i].expr[j] = '\0';
    }
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}
WP *new_wp(char *expr, uint32_t old_val) {
  if (!free_)
    assert(0);
  WP *ret = free_;
  {
    // ret->expr = expr;
    ret->old_val = old_val;
    for (int i = 0; i < strlen(expr); i++) {
      ret->expr[i] = expr[i];
    }
    ret->expr[strlen(expr)] = '\0';
  }
  free_ = free_->next;
  WP *head_copy = head;
  if (!head) {
    head = ret;
    ret->next = NULL;
    return ret;
  }
  while (head_copy->next) {
    head_copy = head_copy->next;
  }
  head_copy->next = ret;
  ret->next = NULL;
  return ret;
}
void free_wp(int NO) {
  // if (wp == NULL)
  assert(NO < NR_WP);
  WP *wp = wp_pool + NO;
  WP *pre = NULL, *cur = head;
  while (cur && cur != wp) {
    pre = cur;
    cur = cur->next;
  }
  if (cur == NULL || cur != wp) {
    assert(0);
  }
  if (pre) {
    pre->next = cur->next;
  } else {
    head = head->next;
  }
  cur->next = free_;
  free_ = cur;
}
// <0 all >=0 specific
void print_wp(int NO) {
  WP *head_copy = head;
  if (!head_copy) {
    printf("No watchpoint\n");
    return;
  }
  if (NO < 0) {
    while (head_copy) {
      printf("NO.%02d: %s : %u\n", head_copy->NO, head_copy->expr,
             head_copy->old_val);
      head_copy = head_copy->next;
    }
  } else {
    while (head_copy) {
      if (head_copy->NO == NO) {
        printf("NO.%02d: %s : %u\n", head_copy->NO, head_copy->expr,
               head_copy->old_val);
        return;
      }
      head_copy = head_copy->next;
    }
    printf("No watchpoint %d\n", NO);
  }
}

// return 0 if no change, 1 if changed , -1 if end
static WP *update_to = NULL;
static bool end = false;
int updateall_wp(int *NO, uint32_t *old_val, uint32_t *new_val) {
  if (end) {
    end = false;
    update_to = NULL;
    return -1;
  }
  if (update_to == NULL) {
    update_to = head;
  }
  while (update_to) {
    uint32_t oldval = update_to->old_val;
    bool success = true;
    uint32_t newval = expr(update_to->expr, &success);
    if (!success)
      assert(0);
    if (oldval != newval) {
      if (NO)
        (*NO = update_to->NO);
      if (old_val)
        *old_val = oldval;
      if (new_val)
        *new_val = newval;
      update_to->old_val = newval;
      update_to = update_to->next;
      return 1;
    }
    update_to = update_to->next;
  }
  end = true;
  return 0;
}
