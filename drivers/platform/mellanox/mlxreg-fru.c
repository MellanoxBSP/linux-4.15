// SPDX-License-Identifier: GPL-2.0+
/*
 * Mellanox register access driver
 *
 * Copyright (C) 2018 Mellanox Technologies
 * Copyright (C) 2018 Vadim Pasternak <vadimp@mellanox.com>
 */

#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/platform_data/mlxreg.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

/* I2C bus IO offsets */
#define MLXREG_FRU_REG_CPLD1_VER_OFFSET		0x00
#define MLXREG_FRU_REG_RESET_CAUSE_OFFSET	0x1d
#define MLXREG_FRU_REG_LED1_OFFSET		0x20
#define MLXREG_FRU_REG_LED2_OFFSET		0x21
#define MLXREG_FRU_REG_MASTER_I2C_OFFSET	0x2e
#define MLXREG_FRU_REG_MASTER_I2C_OFFSET_WP	0x2f
#define MLXREG_FRU_REG_GP1_OFFSET		0x30
#define MLXREG_FRU_REG_WP1_OFFSET		0x31
#define MLXREG_FRU_REG_GP2_OFFSET		0x32
#define MLXREG_FRU_REG_WP2_OFFSET		0x33
#define MLXREG_FRU_REG_ASIC_HEALTH_OFFSET	0x50
#define MLXREG_FRU_REG_ASIC_HEALTH_EVENT_OFFSET	0x51
#define MLXREG_FRU_REG_ASIC_HEALTH_MASK_OFFSET	0x52
#define MLXREG_FRU_REG_FAN_OFFSET		0x88
#define MLXREG_FRU_REG_FAN_EVENT_OFFSET		0x89
#define MLXREG_FRU_REG_FAN_MASK_OFFSET		0x8a
#define MLXREG_FRU_CHANNEL_I2C_REG		0xda

#define MLXREG_FRU_DEFAULT_IRQ			17
#define	MLXREG_FRU_NR_NONE			-1
#define MLXREG_FRU_FAN_MASK			GENMASK(1, 0)
#define MLXREG_FRU_ASIC1_MASK			GENMASK(7, 6)
#define MLXREG_FRU_ASIC2_MASK			GENMASK(5, 4)
#define MLXREG_FRU_LED_LO_NIBBLE_MASK		GENMASK(7, 4)
#define MLXREG_FRU_LED_HI_NIBBLE_MASK		GENMASK(3, 0)
#define MLXREG_FRU_DEFAULT_IRQ			17
#define MLXREG_FRU_CHAN_NUM			8
#define MLXREG_FRU_CHAN_OFFSET			2
#define MLXREG_FRU_SET_VBUS(bus, chan)		((((bus) << 8) & \
						 GENMASK(31, 8)) + (chan) + \
						 MLXREG_FRU_CHAN_OFFSET)
#define MLXREG_FRU_FAB_PRESENCE_REG_BASE	0x7f
#define MLXREG_FRU_FAB_WAKEUP_SIGNAL_REG_BASE	0xaa
#define MLXREG_FRU_FAB_PRESENCE_REG_SHIFT	0x03
#define MLXREG_FRU_FAB_WAKEUP_SIGNAL_REG_SHIFT	0x03
#define MLXREG_FRU_FAB_REG_STEP			8

/*
 * enum mlxreg_fru_type - driver flavours:
 *
 * @MLXREG_FU_FABRIC: type for the default field replaceable unit of the
 *		      fabric type with 1 byte address space registers;
 * @MLXREG_FRU_BLADE: type for the default field replaceable unit of the
 *		      blade type with 1 byte address space registers;
 * @MLXREG_FRU_FABRIC200: type for the default field replaceable unit of the
 *			  fabric type with 1 byte address space registers;
 * @MLXREG_FRU_BLADE200: type for the default field replaceable unit of the
 *			 blade type with 1 byte address space registers;
 */
enum mlxreg_fru_type {
	MLXREG_FRU_FABRIC,
	MLXREG_FRU_BLADE,
	MLXREG_FRU_FABRIC200,
	MLXREG_FRU_BLADE200,
};

/* mlxreg_fru_mux_priv - device private mux data
 * @muxc - mux control object;
 * @client - I2C client handle;
 * @last_chan - last register value for mux access;
 * @regmap: shared register map handle;
 */
