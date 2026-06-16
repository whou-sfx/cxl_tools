#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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

MODULE_INFO(intree, "Y");

KSYMTAB_FUNC(to_cxl_root_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(is_endpoint_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(is_root_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(is_switch_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_endpoint_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_switch_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(is_cxl_port, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_port, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_port, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_root, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_port_to_pci_bus, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_register_pci_bus, "_gpl", "CXL");
KSYMTAB_FUNC(find_cxl_root, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_dport, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_rch_dport, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_endpoint_autoremove, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_enumerate_ports, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_pci_find_port, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_mem_find_port, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_root_decoder_alloc, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_switch_decoder_alloc, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_endpoint_decoder_alloc, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_decoder_add_locked, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_decoder_add, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_decoder_autoremove, "_gpl", "CXL");
KSYMTAB_FUNC(__cxl_driver_register, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_driver_unregister, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_bus_rescan, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_bus_drain, "_gpl", "CXL");
KSYMTAB_FUNC(schedule_cxl_memdev_detach, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_endpoint_get_perf_coordinates, "_gpl", "CXL");
KSYMTAB_DATA(cxl_bus_type, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_debugfs_create_dir, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_nvdimm_bridge, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_find_nvdimm_bridge, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_nvdimm_bridge, "_gpl", "CXL");
KSYMTAB_FUNC(is_cxl_nvdimm, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_nvdimm, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_nvdimm, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_probe_component_regs, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_probe_device_regs, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_map_component_regs, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_map_device_regs, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_find_regblock_instance, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_find_regblock, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_count_regblock, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_map_pmu_regs, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_setup_regs, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_dport_map_rcd_linkcap, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_rcd_component_reg_phys, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_trigger_poison_list, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_inject_poison, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_clear_poison, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_memdev_update_perf, "_gpl", "CXL");
KSYMTAB_FUNC(is_cxl_memdev, "_gpl", "CXL");
KSYMTAB_FUNC(set_exclusive_cxl_commands, "_gpl", "CXL");
KSYMTAB_FUNC(clear_exclusive_cxl_commands, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_setup_fw_upload, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_memdev, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_sanitize_setup_notifier, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_internal_send_cmd, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_enumerate_cmds, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_event_trace_record, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_mem_get_event_records, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_dev_state_identify, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_mem_dpa_fetch, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_get_dirty_count, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_arm_dirty_shutdown, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_set_timestamp, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_mem_get_poison, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_poison_state_init, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_mailbox_init, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_memdev_state_create, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_port_enumerate_dports, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_await_media_ready, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_dvsec_rr_decode, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_hdm_decode_init, "_gpl", "CXL");
KSYMTAB_FUNC(read_cdat_data, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_cor_error_detected, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_error_detected, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_endpoint_decoder_reset_detected, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_gpf_get_dvsec, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_add_passthrough_decoder, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_setup_hdm, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_dpa_debug, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_dpa_setup, "_gpl", "");
KSYMTAB_FUNC(devm_cxl_dpa_reserve, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_port_commit_reap, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_enumerate_decoders, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_pmu_add, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_endpoint_parse_cdat, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_switch_parse_cdat, "_gpl", "CXL");
KSYMTAB_FUNC(is_cxl_region, "_gpl", "CXL");
KSYMTAB_FUNC(is_cxl_pmem_region, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_pmem_region, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxl_dax_region, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_add_to_region, "_gpl", "CXL");
KSYMTAB_FUNC(cxl_port_get_spa_cache_alias, "_gpl", "CXL");
KSYMTAB_FUNC(to_cxlfs, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_setup_features, "_gpl", "CXL");
KSYMTAB_FUNC(devm_cxl_setup_fwctl, "_gpl", "CXL");

MODULE_INFO(depends, "");

