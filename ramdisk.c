/*
 * FUSE: Filesystem in Userspace
  
 * Author: Ronak
 *
 * Created on April 21, 2016, 1:12 AM*/

#define FUSE_USE_VERSION 26


#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include "logger.h"
#include "file_store.h"


#define OSP4_DEBUG 0
/*
Data Structure to hold the file system
*/
/*
struct fileSystem{
	char *filePath;
	struct filesStruct *head;
};

struct filesStruct{
	int isfile;
	char *name;
	char *content;
	mode_t mode;
	struct filesStruct *subdir;
	struct filesStruct *next;
};
*/

static int ram_getattr(const char *path, struct stat *stbuf);
static int ram_opendir(const char *path, struct fuse_file_info *fi);
static int ram_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi);
static int ram_mkdir(const char *path, mode_t mode);
static int ram_unlink(const char *path);
static int ram_rmdir(const char *path);
static int ram_open(const char *path, struct fuse_file_info *fi);
struct filesStruct *getReadNode(const char *path);
static int ram_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi);
static int ram_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi);
static int ram_create(const char *path , mode_t mode, struct fuse_file_info *fi);
void createFile(const char *path);
void createDir(const char *path);
struct filesStruct *findPath(const char *source,int retPrev);
static int ram_rename(const char *source, const char *dest);
void freeFiles(struct filesStruct *dirList);
void ram_destroy (void *destroy);
static int ram_chmod(const char *path, mode_t mode);
static int ram_chown(const char *path, uid_t uid, gid_t gid);
static int ram_utimens(const char * path, const struct timespec tv[2]);

typedef struct _list_elem_st{
	void * data;  /* data will be ramdisk file */
	struct _list_elem_st * next;
}_list_elem;

typedef struct _list_st{
	int elem_number; /* number of elements in the list */
	_list_elem * l_head;
	_list_elem * l_tail;
}_list;

_list * create_list(){
	_list * l_t = NULL;

	l_t = (_list*)malloc(sizeof(_list));

	if(l_t==NULL){
		exit(1);
	}
	l_t->elem_number = 0;
	l_t->l_head = NULL;
	l_t->l_tail = NULL;

	return l_t;
}


int en_list(_list_elem * e_t, _list * l_t){
	_list * l = NULL;
	_list_elem * le = NULL;

	if(e_t==NULL || l_t==NULL){
		return 1;
	}

    l = l_t;
    le = e_t;
    if(l->elem_number==0){ /* if list empty */
    	l->l_head = l->l_tail = le;
    }else{
    	l->l_tail->next = le;
    	l->l_tail = le;
    }
    l->elem_number++;

    return 0;
}


/* checks if the path exists and assigns permission */
static int ram_getattr(const char *path, struct stat *stbuf)
{
        char s[DEBUG_BUF_SIZE];
	struct fileSystem *tempFiles = NULL;
	struct filesStruct *dirList= NULL;
	/*printf("Getattr filepath %s\n",path);*/
	int response = -ENOENT;

#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_getattr called, path:%s\n", path);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
        
        if( path==NULL ){
#if OSP4_DEBUG
		chat_log_level(ramfiles->lgr, "ram_getattr called, path is NULL\n", LEVEL_ERROR);
#endif
		return -EPERM;
	}
        
        tempFiles = (struct fileSystem *)malloc(sizeof(struct fileSystem));
        dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
        
	tempFiles = ramfiles;
	dirList = tempFiles->head;
        
	int found = -1;
	char *tempPath = strndup(path,strlen(path));
	char *pathPtr;
	if(strcmp(path,"/") == 0){
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		response =0;
	}else{
		pathPtr = strtok(tempPath,"/");
		while(pathPtr != NULL){
			while(dirList!= NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(dirList->isfile){
					stbuf->st_mode = S_IFREG | 0666;
					stbuf->st_nlink = 1;
					/*printf("File Name %s\n",dirList->name);*/
					if(dirList->content != NULL)
						stbuf->st_size = strlen(dirList->content);
				}else{
					stbuf->st_mode = S_IFDIR | 0755;
					stbuf->st_nlink = 2;
				}
				response = 0;
				if(pathPtr != NULL){
					found = -1;
					dirList = dirList->subdir;
					response = -ENOENT;
				}else{
					break;
				}
			}
			else{
				response = -ENOENT;
			}
		}
	}
	/*printf("Returning getattr %d\n",response);*/
	free(tempPath);
	return response;
}

/* Opens a directory*/
static int ram_opendir(const char *path, struct fuse_file_info *fi)
{   
    char s[DEBUG_BUF_SIZE];
    int ret = 0;

#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_opendir called, path:%s, fi:%ld\n", path, fi);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif


	if(path==NULL ){
#if OSP4_DEBUG
		chat_log_level(ramfiles->lgr, "ram_opendir, path is NULL\n", LEVEL_ERROR);
#endif
		return -ENOENT;
	}
    
    if( strcmp(path, "/")==0 ){
        return ret;
    }
    
    if( '/'== (* (path+strlen(path)-1)) ){
        return -ENOENT;
    }
    return 0;
}