struct mlxreg_fru_mux_priv {
	struct i2c_mux_core *muxc;
	struct i2c_client *client;
	u8 last_chan;
	void *regmap;
};

/* mlxreg_fru_priv - device private data
 * @client - I2C client handle;
 * @muxc - mux control object;
 * @hotplug - hotplug device;
 * @led - led device;
 * @io_regs - register access device;
 * @muxc - mux control object;
 * @last_chan - last register value for mux access;
 * @regmap: shared register map handle;
 */
struct mlxreg_fru_priv {
	struct i2c_client *client;
	struct i2c_mux_core *muxc;
	struct platform_device *hotplug;
	struct platform_device *led;
	struct platform_device *io_regs;
	struct mlxreg_fru_mux_priv *mux;
};

static bool mlxreg_fru_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MLXREG_FRU_REG_LED1_OFFSET:
	case MLXREG_FRU_REG_LED2_OFFSET:
	case MLXREG_FRU_REG_GP1_OFFSET:
	case MLXREG_FRU_REG_WP1_OFFSET:
	case MLXREG_FRU_REG_GP2_OFFSET:
	case MLXREG_FRU_REG_WP2_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_EVENT_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_MASK_OFFSET:
	case MLXREG_FRU_REG_FAN_EVENT_OFFSET:
	case MLXREG_FRU_REG_FAN_MASK_OFFSET:
	case MLXREG_FRU_CHANNEL_I2C_REG:
		return true;
	}
	return false;
}

static bool mlxreg_fru_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MLXREG_FRU_REG_CPLD1_VER_OFFSET:
	case MLXREG_FRU_REG_RESET_CAUSE_OFFSET:
	case MLXREG_FRU_REG_LED1_OFFSET:
	case MLXREG_FRU_REG_LED2_OFFSET:
	case MLXREG_FRU_REG_GP1_OFFSET:
	case MLXREG_FRU_REG_WP1_OFFSET:
	case MLXREG_FRU_REG_GP2_OFFSET:
	case MLXREG_FRU_REG_WP2_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_EVENT_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_MASK_OFFSET:
	case MLXREG_FRU_REG_FAN_OFFSET:
	case MLXREG_FRU_REG_FAN_EVENT_OFFSET:
	case MLXREG_FRU_REG_FAN_MASK_OFFSET:
	case MLXREG_FRU_CHANNEL_I2C_REG:
		return true;
	}
	return false;
}

static bool mlxreg_fru_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MLXREG_FRU_REG_CPLD1_VER_OFFSET:
	case MLXREG_FRU_REG_RESET_CAUSE_OFFSET:
	case MLXREG_FRU_REG_LED1_OFFSET:
	case MLXREG_FRU_REG_LED2_OFFSET:
	case MLXREG_FRU_REG_GP1_OFFSET:
	case MLXREG_FRU_REG_WP1_OFFSET:
	case MLXREG_FRU_REG_GP2_OFFSET:
	case MLXREG_FRU_REG_WP2_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_EVENT_OFFSET:
	case MLXREG_FRU_REG_ASIC_HEALTH_MASK_OFFSET:
	case MLXREG_FRU_REG_FAN_OFFSET:
	case MLXREG_FRU_REG_FAN_EVENT_OFFSET:
	case MLXREG_FRU_REG_FAN_MASK_OFFSET:
	case MLXREG_FRU_CHANNEL_I2C_REG:
		return true;
	}
	return false;
}

static const struct reg_default mlxreg_fru_regmap_default[] = {
	{ MLXREG_FRU_REG_WP1_OFFSET, 0x00 },
	{ MLXREG_FRU_REG_WP2_OFFSET, 0x00 },
};

/* Configuration for the register map of a device with 1 byte address space. */
static const struct regmap_config mlxreg_fru_regmap_conf = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 255,
	.cache_type = REGCACHE_FLAT,
	.writeable_reg = mlxreg_fru_writeable_reg,
	.readable_reg = mlxreg_fru_readable_reg,
	.volatile_reg = mlxreg_fru_volatile_reg,
	.reg_defaults = mlxreg_fru_regmap_default,
	.num_reg_defaults = ARRAY_SIZE(mlxreg_fru_regmap_default),
};

