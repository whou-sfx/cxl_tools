#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
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

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xf4f9140e, "module_layout" },
	{ 0x130c0ba4, "cdev_del" },
	{ 0x5febae91, "kmalloc_caches" },
	{ 0x8cc71434, "pci_write_config_dword" },
	{ 0x332eb58d, "cdev_init" },
	{ 0x7e1a8238, "pci_free_irq_vectors" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x107d282c, "pcim_enable_device" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0x9ef881e9, "device_destroy" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xeafb8963, "pci_set_master" },
	{ 0xc266c721, "pci_alloc_irq_vectors_affinity" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x810a0d4b, "noop_llseek" },
	{ 0x73434674, "device_create" },
	{ 0xc71dbc57, "cdev_add" },
	{ 0x6ae4dd49, "compat_ptr_ioctl" },
	{ 0xa95a10d6, "_dev_info" },
	{ 0x3a75e84, "devm_free_irq" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x92997ed8, "_printk" },
	{ 0xe2c02f16, "pci_read_config_dword" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x8f6d9c52, "pci_unregister_driver" },
	{ 0x468a6a98, "kmem_cache_alloc_trace" },
	{ 0x1c63d224, "pci_irq_vector" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x37a0cba, "kfree" },
	{ 0x6da62160, "__pci_register_driver" },
	{ 0x6363e5b4, "class_destroy" },
	{ 0x608741b5, "__init_swait_queue_head" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x56051416, "devm_request_threaded_irq" },
	{ 0x7a4fbff4, "__class_create" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("pci:v*d*sv*sd*bc00sc05i02*");
MODULE_ALIAS("pci:v*d*sv*sd*bc05sc02i10*");

MODULE_INFO(srcversion, "FC4733894D128892484EAE2");