/* Directory Listing */
static int ram_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	/*printf("<<r>> readdir called with path %s\n",path);*/
        char s[DEBUG_BUF_SIZE];
	int found =0;
	char *printFile, *dirPath, *pathPtr;
        
	struct fileSystem *tempFiles;
	struct filesStruct *dirList;
        
        tempFiles = (struct fileSystem *)malloc(sizeof(struct fileSystem));
        dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
            
        tempFiles = ramfiles ;
        dirList = tempFiles->head;
        
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_readdir called, path:%s, offset:%d, fi:%ld\n", path, offset, fi);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
        
        if( path==NULL ){
#if OSP4_DEBUG
		chat_log_level(ramfiles->lgr, "ram_readdir, path is NULL\n", LEVEL_ERROR);
#endif
		return -EPERM;
	}
        
	
	dirPath = strndup(path,strlen(path));
	if(strcmp(path,"/") == 0){
		if(dirList == NULL){

		}else{
			while(dirList != NULL){
				printFile = strndup(dirList->name,strlen(dirList->name));
				filler(buf, printFile, NULL, 0);
				free(printFile);
				dirList = dirList->next;
			}
		}
	}else{
		pathPtr = strtok(dirPath,"/");
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				dirList = dirList->subdir;
				if(pathPtr != NULL){
					found = -1;
				}else
					break;
			}
		}
		while(dirList != NULL){
			printFile = strndup(dirList->name,strlen(dirList->name));
			filler(buf, printFile, NULL, 0);
			dirList = dirList->next;
			free(printFile);
		}
	}
	/*printf("\t\tReaddir returned\n");*/
	free(dirPath);
	return 0;
}

/* Create a Directory */
static int ram_mkdir(const char *path, mode_t mode)
{
	/*printf("<<m>> Path in mkdir is %s\n",path);*/
        char s[DEBUG_BUF_SIZE];
	char *dirPath, *pathPtr, *dirName;
	struct filesStruct *parent;
	int i;
        int charcount = 0,  found = -1;
        
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_mkdir called, path:%s\n", path);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
        
        if( path==NULL || (strcmp(path, "/") == 0)){
#if OSP4_DEBUG
		chat_log_level(ramfiles->lgr, "ram_mkdir, path is NULL or invalid\n", LEVEL_ERROR);
#endif
		return -EPERM;
	}
        
	dirPath = strndup(path,strlen(path));
	
        struct filesStruct *dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
        dirList = ramfiles->head;
        
	int dirSize = sizeof(struct filesStruct);
	if((totalMemory - dirSize) < 0){
		//fprintf(stderr,"%s","NO ENOUGH SPACE");
            
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_mkdir , ramdisk(%ld) will be full, cannot write(%d) more\n", totalMemory, dirSize);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
		return -ENOSPC;
		/*exit(-ENOSPC);*/
	}
	struct filesStruct *addDir = (struct filesStruct *)malloc(sizeof(struct filesStruct));
	addDir->isfile = 0;;
	addDir->next = NULL;
	addDir->subdir = NULL;
	addDir->mode = mode;
	for(i=0;i<strlen(dirPath); i++) {
		    if(dirPath[i] == '/') {
		        charcount ++;
		    }
		}
	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		addDir->name = strndup(pathPtr,strlen(pathPtr));
		if(dirList == NULL){
				ramfiles->head = addDir;
			}else{
				while(dirList->next != NULL)
					dirList = dirList->next;
				dirList->next = addDir;
			}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					dirName = pathPtr;
					parent = dirList;
					dirList = dirList->subdir;
				}else
					break;
			}else{
				/*printf("<<m>> Current directory %s\n",parent->name);
				//printf("<<m>> New directory %s\n",dirName);*/
				dirList = parent->subdir;
				addDir->name = strdup(dirName);
				if(dirList == NULL){
					parent->subdir = addDir;
				}else{
					while(dirList->next != NULL)
						dirList = dirList->next;
					dirList->next = addDir;
				}
			}
		}
	}
	
	dirSize = dirSize + strlen(addDir->name);
	totalMemory = totalMemory - dirSize;
        
	free(dirPath);
	return 0;
}

