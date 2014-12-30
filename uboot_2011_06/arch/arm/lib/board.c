/*
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * To match the U-Boot user interface on ARM platforms to the U-Boot
 * standard (as on PPC platforms), some messages with debug character
 * are removed from the default U-Boot build.
 *
 * Define DEBUG here if you want additional info as shown below
 * printed upon startup:
 *
 * U-Boot code: 00F00000 -> 00F3C774  BSS: -> 00FC3274
 * IRQ Stack: 00ebff7c
 * FIQ Stack: 00ebef7c
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <version.h>
#include <net.h>
#include <serial.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <mmc.h>

#ifdef CONFIG_BITBANGMII
#include <miiphy.h>
#endif

#ifdef CONFIG_DRIVER_SMC91111
#include "../drivers/net/smc91111.h"
#endif
#ifdef CONFIG_DRIVER_LAN91C96
#include "../drivers/net/lan91c96.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;

#ifdef CONFIG_HAS_DATAFLASH
extern int  AT91F_DataflashInit(void);
extern void dataflash_print_info(void);
#endif

#ifdef CONFIG_DRIVER_RTL8019
extern void rtl8019_get_enetaddr (uchar * addr);
#endif

#if defined(CONFIG_HARD_I2C) || \
    defined(CONFIG_SOFT_I2C)
#include <i2c.h>
#endif


/************************************************************************
 * Coloured LED functionality
 ************************************************************************
 * May be supplied by boards if desired
 */
inline void __coloured_LED_init(void) {}
void coloured_LED_init(void)
	__attribute__((weak, alias("__coloured_LED_init")));
inline void __red_LED_on(void) {}
void red_LED_on(void) __attribute__((weak, alias("__red_LED_on")));
inline void __red_LED_off(void) {}
void red_LED_off(void) __attribute__((weak, alias("__red_LED_off")));
inline void __green_LED_on(void) {}
void green_LED_on(void) __attribute__((weak, alias("__green_LED_on")));
inline void __green_LED_off(void) {}
void green_LED_off(void) __attribute__((weak, alias("__green_LED_off")));
inline void __yellow_LED_on(void) {}
void yellow_LED_on(void) __attribute__((weak, alias("__yellow_LED_on")));
inline void __yellow_LED_off(void) {}
void yellow_LED_off(void) __attribute__((weak, alias("__yellow_LED_off")));
inline void __blue_LED_on(void) {}
void blue_LED_on(void) __attribute__((weak, alias("__blue_LED_on")));
inline void __blue_LED_off(void) {}
void blue_LED_off(void) __attribute__((weak, alias("__blue_LED_off")));

/*
 ************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * or dropped completely,
 * but let's get it working (again) first...
 */

#if defined(CONFIG_ARM_DCC) && !defined(CONFIG_BAUDRATE)
#define CONFIG_BAUDRATE 115200
#endif
static int init_baudrate(void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_f("baudrate", tmp, sizeof(tmp));

	gd->baudrate = (i > 0)
			? (int) simple_strtoul(tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}

static int display_banner(void)
{
	printf("\n\n%s\n\n", version_string);
	debug("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
	       _TEXT_BASE,
	       _bss_start_ofs + _TEXT_BASE, _bss_end_ofs + _TEXT_BASE);
#ifdef CONFIG_MODEM_SUPPORT
	debug("Modem Support enabled\n");
#endif
#ifdef CONFIG_USE_IRQ
	debug("IRQ Stack: %08lx\n", IRQ_STACK_START);
	debug("FIQ Stack: %08lx\n", FIQ_STACK_START);
#endif

	return (0);
}

/*
 * WARNING: this code looks "cleaner" than the PowerPC version, but
 * has the disadvantage that you either get nothing, or everything.
 * On PowerPC, you might see "DRAM: " before the system hangs - which
 * gives a simple yet clear indication which part of the
 * initialization if failing.
 */
static int display_dram_config(void)
{
	int i;

#ifdef DEBUG
	puts("RAM Configuration:\n");

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		printf("Bank #%d: %08lx ", i, gd->bd->bi_dram[i].start);
		print_size(gd->bd->bi_dram[i].size, "\n");
	}
#else
	ulong size = 0;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++)
		size += gd->bd->bi_dram[i].size;

	puts("DRAM:  ");
	print_size(size, "\n");
	puts("                                                                              \n\n");
