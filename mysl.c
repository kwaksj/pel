#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <term.h>
#define PATH_LENGTH 	200
#define FLAG_ALL 	0x01
#define FLAG_INODE 	0x02
#define FLAG_LONG 	0x04
#define FLAG_RECUR	0x08
#define FILEINFO 	struct file_info
#define FILENAME_SIZE	256

struct file_info{	
	char filename[FILENAME_SIZE+1];
	unsigned long inode;
	unsigned long mode;
	unsigned long nlink;
	unsigned long uid;
	unsigned long gid;
	unsigned long size;	
	time_t atime;  	
	unsigned long blocks;
	struct file_info *next;
};

void seek_dir(char *dir,int opt);
void print_long(FILEINFO *list_head,int opt);
void save_long(FILEINFO **list_head,struct stat *cur_stat,struct dirent *cur_dir,int opt);	
void print_normal(FILEINFO *list_head,int opt);	
void save_normal(FILEINFO **list_head,struct stat *cur_stat,struct dirent *cur_dir,int opt);
void free_list(FILEINFO *list_head);
void sort_list(FILEINFO *list_head);	
int long_len(unsigned long num);	
int main(int argc,char* argv[])
{
	int i,j;
	int opt=0;		
	char path[PATH_LENGTH]="./"; 

	for(i=1;i<argc;i++)
	{
		for(j=1;argv[i][j] != '\0';j++)		
		{
			if(argv[i][0] != '-')	
			{
				strncpy(path,argv[i],PATH_LENGTH-1);	
				break;
			}
			switch(argv[i][j])
			{
				case 'a':		
					opt |= FLAG_ALL;
					break;
				case 'l':
					opt |= FLAG_LONG;
					break;
				case 'i':	
					opt |= FLAG_INODE;
					break;
				case 'R':	
					opt |= FLAG_RECUR;
					break;
			}	
		}	
	}
	
	if(path[strlen(path)-1] != '/')	
	{
		sprintf(path+strlen(path),"/");	
	}

	printf("%c[1;31m",27); 	
	printf("\ndirectory path:");
	seek_dir(path,opt);	
	printf("%c[0m\n",27); 
	return 0;
}

int long_len(unsigned long num)
{	
	int nCount=0;

	while( num%10 || num/10 ){	
		num /=10;
		nCount++;
	}
	
	return nCount;
}

void free_list(FILEINFO *list_head){	
	FILEINFO *tmp_list;

	while(list_head != NULL){    
		tmp_list = list_head;
		list_head = list_head->next;
		free(tmp_list);
	}
}

void sort_list(FILEINFO *list_head){		
	FILEINFO *tmp_list_left;
	FILEINFO *tmp_list_right;
	FILEINFO tmp_list;
	FILEINFO *tmp_listp;
	
	if( list_head == NULL )
		return ;
	if( list_head->next == NULL)
		return ;

	tmp_list_left = list_head;
	tmp_list_right = list_head->next;

	while( tmp_list_left->next != NULL){		
		while(tmp_list_right != NULL){
			if( strcmp(tmp_list_left->filename,tmp_list_right->filename) >0 ){	
				memcpy(&tmp_list , tmp_list_left,sizeof(FILEINFO));
				memcpy(tmp_list_left , tmp_list_right,sizeof(FILEINFO));
				memcpy(tmp_list_right , &tmp_list,sizeof(FILEINFO));
				tmp_listp = tmp_list_left->next;
				tmp_list_left->next = tmp_list_right->next;
				tmp_list_right->next = tmp_listp;	
			}			
			tmp_list_right = tmp_list_right->next;			
		}
		tmp_list_left = tmp_list_left->next;
		tmp_list_right = tmp_list_left->next;	
	}
}

