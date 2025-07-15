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

SYMBOL_CRC(ggml_init, 0x4f0d9579, "_gpl");
SYMBOL_CRC(ggml_free, 0x5f31c14b, "_gpl");
SYMBOL_CRC(ggml_new_tensor, 0xaf806ca2, "_gpl");
SYMBOL_CRC(ggml_new_tensor_1d, 0xcfdfc6b8, "_gpl");
SYMBOL_CRC(ggml_new_tensor_2d, 0x9a28c620, "_gpl");
SYMBOL_CRC(ggml_new_tensor_3d, 0x5de4b31f, "_gpl");
SYMBOL_CRC(ggml_add, 0x6c9b2a55, "_gpl");
SYMBOL_CRC(ggml_mul, 0x47979e22, "_gpl");
SYMBOL_CRC(ggml_mul_mat, 0x7f236007, "_gpl");
SYMBOL_CRC(ggml_rms_norm, 0xb65676b5, "_gpl");
SYMBOL_CRC(ggml_get_rows, 0x53ee02ea, "_gpl");
SYMBOL_CRC(ggml_scale, 0x1bd6cad2, "_gpl");
SYMBOL_CRC(ggml_rope, 0x397ad2c6, "_gpl");
SYMBOL_CRC(ggml_silu, 0xa796b24d, "_gpl");
SYMBOL_CRC(ggml_soft_max, 0xfe44bb55, "_gpl");
SYMBOL_CRC(ggml_print_tensor_info, 0xf5cf41b9, "_gpl");
SYMBOL_CRC(ggml_transpose, 0x323b3e58, "_gpl");
SYMBOL_CRC(ggml_build_forward, 0x3c635b72, "_gpl");
SYMBOL_CRC(ggml_graph_compute, 0x7454d54d, "_gpl");
SYMBOL_CRC(ggml_compute_forward, 0xfe45288e, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x9ee85a72, "filp_open" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xbe9c126d, "proc_create" },
	{ 0xb0e602eb, "memmove" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x92540fbf, "finish_wait" },
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
	{ 0x4129f5ee, "kernel_fpu_begin_mask" },
	{ 0xa916b694, "strnlen" },
	{ 0x38722f80, "kernel_fpu_end" },
	{ 0x40a9b349, "vzalloc" },
	{ 0x599fb41c, "kvmalloc_node" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x9166fada, "strncpy" },
	{ 0x5a60eed2, "kthread_stop" },
	{ 0x85d1595c, "current_task" },
	{ 0x11089ac7, "_ctype" },
	{ 0xc15e9e9, "proc_mkdir" },
	{ 0xfb578fc5, "memset" },
	{ 0x823e6b29, "kernel_read" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0xea2e56c1, "proc_remove" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x6c350627, "kthread_create_on_node" },
	{ 0x675d5d51, "seq_read" },
	{ 0x999e8297, "vfree" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x4d3a6dac, "filp_close" },
	{ 0x2cf56265, "__dynamic_pr_debug" },
	{ 0x2ac91ca, "seq_printf" },
	{ 0xc07351b3, "__SCT__cond_resched" },
	{ 0x7bd3dfe8, "single_release" },
	{ 0x58ffa277, "kmalloc_trace" },
	{ 0x754d539c, "strlen" },
	{ 0x7aa1756e, "kvfree" },
	{ 0x8a8cd00a, "single_open" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xf9a482f9, "msleep" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xe2c17b5d, "__SCT__might_resched" },
	{ 0xf02dd3c3, "kmalloc_caches" },
	{ 0x5219c37a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "AE31735F6B0BE567A95B7DC");