#endif

	return (0);
}

#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
static int init_func_i2c(void)
{
	puts("I2C:   ");
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	puts("ready\n");
	return (0);
}
#endif

#if defined(CONFIG_CMD_PCI) || defined (CONFIG_PCI)
#include <pci.h>
static int arm_pci_init(void)
{
	pci_init();
	return 0;
}
#endif /* CONFIG_CMD_PCI || CONFIG_PCI */

/*
 * Breathe some life into the board...
 *
 * Initialize a serial port as console, and carry out some hardware
 * tests.
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependent #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

int print_cpuinfo(void);

void __dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size =  gd->ram_size;
}
void dram_init_banksize(void)
	__attribute__((weak, alias("__dram_init_banksize")));

init_fnc_t *init_sequence[] = {
#if defined(CONFIG_ARCH_CPU_INIT)
	arch_cpu_init,		/* basic arch cpu dependent setup */
#endif
#if defined(CONFIG_BOARD_EARLY_INIT_F)
	board_early_init_f,
#endif
	timer_init,		/* initialize timer */
#ifdef CONFIG_FSL_ESDHC
	get_clocks,
#endif
	env_init,		/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_banner,		/* say that we are here */
#if defined(CONFIG_DISPLAY_CPUINFO)
	print_cpuinfo,		/* display cpu info (and speed) */
#endif
#if defined(CONFIG_DISPLAY_BOARDINFO)
	checkboard,		/* display board info */
#endif
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
	init_func_i2c,
#endif
	dram_init,		/* configure available RAM banks */
	NULL,
};

void input_value(u8 *str)
{
	if (str)
		strcpy(console_buffer, str);
	else
		console_buffer[0] = '\0';
	while(1)
	{
		if (readline ("==:") > 0)
		{
			strcpy (str, console_buffer);
			break;
		}
		else
			break;
	}
}
int tftp_config(int type, char *argv[])
{
	char *s;
	char default_file[ARGV_LEN], file[ARGV_LEN], devip[ARGV_LEN], srvip[ARGV_LEN], default_ip[ARGV_LEN];

	printf(" Please Input new ones /or Ctrl-C to discard\n");

	memset(default_file, 0, ARGV_LEN);
	memset(file, 0, ARGV_LEN);
	memset(devip, 0, ARGV_LEN);
	memset(srvip, 0, ARGV_LEN);
	memset(default_ip, 0, ARGV_LEN);

	printf("\tInput device IP ");
	s = getenv("ipaddr");
	memcpy(devip, s, strlen(s));
	memcpy(default_ip, s, strlen(s));

	printf("(%s) ", devip);
	input_value(devip);
	setenv("ipaddr", devip);

	printf("\tInput server IP ");
	s = getenv("serverip");
	memcpy(srvip, s, strlen(s));
	memset(default_ip, 0, ARGV_LEN);
	memcpy(default_ip, s, strlen(s));

	printf("(%s) ", srvip);
	input_value(srvip);
	setenv("serverip", srvip);

//	int tmp=sizeof(srvip);
//	srvip[tmp+1] = '\0';
	strncpy(argv[3], srvip, ARGV_LEN);
//	argv[3] = srvip;
	
    printf("..........type=%d\n",type);
	if(type == 1) {
		printf("\tInput Uboot filename ");
		//argv[2] = "uboot.bin";
		strncpy(argv[2], "uboot.bin",ARGV_LEN);
	}
	else if (type == 2) {
		printf("\tInput Linux Kernel filename ");
		//argv[2] = "zImage"; 
		strncpy(argv[2], "zImage", ARGV_LEN);
	}
	else if (type == 3) {
		printf("\tInput Rootfs filename ");
		//argv[2] = "yaffsImg";
		strncpy(argv[2], "yaffsImg", ARGV_LEN);
	}

	s = getenv("bootfile");
	if (s != NULL) {
		memcpy(file, s, strlen(s));
		memcpy(default_file, s, strlen(s));
	}
	printf("(%s) ", file);
	input_value(file);
	if (file == NULL)
		return 1;
	copy_filename (argv[2], file, sizeof(file));
	setenv("bootfile", file);

	return 0;
}
void main_menu_usage(void)                                         
{                                                    
    	printf("\n\n");
              
	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	printf("^                                                                  ^\n");
	printf("^  [1] Download the uboot      into Nand flash by TFTP             ^\n");
	printf("^  [2] Download the kernel     into Nand flash by TFTP             ^\n");
	printf("^  [3] Download the filesystem into Nand flash by TFTP             ^\n");
	printf("^  [4] Boot the linux (YAFFS2)                                     ^\n");
	printf("^  [5] Entr boot command line interface.                           ^\n");
//	printf("^  [e] Erase entire chip !!  (Be Care)                             ^\n");
//	printf("^  [?] Boot the linux (NFS)                                        ^\n");
	printf("^  [q] Quit                                                        ^\n");
	printf("^                                                                  ^\n");
   	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	printf("\nEnter your selection: ");   	                                                                                                     
}     


