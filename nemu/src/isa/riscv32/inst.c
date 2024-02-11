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

#pragma GCC diagnostic ignored "-Wunused-function"
#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/ifetch.h>
#include <stdio.h>
#include <string.h>
// #include 
#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write
enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_R, TYPE_B, TYPE_J,
  TYPE_N, // none
};
extern char* elf_file;
typedef struct  {
    word_t pc;
    word_t size;
    char name[256];//TODO chao guo 256?
}ElfFunTab;
ElfFunTab* func_tab;
static int elffuntab_nums=0;
int load_func_systab();
static char* get_funcname(word_t dnpc);

typedef enum{
    CALL,
    RET,
} funcbtType;
static struct {
    funcbtType type;
    word_t pc;
    word_t dnpc;
    char* name;
}funcbt[256]={
    {.type=0,.pc=0,.dnpc=0,.name=NULL}
};
static int funcbt_num=0;

static void add_func2bt(word_t pc,word_t dnpc,funcbtType type);
void print_func_bt();

#define src1R() do { *src1 = R(rs1);  } while (0) // 取寄存器rs1的值到src1
#define src2R() do { *src2 = R(rs2); } while (0)
// 从指令中抽出立即数how?
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while (0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while (0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while (0)
#define immB() do { *imm = SEXT((BITS(i, 31, 31) << 12) | (BITS(i, 7, 7) << 11) | (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1),    13); } while (0)
#define immR()  do { *imm = SEXT(BITS(i, 31, 25), 7); } while (0)
// decode_operand中用到了宏BITS和SEXT, 它们均在nemu/include/macro.h中定义,
// 分别用于位抽取和符号扩展
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2,
                           word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd = BITS(i, 11, 7);
  MyLog("reg(rd): %d\n", *rd);
  switch (type) {
  case TYPE_I: src1R(); immI(); break;
  case TYPE_U: immU(); break;
  case TYPE_S: src1R(); src2R(); immS(); break;
  case TYPE_J: src1R(); break;
  case TYPE_B: src1R(); src2R(); immB(); break;
  case TYPE_R: src1R(); src2R();immR(); break;
  }
}
// src1 src2是已经取出来的数,rd不是,需要R(rd)赋值

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;//zhu yi zhe li shi unsigned int
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)                   \
  {                                                                            \
    decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type));           \
    __VA_ARGS__;                                                               \
  }
