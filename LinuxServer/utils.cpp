/*
 * @Descripttion: 
 * @Version: 
 * @Author: zsj
 * @Date: 2020-04-20 13:56:44
 * @LastEditors: zsj
 * @LastEditTime: 2020-04-20 14:46:50
 */ 
#include"utils.h"


//编写一个printf_zsj函数,用于打印信息并显示信息所在的位置
int printf_zsj(char * filename,int codeline,const char * format,...){
    
    va_list arg_list;
    va_start(arg_list,format);
    char temp[1024];
    int len = vsnprintf(temp,1024,format,arg_list);
    // std::cout<<"["<<__FILE__<<" : "<<__LINE__<<"] "<<temp;
    printf("[%s : %d]",filename,codeline);
    int ret = printf("%s",temp);
    va_end(arg_list);
    return ret;

}