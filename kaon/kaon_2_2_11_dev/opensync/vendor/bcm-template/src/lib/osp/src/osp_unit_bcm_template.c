#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "build_version.h"
#include "util.h"
#include "log.h"
#include "os_nif.h"

#include "osp_unit.h"
#include <wlcsm_lib_api.h>



bool osp_nvram_get(const char *nvram_key, char *buff, size_t buffsz)
{
    char key[256];
    char *value;
    size_t n;

    snprintf(key, sizeof(key), "%s", nvram_key);
    value = wlcsm_nvram_get(key);
    LOGI("%s: '%s' = '%s'", __func__, key, value ?: "(none)");
    if (!value)
        return false;

	n = snprintf(buff, buffsz, value);
	if (n >= buffsz)
	{
		LOG(ERR, "buffer not large enough");
		return false;
	}
	return true;
}



bool osp_unit_serial_get(char *buff, size_t buffsz)
{
    memset(buff, 0, buffsz);
	return osp_nvram_get("board_serialnumber", buff, buffsz);
}

bool osp_unit_id_get(char *buff, size_t buffsz)
{
	return osp_nvram_get("board_id", buff, buffsz);
}

bool osp_unit_model_get(char *buff, size_t buffsz)
{
	return osp_nvram_get("target_model", buff, buffsz);
}

bool osp_unit_sku_get(char *buff, size_t buffsz)
{
	return osp_nvram_get("sku_number", buff, buffsz);
}

bool osp_unit_hw_revision_get(char *buff, size_t buffsz)
{
	return osp_nvram_get("board_revision", buff, buffsz);
}

bool osp_unit_platform_version_get(char *buff, size_t buffsz)
{
    return osp_nvram_get("platform_version", buff, buffsz);;
}

bool osp_unit_sw_version_get(char *buff, size_t buffsz)
{
    strscpy(buff, app_build_ver_get(), buffsz);
    return true;
}

bool osp_unit_vendor_name_get(char *buff, size_t buffsz)
{
    return osp_nvram_get("vendor_name", buff, buffsz);
}

bool osp_unit_vendor_part_get(char *buff, size_t buffsz)
{
    return osp_nvram_get("vendor_part_number", buff, buffsz);
}

bool osp_unit_manufacturer_get(char *buff, size_t buffsz)
{
    return osp_nvram_get("vendor_manufacturer", buff, buffsz);
}

bool osp_unit_factory_get(char *buff, size_t buffsz)
{
    return osp_nvram_get("vendor_factory", buff, buffsz);
}

bool osp_unit_mfg_date_get(char *buff, size_t buffsz)
{
    return osp_nvram_get("vendor_mfg_date", buff, buffsz);
}
