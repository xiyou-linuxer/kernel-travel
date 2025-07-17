

#include <xkernel/init.h>
#include <xkernel/fdt.h>
#include <xkernel/types.h>
#include <xkernel/printk.h>
// #ifdef CONFIG_OF_EARLY_FLATTREE
bool __init early_init_dt_verify(void *params);

// 解析设备树
bool __init early_init_dt_scan(void *params)
{
    bool status;
    status = early_init_dt_verify(params);
    
    
    
    // pr_info("123455");
    return status;
}


bool __init early_init_dt_verify(void *params)
{
    if(!params){
        return false;
    }  
    //验证设备树头部的有效性  
    if(fdt_check_header(params)){
        return false;
    }
    // 设置设备树指针：
//     initial_boot_params = params;
//     of_fdt_crc32 = crc32_be(~0, initial_boot_params,
// 				fdt_totalsize(initial_boot_params));
	
    return true;

}

// #endif


int fdt_check_header(const void *fdt)
{
    size_t hdrsize;
    
    return 0;
}