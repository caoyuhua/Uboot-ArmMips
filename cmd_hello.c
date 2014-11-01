#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <lmb.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>  

int do_hello(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	printf("\nHello world\n\n");
}
U_BOOT_CMD(hello, 4, 1, do_hello,
        "test:hello",
        "test:hello"
);