// #define SEXT(x,n) (x&((x<<(32-1))&(1<<31)))
// TODO 默认寄存器x1?没找着啊
  // INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
  // li->lui+/addi
  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 +imm, 1, src2));
  //dummy
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(rd) = src1 + SEXT(imm, 12));
  // INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, JAL(s));
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, do{
    uint32_t i = s->isa.inst.val;
    R(rd) = s->pc + 4;
    int d_20 = BITS(i, 31, 31);
    int d_10_1 = BITS(i, 30, 21);
    int d_11 = BITS(i, 20, 20);
    int d_19_12 = BITS(i, 19, 12);
    s->dnpc = s->pc + SEXT((d_20 << 20) | (d_19_12 << 12) | (d_11 << 11)|(d_10_1) << 1,21);
    add_func2bt(s->pc,s->dnpc,CALL);
      // if(is_ret(s->dnpc)){
      //   add_func2bt(s->pc,s->dnpc,RET);
      // }else{
      //   add_func2bt(s->pc,s->dnpc,CALL);
      // }
  }while(0));
  // INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, JALR(s));
  // 00008067
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, do{
    uint32_t i = s->isa.inst.val;                                              
    int t = s->pc + 4;
    int offset = SEXT(BITS(s->isa.inst.val, 31, 20), 12);
    s->dnpc = (src1 + offset) & ~1;
    R(rd) = t;
      if(i==0x00008067){
      // if(is_ret(s->dnpc)){
        add_func2bt(s->pc,s->dnpc,RET);
      }else{
        add_func2bt(s->pc,s->dnpc,CALL);
      }
    }while(0));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, src2));
  //string
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(rd) = Mr(src1 + SEXT(imm, 12), 4)); // checked (string:0x80000038)
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B,if (src1 != src2) s->dnpc = s->pc+SEXT(imm, 13)); // checked (string:800001c0)bnez->
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, if (src1 == src2) s->dnpc = s->pc+SEXT(imm, 13)); // checked(string:800001d0)
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(rd) = src1 - src2); // 忽略算术溢出
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli, I,R(rd) = src1 >>(imm&0b11111)); // checked 80000068
  INSTPAT("0000001 ????? ????? 101 ????? 00100 11", srli, I, assert(0)); //checked 80000068
 //-------------------------------PASS------------------------- 
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R(rd) = ((uint32_t)src1 < (uint32_t)imm)); //checked(string:8000004c) seqz-> 
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(rd) = src1 + src2);
  //
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + SEXT(imm, 12), 1, src2);); // 低位字节?
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R,R(rd)=((uint32_t)src1<(uint32_t)src2));
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli, I,R(rd)=src1<<src2);//对于 RV32I，仅当 shamt[5]=0时，指令才是有效的。 
  INSTPAT("0000001 ????? ????? 001 ????? 00100 11", slli, I,assert(0));
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I,R(rd)=src1&imm);
                                                                         
  // ----------------------------PASS(string)--------------------------
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R,R(rd)=src1^src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R,R(rd)=src1|src2);
  // -----------------------PASS(add-long-long..)-------------------------
  // -----------------------PASS(add)-------------------------
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, src2&0xffff));
  //1100011
  // INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if (src1 >= src2) s->dnpc = s->pc+SEXT(imm, 13)); // checked(string:800001d0)
 //01000000 00110101 11010111 10010011
  INSTPAT("010000 0 ????? ????? 101 ????? 00100 11", srai, I,R(rd)=((int32_t)src1)>>(imm&0b11111));
  INSTPAT("010000 1 ????? ????? 101 ????? 00100 11", srai, I,assert(0));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R,R(rd)=src1<<(src2&0b11111));
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", or, R,R(rd)=src1&src2);
  INSTPAT("???????????? ????? 100 ????? 0010011", xori, I,R(rd)=src1^imm);
  // -----------------------PASS(bits)-------------------------
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, if ((int32_t)src1 >= (int32_t)src2) s->dnpc = s->pc+SEXT(imm, 13)); 
  // -----------------------PASS(bubblesort)-------------------------
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, 
printf("imm: %x\n",imm);
R(rd) =imm); // mdU lei zhi ling zen me yi kai shi jiu zuo yi guo le 
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if (src1 >= src2) s->dnpc = s->pc+SEXT(imm, 13)); 

  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc)); 
  INSTPAT_END();
  /**
   *{ const void ** __instpat_end = &&__instpat_end_;
   *do {
   *  uint64_t key, mask, shift;
   *  pattern_decode("??????? ????? ????? ??? ????? 00101 11", 38, &key, &mask,
   *&shift); if ((((uint64_t)s->isa.inst.val >> shift) & mask) == key) {
   *    {
   *      decode_operand(s, &rd, &src1, &src2, &imm, TYPE_U);
   *      R(rd) = s->pc + imm; // 指令执行
   *    }
   *    goto *(__instpat_end);
   *  }
   *} while (0);
   * //...
   *__instpat_end_: ; }
   * */
  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val =
      inst_fetch(&s->snpc, 4); // 调用inst_ifetch,访问内存取值,更改s->snpc
  return decode_exec(s);
}

