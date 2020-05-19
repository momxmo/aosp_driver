#ifndef _HELLO_ANDROID_H_
#define _HELLO_ANDROID_H_
#include<linux/cdev.h>
#include<linux/semaphore.h>

#define HELLO_DEVICE_NODE_NAME "hello"
#define HELLO_DEVICE_FILE_NAME "hello"
#define HELLO_DEVICE_PROC_NAME "hello"
#define HELLO_DEVICE_CLASS_NAME "hello"

struct hello_android_dev{
    int val; // 代表寄存器，类型为int
	struct semaphore sem; // 信号量，用于同步操作
	struct cdev dev; // 内嵌的字符设备，这个Linux驱动程序自定义字符设备结构体的标准方法
};




#endif