/* Fabric FAN hotplug default data */
static struct mlxreg_core_data mlxerg_fru_fabric_fan_hotplug_items_data[] = {
	{
		.label = "fan1",
		.reg = MLXREG_FRU_REG_FAN_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXREG_FRU_NR_NONE,
	},
	{
		.label = "fan2",
		.reg = MLXREG_FRU_REG_FAN_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXREG_FRU_NR_NONE,
	},
};

/* Fabric ASIC hotplug default data */
static struct mlxreg_core_data mlxreg_fru_fabric_asic_hotplug_items_data[] = {
	{
		.label = "asic1",
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC1_MASK,
		.bit = 7,
		.hpdev.nr = MLXREG_FRU_NR_NONE,
	},
};

/* Blade ASIC hotplug default data */
static struct mlxreg_core_data mlxreg_fru_blade_asic_hotplug_items_data[] = {
	{
		.label = "asic1",
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC1_MASK,
		.bit = 7,
		.hpdev.nr = MLXREG_FRU_NR_NONE,
	},
	{
		.label = "asic2",
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC2_MASK,
		.bit = 5,
		.hpdev.nr = MLXREG_FRU_NR_NONE,
	},
};

static struct mlxreg_core_item mlxreg_fru_fabric_hotplug_items[] = {
	{
		.data = mlxerg_fru_fabric_fan_hotplug_items_data,
		.aggr_mask = MLXREG_FRU_FAN_MASK,
		.reg = MLXREG_FRU_REG_FAN_OFFSET,
		.mask = MLXREG_FRU_FAN_MASK,
		.count = ARRAY_SIZE(mlxerg_fru_fabric_fan_hotplug_items_data),
		.inversed = 1,
		.health = false,
	},
	/*{
		.data = mlxreg_fru_fabric_asic_hotplug_items_data,
		.aggr_mask = MLXREG_FRU_ASIC1_MASK,
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC1_MASK,
		.count = ARRAY_SIZE(mlxreg_fru_fabric_asic_hotplug_items_data),
		.inversed = 0,
		.health = true,
	},*/
};

static struct mlxreg_core_item mlxreg_fru_fabric200_hotplug_items[] = {
	{
		.data = mlxreg_fru_fabric_asic_hotplug_items_data,
		.aggr_mask = MLXREG_FRU_ASIC1_MASK,
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC1_MASK,
		.count = ARRAY_SIZE(mlxreg_fru_fabric_asic_hotplug_items_data),
		.inversed = 0,
		.health = true,
	},
};

static struct mlxreg_core_item mlxreg_fru_blade_hotplug_items[] = {
	{
		.data = mlxreg_fru_blade_asic_hotplug_items_data,
		.aggr_mask = MLXREG_FRU_ASIC1_MASK | MLXREG_FRU_ASIC2_MASK,
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC1_MASK | MLXREG_FRU_ASIC2_MASK,
		.count = ARRAY_SIZE(mlxreg_fru_blade_asic_hotplug_items_data),
		.inversed = 0,
		.health = true,
	},
};

static bool
mlxreg_fru_presence(struct mlxreg_core_hotplug_platform_data *data)
{
	void __iomem *base = mlxreg_core_get_io_context();
	u32 devnum = data->devnum;
	u32 off, bit, regval;

	off = (devnum / MLXREG_FRU_FAB_REG_STEP) *
	      MLXREG_FRU_FAB_PRESENCE_REG_SHIFT + data->presence_reg_base;
	bit = devnum % MLXREG_FRU_FAB_REG_STEP;

	regval = ioread8(base + off);

	return !(regval & BIT(bit));
}

static bool
mlxreg_fru_wakeup_signal(struct mlxreg_core_hotplug_platform_data *data)
{
	void __iomem *base = mlxreg_core_get_io_context();
	u32 devnum = data->devnum;
	u8 off, bit, regval;

	off = (devnum / MLXREG_FRU_FAB_REG_STEP) *
	      MLXREG_FRU_FAB_WAKEUP_SIGNAL_REG_SHIFT + data->wakeup_signal_reg_base;
	bit = devnum % MLXREG_FRU_FAB_REG_STEP;
	regval = ioread8(base + off);

	return regval & BIT(bit);
}