void seek_dir(char *dir,int opt)
{
	DIR *dp;
	
	struct dirent *entry;
	struct stat tmp_stat;
	
	FILEINFO *tmp_list;
	FILEINFO *list_head;
	
	list_head = NULL;
	printf("%c[1;31m",27);	
	printf("%s\n",dir);	

	if((dp = opendir(dir)) == NULL){		
		fprintf(stderr,"directory open error: %s\n",dir);
		return;
	}

	chdir(dir);

	while((entry = readdir(dp)) != NULL){	
		lstat(entry->d_name, &tmp_stat);

		if( opt & FLAG_LONG)
			save_long(&list_head,&tmp_stat,entry,opt);
		else
			save_normal(&list_head,&tmp_stat,entry,opt);
	}
	
	sort_list(list_head);	

	if( opt & FLAG_LONG)	
		print_long(list_head,opt);
	else
		print_normal(list_head,opt);
	
	tmp_list = list_head;

	while( tmp_list != NULL) {
		if(S_ISDIR(tmp_list->mode)){			
			if( strcmp(".",tmp_list->filename) == 0 || strcmp("..",tmp_list->filename) == 0 ){
				tmp_list = tmp_list->next;
				continue;
			}
			if( opt & FLAG_RECUR){          
				printf("%c[1;31m",27);  
				printf("\n\ndirectory path:%s",dir);
				seek_dir(tmp_list->filename,opt);
			}
		}
		tmp_list = tmp_list->next;
	}
	
	chdir("..");
	closedir(dp);
	free_list(list_head);
}

void save_long(FILEINFO **list_head,struct stat *cur_stat,struct dirent *cur_dir,int opt)
{
	FILEINFO *cur_list=(*list_head);

	if( *list_head != NULL)	
		while( cur_list->next != NULL)
			cur_list = cur_list->next;
	if( cur_dir->d_name[0] == '.' )
		if( !(opt & FLAG_ALL) )
			return; 

	if( (*list_head) == NULL){  	
		cur_list = (FILEINFO *)malloc(sizeof(FILEINFO));
		cur_list->next = NULL;
		*list_head = cur_list;
	}else{
		cur_list->next = (FILEINFO *)malloc(sizeof(FILEINFO));
		cur_list = cur_list->next;
		cur_list->next = NULL;
	}

	cur_list->inode = cur_stat->st_ino;	
	cur_list->mode = cur_stat->st_mode;
	strcpy(cur_list->filename ,cur_dir->d_name);
	cur_list->nlink = cur_stat->st_nlink;
	cur_list->uid = cur_stat->st_uid;
	cur_list->gid = cur_stat->st_gid;
	cur_list->size = cur_stat->st_size;
	cur_list->atime = cur_stat->st_atime;  
	cur_list->blocks = cur_stat->st_blocks;
}  

void print_long(FILEINFO *list_head,int opt)
{
	FILEINFO *cur_list;

	unsigned long tmp_perm;
	struct tm *tm_ptr;
	int i;
	
	cur_list = list_head;
	
	while(cur_list != NULL){
		tmp_perm = cur_list->mode;

		if(S_ISREG(cur_list->mode)){	
			if(cur_list->mode & 01001001)
				printf("%c[1;32m",27);
			else
				printf("%c[0m",27);
			printf("REG  ");
		}else if(S_ISDIR(cur_list->mode)){
			printf("%c[1;34m",27);
			printf("DIR  ");
		}else if(S_ISCHR(cur_list->mode)){
			printf("%c[1;37m",27);
			printf("CHR  ");
		}else if(S_ISBLK(cur_list->mode)){
			printf("%c[1;33m",27);
			printf("BLK  ");
		}else if(S_ISFIFO(cur_list->mode)){
			printf("%c[1;37m",27);		
			printf("FIFO ");
		}else if(S_ISLNK(cur_list->mode)){
			printf("%c[1;36m",27);
			printf("LNK  ");
		}else if(S_ISSOCK(cur_list->mode)){
			printf("%c[1;35m",27);
			printf("SOCK ");
		}

		if(opt & FLAG_INODE)
			printf("%u ",(unsigned int)cur_list->inode);	

		for(i=0;i<3;i++)
		{	
			if(tmp_perm & S_IRUSR)
				printf("r");
			else		
				printf("-");
			if(tmp_perm & S_IWUSR)
				printf("w");
			else
				printf("-");
			if(tmp_perm & S_IXUSR)
				printf("x");
			else
				printf("-");
			tmp_perm <<=3;
		}
		
		printf(" %2u",(unsigned int)cur_list->nlink);
		printf(" %5u",(unsigned int)cur_list->uid);
		printf(" %5u",(unsigned int)cur_list->gid);
		printf(" %12u",(unsigned int)cur_list->size);
		tm_ptr = gmtime(&cur_list->atime);
		printf(" %02d/%02d/%2d %02d:%02d",tm_ptr->tm_year%100,tm_ptr->tm_mon,tm_ptr->tm_mday,tm_ptr->tm_hour,tm_ptr->tm_min);
		printf("%5u",(unsigned int)cur_list->blocks);
		printf(" %s",cur_list->filename);
		printf("\n");
		cur_list = cur_list->next;	
	}
}

