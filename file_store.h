/* 
 * File:   file_store.h
 * Author: Ronak
 *
 * Created on April 23, 2016, 3:44 AM
 */

#ifndef FILE_STORE_H
#define FILE_STORE_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

#define OSP4_DEBUG 0

struct fileSystem{
	char *filePath;
	struct filesStruct *head;
	logger lgr;
};

struct filesStruct{
	int isfile;
	char *name;
	char *content;
	mode_t mode;
	struct filesStruct *subdir;
	struct filesStruct *next;
};

int parseandload(struct filesStruct *parent,struct filesStruct *child,char *loaddir,int level);
void load_filesystem();
int storeDir(struct filesStruct *parent,struct filesStruct *dirList, int level, int fd,int offset);
char *substring(char *string, int position, int length);
int match(char *a, char *b);

struct fileSystem *ramfiles = NULL;
char *ImagePath;
signed long int totalMemory;
int saveFile = -1;
int writeMemory = 0;

int match(char *a, char *b)
{
   int position = 0;
   char *x, *y;

   x = a;
   y = b;

   while(*a)
   {
      while(*x==*y)
      {
         x++;
         y++;
         if(*x=='\0'||*y=='\0')
            break;
      }
      if(*y=='\0')
         break;

      a++;
      position++;
      x = a;
      y = b;
   }
   if(*a)
      return position;
   else
      return -1;
}

/* Extract the substring */
char *substring(char *string, int position, int length)
{
    int c;
    char *pointer;
    char s[DEBUG_BUF_SIZE];

    pointer = (char *)malloc(sizeof(char)*(length+1));
    
    if (pointer == NULL)
    {
        //perror("malloc failed");
#if OSP4_DEBUG       
       memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
       sprintf(s, "Substring ,memory allocation fail \n");
       chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
        exit(EXIT_FAILURE);
    }
    
    for (c = 0 ; c < position -1 ; c++){
       string++;
    }
    
    strncpy(pointer,string,length);
    return pointer;
}

int storeDir(struct filesStruct *parent,struct filesStruct *dirList, int level, int fd,int offset){

	char dir[10],dirStruct[10000];
	int written = 0;
	if(level == 1)
		sprintf(dir,"%s","#ROOT#");
	else
		sprintf(dir,"%s%s%s","#dir",parent->name,"#");
	written = pwrite(fd,dir,strlen(dir),offset);
	offset = written + offset;
	/*sprintf(dirStruct,"%s%s",dirStruct,dir);*/
	while(dirList != NULL){
		if(dirList->isfile == 0){
			sprintf(dirStruct,"%s%s%s%d%s%d%s%s%s","#node#",dirList->name,"|",dirList->isfile,"|",(dirList->subdir != NULL)?1:0,"|","NONE","#endnode#");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
		}

		if(dirList->subdir != NULL){
			offset = storeDir(dirList,dirList->subdir,level +1,fd,offset);
		}else if (dirList->isfile){
			/*printf("\tWriting...%s\n",dirList->name);*/
			sprintf(dirStruct,"%s%s%s%d%s%d%s","#node#",dirList->name,"|",dirList->isfile,"|",(dirList->subdir != NULL)?1:0,"|");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
			written = pwrite(fd,dirList->content != NULL ?dirList->content:"NONE",dirList->content != NULL ?strlen(dirList->content):strlen("NONE"),offset);
			offset = written + offset;
			sprintf(dirStruct,"%s","#endnode#");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
			/*printf("\tWrote...%s\n",dirList->name);*/
		}
		dirList = dirList->next;
	}
	if(level == 1)
		sprintf(dir,"%s","#ENDROOT#");
	else
		sprintf(dir,"%s%s%s","#enddir",parent->name,"#");
	written = pwrite(fd,dir,strlen(dir),offset);
	offset = written + offset;
	return offset;
}


