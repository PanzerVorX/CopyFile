#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <dirent.h>
#include <errno.h>
#include <utime.h>

#define BUFSIZE 512

int checkfiletype(char * );
void copyfile(char *,char *);
void copydir(char * ,char * );
void setattribute(char * ,char *);

int main(int argc,char * * argv){
	
	if(argc!=3){
		printf("parameter is error\n");
		exit(-1);
	}
	else if(checkfiletype(argv[1])!=2){//当源文件为文件
		copyfile(argv[1],argv[2]);
	}
	else{//当源文件为目录
		copydir(argv[1],argv[2]);		
	}
}

int checkfiletype(char * name){
	
	struct stat buf;
	lstat(name,&buf);
	switch(buf.st_mode&S_IFMT){
		case S_IFREG:
			return 1;
		case S_IFDIR:
			return 2;
		case S_IFLNK:
			return 3;
		case S_IFCHR:
			return 4;
		case S_IFBLK:
			return 5;
		case S_IFSOCK:
			return 6;
		case S_IFIFO:
			return 7;
		default:
			return 0;
	}
}

void copyfile(char * from,char * to){
	int fromfd=-1,tofd=-1;
	char buf[BUFSIZE];
	int num;
	char totalname[255];//目标位置路径
	char * temp; 
	char dest [255];
	int len;
	int tempbuf[255];
	
	int i=checkfiletype(to);
	if(checkfiletype(to)!=2){//当目标位置为文件
		
		strcpy(totalname,to);
	}
	else{//当目标位置为目录
			
		//完善目录路径尾部的/符号，便于后面进行拼接子文件
		strcpy(totalname,to);
		len=strlen(to);
		if(to[len-1]!='/'){
			strcat(totalname,"/");
		}
		
		temp=strrchr(from,'/');//获取字符串中指定字符最后地址：char *strrchr(const char *str, char c) （关联头文件：string.h）
		if(temp==NULL){
			strcat(totalname,from);
		}
		else{
			int n=temp-from;// 末个/所处元素位置
			//通过strrchr() strncpy()可实参考指定字符截取字符串
			strncpy(dest, temp+1,strlen(from)-n-1);//截取字符串：char *strncpy(char *dest, const char *src, size_t n) 把 src 所指向的字符串的前n个字符复制到 dest（关联头文件：string.h）
			strcat(totalname,dest);
		}
	}
	
	if(checkfiletype(from)==3){
			
		readlink(from,tempbuf,sizeof(tempbuf));
		symlink(tempbuf,totalname);
		setattribute(from,totalname);
	}
	else{
	
		if((fromfd=open(from,O_RDONLY))==-1){
			perror("open from");
			exit(-1);
		}
		if((tofd=open(totalname,O_WRONLY|O_TRUNC|O_CREAT,644))==-1){
			perror("open to");
			exit(-1);
		}
		//读取
		while((num=read(fromfd,buf,BUFSIZE))>0){
			if(write(tofd,buf,num)!=num){
				printf("write %s error\n",to);
			}
		}
		close(fromfd);
		close(tofd);
		setattribute(from,totalname);
	}

	return;
}

void copydir(char * from,char * to){
	
	DIR * d1;
	struct  dirent * dent1 ;
	char fromtotalname[255];//源目录路径
	char tototalname[255];//目标位置路径
	char frombuf[255];
	char tobuf[255];
	struct stat statbuf;
	int len;

	if(checkfiletype(to)!=2){//当目标位置不为目录
		perror("to is not dir");
		exit(-1);
	}
	
	d1=opendir(from);
	if(d1==NULL){
		perror("opendir");
		exit(0);
	}
	
	//完善目录路径尾部的/符号，便于后面进行拼接子文件
	strcpy(tototalname,to);
	len=strlen(to);
	if(to[len-1]!='/'){
		strcat(tototalname,"/");
	}
	
	strcpy(fromtotalname,from);
	len=strlen(from);
	if(from[len-1]!='/'){
		strcat(fromtotalname,"/");
	}
	
	while((dent1=readdir(d1))!=NULL){
		if((dent1->d_name[0])!='.'){
			
			//拼接路径
			strcpy(frombuf,fromtotalname);
			strcat(frombuf,dent1->d_name);
			
			strcpy(tobuf,tototalname);
			strcat(tobuf,dent1->d_name);
			
			if(checkfiletype(frombuf)!=2){
				copyfile(frombuf,tobuf);
			}
			else{
				 mkdir(tobuf,644);
				 setattribute(frombuf,tobuf);
				 copydir(frombuf,tobuf);//递归
			}
		}
	}
	return;
}

void setattribute(char * from ,char * to){
	
	struct stat buf;
	lstat(from,&buf);
	lchown(to,buf.st_uid,buf.st_gid);//属主/组
	
	struct utimbuf temptime;
	temptime.actime=buf.st_atime;
	temptime.modtime=buf.st_mtime;
	int x=utime(to,&temptime);//时间
	chmod(to,(buf.st_mode)&(~S_IFMT));//权限

	return;
}