/* To remove temp files create while using vi editor */
static int ram_unlink(const char *path)
{
	/*printf(">>>>>> Unlink was called with %s \n",path);*/
        char s[DEBUG_BUF_SIZE];
	char *dirPath, *pathPtr;
	struct filesStruct *prev, *parent = NULL;
	prev = NULL;
	int i,charcount = 0,  found = -1;
        
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_unlink called, path:%s\n", path);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif


	if( path==NULL || (strcmp(path, "/") == 0)){
#if OSP4_DEBUG
		chat_log_level(ramfiles->lgr, "ram_unlink, path is NULL\n", LEVEL_ERROR);
#endif
		return -EPERM;
	}
        
	dirPath = strndup(path,strlen(path));
	struct filesStruct *dirList = prev = ramfiles->head;
	for(i=0;i<strlen(dirPath); i++) {
		    if(dirPath[i] == '/') {
		        charcount ++;
		    }
		}
	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		while(dirList->next != NULL){
			if(strcmp(dirList->name,pathPtr) == 0 )
				break;
			prev = dirList;
			dirList = dirList->next;
		}
		/*printf("The prev unlink has filename is %s\n",prev->name);
		//printf("The unlink has filename is %s\n",dirList->name);*/
		if(strcmp(prev->name,dirList->name) == 0){
			ramfiles->head = dirList->next;
		}else{
			prev->next = dirList->next;
		}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				prev = dirList;
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					parent = dirList;
					dirList = dirList->subdir;
					prev = dirList;
				}else
					break;
			}
		}
		/*printf("The prev unlink has filename is %s\n",prev->name);
		//printf("The unlink has filename is %s\n",dirList->name);*/
		if(strcmp(prev->name,dirList->name) == 0){
			parent->subdir = dirList->next;
		}else{
			prev->next = dirList->next;
		}
	}
	/*printf("The prev unlink has filename is %s\n",prev->name);
	//printf("The unlink has filename is %s\n",dirList->name);*/
	int contentSize = 0;
	if(dirList->content != NULL){
		contentSize = strlen(dirList->content);
        }
        
	int dirSize = sizeof(struct filesStruct) + strlen(dirList->name)+ contentSize;
	totalMemory = totalMemory + dirSize;

	if(dirList->content != NULL){
		free(dirList->content);
	}
        
        free(dirList->name);
	free(dirList);
	return 0;
}

/* To delete a directory and sum dirctories */
static int ram_rmdir(const char *path){
	/*printf("Remove was called with path %s\n",path);*/
        char s[DEBUG_BUF_SIZE];
	char *dirPath, *pathPtr;
	struct filesStruct *prev, *parent = NULL;
	prev = NULL;
	int i,charcount = 0,  found = -1;

#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_rmdir called, path:%s\n", path);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
        
        if( path==NULL || (strcmp(path, "/") == 0)){
#if OSP4_DEBUG
		chat_log_level(ramfiles->lgr, "ram_rmdir, path is NULL or invalid\n", LEVEL_ERROR);
#endif
		return -EPERM;
	}
        
        
	dirPath = strndup(path,strlen(path));
	struct filesStruct *dirList = prev = ramfiles->head;
	for(i=0;i<strlen(dirPath); i++) {
		    if(dirPath[i] == '/') {
		        charcount ++;
		    }
		}
	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		while(dirList->next != NULL){
			if(strcmp(dirList->name,pathPtr) == 0 )
				break;
			prev = dirList;
			dirList = dirList->next;
		}
		if(dirList->subdir != NULL)
			return -EPERM;
		/*printf("The prev unlink has filename is %s\n",prev->name);
		//printf("The unlink has filename is %s\n",dirList->name);*/
		if(strcmp(prev->name,dirList->name) == 0){
			ramfiles->head = dirList->next;
		}else{
			prev->next = dirList->next;
		}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				prev = dirList;
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					parent = dirList;
					dirList = dirList->subdir;
					prev = dirList;
				}else
					break;
			}
		}
		if(dirList->subdir != NULL)
			return -ENOTEMPTY;

		if(strcmp(prev->name,dirList->name) == 0){
			parent->subdir = dirList->next;
		}else{
			prev->next = dirList->next;
		}
	}
        
	int dirSize = sizeof(struct filesStruct) + strlen(dirList->name);
	totalMemory = totalMemory + dirSize;

	free(dirList->name);
	free(dirList);
	free(dirPath);
	
        return 0;
}


