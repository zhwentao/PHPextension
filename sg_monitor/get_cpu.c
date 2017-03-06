#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   //头文件
#include <assert.h>
#include "get_cpu.h"

//获取占用物理内存大小rss
int get_phy_mem(const pid_t p)
{
    char file[64] = {0};//文件名
    
    FILE *fd;         //定义文件指针fd
    char line_buff[256] = {0};  //读取行的缓冲区
    sprintf(file,"/proc/%d/status",p);//文件中第11行包含着
    
    fprintf (stderr, "current pid:%d\n", p);                                                                                                  
    fd = fopen (file, "r"); //以R读的方式打开文件再赋给指针fd
    
    				    
    int i;
    char name[32];//存放项目名称
    int vmrss;//存放内存峰值大小
    for (i=0;i<VMRSS_LINE-1;i++)
    {
        fgets (line_buff, sizeof(line_buff), fd);
    }//读到第15行
    fgets (line_buff, sizeof(line_buff), fd);//读取VmRSS这一行的数据,VmRSS在第15行
    sscanf (line_buff, "%s %d", name,&vmrss);
    fprintf (stderr, "====%s：%d====\n", name,vmrss);
    fclose(fd);     //关闭文件fd
    return vmrss;
}

//获取虚拟内存空间大小vsz
extern int get_vsz_mem(const pid_t p)
{
    char file[64] = {0};//文件名
    
    FILE *fd;         //定义文件指针fd
    char line_buff[256] = {0};  //读取行的缓冲区
    sprintf(file,"/proc/%d/status",p);//文件中第11行包含着
    
    fd = fopen(file, "r"); //以R读的方式打开文件再赋给指针fd

    int i;
    char name[32];//存放项目名称
    int vmvsz;//存放内存峰值大小
    for (i=0; i < VMVSZ_LINE-1; i++)
    {
        fgets(line_buff, sizeof(line_buff), fd);
    }//读到第12行
    fgets(line_buff, sizeof(line_buff), fd);//读取VmRSS这一行的数据,VmRSS在第12行
    sscanf(line_buff, "%s %d", name,&vmvsz);
    fclose(fd);     //关闭文件fd
    return vmvsz;
}


int get_total_mem()
{
    char* file = "/proc/meminfo";//文件名
    	      
    FILE *fd;         //定义文件指针fd
    char line_buff[256] = {0};  //读取行的缓冲区                                                                                                
    fd = fopen (file, "r"); //以R读的方式打开文件再赋给指针fd
    
    			   
    int i;
    char name[32];//存放项目名称
    int memtotal;//存放内存峰值大小
    fgets (line_buff, sizeof(line_buff), fd);//读取memtotal这一行的数据,memtotal在第1行
    sscanf (line_buff, "%s %d", name,&memtotal);
    fprintf (stderr, "====%s：%d====\n", name,memtotal);
    fclose(fd);     //关闭文件fd
    return memtotal;
}

unsigned int get_cpu_total_occupy()
{
    FILE *fd;         //定义文件指针fd
    char buff[1024] = {0};  //定义局部变量buff数组为char类型大小为1024
    total_cpu_occupy_t t;
    fd = fopen ("/proc/stat", "r"); //以R读的方式打开stat文件再赋给指针fd
    fgets (buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
    /*下面是将buff的字符串根据参数format后转换为数据的结果存入相应的结构体参数 */
    char name[16];//暂时用来存放字符串
    sscanf (buff, "%s %u %u %u %u", name, &t.user, &t.nice,&t.system, &t.idle);
    
    fclose(fd);     //关闭文件fd
    return ((t.user + t.nice + t.system)*100)/(t.user + t.nice + t.system + t.idle);
}
