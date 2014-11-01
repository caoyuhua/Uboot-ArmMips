#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <u-boot/zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <lmb.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>  

extern u32 usb_download_filesize; 
void menu_shell(void);
void main_menu_usage(void);
char awaitkey(void);
int do_menu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(                                                                                        
	menu, CONFIG_SYS_MAXARGS, 0, do_menu,
	"menu - display a menu,make your choose convenient\n",                                     
	"menu - display a menu,make your choose convenient\n"         
	);                                                                                


int do_menu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])                           
{                                                          
	menu_shell();                                                
			                                                             
	return 0;                                                   
}
	
void menu_shell(void)
{
	char c;
	char cmd_buf[200];

	while(1)
	{
		main_menu_usage();
		 c = awaitkey();
		 printf("%c\n", c);     

		switch (c)
		{
			case '1':
			{
				printf("\nDownload the uboot into Nand flash by DNW\n");
				strcpy(cmd_buf, "dnw 0x20000000");
				run_command(cmd_buf, 0);
				
				strcpy(cmd_buf, "nand erase  0 0x80000");
				run_command(cmd_buf, 0);
					
				sprintf(cmd_buf,"nand write 0x20000000 0 %x",usb_download_filesize);
				run_command(cmd_buf, 0);
				break;
			}
				
			case '2':
			{
				printf("\nDownload the kernel into Nand flash by DNW\n");
				strcpy(cmd_buf, "dnw 0x20000000");
				run_command(cmd_buf, 0);
					
				strcpy(cmd_buf, "nand erase  0x100000 0x300000");
				run_command(cmd_buf, 0);
					
				sprintf(cmd_buf,"nand write 0x20000000 0x100000 %x",usb_download_filesize);
				run_command(cmd_buf, 0);
				break;
			}
				
			case '3':
			{
				printf("\nDownload the filesystem into Nand flash by DNW\n");
				strcpy(cmd_buf, "dnw 0x20000000");
				run_command(cmd_buf, 0);

				strcpy(cmd_buf, "nand erase  0x600000 0x12c00000");/*erase 300M*/
				run_command(cmd_buf, 0);
					
				sprintf(cmd_buf,"nand write.yaffs 0x20000000 0x600000 %x",usb_download_filesize);
				run_command(cmd_buf, 0);
				break;
			}
				
			case '4':                                                                                                              
			{                                                                                                                      
		
				printf("\nDownload the program(Webee210_test) into RAM and run it \n");
				strcpy(cmd_buf, "dnw 0x20000000;go 0x20000000");
				run_command(cmd_buf, 0);                                                                                       
				break;                                                                                               
			}
				
			case '5':
			{
			
				printf("\nBoot the linux (YAFFS2)\n");
				strcpy(cmd_buf, "setenv bootargs noinitrd root=/dev/mtdblock2 init=/linuxrc console=ttySAC0,115200 rootfstype=yaffs2 mem=512M");
				run_command(cmd_buf, 0);
				strcpy(cmd_buf, "setenv bootcmd 'nand read 0x20007fc0 0x100000 0x500000;bootm 0x20007fc0'; save");
				run_command(cmd_buf, 0);
				strcpy(cmd_buf, "nand read 0x20007fc0 0x100000 0x500000");
				run_command(cmd_buf, 0);
				strcpy(cmd_buf, "bootm 0x20007fc0");
				run_command(cmd_buf, 0);
				break;
			}  	
				
			case 'e':
			{
				strcpy(cmd_buf, "nand erase.chip ");
				printf("\nAre you want to erase whole nand flash? (y or n)\n");
			 	c = awaitkey();
				printf("%c\n", c);     
				if('y' == c)
				run_command(cmd_buf, 0);
				break;
			}
				
			case 'q':                    
			{                            
			    printf("\n");
			    return;                  
			}   				
		}
	}

}

void main_menu_usage(void)                                         
{                                                    
	                                             
	printf("\n\n");
	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	printf("^                                                                  ^\n");
	printf("^  [1] Download the uboot      into Nand flash by DNW              ^\n");
	printf("^  [2] Download the kernel     into Nand flash by DNW              ^\n");
	printf("^  [3] Download the filesystem into Nand flash by DNW              ^\n");
	printf("^  [4] Download the program(Webee210_test) into RAM and run it     ^\n");
	printf("^  [5] Boot the linux (YAFFS2)                                     ^\n");
	printf("^  [e] Erase entire chip !!  (Be Care)                             ^\n");
	printf("^  [q] Quit                                                        ^\n");
	printf("^                                                                  ^\n");
   	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	printf("\nEnter your selection: ");   	                                                                                                     
}                      
	


/*keyboard scan*/
char awaitkey(void)     
{
                                                              
	int i;
        char c;
                                  
    	while (1) 
	{
                                            
		if (tstc()) /* we got a key press*/
		return getc();
    	}                                                                               
    	
    	return 0;                                               
}

