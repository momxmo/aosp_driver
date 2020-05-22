#define LOG_TAG "HelloStub"

#include <hardware/hardware.h>
#include <hardware/hello.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#define DEVICE_NAME "/dev/hello"
#define MODULE_NAME "Hello"
#define MODULE_AUTHOR "momxmo"

// HAL层位于用户空间，设备驱动程序位于内核空间，用户空间的程序要和内核空间的驱动程序交互，
// 一般就是通过设备文件了；用户空间的程序如果知道这个设备文件名之后，就可以通过打开文件操作open来打开它，
// 获得一个文件描述符fd，有了这个fd后，就可以通过read、write和ioctl函数来调用设备驱动程序的接口；
// 所以关键的是要知道当你们的外设连上去之后，内核中的SPI驱动是为这个外设创建的设备文件名是什么，
// 这个设备文件可能是在SPI驱动程序里面创建的，也有可能是在外面创建的，这个就要研究一下SPI驱动的代码了。

/*设备打开和关闭接口*/
static int hello_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);
static int hello_device_close(struct hw_device_t* device);

/*设备访问接口*/
static int hello_set_val(struct hello_device_t* dev, int val);
static int hello_get_val(struct hello_device_t* dev, int* val);


/* 模块方法表*/
static struct hw_module_methods_t hello_module_methods = {
    open: hello_device_open
};

/*模块实例变量*/
struct hello_module_t HAL_MODULE_INFO_SYM = { 
// 这里，实例变量名必须为HAL_MODULE_INFO_SYM, tag也必须为HARDWARE_MODULE_TAG
// 这是android硬件抽象层规范规定的。
    common: {
        tag: HARDWARE_MODULE_TAG,
			version_major: 1,
			version_minor: 0,
			id:HELLO_HARDWARE_MODULE_ID,
			name: MODULE_NAME,
			author: MODULE_AUTHOR,
			methods: &hello_module_methods,
	}
};

static int hello_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device) {
    struct hello_device_t* dev;
	dev = (struct hello_device_t*)malloc(sizeof(struct hello_device_t));
	ALOGD("Hello Stub: hello_device_open");

    if(!dev) {
        ALOGE("Hello Stub: failed to alloc space");
		return -EFAULT;
	}

// memset是计算机中C/C++语言初始化函数。作用是将某一块内存中的内容全部设置为指定的值，
//这个函数通常为新申请的内存做初始化工作
	memset(dev, 0, sizeof(struct hello_device_t));
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (hw_module_t*)module;
	dev->common.close = hello_device_close;
	dev->set_val = hello_set_val;
	dev->get_val = hello_get_val;
// open是UNIX系统（包括LINUX、Mac等）的系统调用函数，区别于C语言库函数fopen
// https://baike.baidu.com/item/open/13009226#1_1
	if((dev->fd = open(DEVICE_NAME, O_RDWR)) == -1) {
        ALOGE("Hello Stub:failed to open /dev/hello --%s.", strerror(errno));
		free(dev);
		return -EFAULT;
	}

	*device = &(dev->common);
    ALOGI("Hello Stub: open /dev/hello successfully.");

	return 0;

/*
 DEVICE_NAME定义为"/dev/hello"。由于设备文件是在内核驱动里面通过device_create创建的，
而device_create创建的设备文件默认只有root用户可读写，而hello_device_open一般是由上层APP来调用的，
这些APP一般不具有root权限，这时候就导致打开设备文件失败：

      Hello Stub: failed to open /dev/hello -- Permission denied.
解决办法是类似于Linux的udev规则，打开Android源代码工程目录下，进入到system/core/rootdir目录，
里面有一个名为ueventd.rc文件，往里面添加一行:    /dev/hello 0666 root root
————————————————
版权声明：本文为CSDN博主「罗升阳」的原创文章，遵循 CC 4.0 BY-SA 版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/Luoshengyang/java/article/details/6573809
*/
}

static int hello_device_close(struct hw_device_t* device) {
    struct hello_device_t* hello_device = (struct hello_device_t*)device;
	ALOGD("Hello Stub: hello_device_close");

	if(hello_device) {
        close(hello_device->fd);
		ALOGI("Hello Stub:hello_device_close hello_device->fd:%d", hello_device->fd);
		free(hello_device);
	}

	return 0;
}

static int hello_set_val(struct hello_device_t * dev, int val) {
    ALOGI("Hello Stub: hello_set_val set value %d to device.", val);
// write()会把参数buf所指的内存写入count个字节到参数fd所指的文件内。 fd,是open时打开的 
// 例： int fp = open("/home/test.txt", O_RDWR|O_CREAT);
	int result = write(dev->fd, &val, sizeof(val));
	ALOGI("Hello Stub:hello_set_val result:%d", result);
	return 0;
}

static int hello_get_val(struct hello_device_t* dev, int* val) {
	ALOGD("Hello Stub: hello_get_val");

    if(!val) {
        ALOGE("Hello Stub: error val pointer");
		return -EFAULT;
	}
	
	int result = read(dev->fd, val, sizeof(*val));
	ALOGI("Hello Stub:hello_get_val result:%d", result);
	ALOGI("Hello Stub: get value %d from device.", *val);

	return 0;
}