int load_func_systab(){
    if(!elf_file){
        return 0;
    }
    // riscv64-linux-gnu-readelf -a build/add-riscv32-nemu.elf |grep -oP "(?<='.symtab' contains )[0-9]+"
{
    const char* prefix = "riscv64-linux-gnu-readelf -a ";
    const char* suffix = " |grep -oP \"(?<='.symtab' contains )[0-9]+\"";
    char* command = (char*)malloc(sizeof(char)*(strlen(prefix)+strlen(suffix)+strlen(elf_file)+1));
    strcpy(command,prefix);
    strcat(command,elf_file);
    strcat(command,suffix);
    FILE* pipe = popen(command,"r");
    if (!pipe){
        printf("open the pipe to command %s failed \n",command);
        return 0;
    }
    // printf("%s\n",command);
    // fsa
    if(fscanf(pipe,"%d",&elffuntab_nums)<=0){
    printf("readin symtab_nums error\n");
    }
    if(!elffuntab_nums){
        printf("symtab_nums is null , or failed to load symtab_nums\n");
        return 0;
    }
    pclose(pipe);
    free(command);
}

// riscv64-linux-gnu-readelf -a build/add-riscv32-nemu.elf |grep -P -A 36 "(?<='.symtab' contains )[0-9]+" | awk '{print$2 " " $8}'
{
    const char* prefix =" riscv64-linux-gnu-readelf -a ";
    const char* midfix = "|grep -P -A ";
    const char* suffix = " \"(?<='.symtab' contains )[0-9]+\" |grep FUNC| awk '{print$2 \" \" $3\" \"$8}'";
    char buf[128];//TODO chao guo 128
    char* command = (char*)malloc(sizeof(char)*(strlen(prefix)+strlen(suffix)+strlen(midfix)+strlen(elf_file)+129));
    sprintf(buf,"%d",elffuntab_nums+1);//shouhangwuyong
    strcpy(command,prefix);
    strcat(command,elf_file);
    strcat(command,midfix);
    strcat(command,buf);
    strcat(command,suffix);
    printf("%s\n",command);
    FILE* pipe = popen(command,"r");
    if (!pipe){
        printf("open the pipe to command %s failed \n",command);
        return 0;
    }
    // fseek(pipe,0,SEEK_END);
    // int num_bytes = ftell(pipe);
    fseek(pipe,0,SEEK_SET);
    // struct FunTab a;
    func_tab = (ElfFunTab*)malloc(sizeof(ElfFunTab));
    // fgets(buf,sizeof(buf),pipe);
    int i;
    for(i=0;i<elffuntab_nums&&!feof(pipe);i++){
        // memset(buf,0,sizeof(buf));
        if(!fgets(buf,sizeof(buf),pipe)){
            continue;
        }
        sscanf(buf,"%x %u %s",&func_tab[i].pc,&func_tab[i].size,func_tab[i].name);
    }
    elffuntab_nums=i-1;//EOF jizhi hui daozhi duo du yi ci dan shi mei you shi ji zuo yong
}
return elffuntab_nums;
}
static char* get_funcname(word_t dnpc){
    for(int i =0;i<elffuntab_nums;i++){
        if(dnpc>=func_tab[i].pc&&dnpc<=func_tab[i].pc+func_tab[i].size){
            return func_tab[i].name;
        }
    }
    return NULL;
}
//----------------
void print_func_bt(){
    int indent=4;
    printf("sorted by time\n");
    for(int i = 0;i<funcbt_num;i++){
            printf("0x%x",funcbt[i].pc);
            printf("%*s",indent,"");
            printf("%s: %s@0x%x\n",funcbt[i].type==CALL?"call":"ret",funcbt[i].name,funcbt[i].dnpc);
            indent+=funcbt[i].type==CALL?4:-4;
    }
}
static void add_func2bt(word_t pc,word_t dnpc,funcbtType type){
    funcbt[funcbt_num].pc=pc;
    funcbt[funcbt_num].dnpc=dnpc;
    funcbt[funcbt_num].type=type;
    funcbt[funcbt_num].name=get_funcname(dnpc);
    funcbt_num++;
}
