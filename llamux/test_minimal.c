#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init test_init(void)
{
    printk(KERN_INFO "Test module: Hello!\n");
    return 0;
}

static void __exit test_exit(void)
{
    printk(KERN_INFO "Test module: Goodbye!\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal test module");