void board_init_f_nand(ulong bootflag)
{
	while(1);
}

void board_init_f(ulong bootflag)
{
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	gd_t *id;
	ulong addr, addr_sp;

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *) ((CONFIG_SYS_INIT_SP_ADDR) & ~0x07);
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	memset((void *)gd, 0, sizeof(gd_t));

	gd->mon_len = _bss_end_ofs;

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

	debug("monitor len: %08lX\n", gd->mon_len);
	/*
	 * Ram is setup, size stored in gd !!
	 */
	debug("ramsize: %08lX\n", gd->ram_size);
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	/*
	 * Subtract specified amount of memory to hide so that it won't
	 * get "touched" at all by U-Boot. By fixing up gd->ram_size
	 * the Linux kernel should now get passed the now "corrected"
	 * memory size and won't touch it either. This should work
	 * for arch/ppc and arch/powerpc. Only Linux board ports in
	 * arch/powerpc with bootwrapper support, that recalculate the
	 * memory size from the SDRAM controller setup will have to
	 * get fixed.
	 */
	gd->ram_size -= CONFIG_SYS_MEM_TOP_HIDE;
#endif

	addr = CONFIG_SYS_SDRAM_BASE + gd->ram_size;

#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
	/* reserve kernel log buffer */
	addr -= (LOGBUFF_RESERVE);
	debug("Reserving %dk for kernel logbuffer at %08lx\n", LOGBUFF_LEN,
		addr);
#endif
#endif

#ifdef CONFIG_PRAM
	/*
	 * reserve protected RAM
	 */
	i = getenv_r("pram", (char *)tmp, sizeof(tmp));
	reg = (i > 0) ? simple_strtoul((const char *)tmp, NULL, 10) :
		CONFIG_PRAM;
	addr -= (reg << 10);		/* size is in kB */
	debug("Reserving %ldk for protected RAM at %08lx\n", reg, addr);
#endif /* CONFIG_PRAM */

#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	/* reserve TLB table */
	addr -= (4096 * 4);

	/* round down to next 64 kB limit */
	addr &= ~(0x10000 - 1);

	gd->tlb_addr = addr;
	debug("TLB table at: %08lx\n", addr);
#endif

	/* round down to next 4 kB limit */
	addr &= ~(4096 - 1);
	debug("Top of RAM usable for U-Boot at: %08lx\n", addr);

#ifdef CONFIG_LCD
#ifdef CONFIG_FB_ADDR
	gd->fb_base = CONFIG_FB_ADDR;
#else
	/* reserve memory for LCD display (always full pages) */
	addr = lcd_setmem(addr);
	gd->fb_base = addr;
#endif /* CONFIG_FB_ADDR */
#endif /* CONFIG_LCD */

	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	addr -= gd->mon_len;
	addr &= ~(4096 - 1);

	debug("Reserving %ldk for U-Boot at: %08lx\n", gd->mon_len >> 10, addr);