static void
mlxreg_fru_wakeup_signal_clear(struct mlxreg_core_hotplug_platform_data *data)
{
	void __iomem *base = mlxreg_core_get_io_context();
	u32 devnum = data->devnum;
	u8 off, bit, regval;

	off = (devnum / MLXREG_FRU_FAB_REG_STEP) *
	     MLXREG_FRU_FAB_WAKEUP_SIGNAL_REG_SHIFT + data->wakeup_signal_reg_base;
	bit = devnum % MLXREG_FRU_FAB_REG_STEP;
	regval = ~BIT(bit);
	iowrite8(regval, base + off);
}

static
struct mlxreg_core_hotplug_platform_data mlxreg_fru_fabric_hotplug_data = {
	.items = mlxreg_fru_fabric_hotplug_items,
	.counter = ARRAY_SIZE(mlxreg_fru_fabric_hotplug_items),
	.deferred_irq_set = true,
	.presence = mlxreg_fru_presence,
	.wakeup_signal = mlxreg_fru_wakeup_signal,
	.wakeup_signal_clear = mlxreg_fru_wakeup_signal_clear,
	.presence_reg_base = MLXREG_FRU_FAB_PRESENCE_REG_BASE,
	.wakeup_signal_reg_base = MLXREG_FRU_FAB_WAKEUP_SIGNAL_REG_BASE,
};

static
struct mlxreg_core_hotplug_platform_data mlxreg_fru_fabric200_hotplug_data = {
	.items = mlxreg_fru_fabric200_hotplug_items,
	.counter = ARRAY_SIZE(mlxreg_fru_fabric200_hotplug_items),
	.deferred_irq_set = true,
};

static
struct mlxreg_core_hotplug_platform_data mlxreg_fru_blade_hotplug_data = {
	.items = mlxreg_fru_blade_hotplug_items,
	.counter = ARRAY_SIZE(mlxreg_fru_blade_hotplug_items),
	.deferred_irq_set = true,
};

/* LED default data */
static struct mlxreg_core_data mlxreg_fru_default_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXREG_FRU_REG_LED1_OFFSET,
		.mask = MLXREG_FRU_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:red",
		.reg = MLXREG_FRU_REG_LED1_OFFSET,
		.mask = MLXREG_FRU_LED_LO_NIBBLE_MASK
	},
};

static struct mlxreg_core_platform_data mlxreg_fru_default_led = {
		.data = mlxreg_fru_default_led_data,
		.counter = ARRAY_SIZE(mlxreg_fru_default_led_data),
};

/* LED extended data */
static struct mlxreg_core_data mlxreg_fru_extended_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXREG_FRU_REG_LED1_OFFSET,
		.mask = MLXREG_FRU_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:red",
		.reg = MLXREG_FRU_REG_LED1_OFFSET,
		.mask = MLXREG_FRU_LED_LO_NIBBLE_MASK
	},
	{
		.label = "fan1:green",
		.reg = MLXREG_FRU_REG_LED2_OFFSET,
		.mask = MLXREG_FRU_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan1:red",
		.reg = MLXREG_FRU_REG_LED2_OFFSET,
		.mask = MLXREG_FRU_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan2:green",
		.reg = MLXREG_FRU_REG_LED2_OFFSET,
		.mask = MLXREG_FRU_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan2:red",
		.reg = MLXREG_FRU_REG_LED2_OFFSET,
		.mask = MLXREG_FRU_LED_HI_NIBBLE_MASK,
	},
};

static struct mlxreg_core_platform_data mlxreg_fru_extended_led = {
		.data = mlxreg_fru_extended_led_data,
		.counter = ARRAY_SIZE(mlxreg_fru_extended_led_data),
};

/* Default register access data */
static struct mlxreg_core_data mlxreg_fru_regs_io_data[] = {
	{
		.label = "cpld1_version",
		.reg = MLXREG_FRU_REG_CPLD1_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "pwr",
		.reg = MLXREG_FRU_REG_GP2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0644,
	},
	{
		.label = "reset_pwr_off_or_upgrade",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0444,
	},
	{
		.label = "reset_asic2_pwr_fail",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0444,
	},
	{
		.label = "reset_asic1_pwr_fail",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "reset_sw_reset",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "reset_asic2_fw",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0444,
	},
	{
		.label = "reset_asic1_fw",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_asic2_thermal",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "reset_asic1_thermal",
		.reg = MLXREG_FRU_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0444,
	},
	{
		.label = "asic1_health",
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC1_MASK,
		.bit = 7,
		.mode = 0444,
	},
	{
		.label = "asic2_health",
		.reg = MLXREG_FRU_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXREG_FRU_ASIC2_MASK,
		.bit = 5,
		.mode = 0444,
	},
};

