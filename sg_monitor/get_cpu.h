#ifdef __cplusplus
extern "C"{
#endif

#define VMRSS_LINE 15//VMRSS所在行
#define VMVSZ_LINE 12//VMVSZ所在行

typedef struct        //声明一个occupy的结构体
{
    unsigned int user;  //从系统启动开始累计到当前时刻，处于用户态的运行时间，不包含 nice值为负进程。
    unsigned int nice;  //从系统启动开始累计到当前时刻，nice值为负的进程所占用的CPU时间
    unsigned int system;//从系统启动开始累计到当前时刻，处于核心态的运行时间
    unsigned int idle;  //从系统启动开始累计到当前时刻，除IO等待时间以外的其它等待时间iowait (12256) 从系统启动开始累计到当前时刻，IO等待时间(since 2.5.41)
}total_cpu_occupy_t;

int get_total_mem();//获取系统总内存

extern unsigned int get_cpu_total_occupy();//获取总的CPU时间
extern int get_phy_mem(const pid_t p);//获取占用物理内存
extern int get_vsz_mem(const pid_t p);//获取占用物理内存
		        
#ifdef __cplusplus
}
#endif