#ifndef CONFIG_SPL_BUILD
	/*
	 * reserve memory for malloc() arena
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);
	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof (bd_t);
	bd = (bd_t *) addr_sp;
	gd->bd = bd;
	debug("Reserving %zu Bytes for Board Info at: %08lx\n",
			sizeof (bd_t), addr_sp);

#ifdef CONFIG_MACH_TYPE
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE; /* board id for Linux */
#endif

	addr_sp -= sizeof (gd_t);
	id = (gd_t *) addr_sp;
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

	/* setup stackpointer for exeptions */
	gd->irq_sp = addr_sp;
#ifdef CONFIG_USE_IRQ
	addr_sp -= (CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ);
	debug("Reserving %zu Bytes for IRQ stack at: %08lx\n",
		CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ, addr_sp);
#endif
	/* leave 3 words for abort-stack    */
	addr_sp -= 12;

	/* 8-byte alignment for ABI compliance */
	addr_sp &= ~0x07;
#else
	addr_sp += 128;	/* leave 32 words for abort-stack   */
	gd->irq_sp = addr_sp;
#endif

	debug("New Stack Pointer is: %08lx\n", addr_sp);

#ifdef CONFIG_POST
	post_bootmode_init();
	post_run(NULL, POST_ROM | post_bootmode_get(0));
#endif

	gd->bd->bi_baudrate = gd->baudrate;
	/* Ram ist board specific, so move it to board code ... */
	dram_init_banksize();
	display_dram_config();	/* and display it */

	gd->relocaddr = addr;
	gd->start_addr_sp = addr_sp;
	gd->reloc_off = addr - _TEXT_BASE;
	debug("relocation Offset is: %08lx\n", gd->reloc_off);
	memcpy(id, (void *)gd, sizeof(gd_t));

	relocate_code(addr_sp, id, addr);
	/* NOTREACHED - relocate_code() does not return */
}

#if !defined(CONFIG_SYS_NO_FLASH)
static char *failed = "*** failed ***\n";
#endif

/*
 ************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r(gd_t *id, ulong dest_addr)
{
	char *s;
	bd_t *bd;
	ulong malloc_start;
#if !defined(CONFIG_SYS_NO_FLASH)
	ulong flash_size;
#endif

	gd = id;
	bd = gd->bd;

	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */

	monitor_flash_len = _end_ofs;

	/* Enable caches */
	enable_caches();

	debug("monitor flash len: %08lX\n", monitor_flash_len);
	board_init();	/* Setup chipselects */

#ifdef CONFIG_SERIAL_MULTI
	serial_initialize();
#endif

	debug("Now running in RAM - U-Boot at: %08lx\n", dest_addr);

#ifdef CONFIG_LOGBUFFER
	logbuff_init_ptrs();
#endif
#ifdef CONFIG_POST
	post_output_backlog();
#endif

	/* The Malloc area is immediately below the monitor copy in DRAM */
	malloc_start = dest_addr - TOTAL_MALLOC_LEN;
	mem_malloc_init (malloc_start, TOTAL_MALLOC_LEN);

#if !defined(CONFIG_SYS_NO_FLASH)
	puts("Flash: ");

	flash_size = flash_init();
	if (flash_size > 0) {
# ifdef CONFIG_SYS_FLASH_CHECKSUM
		print_size(flash_size, "");
		/*
		 * Compute and print flash CRC if flashchecksum is set to 'y'
		 *
		 * NOTE: Maybe we should add some WATCHDOG_RESET()? XXX
		 */
		s = getenv("flashchecksum");
		if (s && (*s == 'y')) {
			printf("  CRC: %08X", crc32(0,
				(const unsigned char *) CONFIG_SYS_FLASH_BASE,
				flash_size));
		}
		putc('\n');
# else	/* !CONFIG_SYS_FLASH_CHECKSUM */
		print_size(flash_size, "\n");
# endif /* CONFIG_SYS_FLASH_CHECKSUM */
	} else {
		puts(failed);
		hang();
	}
#endif


#if defined(CONFIG_CMD_NAND)
	puts("NAND:  ");
	nand_init();//nandflash初始化，定义在/drivers/mtd/nand/nand.c中		
