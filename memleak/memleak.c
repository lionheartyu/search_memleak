#define _GNU_SOURCE //这个b玩意要放所有头文件最上面
#include <dlfcn.h>
#include<link.h>

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>



int flag = 1;
#if 0
void * nMalloc(size_t size,const char *filename,int line){
    void *p =malloc(size);

    if(flag){
        char buff[128]={0};
        snprintf(buff,128,"./mem/%p.mem",p);
     //  使用 snprintf 将文件路径格式化到 buff 中，
     //文件路径包括内存地址（p），并将该内存地址存储在“./mem/”目录下，以.p作为扩展名
    
        FILE *fp =fopen(buff,"w");
        if(!fp){ 
            free(p);
            return NULL;
        }
        //如果文件成功打开，写入内存地址和分配大小的信息到文件中
        fprintf(fp,"[+]%s:%d,addr:%p,size:%ld\n",filename,line,p,size);
        fflush(fp);//刷新文件内容到磁盘
        fclose(fp);  
    }

    //printf("nMalloc: %p,size: %ld\n",p,size);
    return p;
}

void nFree(void * ptr){
    if(flag){
        char buff[128]={0};
        snprintf(buff,128,"./mem/%p.mem",ptr);
    
        if(unlink(buff)<0){
            printf("double free :%p",ptr);
            return ;
        }
        //unlink 是一个用于删除文件的系统调用。它会删除路径为 buff 的文件，即在 buff 中存储的文件路径
    }

    //printf("nFree:%p,\n",ptr);
    return free(ptr);
}



#define malloc(size)  nMalloc(size,__FILE__,__LINE__)
#define free(ptr)   nFree(ptr)
//针对单文件


#else

//hook
typedef void *(*malloc_t)(size_t size);
malloc_t malloc_f = NULL;

typedef void *(*free_t)(void *ptr);
free_t free_f = NULL;

int malloc_enable = 1;
int free_enable = 1;



void *ConvertToELF(void *addr) {

	Dl_info info;
	struct link_map *link;
	
	dladdr1(addr, &info, (void **)&link, RTLD_DL_LINKMAP);

	return (void *)((size_t)addr - link->l_addr);
}





void * malloc(size_t size){
    void*p=NULL;
    if(malloc_enable){
        malloc_enable=0;
        p=malloc_f(size);

        void  *caller =__builtin_return_address(0);//0返回上一级 //1 返回上一级的上一级
        //gcc自带    

        char buff[128]={0};
        snprintf(buff,128,"./mem/%p.mem",p);
    
        FILE *fp =fopen(buff,"w");
        if(!fp){ 
            free(p);
            return NULL;
        }
        // fprintf(fp,"[+]%p,addr:%p,size:%ld\n",caller,p,size);//适合16.4.0版本的ubuntu
        fprintf(fp,"[+]%p,addr:%p,size:%ld\n",ConvertToELF(caller),p,size);
        fflush(fp); 

        malloc_enable=1;    
    }else{
        p=malloc_f(size);
    }
    return p;
}

//★addr2line工具
//addr2line -f -e ./memlead -a 地址

void free(void*ptr){

    if(free_enable){
        free_enable=0;
        char buff[128]={0};
        snprintf(buff,128,"./mem/%p.mem",ptr);
    
        if(unlink(buff)<0){
            printf("double free :%p",ptr);
            return ;
        }

        free_f(ptr); 
        free_enable=1;
    }else{
        free_f(ptr); 
    } 
    return ;
}

void init_hook(void){
    if(!malloc_f){
        malloc_f=(malloc_t)dlsym(RTLD_NEXT,"malloc");//夺舍malloc
    }
    if(!free_f){
        free_f=(free_t)dlsym(RTLD_NEXT,"free");//夺舍free
    }
}

#endif



int main(){
    init_hook();
#if 1
    void *p1 =malloc(5);
    void *p2 =malloc(10);//定位到这
    void *p3 =malloc(15);
    void *p4 =malloc(10);
    

    free(p1);
    free(p3);
    free(p4);
#else

void *p1 =nMalloc(5);
void *p2 =nMalloc(10);
void *p3 =nMalloc(15);

nFree(p1);
nFree(p3);

#endif
// getchar();会产生一个1024的内存
}