/* When file is opened */
static int ram_open(const char *path, struct fuse_file_info *fi)
{
	/*printf("<<o>> Open is Called with path %s\n",path);*/
    char s[DEBUG_BUF_SIZE];
    char *dirPath, *pathPtr;
    int i,charcount = 0,  found = -1;
    
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "ram_open called, path:%s, fi:%ld, O_RDONLY:%d, O_WRONLY:%d, O_RDWR:%d, O_APPEND:%d, O_TRUNC:%d\n",
			path, fi, fi->flags&O_RDONLY, fi->flags&O_WRONLY, fi->flags&O_RDWR, fi->flags&O_APPEND, fi->flags&O_TRUNC);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
        
        if(path==NULL || strcmp(path, "/")==0 || '/'== (* (path+strlen(path)-1)) ){
#if OSP4_DEBUG
		chat_log_level(ramfiles->lgr, "ram_open, path is NULL, or just /, or ended with /\n", LEVEL_ERROR);
#endif
		return -ENOENT;
	}
        
	
	dirPath = strndup(path,strlen(path));
	
        struct filesStruct *dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
        dirList = ramfiles->head;
	
        for(i=0;i<strlen(dirPath); i++) {
		    if(dirPath[i] == '/') {
		        charcount ++;
		    }
		}

	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		while(dirList->next != NULL){
			if(strcmp(dirList->name,pathPtr) == 0 ){
				found = 1;
				break;
			}
			dirList = dirList->next;
		}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					dirList = dirList->subdir;
				}else
					break;
			}
		}
	}
	free(dirPath);
	/*printf("\t\topen found %d\n",found);*/
	if(found)
		return 0;
	else
		return -ENOENT;
}

struct filesStruct *getReadNode(const char *path){
	char *dirPath, *pathPtr;
	int i,charcount = 0,  found = -1;
	dirPath = strdup(path);
        
	struct filesStruct *dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
        dirList = ramfiles->head;
	
        for(i=0;i<strlen(dirPath); i++) {
			if(dirPath[i] == '/') {
				charcount ++;
			}
		}

	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		while(dirList != NULL){
			if(strcmp(dirList->name,pathPtr) == 0 ){
				found = 1;
				break;
			}
			dirList = dirList->next;
		}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					dirList = dirList->subdir;
				}else
					break;
			}
		}
	}
	free(dirPath);
	return dirList;
}

/* read from a file, bytes can be specified */
static int ram_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
        char s[DEBUG_BUF_SIZE];
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "r_read called, path:%s, size:%d, offset:%d, fi:%ld\n", path, size, offset, fi);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
        
	int i = 0;
	struct filesStruct *dirList= NULL;
	while (dirList == NULL){
		i++;
		dirList = getReadNode(path);
		if(i>20){
			return 0;
		}
	}
	
	if( dirList->content != NULL){
		size_t len = strlen(dirList->content);
		if (offset < len) {
			if (offset + size > len)
				size = len - offset;
			memcpy(buf, dirList->content + offset, size);
			buf[size] = '\0';
		} else
			size = 0;
	}else{
		size = 0;
	}
	
	return size;
}

/* Write to a file */
static int ram_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi){

        char s[DEBUG_BUF_SIZE];
	char *dirPath, *pathPtr;
	int i,charcount = 0,  found = -1;

#if OSP4_DEBUG
        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
        sprintf(s, "ram_write called, path: %s\n",path);
        chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            
        if (path == NULL){
#if OSP4_DEBUG
            memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
            sprintf(s, "ram_write, Path is NULL\n");
            chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            return -1;        
        }
        
	dirPath = strndup(path,strlen(path));
	
        struct filesStruct *dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
        dirList = ramfiles->head;
	
        for(i=0;i<strlen(dirPath); i++) {
		    if(dirPath[i] == '/') {
		        charcount ++;
		    }
		}
	signed int ramSize = 0;

	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		while(dirList->next != NULL){
			if(strcmp(dirList->name,pathPtr) == 0 )
				break;
			dirList = dirList->next;
		}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					dirList = dirList->subdir;
				}else
					break;
			}
		}
	}

	if(dirList->content == NULL){
		signed int len = strlen(buf);
		ramSize = totalMemory - len;
		if(ramSize < 0){
			return -ENOSPC;
		}
		char *temp = malloc(size+1);
		memset(temp,0,size+1);
		strncpy(temp,buf,size);
		temp[size] = '\0';
		dirList->content = temp;
		totalMemory = totalMemory - size;
		writeMemory = writeMemory + size;
	}else {
		 int newSize = offset + size;
		 int len = strlen(dirList->content);

		if(newSize > len){
                    int diff = newSize - len;
                    ramSize = totalMemory - diff;

                    if(ramSize < 0){
#if OSP4_DEBUG
                        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
                        sprintf(s, "ram_write, ramdisk(%ld) will be full, cannot write(%d) more\n",totalMemory,diff);
                        chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
                        return -ENOSPC;
                    }

                    char *temp = malloc(newSize+1);
                    memset(temp,0,newSize+1);
                    strncpy(temp,dirList->content,len);
                    temp[len] = '\0';
                    free(dirList->content);
                    dirList->content = temp;
                    totalMemory = totalMemory + len - (offset+size);
                    writeMemory = writeMemory + offset + size - len;
		}
		int i,j=0;
		for(i=offset;i<(offset+size);i++){
			dirList->content[i] = buf[j];
			j++;
		}
		dirList->content[i] = '\0';
	}
	free(dirPath);
	return size;
}