void save_normal(FILEINFO **list_head,struct stat *cur_stat,struct dirent *cur_dir,int opt)
{
		FILEINFO *cur_list=*list_head;

		if( *list_head != NULL)
			while( cur_list->next != NULL)
				cur_list = cur_list->next;

		if( cur_dir->d_name[0] == '.')
			if( !(opt & FLAG_ALL) )		
				return; 
		
		if( (*list_head) == NULL){	
			cur_list = (FILEINFO *)malloc(sizeof(FILEINFO));
			cur_list->next = NULL;
			*list_head = cur_list;
		}else{
			cur_list->next = (FILEINFO *)malloc(sizeof(FILEINFO));
			cur_list = cur_list->next;
			cur_list->next = NULL;
		}
		
		cur_list->inode	= cur_stat->st_ino;
		cur_list->mode = cur_stat->st_mode;
		strcpy(cur_list->filename ,cur_dir->d_name);
}

void print_normal(FILEINFO *list_head,int opt)
{
	FILEINFO *cur_list;
	
	int i,j;
	int col_length_name[15];		
	int col_length_inode[15];
	int col_num=0;
	int nCount;
	int index;
	int term_x,term_y;	
	
	for(i=0;i<15;i++){
		col_length_name[i] = 0;
		col_length_inode[i] = 0;
	}
	
	setupterm(NULL,fileno(stdout),(int *)0);
	term_y = tigetnum("lines");	
	term_x = tigetnum("cols");
	
	for(j=15;j>0;j--){
		cur_list = list_head;
		index  =0;
		nCount = 0;
		
		while(cur_list != NULL){	
			if( col_length_name[index%j] < strlen(cur_list->filename) )
				col_length_name[index%j] = strlen(cur_list->filename);
																													
			if( opt & FLAG_INODE){
				if( col_length_inode[index%j] < long_len(cur_list->inode) )		
				col_length_inode[index%j] =long_len(cur_list->inode);
			}
																		
			nCount = 0;

			for(i=0;i<j;i++){					
				nCount += col_length_name[i]+2;								

				if( opt & FLAG_INODE)								
					nCount += col_length_inode[i]+1;									
			}
															
			if(term_x < nCount)	
				break;

			nCount = 0;				
			index ++;						
			index = index %j;
			cur_list = cur_list->next;	
		}
		
		if(cur_list == NULL) {	
			col_num = j;
			break;
		}
	}
	
	cur_list = list_head;
	index = 0;

	while(cur_list != NULL){
		if(S_ISREG(cur_list->mode)){
			if(cur_list->mode & 01001001) 
				printf("%c[1;32m",27);
			else
				printf("%c[0m",27);
		}else if(S_ISDIR(cur_list->mode)){
			printf("%c[1;34m",27);											
		}else if(S_ISCHR(cur_list->mode)){										
			printf("%c[1;37m",27);
		}else if(S_ISBLK(cur_list->mode)){
			printf("%c[1;33m",27);						
		}else if(S_ISFIFO(cur_list->mode)){									
			printf("%c[1;37m",27);			
		}else if(S_ISLNK(cur_list->mode)){									
			printf("%c[1;36m",27);									
		}else if(S_ISSOCK(cur_list->mode)){ 
			printf("%c[1;35m",27);		
		}																				
																						
		if( opt & FLAG_INODE )
			printf(" %*u",col_length_inode[index],(unsigned int)cur_list->inode);
		
		printf(" %*s ",col_length_name[index],cur_list->filename);		
		cur_list = cur_list->next;

		if( index == col_num -1)
			printf("\n");
			
		index ++;
		index = index % col_num;	
	}
}

