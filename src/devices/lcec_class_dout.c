//
//    Copyright (C) 2024 Scott Laird <scott@sigkill.org>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

/// @file
/// @brief Library for digital output devices

#include "lcec_class_dout.h"

#include <stdio.h>

#include "../lcec.h"

static const lcec_pindesc_t slave_pins[] = {
    {HAL_BIT, HAL_IN, offsetof(lcec_class_dout_channel_t, out), "%s.%s.%s.%s"},
    {HAL_TYPE_UNSPECIFIED, HAL_DIR_UNSPECIFIED, -1, NULL},
};

static const lcec_pindesc_t slave_params[] = {
    {HAL_BIT, HAL_RW, offsetof(lcec_class_dout_channel_t, invert), "%s.%s.%s.%s-invert"},
    {HAL_TYPE_UNSPECIFIED, HAL_DIR_UNSPECIFIED, -1, NULL},
};

/// @brief Allocates a block of memory for holding the result of `count`
/// calls to `lcec_dout_register_device()`.
///
/// It is the caller's responsibility to verify that the result is not
/// NULL.
///
/// @param count The number of channels to allocate room for.
lcec_class_dout_channels_t *lcec_dout_allocate_channels(int count) {
  lcec_class_dout_channels_t *channels;

  channels = hal_malloc(sizeof(lcec_class_dout_channels_t));
  if (channels == NULL) {
    return NULL;
  }
  channels->count = count;
  channels->channels = hal_malloc(sizeof(lcec_class_dout_channel_t *) * count);
  if (channels->channels == NULL) {
    return NULL;
  }

  return channels;
}

/// @brief Register a single digital-output channel and publishes it as a LinuxCNC HAL pin.
///
/// @param pdo_entry_regs A pointer to the pdo_entry_regs passed into the device `_init` function.
/// @param slave The slave, from `_init`.
/// @param id The pin ID.  Used for naming.  Should generally start at 0 and increment once per digital out pin.
/// @param idx The PDO index for the digital output.
/// @param sindx The PDO sub-index for the digital output.
///
/// See lcec_el2xxx.c for an example of use.
lcec_class_dout_channel_t *lcec_dout_register_channel(
    ec_pdo_entry_reg_t **pdo_entry_regs, struct lcec_slave *slave, int id, uint16_t idx, uint16_t sidx) {
  char name[32];

  snprintf(name, 32, "dout-%d", id);

  return lcec_dout_register_channel_named(pdo_entry_regs, slave, idx, sidx, name);
}

/// @brief Register a single digital-output channel and publishes it as a LinuxCNC HAL pin.
///
/// @param pdo_entry_regs A pointer to the pdo_entry_regs passed into the device `_init` function.
/// @param slave The slave, from `_init`.
/// @param id The pin ID.  Used for naming.  Should generally start at 0 and increment once per digital out pin.
/// @param idx The PDO index for the digital output.
/// @param sindx The PDO sub-index for the digital output.
/// @param name The base pin name to use, usually `dout-<ID>`.
///
/// See lcec_el2xxx.c for an example of use.
lcec_class_dout_channel_t *lcec_dout_register_channel_named(
    ec_pdo_entry_reg_t **pdo_entry_regs, struct lcec_slave *slave, uint16_t idx, uint16_t sidx, char *name) {
  lcec_class_dout_channel_t *data;
  int err;

  data = hal_malloc(sizeof(lcec_class_dout_channel_t));
  if (data == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "hal_malloc() for slave %s.%s pin %s failed\n", slave->master->name, slave->name, name);
    return NULL;
  }
  memset(data, 0, sizeof(lcec_class_dout_channel_t));

  LCEC_PDO_INIT((*pdo_entry_regs), slave->index, slave->vid, slave->pid, idx, sidx, &data->pdo_os, &data->pdo_bp);
  err = lcec_pin_newf_list(data, slave_pins, LCEC_MODULE_NAME, slave->master->name, slave->name, name);
  if (err != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "lcec_pin_newf_list for slave %s.%s pin %s failed\n", slave->master->name, slave->name, name);
    return NULL;
  }
  err = lcec_param_newf_list(data, slave_params, LCEC_MODULE_NAME, slave->master->name, slave->name, name);
  if (err != 0) {
    rtapi_print_msg(
        RTAPI_MSG_ERR, LCEC_MSG_PFX "lcec_params_newf_list for slave %s.%s pin %s failed\n", slave->master->name, slave->name, name);
    return NULL;
  }

  return data;
}

/// @brief Write data to a digital out port.
///
/// Call this once per channel registered, from inside of your device's
/// write function.
///
/// @param slave The slave, passed from the per-device `_write`.
/// @param data A lcec_class_dout_channel_t *, as returned by lcec_dout_register_channel.
void lcec_dout_write(struct lcec_slave *slave, lcec_class_dout_channel_t *data) {
  lcec_master_t *master = slave->master;
  uint8_t *pd = master->process_data;
  hal_bit_t s;

  s = *(data->out);
  if (data->invert) {
    s = !s;
  }

  EC_WRITE_BIT(&pd[data->pdo_os], data->pdo_bp, s);
}

/// @brief Write data to all digital out channels attached to this device.
///
/// @param slave The slave, passed from the per-device `_write`.
/// @param channels A `lcec_class_dout_channels_t *`, as returned by
/// `lcec_dout_register_channel`.
void lcec_dout_write_all(struct lcec_slave *slave, lcec_class_dout_channels_t *channels) {
  for (int i = 0; i < channels->count; i++) {
    lcec_class_dout_channel_t *channel = channels->channels[i];

    lcec_dout_write(slave, channel);
  }
}
