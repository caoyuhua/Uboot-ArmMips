/*
 * (c) Copyright 2004
 * Techware Information Technology, Inc.
 * Ming-Len Wu <minglen_wu@techware.com.tw>
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
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
 *
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)/*程序的入口点是_start；_start为程序运行地址定义在start.s中且其值随程序的运行不断变化的*/
SECTIONS
{
	. = 0x00000000;/*启动设备的前4K代码被拷贝到片内内存，且片内内存地址被映射为0x0000 0000并从0x00000000开始执行，故4k程序的链接地址要定义为0x0000 0000*/

	. = ALIGN(4);
	.text :/*norflash片内可执行(类似内存，但uboot中也将norflash代码拷贝到内存因为norflash片内执行速度慢)，nandflash需将程序拷贝到内存才能执行*/
	{
		arch/arm/cpu/arm920t/start.o	(.text)/*第一部分代码:cpu上电后固化在片内norflash中代码判断启动设备类型(sd启动/NANDFlash/norflash启动)，从启动设备中拷贝4K代码到片内内存*/
		*(.text)/*其他部分代码:第一阶段的代码在片内内存中执行--->norflash启动(cpu_init_cirt中初始化，copy_loop直接读取norflash中数据将board_init_r到bss_start整个uboot映像搬运到片外内存并跳到片外内存执行);nandflash启动（在汇编cpu_init_cirt或board_init_f中初始化，在nand_boot()中搬运boot代码到片外内存*/
	}

	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }/*告诉编译器只读数据段紧紧排列在代码段后面*/

	. = ALIGN(4);
	.data : {/*告诉编译器读写数据段紧排列在只读数据段后面*/
		*(.data)
	}

	. = ALIGN(4);

	. = .;
	__u_boot_cmd_start = .;/*把__u_boot_cmd_start赋值为当前位置，然后紧紧跟着定义u_boot_cmd段并将所有uboot命令放在此段*/
	.u_boot_cmd : { *(.u_boot_cmd) }
	__u_boot_cmd_end = .;

	. = ALIGN(4);

	.rel.dyn : {
		__rel_dyn_start = .;
		*(.rel*)
		__rel_dyn_end = .;
	}

	.dynsym : {
		__dynsym_start = .;
		*(.dynsym)
	}

	_end = .;

	.bss __rel_dyn_start (OVERLAY) : {
		__bss_start = .;
		*(.bss)
		 . = ALIGN(4);
		__bss_end__ = .;
	}

	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
}
