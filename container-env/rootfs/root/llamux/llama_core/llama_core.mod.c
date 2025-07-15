#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

SYMBOL_CRC(ggml_init, 0xeafd4b49, "_gpl");
SYMBOL_CRC(ggml_free, 0x77da4351, "_gpl");
SYMBOL_CRC(ggml_new_tensor, 0xfcaa7ede, "_gpl");
SYMBOL_CRC(ggml_new_tensor_1d, 0xd87e1141, "_gpl");
SYMBOL_CRC(ggml_new_tensor_2d, 0xed49bfc6, "_gpl");
SYMBOL_CRC(ggml_new_tensor_3d, 0x688ae25a, "_gpl");
SYMBOL_CRC(ggml_add, 0x7131391e, "_gpl");
SYMBOL_CRC(ggml_mul, 0x22718c29, "_gpl");
SYMBOL_CRC(ggml_mul_mat, 0xb54d38bf, "_gpl");
SYMBOL_CRC(ggml_rms_norm, 0xcc3855b6, "_gpl");
SYMBOL_CRC(ggml_get_rows, 0xdaa396d6, "_gpl");
SYMBOL_CRC(ggml_scale, 0xbe63a402, "_gpl");
SYMBOL_CRC(ggml_rope, 0x00a08b19, "_gpl");
SYMBOL_CRC(ggml_silu, 0x45df46a9, "_gpl");
SYMBOL_CRC(ggml_soft_max, 0xa8f060bf, "_gpl");
SYMBOL_CRC(ggml_print_tensor_info, 0xf062116d, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x88db9f48, "__check_object_size" },
	{ 0xc6d09aa9, "release_firmware" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xbe9c126d, "proc_create" },
	{ 0xb0e602eb, "memmove" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x329be71d, "request_firmware" },
	{ 0x69acdf38, "memcpy" },
	{ 0x37a0cba, "kfree" },
	{ 0xdf437eec, "seq_lseek" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xe2964344, "__wake_up" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xf9bd2096, "wake_up_process" },
	{ 0x92997ed8, "_printk" },
	{ 0x1000e51, "schedule" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0xa916b694, "strnlen" },
	{ 0x40a9b349, "vzalloc" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x9166fada, "strncpy" },
	{ 0x5a60eed2, "kthread_stop" },
	{ 0x11089ac7, "_ctype" },
	{ 0xc15e9e9, "proc_mkdir" },
	{ 0xfb578fc5, "memset" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0xea2e56c1, "proc_remove" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x6c350627, "kthread_create_on_node" },
	{ 0x675d5d51, "seq_read" },
	{ 0x999e8297, "vfree" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x2cf56265, "__dynamic_pr_debug" },
	{ 0x2ac91ca, "seq_printf" },
	{ 0x7bd3dfe8, "single_release" },
	{ 0x58ffa277, "kmalloc_trace" },
	{ 0x754d539c, "strlen" },
	{ 0x8a8cd00a, "single_open" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xf9a482f9, "msleep" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xe2c17b5d, "__SCT__might_resched" },
	{ 0xf02dd3c3, "kmalloc_caches" },
	{ 0x5219c37a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "0ADCE077555D0893744BE13");