int parseandload(struct filesStruct *parent,struct filesStruct *child,char *loaddir,int level){
        char s[DEBUG_BUF_SIZE];
	int start = 0,end = 0;
	int hasChild = -1;
	char *nodeName,*localdir,*contentVal;
	localdir = strdup(loaddir);
	char dirstart[100] = "\0";
	char dirend[100] = "\0";
	char *filePtr,*nodeptr;
        
#if OSP4_DEBUG
        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
        sprintf(s, "Parse and Load called\n");
        chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif

	start = match(localdir,"#node#");
	end = match(localdir,"#endnode#");
	while(start != -1 || end != -1){
		int remLength;
		int subsize = end - (start + strlen("#node#"));
		filePtr = (substring(localdir,start + strlen("#node#")+1,subsize));
		nodeptr = strdup(filePtr);
		struct filesStruct *addNode = (struct filesStruct *)malloc(sizeof(struct filesStruct));
		nodeName = strtok(nodeptr,"|");
		addNode->name = strdup(nodeName);
		addNode->isfile = atoi(strtok(NULL,"|"));
		hasChild = atoi(strtok(NULL,"|"));
		contentVal = strtok(NULL,"|");
		addNode->content = strdup(contentVal);
		addNode->subdir = NULL;
		addNode->next = NULL;
		if((totalMemory - strlen(nodeName) - sizeof(struct filesStruct *)) < 0){
#if OSP4_DEBUG       
                   memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
                   sprintf(s, "Parse and Load ,file system short of memory\n");
                   chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
				return -ENOSPC;
		}
		else{
                    totalMemory = totalMemory - strlen(nodeName) - sizeof(struct filesStruct *);
                }
		struct filesStruct *dirList;
		if(level == 1){
			dirList = ramfiles->head;
			if(dirList == NULL){
				ramfiles->head = addNode;
				dirList = ramfiles->head;
			}else{
				while(dirList->next != NULL)
					dirList = dirList->next;
				dirList->next = addNode;
				dirList = addNode;
			}
		}else{
			/*printf("level = %d\n",level);*/
			dirList = parent->subdir;
			if(dirList == NULL){
				parent->subdir = addNode;
				dirList = parent->subdir;
				/*printf("Node added\n");*/
			}else{
				while(dirList->next != NULL)
					dirList = dirList->next;
				dirList->next = addNode;
				dirList = addNode;
			}
		}
		remLength = strlen(localdir)-(strlen("#endnode#") + subsize);
		localdir = (substring(localdir,end + strlen("#endnode#")+1,remLength));
		
                if(hasChild == 1){
			sprintf(dirstart,"%s%s%s","#dir",nodeName,"#");
			sprintf(dirend,"%s%s%s","#enddir",nodeName,"#");
			start = match(localdir,dirstart);
			end = match(localdir,dirend);
			int subsize = end - (start + strlen(dirstart));
			filePtr = substring(localdir,start + strlen(dirstart)+1,subsize);
			parseandload(dirList,dirList->subdir,filePtr,level+1);
			remLength = strlen(localdir)-(strlen(dirend) + subsize);
			localdir = substring(localdir,end + strlen(dirend)+1,remLength);
		}
		start = match(localdir,"#node#");
		end = match(localdir,"#endnode#");
		free(nodeptr);
	}
	free(localdir);
	return 0;
}



void load_filesystem(){
    char s[DEBUG_BUF_SIZE];
    char *filebuf ;
    int start,end;
    char *filePtr;
    FILE *fp;
    char *filePath = (char*)malloc(sizeof(char)*1000);

    if(ImagePath[0] == '/'){

        strcpy(filePath,ImagePath);

    }else{/* if path is relative to current working dir */
	strcpy(filePath,getcwd(s,sizeof(char)*512));
	strcat(filePath,"/");
        strcat(filePath,ImagePath);
    }

    ImagePath = filePath;

    fp=fopen(ImagePath, "r");

    if (fp != NULL) {
        if (fseek(fp, 0L, SEEK_END) == 0) {
            long bufsize = ftell(fp);
            
            if (bufsize == -1) { 
#if OSP4_DEBUG       
           memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
    	   sprintf(s, "load_filesystem ,error on buffsize - path:%s\n", ImagePath);
           chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif        
	               //printf("Error on buffsize\n");
            }
            
            filebuf = malloc(sizeof(char) * (bufsize + 1));
            if (fseek(fp, 0L, SEEK_SET) != 0) { 
#if OSP4_DEBUG       
           memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
    	   sprintf(s, "load_filesystem ,error on seek - path:%s\n", ImagePath);
           chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif        
	               //printf("Error on seek\n");
            }
            
            size_t newLen = fread(filebuf, sizeof(char), bufsize, fp);
            if (newLen == 0) {
#if OSP4_DEBUG       
           memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
    	   sprintf(s, "load_filesystem ,This is new file system - path:%s\n", ImagePath);
           chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif        
	   //                fputs("Error reading file\n", stderr);
            } else {
                filebuf[++newLen] = '\0';
            }

	} 
        fclose(fp);

        start = match(filebuf,"#ROOT#");
        end = match(filebuf,"#ENDROOT#");

        int subsize = end - (start + strlen("#ROOT#"));
        //printf("\nstart=%d\tend=%d\tsubsize=%d\n",start,end,subsize);
        struct filesStruct *temp = ramfiles->head;
        if(subsize > 0){
                filePtr = strdup(substring(filebuf,start + strlen("#ROOT#")+1,subsize));

	        //struct filesStruct *temp = ramfiles->head;
	        parseandload(temp,temp,filePtr,1);

        	free(filePtr);
        }
        //printf("Parser returned\n");
    }else{
#if OSP4_DEBUG       
        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "load_filesystem ,file not loaded path:%s\n", ImagePath);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif        
	
    
    }

}




#endif /* FILE_STORE_H */