#endif

#if defined(CONFIG_CMD_ONENAND)
	onenand_init();
#endif

#ifdef CONFIG_GENERIC_MMC
       puts("MMC:   ");
       mmc_initialize(bd);
#endif

#ifdef CONFIG_HAS_DATAFLASH
	AT91F_DataflashInit();
	dataflash_print_info();
#endif

	/* initialize environment */
	env_relocate();

#if defined(CONFIG_CMD_PCI) || defined(CONFIG_PCI)
	arm_pci_init();
#endif

	/* IP Address */
	gd->bd->bi_ip_addr = getenv_IPaddr("ipaddr");

	stdio_init();	/* get the devices list going. */

	jumptable_init();

#if defined(CONFIG_API)
	/* Initialize API */
	api_init();
#endif

	console_init_r();	/* fully init console as a device */

#if defined(CONFIG_ARCH_MISC_INIT)
	/* miscellaneous arch dependent initialisations */
	arch_misc_init();
#endif
#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r();
#endif

	 /* set up exceptions */
	interrupt_init();
	/* enable exceptions */
	enable_interrupts();

	/* Perform network card initialisation if necessary */
#if defined(CONFIG_DRIVER_SMC91111) || defined (CONFIG_DRIVER_LAN91C96)
	/* XXX: this needs to be moved to board init */
	if (getenv("ethaddr")) {
		uchar enetaddr[6];
		eth_getenv_enetaddr("ethaddr", enetaddr);
		smc_set_mac_addr(enetaddr);
	}
#endif /* CONFIG_DRIVER_SMC91111 || CONFIG_DRIVER_LAN91C96 */

	/* Initialize from environment */
	s = getenv("loadaddr");
	if (s != NULL)
		load_addr = simple_strtoul(s, NULL, 16);
#if defined(CONFIG_CMD_NET)
	s = getenv("bootfile");
	if (s != NULL)
		copy_filename(BootFile, s, sizeof(BootFile));
#endif

#ifdef BOARD_LATE_INIT
	board_late_init();
#endif

#ifdef CONFIG_BITBANGMII
	bb_miiphy_init();
#endif
#if defined(CONFIG_CMD_NET)
#if defined(CONFIG_NET_MULTI)
	puts("Net:   ");
#endif
	eth_initialize(gd->bd);
	puts("\n ");
	puts("\n");
#if defined(CONFIG_RESET_PHY_R)
	debug("Reset Ethernet PHY\n");
	reset_phy();
#endif
#endif

#ifdef CONFIG_POST
	post_run(NULL, POST_RAM | post_bootmode_get(0));
#endif

#if defined(CONFIG_PRAM) || defined(CONFIG_LOGBUFFER)
	/*
	 * Export available size of memory for Linux,
	 * taking into account the protected RAM at top of memory
	 */
	{
		ulong pram;
		uchar memsz[32];
#ifdef CONFIG_PRAM
		char *s;

		s = getenv("pram");
		if (s != NULL)
			pram = simple_strtoul(s, NULL, 10);
		else
			pram = CONFIG_PRAM;
#else
		pram = 0;
#endif
#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
		/* Also take the logbuffer into account (pram is in kB) */
		pram += (LOGBUFF_LEN + LOGBUFF_OVERHEAD) / 1024;
#endif
#endif
		sprintf((char *)memsz, "%ldk", (gd->ram_size / 1024) - pram);
		setenv("mem", (char *)memsz);
	}
#endif


	int timer1= CONFIG_BOOTDELAY;
	unsigned char BootType='4';
	int my_tmp=0;
	char cmd_buf[200]={0};
	int i =0;
	char *argv[4];	
	//argv[2] = &file_name_space[0];
	//memset(file_name_space,0,ARGV_LEN);
	
	/*config bootdelay via environment parameter: bootdelay */
	{
		char * s;
		s = getenv ("bootdelay");
		timer1 = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;
	}
    main_menu_usage(); 
	while (timer1 > 0) {
		--timer1;
		//delay 100 * 10ms
		for (i=0; i<100; ++i) {
			if ((my_tmp = tstc()) != 0) {	
				timer1 = 0; 
				BootType = getc();
				if ((BootType < '1' || BootType > '5') && (BootType != 'e') && (BootType != 'q'))
					BootType = '4';
				printf("\n\rYou choosed %c\n\n", BootType);
				break;
			}
			udelay (10000);
		}
		printf ("\b\b\b%2d ", timer1);
	}
	
	putc ('\n');