static struct mlxreg_core_platform_data mlxreg_fru_regs_io = {
		.data = mlxreg_fru_regs_io_data,
		.counter = ARRAY_SIZE(mlxreg_fru_regs_io_data),
};

static int mlxreg_fru_mux_set(struct i2c_adapter *adap,
			      struct i2c_client *client, u32 chan)
{
	int ret = -ENODEV;

	if (adap->algo->master_xfer) {
		struct i2c_msg msg;
		u8 msgbuf[] = {MLXREG_FRU_CHANNEL_I2C_REG, chan};

		msg.addr = client->addr;
		msg.flags = 0;
		msg.len = 2;
		msg.buf = msgbuf;
		ret = __i2c_transfer(adap, &msg, 1);

		if (ret >= 0 && ret != 1)
			ret = -EREMOTEIO;
	} else if (adap->algo->smbus_xfer) {
		union i2c_smbus_data data;

		data.byte = chan;
		ret = adap->algo->smbus_xfer(adap, client->addr,
					     client->flags, I2C_SMBUS_WRITE,
					     MLXREG_FRU_CHANNEL_I2C_REG,
					     I2C_SMBUS_BYTE_DATA, &data);
	}

	return ret;
}

static int mlxreg_fru_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
	struct mlxreg_fru_mux_priv *mux = i2c_mux_priv(muxc);
	u8 regval = chan + 1;
	int err = 0;

	/* Only select the channel if its different from the last channel */
	if (mux->last_chan != regval) {
		err = mlxreg_fru_mux_set(muxc->parent, mux->client, regval);
		mux->last_chan = err < 0 ? 0 : regval;
	}

	return err;
}

static int mlxreg_fru_mux_deselect(struct i2c_mux_core *muxc, u32 chan)
{
	struct mlxreg_fru_mux_priv *mux = i2c_mux_priv(muxc);

	/* Deselect active channel */
	mux->last_chan = 0;

	return mlxreg_fru_mux_set(muxc->parent, mux->client, mux->last_chan);
}

