#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/slab.h>


#include "hello.h"

/*
在hello目录中增加hello.c文件，这是驱动程序的实现部分。驱动程序的功能主要是向上层提供访问设备的寄存器的值，包括读和写。
这里，提供了三种访问设备寄存器的方法:
一是通过proc文件系统来访问，
二是通过传统的设备文件的方法来访问，
三是通过devfs文件系统来访问。
下面分段描述该驱动程序的实现。
————————————————
版权声明：本文为CSDN博主「罗升阳」的原创文章，遵循 CC 4.0 BY-SA 版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/luoshengyang/java/article/details/6568411
*/


/* 主设备和从设备号变量 */
static int hello_major = 0;
static int hello_minor = 0;

/* 设备类别和设备变量 */
static struct class* hello_class = NULL;
static struct hello_android_dev* hello_dev = NULL;

/* 传统的设备文件操作方法 */
static int hello_open(struct inode* inode, struct file* filp);
static int hello_release(struct inode* inode, struct file* filp);
static ssize_t hello_read(struct file* filp, char __user *buf, size_t cout, loff_t* f_pos);
static ssize_t hello_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos);

/* 设备文件操作方法表 */
static struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .release = hello_release,
    .read = hello_read,
    .write = hello_write,
};

/* 访问设置属性方法*/
static ssize_t hello_val_show(struct device* dev, struct device_attribute* attr, char* buf);
static ssize_t hello_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);

/* 定义设备属性*/
static DEVICE_ATTR(val, S_IRUGO | S_IWUSR, hello_val_show, hello_val_store);

/* 打开设备方法*/
static int hello_open(struct inode* inode, struct file* filp) {
    struct hello_android_dev* dev;

    /* 将自定义设备结构体保存在文件指针的私有数据域中，以便访问设备时拿来用*/
	dev = (struct hello_android_dev*)container_of(inode->i_cdev, struct hello_android_dev, dev);
	filp->private_data = dev;

	return 0;
}

/* 设备文件释放时调用，空实现*/
static int hello_release(struct inode* inode, struct file* filp) {
    return 0;
}

/*读取设备的寄存器val的值*/
static ssize_t hello_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos) {
    ssize_t err = 0;
	struct hello_android_dev* dev = filp->private_data;
	
	/* 同步访问 */
	if(down_interruptible(&(dev->sem))) {
	    return -ERESTARTSYS;	
	}

	if(count < sizeof(dev->val)) {
	    goto out;
	}
	
	/* 将寄存器val的值拷贝到用户提供的缓冲区*/
	if(copy_to_user(buf, &(dev->val), sizeof(dev->val))) {
	    err = -EFAULT;
		goto out;
	}

	err = sizeof(dev->val);

out:
    up(&(dev->sem));
	return err; 
}

/*写设备的寄存器值val*/
static ssize_t hello_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos) {
    struct hello_android_dev* dev = filp->private_data;
	ssize_t err = 0;

	/*同步访问*/
	if(down_interruptible(&(dev->sem))) {
        return -ERESTARTSYS;
    }

	if(count != sizeof(dev->val)) {
		goto out;
	}

	/*将用户提供的缓冲区的值写到设备寄存器去*/
	if(copy_from_user(&(dev->val), buf, count)) {
		err = -EFAULT;
		goto out;
	}

	err = sizeof(dev->val);

out:
    
	up(&(dev->sem));
	return err;
}

/*读取寄存器val的值到缓冲区buf中，内部使用*/
static ssize_t _hello_get_val(struct hello_android_dev* dev, char* buf) {
    int val = 0;

	/*同步访问*/
	if(down_interruptible(&(dev->sem))) {
		return -ERESTARTSYS;
	}

	val = dev->val;
	up(&(dev->sem));

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

/*把缓冲区buf的值写到设备寄存器val中去，内部使用*/
static ssize_t _hello_set_val(struct hello_android_dev* dev, const char* buf, size_t count) {
    int val = 0;

	/*将字符串转换成数字*/
	val = simple_strtol(buf, NULL, 10);

	/*同步访问*/
	if(down_interruptible(&(dev->sem))) {
	    return ERESTARTSYS;
	}

	dev->val = val;
	up(&(dev->sem));

	return count;
}

/*读取设备属性val*/
static ssize_t hello_val_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct hello_android_dev* hdev = (struct hello_android_dev*)dev_get_drvdata(dev);
	
	return _hello_get_val(hdev, buf);
}

/*写设备属性val*/
static ssize_t hello_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {
     struct hello_android_dev* hdev = (struct hello_android_dev*)dev_get_drvdata(dev);
	 return _hello_set_val(hdev, buf, count);
}

/*读取设备寄存器val的值，保存在page缓存区中*/
static ssize_t hello_proc_read(char* page, char** start, off_t off, int count, int* eof, void* data) {
    if(off > 0) {
		*eof = 1;
		return 0;
	}

	return _hello_get_val(hello_dev, page);
}

/*把缓冲区的值buff保存到设备寄存器val中去*/
static ssize_t hello_proc_write(struct file* filp, const char __user *buff, unsigned long len, void* data) {
    int err = 0;
	char* page = NULL;

	if(len > PAGE_SIZE) {
	    printk(KERN_ALERT"The buff is too large:%lu.\n", len);
		return -EFAULT;
	}

	page = (char*)__get_free_page(GFP_KERNEL);
	if(!page) {
	    printk(KERN_ALERT"Failed to alloc page.\n");
		return -ENOMEM;
	}

	/*先把用户提供的缓冲区值拷贝到内核缓冲区去*/
	if(copy_from_user(page, buff, len)) {
	    printk(KERN_ALERT"Failed to copy buff from user.\n");
		err = -EFAULT;
		goto out;
	}

	err = _hello_set_val(hello_dev, page, len);

out:
    free_page((unsigned long)page);
	return err;
}

