
#ifndef ANDROID_HELLO_INTERFACE_H
#define ANDROID_HELLO_INTERFACE_H
#include <hardware/hardware.h>

//__BEGIN_DECLS

#if defined(__cplusplus)
extern "C" {
#endif
    /* define module ID */
    #define HELLO_HARDWARE_MODULE_ID "hello"

    /* hardware module struct */
    struct hello_module_t {
	    struct hw_module_t common;
    };

    /* hardware interface struct */
    struct hello_device_t {
        struct hw_device_t common;
	    int fd; // 设备文件描述符，对应我们将要处理的设备文件"/dev/hello"
	    int (*set_val)(struct hello_device_t* dev, int val); // set_val 为HAL对上提供的函数接口
	    int (*get_val)(struct hello_device_t* dev, int* val); // // get_val 为HAL对上提供的函数接口
    };
	
#if defined(__cplusplus)
}
#endif
//__END_DECLS

#endif