/* To create a new file */
static int ram_create(const char *path , mode_t mode, struct fuse_file_info *fi){
        char s[DEBUG_BUF_SIZE];
	char *dirPath, *pathPtr, *dirName;

	struct filesStruct *parent;
	int i,charcount = 0,  found = -1;
        
#if OSP4_DEBUG
        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
        sprintf(s, "ram_create called, path: %s\n",path);
        chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            
        if (path == NULL){
#if OSP4_DEBUG
            memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
            sprintf(s, "ram_create, Path is NULL\n");
            chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            return -1;        
        }
        
	dirPath = strdup(path);
	struct filesStruct *dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
        dirList = ramfiles->head;
        
	int dirSize = sizeof(struct filesStruct);
	int diffSize = totalMemory - dirSize;
	if(diffSize < 0){
		//fprintf(stderr,"%s","NO ENOUGH SPACE");
#if OSP4_DEBUG
            memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
            sprintf(s, "ram_create, ramdisk(%ld) will be full, cannot create(%d) more\n",totalMemory,dirSize);
            chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            
            return -ENOSPC;
	}
	struct filesStruct *addFile = (struct filesStruct *)malloc(sizeof(struct filesStruct));
	addFile->isfile = 1;
	addFile->next = NULL;
	addFile->subdir = NULL;
	addFile->content = NULL;
	addFile->mode = mode;
	for(i=0;i<strlen(dirPath); i++) {
		    if(dirPath[i] == '/') {
		        charcount ++;
		    }
		}
	if(charcount == 1){
		pathPtr = strtok(dirPath,"/");
		addFile->name = strdup(pathPtr);
		if(dirList == NULL){
				ramfiles->head = addFile;
			}else{

				while(dirList->next != NULL)
					dirList = dirList->next;
				dirList->next = addFile;
			}
	}else{
		pathPtr = strtok(dirPath,"/");
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					dirName = pathPtr;
					parent = dirList;
					dirList = dirList->subdir;
				}else
					break;
			}else{
				dirList = parent->subdir;
				addFile->name = strdup(dirName);
				if(dirList == NULL){
					parent->subdir = addFile;
				}else{

					while(dirList->next != NULL)
						dirList = dirList->next;
					dirList->next = addFile;
				}
			}
		}
	}
	
	totalMemory = totalMemory - (dirSize + strlen(addFile->name));
	
	free(dirPath);
	return 0;
}

void createFile(const char *path){
        char s[DEBUG_BUF_SIZE];
	char *dirPath, *pathPtr, *dirName;

	struct filesStruct *parent;
	int i,charcount = 0,  found = -1;
        
#if OSP4_DEBUG
        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
        sprintf(s, "Create File called, path: %s\n",path);
        chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            
        if (path == NULL){
#if OSP4_DEBUG
            memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
            sprintf(s, "Create File, Path is NULL\n");
            chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            return;        
        }
        
	dirPath = strdup(path);
	struct filesStruct *dirList = (struct filesStruct *)malloc(sizeof(struct filesStruct));
        dirList = ramfiles->head;
        
	struct filesStruct *addFile = (struct filesStruct *)malloc(sizeof(struct filesStruct));
	addFile->isfile = 1;
	addFile->next = NULL;
	addFile->subdir = NULL;
	addFile->content = NULL;
	for(i=0;i<strlen(dirPath); i++) {
		    if(dirPath[i] == '/') {
		        charcount ++;
		    }
		}
	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		addFile->name = strndup(pathPtr,strlen(pathPtr));
		if(dirList == NULL){
				ramfiles->head = addFile;
			}else{
				while(dirList->next != NULL)
					dirList = dirList->next;
				dirList->next = addFile;
			}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					dirName = pathPtr;
					parent = dirList;
					dirList = dirList->subdir;
				}else
					break;
			}else{
				dirList = parent->subdir;
				addFile->name = strndup(dirName,strlen(dirName));
				if(dirList == NULL){
					parent->subdir = addFile;
				}else{
					while(dirList->next != NULL)
						dirList = dirList->next;
					dirList->next = addFile;
				}
			}
		}
	}
	free(dirPath);
}