/*创建/proc/hello/文件*/
static void hello_create_proc(void) {
    struct proc_dir_entry* entry;  
	entry = proc_create(HELLO_DEVICE_PROC_NAME, 0644, 0, &hello_fops);
	/*entry = create_proc_entry(HELLO_DEVICE_PROC_NAME, 0, NULL); // 该函数已被替代
	if(entry) {
	    entry->owner = THIS_MODULE;
		entry->read_proc = hello_proc_read;
		entry->write_proc = hello_proc_write;
	}*/
}

/*删除/proc/hello文件*/
static void hello_remove_proc(void) {
    remove_proc_entry(HELLO_DEVICE_PROC_NAME, NULL);
}

/* 最后，定义模块加载和卸载方法，这里只要执行设备注册和初始化操作 */
/*初始化设备*/
static int __hello_setup_dev(struct hello_android_dev* dev) {
    int err;
	dev_t devno = MKDEV(hello_major, hello_minor);

	memset(dev, 0, sizeof(struct hello_android_dev));

	cdev_init(&(dev->dev), &hello_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &hello_fops;

	/*注册字符设备*/
	err = cdev_add(&(dev->dev), devno, 1);
	if(err) {
	    return err;
	}

	/* 初始化信号量和寄存器val的值*/
	sema_init(&(dev->sem), 1);
    //init_MUTEX(&(dev->sem)); 2.6.25及以后的linux内核版本废除了init_MUTEX函数
	dev->val = 0;

	return 0;
}

/* 模块加载方法*/
static int __init hello_init(void) { 
//宏定义__init，用于告诉编译器相关函数或变量仅用于初始化。
//编译器将标__init的所有代码存在特殊的内存段中，初始化结束后就释放这段内存。
    int err = -1;
	dev_t dev = 0;
	struct device* temp = NULL;

	printk(KERN_ALERT"Initializeing hello device.\n");

	/* 动态分配主设备和从设备号*/
	err = alloc_chrdev_region(&dev, 0, 1, HELLO_DEVICE_NODE_NAME);
	if(err < 0) {
        printk(KERN_ALERT"Failed to alloc char dev region.\n");
		goto fail;
    }

	hello_major = MAJOR(dev);
	hello_minor = MINOR(dev);

	/*分配hello设备结构体变量*/
	hello_dev = kmalloc(sizeof(struct hello_android_dev), GFP_KERNEL);
	if(!hello_dev) {
	    err = -ENOMEM;
		printk(KERN_ALERT"Failed to alloc hello_dev.\n");
		goto unregister;
	}

	/*初始化设备*/
	err = __hello_setup_dev(hello_dev);
	if(err) {
	    printk(KERN_ALERT"Failed to setup dev: %d.\n", err);
		goto cleanup;
	}

	/*在/sys/class/目录下创建设备类别目录hello*/
	hello_class = class_create(THIS_MODULE, HELLO_DEVICE_CLASS_NAME);
	if(IS_ERR(hello_class)) {
	    err = PTR_ERR(hello_class);
		printk(KERN_ALERT"Failed to create hello class.\n");
		goto destroy_cdev;
	}

	/*在/dev目录和/sys/class/hello目录下分别创建设备文件hello*/
	temp = device_create(hello_class, NULL, dev, "%s", HELLO_DEVICE_FILE_NAME);
	if(IS_ERR(temp)) {
	    err = PTR_ERR(temp);
		printk(KERN_ALERT"Failed to create hello device.");
		goto destroy_class;
	}

	/*在sys/class/hello/hello/目录下创建属性文件val*/
	err = device_create_file(temp, &dev_attr_val);
	if(err < 0) {
	    printk(KERN_ALERT"Failed to create attribute val.");
		goto destroy_device;
	}

	dev_set_drvdata(temp, hello_dev);

	/*创建/proc/hello文件*/
	hello_create_proc();

	printk(KERN_ALERT"Succedded to initialize hello device.\n");
	return 0;

destroy_device:
    device_destroy(hello_class, dev);

destroy_class:
    class_destroy(hello_class);

destroy_cdev:
    cdev_del(&(hello_dev->dev));

cleanup:
    kfree(hello_dev);

unregister:
    unregister_chrdev_region(MKDEV(hello_major, hello_minor), 1);

fail:
    return err;
}

/*模块卸载方法*/
static void __exit hello_exit(void) {
//__exit修饰词标记函数只在模块卸载时使用。如果模块被直接编进内核则该函数就不会被调用。
//如果内核编译时没有包含该模块,则此标记的函数将被简单地丢弃。
    dev_t devno = MKDEV(hello_major, hello_minor);

	printk(KERN_ALERT"Destory hello device.\n");

	/* 删除/proc/hello文件*/
	hello_remove_proc();

	/*销毁设备类别和设备*/
	if(hello_class) {
	    device_destroy(hello_class, MKDEV(hello_major, hello_minor));
		class_destroy(hello_class);
	}

	/*删除字符设备和释放设备内存*/
	if(hello_dev) {
	    cdev_del(&(hello_dev->dev));
		kfree(hello_dev);
	}

	/*释放设备号*/
	unregister_chrdev_region(devno, 1);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("First Android Driver");

module_init(hello_init);
module_exit(hello_exit);