switch(BootType)
{
	case '1':
		printf("\n");
		tftp_config(1,argv);
		sprintf(cmd_buf,"tftpboot 0xc0008000 %s:%s",argv[3],argv[2]);
/*
		strcpy(cmd_buf, "tftpboot 0xc0008000 ");	
		tmp_str = strlen("tftpboot 0xc0008000 ");
		strcpy(&cmd_buf[tmp_str], argv[3]);	
		tmp_str += strlen(argv[3]);
		strcpy(&cmd_buf[tmp_str], ":");
		strcpy(&cmd_buf[tmp_str+1], argv[2]);	
*/		
		printf("..........cmd_buf=%s\n",cmd_buf);
		run_command(cmd_buf, 0);
		printf("..........NetBootFileXferSize=%x\n",NetBootFileXferSize);
		if(NetBootFileXferSize <= 0)
			return;
			
		strcpy(cmd_buf, "nand erase  0 0x100000");
		run_command(cmd_buf, 0);
		
		sprintf(cmd_buf,"nand write 0xc0008000 0 0x100000");
		run_command(cmd_buf, 0);
		do_reset();
		break;
	
	case '2':
		printf("\n");
		tftp_config(2,argv);
		sprintf(cmd_buf,"tftpboot 0xc0008000 %s:%s",argv[3],argv[2]);	
		printf("..........cmd_buf=%s\n",cmd_buf);
		run_command(cmd_buf, 0);
		if(NetBootFileXferSize <= 0)
			return;
		
		strcpy(cmd_buf, "nand erase  0xa00000 0x300000");
		run_command(cmd_buf, 0);
		
		sprintf(cmd_buf,"nand write 0xc0008000 0xa00000 0x300000");
		run_command(cmd_buf, 0);
		do_reset();
		break;

	case '3':
		printf("\n");
		tftp_config(3,argv);
		sprintf(cmd_buf,"tftpboot 0xc0008000 %s:%s",argv[3],argv[2]);	
		printf("..........cmd_buf=%s\n",cmd_buf);
		run_command(cmd_buf, 0);
		if(NetBootFileXferSize <= 0)
			return;
		
		strcpy(cmd_buf, "nand erase  0xd00000 0x6400000");/*erase 100M*/
		run_command(cmd_buf, 0);
		
		sprintf(cmd_buf,"nand write.yaffs 0xc0008000 0xd00000 %x",NetBootFileXferSize);
		run_command(cmd_buf, 0);
		do_reset();
		break;
		
	case '4':
		printf("\n");
		printf("Boot the linux (YAFFS2)\n");
		strcpy(cmd_buf, "setenv bootargs root=/dev/mtdblock4 rootfstype=yaffs2 init=/init console=ttySAC0,115200");
		run_command(cmd_buf, 0);
		strcpy(cmd_buf, "setenv bootcmd nand read 0xc0008000 0xa00000 0x300000;bootm 0xc0008000; save");
		run_command(cmd_buf, 0);
		strcpy(cmd_buf, "nand read 0xc0008000 0xa00000 0x300000");
		run_command(cmd_buf, 0);
		strcpy(cmd_buf, "bootm 0xc0008000");
		run_command(cmd_buf, 0);
		break;

	case '5':
		printf("   \n%c: System Enter Boot Command Line Interface.\n",BootType);
		printf ("\n%s\n", version_string);
		/* main_loop() can return to retry autoboot, if so just run it again. */
		//for (;;) {					
		//	main_loop ();
		//}

	break;	

	case 'q':
	{							 
		printf("\n");
		return; 				 
		break;					 
	}	
	default:
		printf("err:please reset soc...\n");
		break;	
}
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang(void)
{
	puts("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
