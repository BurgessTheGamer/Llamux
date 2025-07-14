#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

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

KSYMTAB_FUNC(ggml_init, "_gpl", "");
KSYMTAB_FUNC(ggml_free, "_gpl", "");
KSYMTAB_FUNC(ggml_new_tensor, "_gpl", "");
KSYMTAB_FUNC(ggml_new_tensor_1d, "_gpl", "");
KSYMTAB_FUNC(ggml_new_tensor_2d, "_gpl", "");
KSYMTAB_FUNC(ggml_new_tensor_3d, "_gpl", "");
KSYMTAB_FUNC(ggml_add, "_gpl", "");
KSYMTAB_FUNC(ggml_mul, "_gpl", "");
KSYMTAB_FUNC(ggml_mul_mat, "_gpl", "");
KSYMTAB_FUNC(ggml_rms_norm, "_gpl", "");
KSYMTAB_FUNC(ggml_get_rows, "_gpl", "");
KSYMTAB_FUNC(ggml_scale, "_gpl", "");
KSYMTAB_FUNC(ggml_rope, "_gpl", "");
KSYMTAB_FUNC(ggml_silu, "_gpl", "");
KSYMTAB_FUNC(ggml_soft_max, "_gpl", "");
KSYMTAB_FUNC(ggml_print_tensor_info, "_gpl", "");

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

static const char ____versions[]
__used __section("__versions") =
	"\x1c\x00\x00\x00\x48\x9f\xdb\x88"
	"__check_object_size\0"
	"\x18\x00\x00\x00\xc2\x9c\xc4\x13"
	"_copy_from_user\0"
	"\x14\x00\x00\x00\xc2\xde\x7b\x8e"
	"proc_create\0"
	"\x10\x00\x00\x00\xeb\x02\xe6\xb0"
	"memmove\0"
	"\x14\x00\x00\x00\x6e\x4a\x6e\x65"
	"snprintf\0\0\0\0"
	"\x14\x00\x00\x00\xbf\x0f\x54\x92"
	"finish_wait\0"
	"\x10\x00\x00\x00\x38\xdf\xac\x69"
	"memcpy\0\0"
	"\x10\x00\x00\x00\xba\x0c\x7a\x03"
	"kfree\0\0\0"
	"\x14\x00\x00\x00\x76\x02\xcd\x2b"
	"seq_lseek\0\0\0"
	"\x20\x00\x00\x00\x95\xd4\x26\x8c"
	"prepare_to_wait_event\0\0\0"
	"\x1c\x00\x00\x00\x6e\x64\xf7\xb3"
	"kthread_should_stop\0"
	"\x14\x00\x00\x00\x44\x43\x96\xe2"
	"__wake_up\0\0\0"
	"\x18\x00\x00\x00\x8c\x89\xd4\xcb"
	"fortify_panic\0\0\0"
	"\x14\x00\x00\x00\xbb\x6d\xfb\xbd"
	"__fentry__\0\0"
	"\x18\x00\x00\x00\x2c\xd0\xf6\x8e"
	"wake_up_process\0"
	"\x10\x00\x00\x00\x7e\x3a\x2c\x12"
	"_printk\0"
	"\x14\x00\x00\x00\x51\x0e\x00\x01"
	"schedule\0\0\0\0"
	"\x1c\x00\x00\x00\xcb\xf6\xfd\xf0"
	"__stack_chk_fail\0\0\0\0"
	"\x10\x00\x00\x00\x94\xb6\x16\xa9"
	"strnlen\0"
	"\x10\x00\x00\x00\x49\xb3\xa9\x40"
	"vzalloc\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x18\x00\x00\x00\x75\x79\x48\xfe"
	"init_wait_entry\0"
	"\x1c\x00\x00\x00\x63\xa5\x03\x4c"
	"random_kmalloc_seed\0"
	"\x14\x00\x00\x00\x4b\x8d\xfa\x4d"
	"mutex_lock\0\0"
	"\x10\x00\x00\x00\xda\xfa\x66\x91"
	"strncpy\0"
	"\x18\x00\x00\x00\xa6\xb9\xbd\x68"
	"kthread_stop\0\0\0\0"
	"\x10\x00\x00\x00\xc7\x9a\x08\x11"
	"_ctype\0\0"
	"\x14\x00\x00\x00\xc6\x68\x5c\xed"
	"proc_mkdir\0\0"
	"\x10\x00\x00\x00\xc5\x8f\x57\xfb"
	"memset\0\0"
	"\x1c\x00\x00\x00\xca\x39\x82\x5b"
	"__x86_return_thunk\0\0"
	"\x20\x00\x00\x00\x54\xea\xa5\xd9"
	"__init_waitqueue_head\0\0\0"
	"\x14\x00\x00\x00\xbb\x98\x3b\x46"
	"proc_remove\0"
	"\x10\x00\x00\x00\x5a\x25\xd5\xe2"
	"strcmp\0\0"
	"\x20\x00\x00\x00\x57\x39\x1d\x04"
	"kthread_create_on_node\0\0"
	"\x10\x00\x00\x00\xa7\xb0\x39\x2d"
	"kstrdup\0"
	"\x14\x00\x00\x00\xdd\xe1\x2e\xb4"
	"seq_read\0\0\0\0"
	"\x10\x00\x00\x00\x97\x82\x9e\x99"
	"vfree\0\0\0"
	"\x18\x00\x00\x00\x38\xf0\x13\x32"
	"mutex_unlock\0\0\0\0"
	"\x1c\x00\x00\x00\x65\x62\xf5\x2c"
	"__dynamic_pr_debug\0\0"
	"\x14\x00\x00\x00\xac\x44\xc1\x7a"
	"seq_printf\0\0"
	"\x18\x00\x00\x00\x63\xd6\x6d\x73"
	"single_release\0\0"
	"\x18\x00\x00\x00\x3c\x7b\x32\xdc"
	"kmalloc_trace\0\0\0"
	"\x2c\x00\x00\x00\xc6\xfa\xb1\x54"
	"__ubsan_handle_load_invalid_value\0\0\0"
	"\x10\x00\x00\x00\x9c\x53\x4d\x75"
	"strlen\0\0"
	"\x14\x00\x00\x00\x9f\x4a\x26\x31"
	"single_open\0"
	"\x10\x00\x00\x00\x8f\x68\xee\xd6"
	"vmalloc\0"
	"\x10\x00\x00\x00\xf9\x82\xa4\xf9"
	"msleep\0\0"
	"\x14\x00\x00\x00\x45\x3a\x23\xeb"
	"__kmalloc\0\0\0"
	"\x20\x00\x00\x00\x5d\x7b\xc1\xe2"
	"__SCT__might_resched\0\0\0\0"
	"\x18\x00\x00\x00\x81\x09\xac\x29"
	"kmalloc_caches\0\0"
	"\x18\x00\x00\x00\x76\xf2\x0f\x5e"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "A7C2A5FE3A9ADFAF6860256");
