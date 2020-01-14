#include "softwaredisk.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
	init_software_disk();
	char block[512];
	for(int i = 0; i < 512; i++){
		block[i]='\0';
	}
	write_sd_block(block, 0);
	write_sd_block(block, 1);
}