void createDir(const char *path){
        char s[DEBUG_BUF_SIZE];
	char *dirPath, *pathPtr, *dirName;
	struct filesStruct *parent;
	int i,charcount = 0,  found = -1;
        
#if OSP4_DEBUG
        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
        sprintf(s, "Create directory called, path: %s\n",path);
        chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            
        if (path == NULL){
#if OSP4_DEBUG
            memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
            sprintf(s, "Create directory, Path is NULL\n");
            chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
            return;        
        }
        
        
	dirPath = strdup(path);
	struct filesStruct *dirList = ramfiles->head;
	struct filesStruct *addDir = (struct filesStruct *)malloc(sizeof(struct filesStruct));
	addDir->isfile = 0;;
	addDir->next = NULL;
	addDir->subdir = NULL;
	for(i=0;dirPath[i]; i++) {
		if(dirPath[i] == '/') {
			charcount ++;
		}
	}
	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		addDir->name = strndup(pathPtr,strlen(pathPtr));
		if(dirList == NULL){
				ramfiles->head = addDir;
			}else{
				while(dirList->next != NULL)
					dirList = dirList->next;
				dirList->next = addDir;
			}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					dirName = pathPtr;
					parent = dirList;
					dirList = dirList->subdir;
				}else
					break;
			}else{
				/*printf("<<m>> Current directory %s\n",parent->name);
				//printf("<<m>> New directory %s\n",dirName);*/
				dirList = parent->subdir;
				addDir->name = strndup(dirName,strlen(dirName));
				if(dirList == NULL){
					parent->subdir = addDir;
				}else{
					/*Root already has files*/
					while(dirList->next != NULL)
						dirList = dirList->next;
					dirList->next = addDir;
				}
			}
		}
	}
	free(dirPath);
}

