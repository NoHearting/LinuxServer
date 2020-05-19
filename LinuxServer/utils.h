/*
 * @Descripttion: 
 * @Version: 
 * @Author: zsj
 * @Date: 2020-04-20 12:02:21
 * @LastEditors: zsj
 * @LastEditTime: 2020-04-20 15:39:03
 */
#ifndef UTILS_H
#define UTILS_H

#include<stdarg.h>
#include<stdio.h>
#include<iostream>



/**
 * @brief 对printf的一层薄的封装,使之打印信息的时候可以显示处对应的文件和代码行数,便于调试
 * 
 * @param format 输出表达式
 * @return int 打印的字符数
 */ 
int printf_zsj(char * filename,int codeline,const char * format,...);



#endif