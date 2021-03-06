/*!
 * Copyright (c) 2020 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 *
 * This file is part of tuxedo-cc-wmi.
 *
 * tuxedo-cc-wmi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <linux/acpi.h>
#include <linux/wmi.h>

#define UNIWILL_WMI_MGMT_GUID_BA            "ABBC0F6D-8EA1-11D1-00A0-C90629100000"
#define UNIWILL_WMI_MGMT_GUID_BB            "ABBC0F6E-8EA1-11D1-00A0-C90629100000"
#define UNIWILL_WMI_MGMT_GUID_BC            "ABBC0F6F-8EA1-11D1-00A0-C90629100000"

#define UNIWILL_WMI_EVENT_GUID_0            "ABBC0F70-8EA1-11D1-00A0-C90629100000"
#define UNIWILL_WMI_EVENT_GUID_1            "ABBC0F71-8EA1-11D1-00A0-C90629100000"
#define UNIWILL_WMI_EVENT_GUID_2            "ABBC0F72-8EA1-11D1-00A0-C90629100000"

union uw_ec_read_return {
    u32 dword;
    struct {
        u8 data_low;
        u8 data_high;
    } bytes;
};

union uw_ec_write_return {
    u32 dword;
    struct {
        u8 addr_low;
        u8 addr_high;
        u8 data_low;
        u8 data_high;
    } bytes;
};

static u32 uniwill_wmi_ec_evaluate(u8 addr_low, u8 addr_high, u8 data_low, u8 data_high, u8 read_flag, u32 *return_buffer)
{
    acpi_status status;
    union acpi_object *out_acpi;
    u32 e_result = 0;

    // Kernel buffer for input argument
    u32 *wmi_arg = (u32 *) kmalloc(sizeof(u32)*10, GFP_KERNEL);
    // Byte reference to the input buffer
    u8 *wmi_arg_bytes = (u8 *) wmi_arg;

    u8 wmi_instance = 0x00;
    u32 wmi_method_id = 0x04;
    struct acpi_buffer wmi_in = { (acpi_size) sizeof(wmi_arg), wmi_arg};
    struct acpi_buffer wmi_out = { ACPI_ALLOCATE_BUFFER, NULL };

    // Zero input buffer
    memset(wmi_arg, 0x00, 10 * sizeof(u32));

    // Configure the input buffer
    wmi_arg_bytes[0] = addr_low;
    wmi_arg_bytes[1] = addr_high;
    wmi_arg_bytes[2] = data_low;
    wmi_arg_bytes[3] = data_high;

    if (read_flag != 0) {
        wmi_arg_bytes[5] = 0x01;
    }
    
    status = wmi_evaluate_method(UNIWILL_WMI_MGMT_GUID_BC, wmi_instance, wmi_method_id, &wmi_in, &wmi_out);
    out_acpi = (union acpi_object *) wmi_out.pointer;

    if (out_acpi && out_acpi->type == ACPI_TYPE_BUFFER) {
        memcpy(return_buffer, out_acpi->buffer.pointer, out_acpi->buffer.length);
    } /* else if (out_acpi && out_acpi->type == ACPI_TYPE_INTEGER) {
        e_result = (u32) out_acpi->integer.value;
    }*/
    if (ACPI_FAILURE(status)) {
        pr_err("uniwill_wmi.h: Error evaluating method\n");
        e_result = -EIO;
    }

    kfree(out_acpi);
    kfree(wmi_arg);

    return e_result;
}

static u32 uniwill_wmi_ec_read(u8 addr_low, u8 addr_high, union uw_ec_read_return *output)
{
    u32 uw_data[10];
    u32 ret = uniwill_wmi_ec_evaluate(addr_low, addr_high, 0x00, 0x00, 1, uw_data);
    output->dword = uw_data[0];
    return ret;
}

static u32 uniwill_wmi_ec_write(u8 addr_low, u8 addr_high, u8 data_low, u8 data_high, union uw_ec_write_return *output)
{
    u32 uw_data[10];
    u32 ret = uniwill_wmi_ec_evaluate(addr_low, addr_high, data_low, data_high, 0, uw_data);
    output->dword = uw_data[0];
    return ret;
}

static u32 uniwill_identify(void)
{
    int status;

    // Look for for GUIDs used on uniwill devices
    status =
        wmi_has_guid(UNIWILL_WMI_EVENT_GUID_0) &&
        wmi_has_guid(UNIWILL_WMI_EVENT_GUID_1) &&
        wmi_has_guid(UNIWILL_WMI_EVENT_GUID_2) &&
        wmi_has_guid(UNIWILL_WMI_MGMT_GUID_BA) &&
        wmi_has_guid(UNIWILL_WMI_MGMT_GUID_BB) &&
        wmi_has_guid(UNIWILL_WMI_MGMT_GUID_BC);

    if (!status)
    {
        pr_debug("probe: At least one Uniwill GUID missing\n");
        return -ENODEV;
    }

    return 0;
}