struct filesStruct *findPath(const char *source,int retPrev){
	char *dirPath, *pathPtr;
	struct filesStruct *prev, *parent = NULL;
	prev = NULL;
	int i,charcount = 0,  found = -1;
	dirPath = strdup(source);
	struct filesStruct *dirList = prev = ramfiles->head;
	for(i=0;dirPath[i]; i++) {
		if(dirPath[i] == '/') {
			charcount ++;
		}
	}
	pathPtr = strtok(dirPath,"/");
	if(charcount == 1){
		while(dirList->next != NULL){
			if(strcmp(dirList->name,pathPtr) == 0 )
				break;
			prev = dirList;
			dirList = dirList->next;
		}
	}else{
		while(pathPtr != NULL){
			while(dirList != NULL){
				if(strcmp(dirList->name,pathPtr) == 0){
					found = 1;
					break;
				}
				prev = dirList;
				dirList = dirList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(found == 1){
				if(pathPtr != NULL){
					found = -1;
					parent = dirList;
					dirList = dirList->subdir;
					prev = dirList;
				}else
					break;
			}
		}
	}
	if(retPrev == 1)
		return prev;
	else if (retPrev == 0)
		return dirList;
	else if (retPrev == 2)
		return parent;
	return NULL;
}

/* Change the file name */
static int ram_rename(const char *source, const char *dest){

	char *dirPath;
	int i,charcount = 0;
        char s[DEBUG_BUF_SIZE];
        
        if (source == NULL || dest == NULL){
#if OSP4_DEBUG
            memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
            sprintf(s, "ram_rename, Source or destination is NULL\n");
            chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
        
#endif
            return -1;
        }
	dirPath = strdup(source);

	struct filesStruct *dirList = findPath(source,0);
	struct filesStruct *prev = findPath(source,1);
	
	for(i=0;dirPath[i]; i++) {
		if(dirPath[i] == '/') {
			charcount ++;
		}
	}
	if(charcount == 1){
		if(strcmp(prev->name,dirList->name) == 0){
			ramfiles->head = dirList->next;
		}else{
			prev->next = dirList->next;
		}
	}else{
		if(strcmp(prev->name,dirList->name) == 0){
			struct filesStruct *parent = findPath(source,2);
			parent->subdir = dirList->next;
		}else{
			prev->next = dirList->next;
		}
	}
	dirList->next = NULL;
	if(dirList->isfile == 0){
		createDir(dest);
	}else{
		createFile(dest);
	}

	struct filesStruct *destDirList = findPath(dest,0);
	struct filesStruct *destPrev = findPath(dest,1);

	if(dirList->isfile == 0){
		if(strcmp(destPrev->name,destDirList->name)==0){
			struct filesStruct *dirparent = findPath(dest,2);
			if(dirparent == NULL){
				ramfiles->head = dirList;
				dirList->name = strdup(destDirList->name);
				dirList->next = destDirList->next;
			}else{
				dirparent->subdir = dirList;
				dirList->name = strdup(destDirList->name);
				dirList->next = destDirList->next;
			}
		}else{
			destPrev->next = dirList;
			dirList->next = destDirList->next;
			dirList->name = strdup(destDirList->name);
		}
	}else{
		destDirList->content = strdup(dirList->content);
	}
	/*printf("Rename returned\n");*/
	return 0;
}

/* Saves the filesystem in memory to a file in hard disk */
/*
int storeDir(struct filesStruct *parent,struct filesStruct *dirList, int level, int fd,int offset){

	char dir[10],dirStruct[10000];
	int written = 0;
	if(level == 1)
		sprintf(dir,"%s","#ROOT#");
	else
		sprintf(dir,"%s%s%s","#dir",parent->name,"#");
	written = pwrite(fd,dir,strlen(dir),offset);
	offset = written + offset;
	//sprintf(dirStruct,"%s%s",dirStruct,dir);
	while(dirList != NULL){
		if(dirList->isfile == 0){
			sprintf(dirStruct,"%s%s%s%d%s%d%s%s%s","#node#",dirList->name,"|",dirList->isfile,"|",(dirList->subdir != NULL)?1:0,"|","NONE","#endnode#");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
		}

		if(dirList->subdir != NULL){
			offset = storeDir(dirList,dirList->subdir,level +1,fd,offset);
		}else if (dirList->isfile){
			//printf("\tWriting...%s\n",dirList->name);
			sprintf(dirStruct,"%s%s%s%d%s%d%s","#node#",dirList->name,"|",dirList->isfile,"|",(dirList->subdir != NULL)?1:0,"|");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
			written = pwrite(fd,dirList->content != NULL ?dirList->content:"NONE",dirList->content != NULL ?strlen(dirList->content):strlen("NONE"),offset);
			offset = written + offset;
			sprintf(dirStruct,"%s","#endnode#");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
			//printf("\tWrote...%s\n",dirList->name);
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

 */
/* Free the memory of files when the files are removed */
void freeFiles(struct filesStruct *dirList){
    char s[DEBUG_BUF_SIZE];
    int dirSize = 0;

#if OSP4_DEBUG
    memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
    sprintf(s, "Free files called\n");
    chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
    
#if OSP4_DEBUG
    if (dirList == NULL){
        memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
        sprintf(s, "Free files, Dirlist is null. Nothing to free\n");
        chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
    }
#endif


    while(dirList != NULL){
        if(dirList->subdir != NULL)
                freeFiles(dirList->subdir);

        if(dirList->isfile == 1){
            int length=0;
            if(dirList->content != NULL)
                    length = strlen(dirList->content);
            else
                    length = 0;
            dirSize = strlen(dirList->name) + length +sizeof(struct filesStruct);
        }
        else{
            dirSize = strlen(dirList->name) + sizeof(struct filesStruct);
        }
        
        totalMemory = totalMemory + dirSize;
        free(dirList->name);
        if(dirList->isfile == 1){
                if(dirList->content != NULL)
                        free(dirList->content);
        }
        free(dirList);
        dirList = dirList->next;
    }
}

/* Remove the path */
void ram_destroy (void *destroy){
    if(saveFile == 1){
        char s[DEBUG_BUF_SIZE];
        FILE  *fptr;
        fptr = fopen(ImagePath,"w");
        fclose(fptr);

        struct filesStruct *temp = ramfiles->head;
        int fd = open(ImagePath,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
#if OSP4_DEBUG
        if(fd == -1){
            memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
            sprintf(s, "ram_destroy, Image path open error:%s\n", ImagePath);
            chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
        }
#endif                
        if(fd !=-1){
            storeDir(temp, temp,1,fd,0);
        }
        close(fd);
    }
#if OSP4_DEBUG       
        destroy_logger(ramfiles->lgr);
#endif        
	struct filesStruct *dirList = ramfiles->head;
	freeFiles(dirList);
}

static int ram_chmod(const char *path, mode_t mode){
	char s[DEBUG_BUF_SIZE];
	int ret=0;
        
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "r_chmod called, path:%s, mode:%d\n", path, mode);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif

    return ret;
}

static int ram_chown(const char *path, uid_t uid, gid_t gid){
	char s[DEBUG_BUF_SIZE];
	int ret=0;
        
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "r_chmod called, path:%s, uid:%d, gid:%d\n", path, uid, gid);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif
    return ret;
}

static int ram_utimens(const char * path, const struct timespec tv[2]){
	char s[DEBUG_BUF_SIZE];
	int ret=0;
        
#if OSP4_DEBUG
	memset(s, 0, sizeof(char)*DEBUG_BUF_SIZE);
	sprintf(s, "r_utimens called, path:%s\n", path);
	chat_log_level(ramfiles->lgr, s, LEVEL_DEBUG);
#endif

    return ret;
}

/* returns the index the char */
/*
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

// Extract the substring 
char *substring(char *string, int position, int length)
{
    int c;
    char *pointer;

    pointer = (char *)malloc(sizeof(char)*(length+1));
    
    if (pointer == NULL)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    
    for (c = 0 ; c < position -1 ; c++){
       string++;
    }
    
    strncpy(pointer,string,length);
    return pointer;
}

*/