static int
mlxreg_fru_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct mlxreg_core_hotplug_platform_data *hotplug = NULL;
	struct mlxreg_core_platform_data *regs_io;
	struct mlxreg_core_platform_data *led;
	struct device *dev = &client->dev;
	int channels[MLXREG_FRU_CHAN_NUM];
	struct mlxreg_fru_mux_priv *mux;
	struct mlxreg_fru_priv *data;
	void *regmap;
	int i;
	int err;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA |
				     I2C_FUNC_SMBUS_I2C_BLOCK))
		return -ENODEV;

	switch (id->driver_data) {
	case MLXREG_FRU_BLADE:
	case MLXREG_FRU_BLADE200:
		/*hotplug = &mlxreg_fru_blade_hotplug_data;*/
		regs_io = &mlxreg_fru_regs_io;
		led = &mlxreg_fru_default_led;
		break;
	case MLXREG_FRU_FABRIC200:
		/*hotplug = &mlxreg_fru_fabric200_hotplug_data;*/
		regs_io = &mlxreg_fru_regs_io;
		led = &mlxreg_fru_default_led;
		break;
	case MLXREG_FRU_FABRIC:
		hotplug = &mlxreg_fru_fabric_hotplug_data;
		regs_io = &mlxreg_fru_regs_io;
		led = &mlxreg_fru_extended_led;
		break;
	default:
		return -EINVAL;
	}

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	regmap = devm_regmap_init_i2c(client, &mlxreg_fru_regmap_conf);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	for (i = 0; i < MLXREG_FRU_CHAN_NUM; i++)
		channels[i] = MLXREG_FRU_SET_VBUS(client->adapter->nr, i);
	data->muxc = i2c_mux_alloc(adap, dev, MLXREG_FRU_CHAN_NUM,
				   sizeof(*mux), 0, mlxreg_fru_mux_select,
				   mlxreg_fru_mux_deselect);
	if (IS_ERR(data->muxc))
		return PTR_ERR(data->muxc);

	mux = i2c_mux_priv(data->muxc);
	mux->regmap = regmap;
	mux->client = client;

	/* Create an adapter for each channel. */
	for (i = 0; i < MLXREG_FRU_CHAN_NUM; i++) {
		err = i2c_mux_add_adapter(data->muxc, channels[i], i, 0);
		if (err)
			goto fail_register_mux;
	}

	if (hotplug) {
		if (client->irq)
			hotplug->irq = client->irq;
		else
			hotplug->irq = MLXREG_FRU_DEFAULT_IRQ;
		hotplug->deferred_nr = channels[MLXREG_FRU_CHAN_NUM - 1];
		hotplug->regmap = regmap;
		data->hotplug = platform_device_register_resndata(dev,
						"mlxreg-hotplug",
						client->adapter->nr, NULL, 0,
						hotplug, sizeof(*hotplug));
		if (IS_ERR(data->hotplug)) {
			err = PTR_ERR(data->hotplug);
			goto fail_hotplug_register;
		}
	}

	/* Set default registers. */
	for (i = 0; i < mlxreg_fru_regmap_conf.num_reg_defaults; i++) {
		err = regmap_write(regmap, mlxreg_fru_regmap_default[i].reg,
				   mlxreg_fru_regmap_default[i].def);
		if (err)
			goto fail_default_register_set;
	}

	/* Sync registers with hardware. */
	regcache_mark_dirty(regmap);
	err = regcache_sync(regmap);
	if (err)
		goto fail_default_register_set;

	/* Add registers io access driver. */
	if (regs_io) {
		regs_io->regmap = regmap;
		data->io_regs = platform_device_register_resndata(dev,
							"mlxreg-io",
							client->adapter->nr,
							NULL, 0, regs_io,
							sizeof(*regs_io));
		if (IS_ERR(data->io_regs)) {
			err = PTR_ERR(data->io_regs);
			goto fail_default_register_set;
		}
	}

	/* Add LED driver. */
	if (led) {
		led->regmap = regmap;
		data->led = platform_device_register_resndata(dev,
							"leds-mlxreg",
							client->adapter->nr,
							NULL, 0, led,
							sizeof(*led));
		if (IS_ERR(data->led)) {
			err = PTR_ERR(data->led);
			goto fail_default_register_led;
		}
	}

	data->client = client;
	i2c_set_clientdata(client, data);

	return 0;

fail_default_register_led:
	if (data->io_regs)
		platform_device_unregister(data->io_regs);
fail_default_register_set:
	if (data->hotplug)
		platform_device_unregister(data->hotplug);
fail_hotplug_register:
fail_register_mux:
	i2c_mux_del_adapters(data->muxc);
	return err;
}

static int mlxreg_fru_remove(struct i2c_client *client)
{
	struct mlxreg_fru_priv *data = i2c_get_clientdata(client);

	if (data->led)
		platform_device_unregister(data->led);
	if (data->io_regs)
		platform_device_unregister(data->io_regs);
	if (data->hotplug)
		platform_device_unregister(data->hotplug);
	if (data->muxc)
		i2c_mux_del_adapters(data->muxc);

	return 0;
}

static const struct i2c_device_id mlxreg_fru_id[] = {
	{ "mlxreg_fru_fabric", MLXREG_FRU_FABRIC },
	{ "mlxreg_fru_blade", MLXREG_FRU_BLADE },
	{ "mlxreg_fru_fabric200", MLXREG_FRU_FABRIC200 },
	{ "mlxreg_fru_blade200", MLXREG_FRU_BLADE200 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mlxreg_fru_id);

static struct i2c_driver mlxreg_fru_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
	    .name = "mlxreg-fru",
	},
	.probe = mlxreg_fru_probe,
	.remove = mlxreg_fru_remove,
	.id_table = mlxreg_fru_id,
};

module_i2c_driver(mlxreg_fru_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@mellanox.com>");
MODULE_DESCRIPTION("Mellanox regmap field replaceable unit control driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("mlxreg-fru");