/* Load the filesystem in disk to memory */
/*
int parseandload(struct filesStruct *parent,struct filesStruct *child,char *loaddir,int level){

	int start = 0,end = 0;
	int hasChild = -1;
	char *nodeName,*localdir,*contentVal;
	localdir = strdup(loaddir);
	char dirstart[100] = "\0";
	char dirend[100] = "\0";
	char *filePtr,*nodeptr;

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
			//fprintf(stderr,"%s","NO ENOUGH SPACE");
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
			//printf("level = %d\n",level);
			dirList = parent->subdir;
			if(dirList == NULL){
				parent->subdir = addNode;
				dirList = parent->subdir;
				//printf("Node added\n");
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
    char *filebuf ;
    int start,end;
    char *filePtr;
    FILE *fp;
    fp=fopen(ImagePath, "r");

    if (fp != NULL) {
        if (fseek(fp, 0L, SEEK_END) == 0) {
            long bufsize = ftell(fp);
            if (bufsize == -1) { 
                //printf("Error on buffsize\n");
            }
            filebuf = malloc(sizeof(char) * (bufsize + 1));
            if (fseek(fp, 0L, SEEK_SET) != 0) { 
                //printf("Error on seek\n");
            }
            size_t newLen = fread(filebuf, sizeof(char), bufsize, fp);
            if (newLen == 0) {
                    //fputs("Error reading file\n", stderr);
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
    }

}
*/


void usage(){
    printf("USAGE: ./ramdisk <mount point dir> <size of ramdisk(MB)> [<path to filesystem image>]\n");
}
/* Supported Fuse calls */
static struct fuse_operations ramdisk_oper = {
	.getattr	= ram_getattr,
	.readdir	= ram_readdir,
	.mkdir		= ram_mkdir,
	.unlink		= ram_unlink,
	.rmdir		= ram_rmdir,
	.open		= ram_open,
	.read		= ram_read,
	.write		= ram_write,
	.create		= ram_create,
	.opendir 	= ram_opendir,
	.destroy	= ram_destroy,
        .chmod          = ram_chmod,
        .chown          = ram_chown,
        .utimens        = ram_utimens,
};

int main(int argc, char *argv[]){
	char *filebuf ;
	char tempChar='/';
        
        struct fileSystem *temp;
        temp = (struct fileSystem *)malloc(sizeof(struct fileSystem));
	temp->head = NULL;
	temp->filePath = &tempChar;
	ramfiles = temp;

        if(argc == 4 ){
            saveFile = 1;
            ImagePath = argv[3];
            argc -= 2;
        }else if (argc == 3){
            saveFile = 0;
            argc -= 1;
        }else {
            usage();
            exit(-1);
        }
        totalMemory = atoi(argv[2]);

        totalMemory = totalMemory * 1024 * 1024;
        
        if (totalMemory < 0){
#if OSP4_DEBUG        
            printf("Size of file system should be greater than zero\n");
#endif
            exit(1);
        }
        
#if OSP4_DEBUG
	ramfiles->lgr = create_logger(LEVEL_DEBUG, NULL);
#endif
        
        if(saveFile == 1){
            load_filesystem();
        }
        
	/* with debug*/
        /*
	if(debug == 1){
		if(argc > 4 ){
			saveFile = 1;
			mountPoint = argv[4];
		}else{
			saveFile = 0;
		}
		totalMemory = atoi(argv[3]);
		argCount = 3;
	}
*/

	
	/*printf("\tMemorysize=%d\n",totalMemory);*/
        
        
/*
	if(saveFile == 1){
		FILE *fp;
		fp=fopen(mountPoint, "r");

		if (fp != NULL) {
			if (fseek(fp, 0L, SEEK_END) == 0) {
				long bufsize = ftell(fp);
				if (bufsize == -1) { printf("Error on buffsize\n");}
				filebuf = malloc(sizeof(char) * (bufsize + 1));
				if (fseek(fp, 0L, SEEK_SET) != 0) { printf("Error on seek\n");}
				size_t newLen = fread(filebuf, sizeof(char), bufsize, fp);
				if (newLen == 0) {
					//fputs("Error reading file\n", stderr);
				} else {
					filebuf[++newLen] = '\0';
				}
			}
			fclose(fp);

			int start,end;
			char *filePtr;
			start = match(filebuf,"#ROOT#");
			end = match(filebuf,"#ENDROOT#");

			int subsize = end - (start + strlen("#ROOT#"));
			printf("\nstart=%d\tend=%d\tsubsize=%d\n",start,end,subsize);
			struct filesStruct *temp = ramfiles->head;
			if(subsize > 0){
				filePtr = strdup(substring(filebuf,start + strlen("#ROOT#")+1,subsize));
			
			//struct filesStruct *temp = ramfiles->head;
			parseandload(temp,temp,filePtr,1);
			
			free(filePtr);
			}
			//printf("Parser returned\n");
		}
	}
*/
	return fuse_main(argc, argv, &ramdisk_oper, NULL);
}
