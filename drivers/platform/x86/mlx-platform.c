// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/*
 * Mellanox platform driver
 *
 * Copyright (C) 2016-2018 Mellanox Technologies
 * Copyright (C) 2016-2018 Vadim Pasternak <vadimp@mellanox.com>
 */

#include <asm/intel-family.h>
#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/platform_data/i2c-mux-reg.h>
#include <linux/platform_data/mlxreg.h>
#include <linux/regmap.h>

#define MLX_PLAT_DEVICE_NAME		"mlxplat"

/* LPC bus IO offsets */
#define MLXPLAT_CPLD_LPC_I2C_BASE_ADRR		0x2000
#define MLXPLAT_CPLD_LPC_REG_BASE_ADRR		0x2500
#define MLXPLAT_CPLD_LPC_REG_CPLD1_VER_OFFSET	0x00
#define MLXPLAT_CPLD_LPC_REG_CPLD2_VER_OFFSET	0x01
#define MLXPLAT_CPLD_LPC_REG_CPLD3_VER_OFFSET	0x02
#define MLXPLAT_CPLD_LPC_REG_CPLD4_VER_OFFSET	0x03
#define MLXPLAT_CPLD_LPC_REG_CPLD1_PN_OFFSET	0x04
#define MLXPLAT_CPLD_LPC_REG_CPLD2_PN_OFFSET	0x06
#define MLXPLAT_CPLD_LPC_REG_CPLD3_PN_OFFSET	0x08
#define MLXPLAT_CPLD_LPC_REG_CPLD4_PN_OFFSET	0x0a
#define MLXPLAT_CPLD_LPC_REG_PG_EVENT_OFFSET	0x12
#define MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET	0x1d
#define MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET	0x1e
#define MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET	0x1f
#define MLXPLAT_CPLD_LPC_REG_LED1_OFFSET	0x20
#define MLXPLAT_CPLD_LPC_REG_LED2_OFFSET	0x21
#define MLXPLAT_CPLD_LPC_REG_LED3_OFFSET	0x22
#define MLXPLAT_CPLD_LPC_REG_LED4_OFFSET	0x23
#define MLXPLAT_CPLD_LPC_REG_LED5_OFFSET	0x24
#define MLXPLAT_CPLD_LPC_REG_LED6_OFFSET	0x25
#define MLXPLAT_CPLD_LPC_REG_FAN_DIRECTION	0x2a
#define MLXPLAT_CPLD_LPC_REG_GP0_RO_OFFSET	0x2b
#define MLXPLAT_CPLD_LPC_REG_GP0_OFFSET		0x2e
#define MLXPLAT_CPLD_LPC_REG_MASTER_WP_OFFSET	0x2f
#define MLXPLAT_CPLD_LPC_REG_GP1_OFFSET		0x30
#define MLXPLAT_CPLD_LPC_REG_WP1_OFFSET		0x31
#define MLXPLAT_CPLD_LPC_REG_GP2_OFFSET		0x32
#define MLXPLAT_CPLD_LPC_REG_WP2_OFFSET		0x33
#define MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET	0x37
#define MLXPLAT_CPLD_LPC_REG_GEO_ADDR_OFFSET	0x39
#define MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET	0x3a
#define MLXPLAT_CPLD_LPC_REG_AGGR_MASK_OFFSET	0x3b
#define MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET	0x40
#define MLXPLAT_CPLD_LPC_REG_AGGRLO_MASK_OFFSET	0x41
#define MLXPLAT_CPLD_LPC_REG_AGGRCO_OFFSET	0x42
#define MLXPLAT_CPLD_LPC_REG_AGGRCO_MASK_OFFSET	0x43
#define MLXPLAT_CPLD_LPC_REG_AGGRHA2_OFFSET	0x44
#define MLXPLAT_CPLD_LPC_REG_AGGRHA2_MASK_OFFSET 0x45
#define MLXPLAT_CPLD_LPC_REG_AGGRCX_OFFSET	0x44
#define MLXPLAT_CPLD_LPC_REG_AGGRCX_MASK_OFFSET 0x45
#define MLXPLAT_CPLD_LPC_REG_DB_OFFSET		0x47
#define MLXPLAT_CPLD_LPC_REG_DB_EVENT_OFFSET	0x48
#define MLXPLAT_CPLD_LPC_REG_DB_MASK_OFFSET	0x49
#define MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET 0x50
#define MLXPLAT_CPLD_LPC_REG_ASIC_EVENT_OFFSET	0x51
#define MLXPLAT_CPLD_LPC_REG_ASIC_MASK_OFFSET	0x52
#define MLXPLAT_CPLD_LPC_REG_HA_OFFSET		0x53
#define MLXPLAT_CPLD_LPC_REG_HA_EVENT_OFFSET	0x54
#define MLXPLAT_CPLD_LPC_REG_HA_MASK_OFFSET	0x55
#define MLXPLAT_CPLD_LPC_REG_PSU_OFFSET		0x58
#define MLXPLAT_CPLD_LPC_REG_PSU_EVENT_OFFSET	0x59
#define MLXPLAT_CPLD_LPC_REG_PSU_MASK_OFFSET	0x5a
#define MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET	0x5b
#define MLXPLAT_CPLD_LPC_REG_PSU2_EVENT_OFFSET	0x5c
#define MLXPLAT_CPLD_LPC_REG_PSU2_MASK_OFFSET	0x5d
#define MLXPLAT_CPLD_LPC_REG_PWR_OFFSET		0x64
#define MLXPLAT_CPLD_LPC_REG_PWR_EVENT_OFFSET	0x65
#define MLXPLAT_CPLD_LPC_REG_PWR_MASK_OFFSET	0x66
#define MLXPLAT_CPLD_LPC_REG_PWR2_OFFSET	0x67
#define MLXPLAT_CPLD_LPC_REG_PWR2_EVENT_OFFSET	0x68
#define MLXPLAT_CPLD_LPC_REG_PWR2_MASK_OFFSET	0x69
#define MLXPLAT_CPLD_LPC_REG_BL1_OFFSET		0x70
#define MLXPLAT_CPLD_LPC_REG_BL1_EVENT_OFFSET	0x71
#define MLXPLAT_CPLD_LPC_REG_BL1_MASK_OFFSET	0x72
#define MLXPLAT_CPLD_LPC_REG_BL2_OFFSET		0x73
#define MLXPLAT_CPLD_LPC_REG_BL2_EVENT_OFFSET	0x74
#define MLXPLAT_CPLD_LPC_REG_BL2_MASK_OFFSET	0x75
#define MLXPLAT_CPLD_LPC_REG_BL3_OFFSET		0x76
#define MLXPLAT_CPLD_LPC_REG_BL3_EVENT_OFFSET	0x77
#define MLXPLAT_CPLD_LPC_REG_BL3_MASK_OFFSET	0x78
#define MLXPLAT_CPLD_LPC_REG_HA1_OFFSET		0x7c
#define MLXPLAT_CPLD_LPC_REG_HA2_OFFSET		0x7d
#define MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET	0x7f
#define MLXPLAT_CPLD_LPC_REG_FAB1_EVENT_OFFSET	0x80
#define MLXPLAT_CPLD_LPC_REG_FAB1_MASK_OFFSET	0x81
#define MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET	0x82
#define MLXPLAT_CPLD_LPC_REG_FAB2_EVENT_OFFSET	0x83
#define MLXPLAT_CPLD_LPC_REG_FAB2_MASK_OFFSET	0x84
#define MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET	0x85
#define MLXPLAT_CPLD_LPC_REG_FAB3_EVENT_OFFSET	0x86
#define MLXPLAT_CPLD_LPC_REG_FAB3_MASK_OFFSET	0x87
#define MLXPLAT_CPLD_LPC_REG_FAN_OFFSET		0x88
#define MLXPLAT_CPLD_LPC_REG_FAN_EVENT_OFFSET	0x89
#define MLXPLAT_CPLD_LPC_REG_FAN_MASK_OFFSET	0x8a
#define MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET	0x8b
#define MLXPLAT_CPLD_LPC_REG_FAN2_EVENT_OFFSET	0x8c
#define MLXPLAT_CPLD_LPC_REG_FAN2_MASK_OFFSET	0x8d
#define MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET	0x8e
#define MLXPLAT_CPLD_LPC_REG_FAN3_EVENT_OFFSET	0x8f
#define MLXPLAT_CPLD_LPC_REG_FAN3_MASK_OFFSET	0x90
#define MLXPLAT_CPLD_LPC_REG_BLI1_OFFSET	0x9b
#define MLXPLAT_CPLD_LPC_REG_BLI1_MASK_OFFSET	0x9c
#define MLXPLAT_CPLD_LPC_REG_BLI2_OFFSET	0x9e
#define MLXPLAT_CPLD_LPC_REG_BLI2_MASK_OFFSET	0x9f
#define MLXPLAT_CPLD_LPC_REG_BLI3_OFFSET	0xa1
#define MLXPLAT_CPLD_LPC_REG_BLI3_MASK_OFFSET	0xa2
#define MLXPLAT_CPLD_LPC_REG_FABI1_OFFSET	0xaa
#define MLXPLAT_CPLD_LPC_REG_FABI1_MASK_OFFSET	0xab
#define MLXPLAT_CPLD_LPC_REG_FABI2_OFFSET	0xad
#define MLXPLAT_CPLD_LPC_REG_FABI2_MASK_OFFSET	0xae
#define MLXPLAT_CPLD_LPC_REG_FABI3_OFFSET	0xb0
#define MLXPLAT_CPLD_LPC_REG_FABI3_MASK_OFFSET	0xb1
#define MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET	0xb2
#define MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET	0xb3
#define MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET	0xb4
#define MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET	0xb7
#define MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET	0xb8
#define MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET	0xb9
#define MLXPLAT_CPLD_LPC_REG_CHAS_TYPE_OFFSET	0xbd
#define MLXPLAT_CPLD_LPC_REG_WD_CLEAR_OFFSET	0xc7
#define MLXPLAT_CPLD_LPC_REG_WD_CLEAR_WP_OFFSET	0xc8
#define MLXPLAT_CPLD_LPC_REG_WD1_TMR_OFFSET	0xc9
#define MLXPLAT_CPLD_LPC_REG_WD1_ACT_OFFSET	0xcb
#define MLXPLAT_CPLD_LPC_REG_WD2_TMR_OFFSET	0xcd
#define MLXPLAT_CPLD_LPC_REG_WD2_TLEFT_OFFSET	0xce
#define MLXPLAT_CPLD_LPC_REG_WD2_ACT_OFFSET	0xcf
#define MLXPLAT_CPLD_LPC_REG_WD3_TMR_OFFSET	0xd1
#define MLXPLAT_CPLD_LPC_REG_WD3_TLEFT_OFFSET	0xd2
#define MLXPLAT_CPLD_LPC_REG_WD3_ACT_OFFSET	0xd3
#define MLXPLAT_CPLD_LPC_REG_CPLD1_MVER_OFFSET	0xde
#define MLXPLAT_CPLD_LPC_REG_CPLD2_MVER_OFFSET	0xdf
#define MLXPLAT_CPLD_LPC_REG_CPLD3_MVER_OFFSET	0xe0
#define MLXPLAT_CPLD_LPC_REG_CPLD4_MVER_OFFSET	0xe1
#define MLXPLAT_CPLD_LPC_REG_UFM_VERSION_OFFSET	0xe2
#define MLXPLAT_CPLD_LPC_REG_PWM1_OFFSET	0xe3
#define MLXPLAT_CPLD_LPC_REG_TACHO1_OFFSET	0xe4
#define MLXPLAT_CPLD_LPC_REG_TACHO2_OFFSET	0xe5
#define MLXPLAT_CPLD_LPC_REG_TACHO3_OFFSET	0xe6
#define MLXPLAT_CPLD_LPC_REG_TACHO4_OFFSET	0xe7
#define MLXPLAT_CPLD_LPC_REG_TACHO5_OFFSET	0xe8
#define MLXPLAT_CPLD_LPC_REG_TACHO6_OFFSET	0xe9
#define MLXPLAT_CPLD_LPC_REG_TACHO7_OFFSET	0xeb
#define MLXPLAT_CPLD_LPC_REG_TACHO8_OFFSET	0xec
#define MLXPLAT_CPLD_LPC_REG_TACHO9_OFFSET	0xed
#define MLXPLAT_CPLD_LPC_REG_TACHO10_OFFSET	0xee
#define MLXPLAT_CPLD_LPC_REG_TACHO11_OFFSET	0xef
#define MLXPLAT_CPLD_LPC_REG_TACHO12_OFFSET	0xf0
#define MLXPLAT_CPLD_LPC_REG_TACHO13_OFFSET	0xf1
#define MLXPLAT_CPLD_LPC_REG_TACHO14_OFFSET	0xf2
#define MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET	0xf5
#define MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET	0xf6
#define MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET	0xf7
#define MLXPLAT_CPLD_LPC_REG_TACHO_SPEED_OFFSET	0xf8
#define MLXPLAT_CPLD_LPC_REG_PSU_I2C_CAP_OFFSET 0xf9
#define MLXPLAT_CPLD_LPC_REG_CONFIG1_OFFSET	0xfb
#define MLXPLAT_CPLD_LPC_REG_CONFIG2_OFFSET	0xfc
#define MLXPLAT_CPLD_LPC_IO_RANGE		0x100
#define MLXPLAT_CPLD_LPC_I2C_CH1_OFF		0xdb
#define MLXPLAT_CPLD_LPC_I2C_CH2_OFF		0xda
#define MLXPLAT_CPLD_LPC_I2C_CH3_OFF		0xdc

#define MLXPLAT_CPLD_LPC_PIO_OFFSET		0x10000UL
#define MLXPLAT_CPLD_LPC_REG1	((MLXPLAT_CPLD_LPC_REG_BASE_ADRR + \
				  MLXPLAT_CPLD_LPC_I2C_CH1_OFF) | \
				  MLXPLAT_CPLD_LPC_PIO_OFFSET)
#define MLXPLAT_CPLD_LPC_REG2	((MLXPLAT_CPLD_LPC_REG_BASE_ADRR + \
				  MLXPLAT_CPLD_LPC_I2C_CH2_OFF) | \
				  MLXPLAT_CPLD_LPC_PIO_OFFSET)
#define MLXPLAT_CPLD_LPC_REG3	((MLXPLAT_CPLD_LPC_REG_BASE_ADRR + \
				  MLXPLAT_CPLD_LPC_I2C_CH3_OFF) | \
				  MLXPLAT_CPLD_LPC_PIO_OFFSET)

/* Masks for aggregation, psu, pwr and fan event in CPLD related registers. */
#define MLXPLAT_CPLD_AGGR_ASIC_MASK_DEF	0x04
#define MLXPLAT_CPLD_AGGR_PSU_MASK_DEF	0x08
#define MLXPLAT_CPLD_AGGR_PWR_MASK_DEF	0x08
#define MLXPLAT_CPLD_AGGR_FAN_MASK_DEF	0x40
#define MLXPLAT_CPLD_AGGR_MASK_DEF	(MLXPLAT_CPLD_AGGR_ASIC_MASK_DEF | \
					 MLXPLAT_CPLD_AGGR_PSU_MASK_DEF | \
					 MLXPLAT_CPLD_AGGR_FAN_MASK_DEF)
#define MLXPLAT_CPLD_AGGR_ASIC_MASK_NG	0x01
#define MLXPLAT_CPLD_AGGR_MASK_NG_DEF	0x04
#define MLXPLAT_CPLD_AGGR_MASK_COMEX	BIT(0)
#define MLXPLAT_CPLD_LOW_AGGR_MASK_LOW	0xc1
#define MLXPLAT_CPLD_LOW_AGGR_MASK_I2C	BIT(6)
#define MLXPLAT_CPLD_AGGR_BFF_MASK_MOD	BIT(0)
#define MLXPLAT_CPLD_AGGR_PSU_MASK_MOD	BIT(1)
#define MLXPLAT_CPLD_AGGR_EXTINT_MASK_MOD BIT(2)
#define MLXPLAT_CPLD_AGGR_MASK_MODULAR	(MLXPLAT_CPLD_AGGR_BFF_MASK_MOD | \
					 MLXPLAT_CPLD_AGGR_PSU_MASK_MOD | \
					 MLXPLAT_CPLD_AGGR_EXTINT_MASK_MOD)
#define MLXPLAT_CPLD_LOW_AGGR_MASK_MOD	GENMASK(5, 0)
#define MLXPLAT_CPLD_PSU_MASK		GENMASK(1, 0)
#define MLXPLAT_CPLD_PWR_MASK		GENMASK(1, 0)
#define MLXPLAT_CPLD_PSU_EXT_MASK	GENMASK(3, 0)
#define MLXPLAT_CPLD_PWR_EXT_MASK	GENMASK(3, 0)
#define MLXPLAT_CPLD_FAN_MASK		GENMASK(3, 0)
#define MLXPLAT_CPLD_ASIC_MASK		GENMASK(1, 0)
#define MLXPLAT_CPLD_FAN_NG_MASK	GENMASK(6, 0)
#define MLXPLAT_CPLD_FAN_QMB8700_MASK	GENMASK(5, 0)
#define MLXPLAT_CPLD_MODULAR_MASK	GENMASK(7, 0)
#define MLXPLAT_CPLD_HA_MASK		BIT(6)
#define MLXPLAT_CPLD_LED_LO_NIBBLE_MASK	GENMASK(7, 4)
#define MLXPLAT_CPLD_LED_HI_NIBBLE_MASK	GENMASK(3, 0)
#define MLXPLAT_CPLD_VOLTREG_UPD_MASK	GENMASK(5, 4)
#define MLXPLAT_CPLD_I2C_CAP_BIT	0x04
#define MLXPLAT_CPLD_I2C_CAP_MASK	GENMASK(5, MLXPLAT_CPLD_I2C_CAP_BIT)

/* Masks for aggregation for comex carriers */
#define MLXPLAT_CPLD_AGGR_MASK_CARRIER	BIT(1)
#define MLXPLAT_CPLD_AGGR_MASK_CARR_DEF	(MLXPLAT_CPLD_AGGR_ASIC_MASK_DEF | \
					 MLXPLAT_CPLD_AGGR_MASK_CARRIER)
#define MLXPLAT_CPLD_LOW_AGGRCX_MASK	0xc1

/* Default I2C parent bus number */
#define MLXPLAT_CPLD_PHYS_ADAPTER_DEF_NR	1

/* Maximum number of possible physical buses equipped on system */
#define MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM	16
#define MLXPLAT_CPLD_MAX_PHYS_EXT_ADAPTER_NUM	24

/* Last mux number on modular system */
#define MLXPLAT_CPLD_MODULAR_MUX_LAST		113

/* Number of channels in group */
#define MLXPLAT_CPLD_DEFAULT_GRP_NUM		2
#define MLXPLAT_CPLD_GRP_CHNL_NUM		8
#define MLXPLAT_CPLD_EXT_GRP_CHNL_NUM		24

/* Start channel numbers */
#define MLXPLAT_CPLD_CH1			2
#define MLXPLAT_CPLD_CH2			10
#define MLXPLAT_CPLD_CH3			18
#define MLXPLAT_CPLD_CH3_1			42
#define MLXPLAT_CPLD_CH3_2			66
#define MLXPLAT_CPLD_CH3_3			90

/* Number of LPC attached MUX platform devices */
#define MLXPLAT_CPLD_LPC_MUX_DEVS_DEFAULT	2
#define MLXPLAT_CPLD_LPC_MUX_DEVS_MAX		6

/* Hotplug devices adapter numbers */
#define MLXPLAT_CPLD_NR_NONE			-1
#define MLXPLAT_CPLD_PSU_DEFAULT_NR		10
#define MLXPLAT_CPLD_PSU_MSNXXXX_NR		4
#define MLXPLAT_CPLD_PSU_MSNXXXX_NR2		3
#define MLXPLAT_CPLD_FAN1_DEFAULT_NR		11
#define MLXPLAT_CPLD_FAN2_DEFAULT_NR		12
#define MLXPLAT_CPLD_FAN3_DEFAULT_NR		13
#define MLXPLAT_CPLD_FAN4_DEFAULT_NR		14

#define MLXPLAT_CPLD_LPC_REG_GEO_ADDR_VALUE1	0x25
#define MLXPLAT_CPLD_LPC_REG_GEO_ADDR_VALUE2	0x26

/* Masks and default values for watchdogs */
#define MLXPLAT_CPLD_WD1_CLEAR_MASK	GENMASK(7, 1)
#define MLXPLAT_CPLD_WD2_CLEAR_MASK	(GENMASK(7, 0) & ~BIT(1))

#define MLXPLAT_CPLD_WD_TYPE1_TO_MASK	GENMASK(7, 4)
#define MLXPLAT_CPLD_WD_TYPE2_TO_MASK	0
#define MLXPLAT_CPLD_WD_RESET_ACT_MASK	GENMASK(7, 1)
#define MLXPLAT_CPLD_WD_FAN_ACT_MASK	(GENMASK(7, 0) & ~BIT(4))
#define MLXPLAT_CPLD_WD_COUNT_ACT_MASK	(GENMASK(7, 0) & ~BIT(7))
#define MLXPLAT_CPLD_WD_DFLT_TIMEOUT	30
#define MLXPLAT_CPLD_WD_MAX_DEVS	2

/* mlxplat_priv - platform private data
 * @pdev_i2c - i2c controller platform device
 * @pdev_mux - array of mux platform devices
 * @pdev_hotplug - hotplug platform devices
 * @pdev_led - led platform devices
 * @pdev_io_regs - register access platform devices
 * @pdev_fan - FAN platform devices
 * @pdev_wd - array of watchdog platform devices
 * @regmap: device register map
 */
struct mlxplat_priv {
	struct platform_device *pdev_i2c;
	struct platform_device *pdev_mux[MLXPLAT_CPLD_LPC_MUX_DEVS_MAX];
	struct platform_device *pdev_hotplug;
	struct platform_device *pdev_led;
	struct platform_device *pdev_io_regs;
	struct platform_device *pdev_fan;
	struct platform_device *pdev_wd[MLXPLAT_CPLD_WD_MAX_DEVS];
	void *regmap;
};

/* Regions for LPC I2C controller and LPC base register space */
static const struct resource mlxplat_lpc_resources[] = {
	[0] = DEFINE_RES_NAMED(MLXPLAT_CPLD_LPC_I2C_BASE_ADRR,
			       MLXPLAT_CPLD_LPC_IO_RANGE,
			       "mlxplat_cpld_lpc_i2c_ctrl", IORESOURCE_IO),
	[1] = DEFINE_RES_NAMED(MLXPLAT_CPLD_LPC_REG_BASE_ADRR,
			       MLXPLAT_CPLD_LPC_IO_RANGE,
			       "mlxplat_cpld_lpc_regs",
			       IORESOURCE_IO),
};

/* Platform i2c next generation systems data */
static struct mlxreg_core_data mlxplat_mlxcpld_i2c_ng_items_data[] = {
	{
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_I2C_CAP_OFFSET,
		.mask = MLXPLAT_CPLD_I2C_CAP_MASK,
		.bit = MLXPLAT_CPLD_I2C_CAP_BIT,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_i2c_ng_items[] = {
	{
		.data = mlxplat_mlxcpld_i2c_ng_items_data,
	},
};

/* Platform next generation systems i2c data */
static struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_i2c_ng_data = {
	.items = mlxplat_mlxcpld_i2c_ng_items,
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_COMEX,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRCO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_I2C,
};

/* Platform modular's system channels number per mux object */
static const int mlxplat_modular_chan_num[] = {
	MLXPLAT_CPLD_GRP_CHNL_NUM,
	MLXPLAT_CPLD_GRP_CHNL_NUM,
	MLXPLAT_CPLD_EXT_GRP_CHNL_NUM,
	MLXPLAT_CPLD_EXT_GRP_CHNL_NUM,
	MLXPLAT_CPLD_EXT_GRP_CHNL_NUM,
	MLXPLAT_CPLD_EXT_GRP_CHNL_NUM,
};

/* Platform default channels */
static const int mlxplat_default_channels[][MLXPLAT_CPLD_GRP_CHNL_NUM] = {
	{
		MLXPLAT_CPLD_CH1, MLXPLAT_CPLD_CH1 + 1, MLXPLAT_CPLD_CH1 + 2,
		MLXPLAT_CPLD_CH1 + 3, MLXPLAT_CPLD_CH1 + 4, MLXPLAT_CPLD_CH1 +
		5, MLXPLAT_CPLD_CH1 + 6, MLXPLAT_CPLD_CH1 + 7
	},
	{
		MLXPLAT_CPLD_CH2, MLXPLAT_CPLD_CH2 + 1, MLXPLAT_CPLD_CH2 + 2,
		MLXPLAT_CPLD_CH2 + 3, MLXPLAT_CPLD_CH2 + 4, MLXPLAT_CPLD_CH2 +
		5, MLXPLAT_CPLD_CH2 + 6, MLXPLAT_CPLD_CH2 + 7
	},
};

/* Platform channels for MSN21xx system family */
static const int mlxplat_msn21xx_channels[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

static const int mlxplat_modular_channels[][MLXPLAT_CPLD_EXT_GRP_CHNL_NUM] = {
	{
		1, 2, 3, 4, 5, 6, 7, 8
	},
	{
		MLXPLAT_CPLD_CH2, MLXPLAT_CPLD_CH2 + 1, MLXPLAT_CPLD_CH2 + 2,
		MLXPLAT_CPLD_CH2 + 3, MLXPLAT_CPLD_CH2 + 4, MLXPLAT_CPLD_CH2 +
		5, MLXPLAT_CPLD_CH2 + 6, MLXPLAT_CPLD_CH2 + 7
	},
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24
	},
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24
	},
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24
	},
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24
	},
};

static const int
mlxplat_modular200_channels[][MLXPLAT_CPLD_EXT_GRP_CHNL_NUM] = {
{
	1, 2, 3, 4, 5, 6, 7, 8
},
{
	1, 2, 3, 4, 5, 6, 7, 8
},
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 23, 24
},
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 23, 24
},
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 23, 24
},
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 23, 24
},
};

/* Platform mux data */
static struct i2c_mux_reg_platform_data mlxplat_default_mux_data[] = {
	{
		.parent = 1,
		.base_nr = MLXPLAT_CPLD_CH1,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG1,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 1,
		.base_nr = MLXPLAT_CPLD_CH2,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG2,
		.reg_size = 1,
		.idle_in_use = 1,
	},
};

/* Platform mux data */
static struct i2c_mux_reg_platform_data mlxplat_modular_mux_data[] = {
	{
		.parent = 1,
		.base_nr = MLXPLAT_CPLD_CH1,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG1,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 1,
		.base_nr = MLXPLAT_CPLD_CH2,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG2,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 2,
		.base_nr = MLXPLAT_CPLD_CH3,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG3,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 3,
		.base_nr = MLXPLAT_CPLD_CH3_1,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG3,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 4,
		.base_nr = MLXPLAT_CPLD_CH3_2,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG3,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 7,
		.base_nr = MLXPLAT_CPLD_CH3_3,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG3,
		.reg_size = 1,
		.idle_in_use = 1,
	},
};

/* Platform mux configuration variables */
static int mlxplat_max_adap_num;
static int mlxplat_mux_num;
static struct i2c_mux_reg_platform_data *mlxplat_mux_data;

/* Platform extended mux data */
static struct i2c_mux_reg_platform_data mlxplat_extended_mux_data[] = {
	{
		.parent = 1,
		.base_nr = MLXPLAT_CPLD_CH1,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG1,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 1,
		.base_nr = MLXPLAT_CPLD_CH2,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG3,
		.reg_size = 1,
		.idle_in_use = 1,
	},
	{
		.parent = 1,
		.base_nr = MLXPLAT_CPLD_CH3,
		.write_only = 1,
		.reg = (void __iomem *)MLXPLAT_CPLD_LPC_REG2,
		.reg_size = 1,
		.idle_in_use = 1,
	},

};

/* Platform hotplug devices */
static struct i2c_board_info mlxplat_mlxcpld_psu[] = {
	{
		I2C_BOARD_INFO("24c02", 0x51),
	},
	{
		I2C_BOARD_INFO("24c02", 0x50),
	},
};

static struct i2c_board_info mlxplat_mlxcpld_pwr[] = {
	{
		I2C_BOARD_INFO("dps460", 0x59),
	},
	{
		I2C_BOARD_INFO("dps460", 0x58),
	},
};

static struct i2c_board_info mlxplat_mlxcpld_fan[] = {
	{
		I2C_BOARD_INFO("24c32", 0x50),
	},
	{
		I2C_BOARD_INFO("24c32", 0x50),
	},
	{
		I2C_BOARD_INFO("24c32", 0x50),
	},
	{
		I2C_BOARD_INFO("24c32", 0x50),
	},
};

/* Platform hotplug comex carrier system family data */
static struct mlxreg_core_data mlxplat_mlxcpld_comex_psu_items_data[] = {
	{
		.label = "psu1",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu2",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

/* Platform hotplug default data */
static struct mlxreg_core_data mlxplat_mlxcpld_default_psu_items_data[] = {
	{
		.label = "psu1",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(0),
		.hpdev.brdinfo = &mlxplat_mlxcpld_psu[0],
		.hpdev.nr = MLXPLAT_CPLD_PSU_DEFAULT_NR,
	},
	{
		.label = "psu2",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(1),
		.hpdev.brdinfo = &mlxplat_mlxcpld_psu[1],
		.hpdev.nr = MLXPLAT_CPLD_PSU_DEFAULT_NR,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_default_pwr_items_data[] = {
	{
		.label = "pwr1",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(0),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[0],
		.hpdev.nr = MLXPLAT_CPLD_PSU_DEFAULT_NR,
	},
	{
		.label = "pwr2",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(1),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[1],
		.hpdev.nr = MLXPLAT_CPLD_PSU_DEFAULT_NR,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_default_fan_items_data[] = {
	{
		.label = "fan1",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(0),
		.hpdev.brdinfo = &mlxplat_mlxcpld_fan[0],
		.hpdev.nr = MLXPLAT_CPLD_FAN1_DEFAULT_NR,
	},
	{
		.label = "fan2",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(1),
		.hpdev.brdinfo = &mlxplat_mlxcpld_fan[1],
		.hpdev.nr = MLXPLAT_CPLD_FAN2_DEFAULT_NR,
	},
	{
		.label = "fan3",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(2),
		.hpdev.brdinfo = &mlxplat_mlxcpld_fan[2],
		.hpdev.nr = MLXPLAT_CPLD_FAN3_DEFAULT_NR,
	},
	{
		.label = "fan4",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(3),
		.hpdev.brdinfo = &mlxplat_mlxcpld_fan[3],
		.hpdev.nr = MLXPLAT_CPLD_FAN4_DEFAULT_NR,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_default_asic_items_data[] = {
	{
		.label = "asic1",
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_default_items[] = {
	{
		.data = mlxplat_mlxcpld_default_psu_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_PSU_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = MLXPLAT_CPLD_PSU_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_psu),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_PWR_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_pwr),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_fan_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_FAN_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = MLXPLAT_CPLD_FAN_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_fan),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_ASIC_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_comex_items[] = {
	{
		.data = mlxplat_mlxcpld_comex_psu_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_CARRIER,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = MLXPLAT_CPLD_PSU_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_psu),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_CARRIER,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_pwr),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_fan_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_CARRIER,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = MLXPLAT_CPLD_FAN_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_fan),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_ASIC_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_default_data = {
	.items = mlxplat_mlxcpld_default_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_default_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_DEF,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_LOW,
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_comex_data = {
	.items = mlxplat_mlxcpld_comex_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_comex_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_CARR_DEF,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRCX_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGRCX_MASK,
};

static struct mlxreg_core_data mlxplat_mlxcpld_msn21xx_pwr_items_data[] = {
	{
		.label = "pwr1",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "pwr2",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_default_db_items_data[] = {
	{
		.label = "holder1",
		.reg = MLXPLAT_CPLD_LPC_REG_DB_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "holder2",
		.reg = MLXPLAT_CPLD_LPC_REG_DB_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "doorbell",
		.reg = MLXPLAT_CPLD_LPC_REG_DB_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

/* Platform hotplug MSN21xx system family data */
static struct mlxreg_core_item mlxplat_mlxcpld_msn21xx_items[] = {
	{
		.data = mlxplat_mlxcpld_msn21xx_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_PWR_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_msn21xx_pwr_items_data),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_ASIC_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
	{
		.data = mlxplat_mlxcpld_default_db_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_DB_OFFSET,
		.mask = BIT(2),
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_db_items_data),
		.inversed = 1,
		.health = false,
	},
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_msn21xx_data = {
	.items = mlxplat_mlxcpld_msn21xx_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_msn21xx_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_DEF,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_LOW,
};

/* Platform hotplug msn274x system family data */
static struct mlxreg_core_data mlxplat_mlxcpld_msn274x_psu_items_data[] = {
	{
		.label = "psu1",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(0),
		.hpdev.brdinfo = &mlxplat_mlxcpld_psu[0],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR,
	},
	{
		.label = "psu2",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(1),
		.hpdev.brdinfo = &mlxplat_mlxcpld_psu[1],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_default_ng_pwr_items_data[] = {
	{
		.label = "pwr1",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(0),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[0],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR,
	},
	{
		.label = "pwr2",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(1),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[1],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_msn274x_fan_items_data[] = {
	{
		.label = "fan1",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan2",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan3",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan4",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_msn274x_items[] = {
	{
		.data = mlxplat_mlxcpld_msn274x_psu_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = MLXPLAT_CPLD_PSU_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_msn274x_psu_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_ng_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_pwr_items_data),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_msn274x_fan_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = MLXPLAT_CPLD_FAN_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_msn274x_fan_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_msn274x_data = {
	.items = mlxplat_mlxcpld_msn274x_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_msn274x_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_LOW,
};

/* Platform hotplug MSN201x system family data */
static struct mlxreg_core_data mlxplat_mlxcpld_msn201x_pwr_items_data[] = {
	{
		.label = "pwr1",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "pwr2",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_msn201x_items[] = {
	{
		.data = mlxplat_mlxcpld_msn201x_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_PWR_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_msn201x_pwr_items_data),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_ASIC_MASK_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_msn201x_data = {
	.items = mlxplat_mlxcpld_msn201x_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_msn201x_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_DEF,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_LOW,
};

/* Platform hotplug next generation system family data */
static struct mlxreg_core_data mlxplat_mlxcpld_default_ng_psu_items_data[] = {
	{
		.label = "psu1",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu2",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_default_ng_fan_items_data[] = {
	{
		.label = "fan1",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan2",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(1),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan3",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(2),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan4",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(3),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan5",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(4),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan6",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(5),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan7",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(6),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_qmb8700_fan_items_data[] = {
	{
		.label = "fan1",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan2",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(1),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan3",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(2),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan4",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(3),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan5",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(4),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan6",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(5),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_default_ng_items[] = {
	{
		.data = mlxplat_mlxcpld_default_ng_psu_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = MLXPLAT_CPLD_PSU_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_psu_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_ng_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_pwr_items_data),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_ng_fan_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = MLXPLAT_CPLD_FAN_NG_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_fan_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_qmb8700_items[] = {
	{
		.data = mlxplat_mlxcpld_default_ng_psu_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = MLXPLAT_CPLD_PSU_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_psu_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_ng_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_pwr_items_data),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_qmb8700_fan_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = MLXPLAT_CPLD_FAN_QMB8700_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_qmb8700_fan_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_default_ng_data = {
	.items = mlxplat_mlxcpld_default_ng_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF | MLXPLAT_CPLD_AGGR_MASK_COMEX,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_LOW,
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_qmb8700_data = {
	.items = mlxplat_mlxcpld_qmb8700_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_qmb8700_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF | MLXPLAT_CPLD_AGGR_MASK_COMEX,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_LOW,
};

/* Platform hotplug extended system family data */
static struct mlxreg_core_data mlxplat_mlxcpld_ext_psu_items_data[] = {
	{
		.label = "psu1",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu2",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu3",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu4",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_ext_pwr_items_data[] = {
	{
		.label = "pwr1",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(0),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[0],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR,
	},
	{
		.label = "pwr2",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(1),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[1],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR,
	},
	{
		.label = "pwr3",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(2),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[0],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR2,
	},
	{
		.label = "pwr4",
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = BIT(3),
		.hpdev.brdinfo = &mlxplat_mlxcpld_pwr[1],
		.hpdev.nr = MLXPLAT_CPLD_PSU_MSNXXXX_NR2,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_ext_items[] = {
	{
		.data = mlxplat_mlxcpld_ext_psu_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = MLXPLAT_CPLD_PSU_EXT_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_PSU_I2C_CAP_OFFSET,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_ext_psu_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_ext_pwr_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_PWR_OFFSET,
		.mask = MLXPLAT_CPLD_PWR_EXT_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_PSU_I2C_CAP_OFFSET,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_ext_pwr_items_data),
		.inversed = 0,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_ng_fan_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = MLXPLAT_CPLD_FAN_NG_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_fan_items_data),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_ext_data = {
	.items = mlxplat_mlxcpld_ext_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_ext_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_NG_DEF | MLXPLAT_CPLD_AGGR_MASK_COMEX,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_LOW,
};

/* Platform hotplug modular system family data */
static struct mlxreg_core_data mlxplat_mlxcpld_modular_psu_items_data1[] = {
	{
		.label = "psu1",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu2",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu3",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu4",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu5",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu6",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu7",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu8",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_psu_items_data2[] = {
	{
		.label = "psu9",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu10",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu11",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu12",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu13",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu14",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu15",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "psu16",
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_fan_items_data1[] = {
	{
		.label = "fan1",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan2",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan3",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan4",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan5",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan6",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan7",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan8",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_fan_items_data2[] = {
	{
		.label = "fan9",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan10",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan11",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan12",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan13",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan14",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan15",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan16",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_fan_items_data3[] = {
	{
		.label = "fan17",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan18",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan19",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "fan20",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_fabric_items_data1[] = {
	{
		.label = "spine1",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine2",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine3",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine4",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine5",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine6",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine7",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine8",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_fabric_items_data2[] = {
	{
		.label = "spine9",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine10",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine11",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine12",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine13",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine14",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine15",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine16",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_fabric_items_data3[] = {
	{
		.label = "spine17",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine18",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine19",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "spine20",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_blade_items_data1[] = {
	{
		.label = "leaf1",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf2",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf3",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf4",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf5",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf6",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf7",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf8",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_blade_items_data2[] = {
	{
		.label = "leaf9",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf10",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf11",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf12",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf13",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf14",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf15",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(6),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf16",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = BIT(7),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_blade_items_data3[] = {
	{
		.label = "leaf17",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf18",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf19",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "leaf20",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_modular_mgmt_items_data[] = {
	{
		.label = "holder1",
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = BIT(0),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "holder2",
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = BIT(1),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "holder3",
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = BIT(2),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "holder4",
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = BIT(3),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "holder5",
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = BIT(4),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "holder6",
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = BIT(5),
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
	{
		.label = "mgmt",
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = MLXPLAT_CPLD_HA_MASK,
		.hpdev.nr = MLXPLAT_CPLD_NR_NONE,
	},
};

static struct mlxreg_core_item mlxplat_mlxcpld_modular_items[] = {
	{
		.data = mlxplat_mlxcpld_modular_psu_items_data1,
		.aggr_mask = MLXPLAT_CPLD_AGGR_PSU_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_psu_items_data1),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_psu_items_data2,
		.aggr_mask = MLXPLAT_CPLD_AGGR_PSU_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_psu_items_data2),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_fan_items_data1,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_fan_items_data1),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_fan_items_data2,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_fan_items_data2),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_fan_items_data3,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET,
		.mask = GENMASK(3, 0),
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_fan_items_data3),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_default_asic_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_default_asic_items_data),
		.inversed = 0,
		.health = true,
	},
	{
		.data = mlxplat_mlxcpld_modular_fabric_items_data1,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_fabric_items_data1),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_fabric_items_data2,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_fabric_items_data2),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_fabric_items_data3,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET,
		.mask = GENMASK(3, 0),
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_fabric_items_data3),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_blade_items_data1,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_blade_items_data1),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_blade_items_data2,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_OFFSET,
		.mask = MLXPLAT_CPLD_MODULAR_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_blade_items_data2),
		.inversed = 1,
		.health = false,
	},
	{
		.data = mlxplat_mlxcpld_modular_blade_items_data3,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_OFFSET,
		.mask = GENMASK(3, 0),
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_blade_items_data3),
		.inversed = 1,
		.health = false,
	},
	{
		.data =  mlxplat_mlxcpld_modular_mgmt_items_data,
		.aggr_mask = MLXPLAT_CPLD_AGGR_BFF_MASK_MOD,
		.reg = MLXPLAT_CPLD_LPC_REG_HA_OFFSET,
		.mask = MLXPLAT_CPLD_HA_MASK,
		.count = ARRAY_SIZE(mlxplat_mlxcpld_modular_mgmt_items_data),
		.inversed = 1,
		.health = false,
	},
};

static
struct mlxreg_core_hotplug_platform_data mlxplat_mlxcpld_modular_data = {
	.items = mlxplat_mlxcpld_modular_items,
	.counter = ARRAY_SIZE(mlxplat_mlxcpld_modular_items),
	.cell = MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET,
	.mask = MLXPLAT_CPLD_AGGR_MASK_MODULAR,
	.cell_low = MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET,
	.mask_low = MLXPLAT_CPLD_LOW_AGGR_MASK_MOD,
};

/* Platform led default data */
static struct mlxreg_core_data mlxplat_mlxcpld_default_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK
	},
	{
		.label = "psu:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "psu:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan1:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan1:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan2:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan2:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan3:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan3:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan4:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan4:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
};

static struct mlxreg_core_platform_data mlxplat_default_led_data = {
		.data = mlxplat_mlxcpld_default_led_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_default_led_data),
};

/* Platform led MSN21xx system family data */
static struct mlxreg_core_data mlxplat_mlxcpld_msn21xx_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK
	},
	{
		.label = "fan:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "psu1:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "psu1:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "psu2:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "psu2:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "uid:blue",
		.reg = MLXPLAT_CPLD_LPC_REG_LED5_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
};

static struct mlxreg_core_platform_data mlxplat_msn21xx_led_data = {
		.data = mlxplat_mlxcpld_msn21xx_led_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_msn21xx_led_data),
};

/* Platform led for default data for 200GbE systems */
static struct mlxreg_core_data mlxplat_mlxcpld_default_ng_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK
	},
	{
		.label = "psu:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "psu:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan1:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "fan1:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "fan2:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "fan2:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "fan3:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "fan3:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "fan4:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "fan4:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "fan5:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(4),
	},
	{
		.label = "fan5:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(4),
	},
	{
		.label = "fan6:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(5),
	},
	{
		.label = "fan6:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(5),
	},
	{
		.label = "fan7:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED6_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(6),
	},
	{
		.label = "fan7:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED6_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(6),
	},
	{
		.label = "uid:blue",
		.reg = MLXPLAT_CPLD_LPC_REG_LED5_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
};

static struct mlxreg_core_platform_data mlxplat_default_ng_led_data = {
		.data = mlxplat_mlxcpld_default_ng_led_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_led_data),
};

/* Platform led data for QMB8700 system */
static struct mlxreg_core_data mlxplat_mlxcpld_qmb8700_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK
	},
	{
		.label = "psu:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "psu:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan1:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "fan1:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "fan2:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "fan2:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "fan3:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "fan3:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "fan4:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "fan4:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "fan5:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(4),
	},
	{
		.label = "fan5:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(4),
	},
	{
		.label = "fan6:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(5),
	},
	{
		.label = "fan6:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED4_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET,
		.bit = BIT(5),
	},
	{
		.label = "uid:blue",
		.reg = MLXPLAT_CPLD_LPC_REG_LED5_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
};

static struct mlxreg_core_platform_data mlxplat_qmb8700_led_data = {
		.data = mlxplat_mlxcpld_qmb8700_led_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_qmb8700_led_data),
};

/* Platform led for Comex based 100GbE systems */
static struct mlxreg_core_data mlxplat_mlxcpld_comex_100G_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK
	},
	{
		.label = "psu:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "psu:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan1:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan1:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan2:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan2:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan3:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan3:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan4:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan4:red",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "uid:blue",
		.reg = MLXPLAT_CPLD_LPC_REG_LED5_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
};

static struct mlxreg_core_platform_data mlxplat_comex_100G_led_data = {
		.data = mlxplat_mlxcpld_comex_100G_led_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_comex_100G_led_data),
};

/* Platform led for default data for modular systems */
static struct mlxreg_core_data mlxplat_mlxcpld_modular_led_data[] = {
	{
		.label = "status:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "status:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK
	},
	{
		.label = "psu:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "psu:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED1_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan1:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan1:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "fan2:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "fan2:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED2_OFFSET,
		.mask = MLXPLAT_CPLD_LED_HI_NIBBLE_MASK,
	},
	{
		.label = "master:green",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
	{
		.label = "master:orange",
		.reg = MLXPLAT_CPLD_LPC_REG_LED3_OFFSET,
		.mask = MLXPLAT_CPLD_LED_LO_NIBBLE_MASK,
	},
};

static struct mlxreg_core_platform_data mlxplat_modular_led_data = {
		.data = mlxplat_mlxcpld_modular_led_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_modular_led_data),
};

/* Platform register access default */
static struct mlxreg_core_data mlxplat_mlxcpld_default_regs_io_data[] = {
	{
		.label = "cpld1_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld1_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld2_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld1_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "reset_long_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0444,
	},
	{
		.label = "reset_short_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0444,
	},
	{
		.label = "reset_aux_pwr_or_ref",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "reset_main_pwr_fail",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "reset_sw_reset",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0444,
	},
	{
		.label = "reset_fw_reset",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_hotswap_or_wd",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "reset_asic_thermal",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0444,
	},
	{
		.label = "psu1_on",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0200,
	},
	{
		.label = "psu2_on",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0200,
	},
	{
		.label = "pwr_cycle",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0200,
	},
	{
		.label = "pwr_down",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0200,
	},
	{
		.label = "select_iio",
		.reg = MLXPLAT_CPLD_LPC_REG_GP2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0644,
	},
	{
		.label = "asic_health",
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.bit = 1,
		.mode = 0444,
	},
};

static struct mlxreg_core_platform_data mlxplat_default_regs_io_data = {
		.data = mlxplat_mlxcpld_default_regs_io_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_default_regs_io_data),
};

/* Platform register access MSN21xx, MSN201x, MSN274x systems families data */
static struct mlxreg_core_data mlxplat_mlxcpld_msn21xx_regs_io_data[] = {
	{
		.label = "cpld1_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld1_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld2_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld1_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "reset_long_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0444,
	},
	{
		.label = "reset_short_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0444,
	},
	{
		.label = "reset_aux_pwr_or_ref",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "reset_sw_reset",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "reset_main_pwr_fail",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0444,
	},
	{
		.label = "reset_asic_thermal",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_hotswap_or_halt",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "reset_sff_wd",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "psu1_on",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0200,
	},
	{
		.label = "psu2_on",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0200,
	},
	{
		.label = "pwr_cycle",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0200,
	},
	{
		.label = "pwr_down",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0200,
	},
	{
		.label = "select_iio",
		.reg = MLXPLAT_CPLD_LPC_REG_GP2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0644,
	},
	{
		.label = "asic_health",
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.bit = 1,
		.mode = 0444,
	},
};

static struct mlxreg_core_platform_data mlxplat_msn21xx_regs_io_data = {
		.data = mlxplat_mlxcpld_msn21xx_regs_io_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_msn21xx_regs_io_data),
};

/* Platform register access for modular systems families data */
static struct mlxreg_core_data mlxplat_mlxcpld_modular_regs_io_data[] = {
	{
		.label = "cpld1_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld1_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld2_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld1_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "reset_long_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0444,
	},
	{
		.label = "reset_short_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0444,
	},
	{
		.label = "reset_aux_pwr_or_ref",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "reset_main_pwr_fail",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "reset_sw_reset",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0444,
	},
	{
		.label = "reset_fw_reset",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_asic_thermal",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0444,
	},
	{
		.label = "reset_aux_pwr_off_or_upgrade",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "reset_swb_pwr_fail",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "reset_fabric",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_hotswap_or_wd",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "master_i2c",
		.reg = MLXPLAT_CPLD_LPC_REG_GP0_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "pwr_cycle",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0200,
	},
	{
		.label = "select_iio",
		.reg = MLXPLAT_CPLD_LPC_REG_GP2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0644,
	},
	{
		.label = "geo_addr",
		.reg = MLXPLAT_CPLD_LPC_REG_GEO_ADDR_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "asic_health",
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.bit = 1,
		.mode = 0444,
	},
	{
		.label = "chassis_type",
		.reg = MLXPLAT_CPLD_LPC_REG_CHAS_TYPE_OFFSET,
		.mask = GENMASK(2, 0),
		.bit = 1,
		.mode = 0444,
	},
	{
		.label = "pwr_leaf1",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf2",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf3",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf4",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf5",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf6",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf7",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf8",
		.reg = MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf9",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf10",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf11",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf12",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf13",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf14",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf15",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf16",
		.reg = MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf17",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf18",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf19",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0644,
	},
	{
		.label = "pwr_leaf20",
		.reg = MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "pwr_spine1",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0644,
	},
	{
		.label = "pwr_spine2",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0644,
	},
	{
		.label = "pwr_spine3",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0644,
	},
	{
		.label = "pwr_spine4",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "pwr_spine5",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0644,
	},
	{
		.label = "pwr_spine6",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0644,
	},
	{
		.label = "pwr_spine7",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0644,
	},
	{
		.label = "pwr_spine8",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0644,
	},
	{
		.label = "pwr_spine9",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0644,
	},
	{
		.label = "pwr_spine10",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0644,
	},
	{
		.label = "pwr_spine11",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0644,
	},
	{
		.label = "pwr_spine12",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "pwr_spine13",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0644,
	},
	{
		.label = "pwr_spine14",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0644,
	},
	{
		.label = "pwr_spine15",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0644,
	},
	{
		.label = "pwr_spine16",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0644,
	},
	{
		.label = "pwr_spine17",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0644,
	},
	{
		.label = "pwr_spine18",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0644,
	},
	{
		.label = "pwr_spine19",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0644,
	},
	{
		.label = "pwr_spine20",
		.reg = MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "other_cpu_present",
		.reg = MLXPLAT_CPLD_LPC_REG_HA1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0444,
	},
	{
		.label = "master_present_in",
		.reg = MLXPLAT_CPLD_LPC_REG_HA1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0444,
	},
	{
		.label = "ha_support_ind1_in",
		.reg = MLXPLAT_CPLD_LPC_REG_HA1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "ha_support_ind2_in",
		.reg = MLXPLAT_CPLD_LPC_REG_HA1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "ha_reset_ind1_in",
		.reg = MLXPLAT_CPLD_LPC_REG_HA1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0444,
	},
	{
		.label = "ha_reset_ind2_in",
		.reg = MLXPLAT_CPLD_LPC_REG_HA1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "pwr_on_other_cpu",
		.reg = MLXPLAT_CPLD_LPC_REG_HA2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0200,
	},
	{
		.label = "master_present_out",
		.reg = MLXPLAT_CPLD_LPC_REG_HA2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0200,
	},
	{
		.label = "ha_support_ind1_out",
		.reg = MLXPLAT_CPLD_LPC_REG_HA2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0200,
	},
	{
		.label = "ha_support_ind2_out",
		.reg = MLXPLAT_CPLD_LPC_REG_HA2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0200,
	},
	{
		.label = "ha_reset_ind1_out",
		.reg = MLXPLAT_CPLD_LPC_REG_HA2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0200,
	},
	{
		.label = "ha_reset_ind2_out",
		.reg = MLXPLAT_CPLD_LPC_REG_HA2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0200,
	},
	{
		.label = "reset_other_cpu",
		.reg = MLXPLAT_CPLD_LPC_REG_HA2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0200,
	},
};

static struct mlxreg_core_platform_data mlxplat_modular_regs_io_data = {
		.data = mlxplat_mlxcpld_modular_regs_io_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_modular_regs_io_data),
};

/* Platform register access for next generation systems families data */
static struct mlxreg_core_data mlxplat_mlxcpld_default_ng_regs_io_data[] = {
	{
		.label = "cpld1_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld3_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD3_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld4_version",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD4_VER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld1_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld2_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld3_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD3_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld4_pn",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD4_PN_OFFSET,
		.bit = GENMASK(15, 0),
		.mode = 0444,
		.regnum = 2,
	},
	{
		.label = "cpld1_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD1_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld2_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD2_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld3_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD3_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "cpld4_version_min",
		.reg = MLXPLAT_CPLD_LPC_REG_CPLD4_MVER_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "reset_long_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0444,
	},
	{
		.label = "reset_short_pb",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0444,
	},
	{
		.label = "reset_aux_pwr_or_ref",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "reset_from_comex",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0444,
	},
	{
		.label = "reset_from_asic",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_swb_wd",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "reset_asic_thermal",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(7),
		.mode = 0444,
	},
	{
		.label = "reset_comex_pwr_fail",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "reset_platform",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0444,
	},
	{
		.label = "reset_soc",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_comex_wd",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "reset_voltmon_upgrade_fail",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0444,
	},
	{
		.label = "reset_system",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0444,
	},
	{
		.label = "reset_sw_pwr_off",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0444,
	},
	{
		.label = "reset_comex_thermal",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0444,
	},
	{
		.label = "reset_reload_bios",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(5),
		.mode = 0444,
	},
	{
		.label = "reset_ac_pwr_fail",
		.reg = MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.mode = 0444,
	},
	{
		.label = "psu1_on",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(0),
		.mode = 0200,
	},
	{
		.label = "psu2_on",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(1),
		.mode = 0200,
	},
	{
		.label = "pwr_cycle",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(2),
		.mode = 0200,
	},
	{
		.label = "pwr_down",
		.reg = MLXPLAT_CPLD_LPC_REG_GP1_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0200,
	},
	{
		.label = "jtag_enable",
		.reg = MLXPLAT_CPLD_LPC_REG_GP2_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0644,
	},
	{
		.label = "asic_health",
		.reg = MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET,
		.mask = MLXPLAT_CPLD_ASIC_MASK,
		.bit = 1,
		.mode = 0444,
	},
	{
		.label = "fan_dir",
		.reg = MLXPLAT_CPLD_LPC_REG_FAN_DIRECTION,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "voltreg_update_status",
		.reg = MLXPLAT_CPLD_LPC_REG_GP0_RO_OFFSET,
		.mask = MLXPLAT_CPLD_VOLTREG_UPD_MASK,
		.bit = 5,
		.mode = 0444,
	},
	{
		.label = "vpd_wp",
		.reg = MLXPLAT_CPLD_LPC_REG_GP0_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(3),
		.mode = 0644,
	},
	{
		.label = "pcie_asic_reset_dis",
		.reg = MLXPLAT_CPLD_LPC_REG_GP0_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(4),
		.mode = 0644,
	},
	{
		.label = "config1",
		.reg = MLXPLAT_CPLD_LPC_REG_CONFIG1_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "config2",
		.reg = MLXPLAT_CPLD_LPC_REG_CONFIG2_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
	{
		.label = "ufm_version",
		.reg = MLXPLAT_CPLD_LPC_REG_UFM_VERSION_OFFSET,
		.bit = GENMASK(7, 0),
		.mode = 0444,
	},
};

static struct mlxreg_core_platform_data mlxplat_default_ng_regs_io_data = {
		.data = mlxplat_mlxcpld_default_ng_regs_io_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_default_ng_regs_io_data),
};

/* Platform FAN default */
static struct mlxreg_core_data mlxplat_mlxcpld_default_fan_data[] = {
	{
		.label = "pwm1",
		.reg = MLXPLAT_CPLD_LPC_REG_PWM1_OFFSET,
	},
	{
		.label = "tacho1",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO1_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "tacho2",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO2_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "tacho3",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO3_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "tacho4",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO4_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "tacho5",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO5_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(4),
	},
	{
		.label = "tacho6",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO6_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(5),
	},
	{
		.label = "tacho7",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO7_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(6),
	},
	{
		.label = "tacho8",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO8_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(7),
	},
	{
		.label = "tacho9",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO9_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "tacho10",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO10_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "tacho11",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO11_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "tacho12",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO12_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "tacho13",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO13_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(4),
	},
	{
		.label = "tacho14",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO14_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(5),
	},
	{
		.label = "conf",
		.capability = MLXPLAT_CPLD_LPC_REG_TACHO_SPEED_OFFSET,
	},
};

static struct mlxreg_core_platform_data mlxplat_default_fan_data = {
		.data = mlxplat_mlxcpld_default_fan_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_default_fan_data),
};

/* Platform FAN default */
static struct mlxreg_core_data mlxplat_mlxcpld_qmb8700_fan_data[] = {
	{
		.label = "pwm1",
		.reg = MLXPLAT_CPLD_LPC_REG_PWM1_OFFSET,
	},
	{
		.label = "tacho1",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO1_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "tacho2",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO2_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "tacho3",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO3_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "tacho4",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO4_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "tacho5",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO5_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(4),
	},
	{
		.label = "tacho6",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO6_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(5),
	},
	{
		.label = "tacho7",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO7_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(6),
	},
	{
		.label = "tacho8",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO8_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET,
		.bit = BIT(7),
	},
	{
		.label = "tacho9",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO9_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(0),
	},
	{
		.label = "tacho10",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO10_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(1),
	},
	{
		.label = "tacho11",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO11_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(2),
	},
	{
		.label = "tacho12",
		.reg = MLXPLAT_CPLD_LPC_REG_TACHO12_OFFSET,
		.mask = GENMASK(7, 0),
		.capability = MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET,
		.bit = BIT(3),
	},
	{
		.label = "conf",
		.capability = MLXPLAT_CPLD_LPC_REG_TACHO_SPEED_OFFSET,
	},
};

static struct mlxreg_core_platform_data mlxplat_qmb8700_fan_data = {
		.data = mlxplat_mlxcpld_qmb8700_fan_data,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_qmb8700_fan_data),
};

/* Watchdog type1: hardware implementation version1
 * (MSN2700, MSN2410, MSN2740, MSN2100 and MSN2140 systems).
 */
static struct mlxreg_core_data mlxplat_mlxcpld_wd_main_regs_type1[] = {
	{
		.label = "action",
		.reg = MLXPLAT_CPLD_LPC_REG_WD1_ACT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_RESET_ACT_MASK,
		.bit = 0,
	},
	{
		.label = "timeout",
		.reg = MLXPLAT_CPLD_LPC_REG_WD1_TMR_OFFSET,
		.mask = MLXPLAT_CPLD_WD_TYPE1_TO_MASK,
		.health_cntr = MLXPLAT_CPLD_WD_DFLT_TIMEOUT,
	},
	{
		.label = "ping",
		.reg = MLXPLAT_CPLD_LPC_REG_WD_CLEAR_OFFSET,
		.mask = MLXPLAT_CPLD_WD1_CLEAR_MASK,
		.bit = 0,
	},
	{
		.label = "reset",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.bit = 6,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_wd_aux_regs_type1[] = {
	{
		.label = "action",
		.reg = MLXPLAT_CPLD_LPC_REG_WD2_ACT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_FAN_ACT_MASK,
		.bit = 4,
	},
	{
		.label = "timeout",
		.reg = MLXPLAT_CPLD_LPC_REG_WD2_TMR_OFFSET,
		.mask = MLXPLAT_CPLD_WD_TYPE1_TO_MASK,
		.health_cntr = MLXPLAT_CPLD_WD_DFLT_TIMEOUT,
	},
	{
		.label = "ping",
		.reg = MLXPLAT_CPLD_LPC_REG_WD_CLEAR_OFFSET,
		.mask = MLXPLAT_CPLD_WD1_CLEAR_MASK,
		.bit = 1,
	},
};

static struct mlxreg_core_platform_data mlxplat_mlxcpld_wd_set_type1[] = {
	{
		.data = mlxplat_mlxcpld_wd_main_regs_type1,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_wd_main_regs_type1),
		.version = MLX_WDT_TYPE1,
		.identity = "mlx-wdt-main",
	},
	{
		.data = mlxplat_mlxcpld_wd_aux_regs_type1,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_wd_aux_regs_type1),
		.version = MLX_WDT_TYPE1,
		.identity = "mlx-wdt-aux",
	},
};

/* Watchdog type2: hardware implementation version 2
 * (all systems except (MSN2700, MSN2410, MSN2740, MSN2100 and MSN2140).
 */
static struct mlxreg_core_data mlxplat_mlxcpld_wd_main_regs_type2[] = {
	{
		.label = "action",
		.reg = MLXPLAT_CPLD_LPC_REG_WD2_ACT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_RESET_ACT_MASK,
		.bit = 0,
	},
	{
		.label = "timeout",
		.reg = MLXPLAT_CPLD_LPC_REG_WD2_TMR_OFFSET,
		.mask = MLXPLAT_CPLD_WD_TYPE2_TO_MASK,
		.health_cntr = MLXPLAT_CPLD_WD_DFLT_TIMEOUT,
	},
	{
		.label = "timeleft",
		.reg = MLXPLAT_CPLD_LPC_REG_WD2_TLEFT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_TYPE2_TO_MASK,
	},
	{
		.label = "ping",
		.reg = MLXPLAT_CPLD_LPC_REG_WD2_ACT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_RESET_ACT_MASK,
		.bit = 0,
	},
	{
		.label = "reset",
		.reg = MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET,
		.mask = GENMASK(7, 0) & ~BIT(6),
		.bit = 6,
	},
};

static struct mlxreg_core_data mlxplat_mlxcpld_wd_aux_regs_type2[] = {
	{
		.label = "action",
		.reg = MLXPLAT_CPLD_LPC_REG_WD3_ACT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_FAN_ACT_MASK,
		.bit = 4,
	},
	{
		.label = "timeout",
		.reg = MLXPLAT_CPLD_LPC_REG_WD3_TMR_OFFSET,
		.mask = MLXPLAT_CPLD_WD_TYPE2_TO_MASK,
		.health_cntr = MLXPLAT_CPLD_WD_DFLT_TIMEOUT,
	},
	{
		.label = "timeleft",
		.reg = MLXPLAT_CPLD_LPC_REG_WD3_TLEFT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_TYPE2_TO_MASK,
	},
	{
		.label = "ping",
		.reg = MLXPLAT_CPLD_LPC_REG_WD3_ACT_OFFSET,
		.mask = MLXPLAT_CPLD_WD_FAN_ACT_MASK,
		.bit = 4,
	},
};

static struct mlxreg_core_platform_data mlxplat_mlxcpld_wd_set_type2[] = {
	{
		.data = mlxplat_mlxcpld_wd_main_regs_type2,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_wd_main_regs_type2),
		.version = MLX_WDT_TYPE2,
		.identity = "mlx-wdt-main",
	},
	{
		.data = mlxplat_mlxcpld_wd_aux_regs_type2,
		.counter = ARRAY_SIZE(mlxplat_mlxcpld_wd_aux_regs_type2),
		.version = MLX_WDT_TYPE2,
		.identity = "mlx-wdt-aux",
	},
};

static bool mlxplat_mlxcpld_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MLXPLAT_CPLD_LPC_REG_LED1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED4_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED5_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED6_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP0_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_MASTER_WP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WP1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WP2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGR_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRLO_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRCO_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD_CLEAR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD_CLEAR_WP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD1_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD1_ACT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD2_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD2_ACT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD3_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD3_ACT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWM1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PG_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRHA2_MASK_OFFSET:
		return true;
	}
	return false;
}

static bool mlxplat_mlxcpld_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MLXPLAT_CPLD_LPC_REG_CPLD1_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD2_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD3_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD4_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD1_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD2_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD3_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD4_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED4_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED5_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED6_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_DIRECTION:
	case MLXPLAT_CPLD_LPC_REG_GP0_RO_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP0_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_MASTER_WP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WP1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WP2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GEO_ADDR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGR_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRLO_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRCO_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRCO_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRHA2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRHA2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CHAS_TYPE_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD_CLEAR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD_CLEAR_WP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD1_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD1_ACT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD2_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD2_TLEFT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD2_ACT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD3_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD3_TLEFT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD3_ACT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD1_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD2_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD3_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD4_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWM1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO4_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO5_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO6_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO7_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO8_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO9_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO10_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO11_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO12_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO13_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO14_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO_SPEED_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_I2C_CAP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CONFIG1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CONFIG2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_UFM_VERSION_OFFSET:
		return true;
	}
	return false;
}

static bool mlxplat_mlxcpld_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MLXPLAT_CPLD_LPC_REG_CPLD1_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD2_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD3_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD4_VER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD1_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD2_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD3_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD4_PN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_RESET_CAUSE_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_RST_CAUSE1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_RST_CAUSE2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED4_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED5_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_LED6_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_DIRECTION:
	case MLXPLAT_CPLD_LPC_REG_GP0_RO_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP0_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_MASTER_WP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GP2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_GEO_ADDR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGR_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRLO_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRLO_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRCO_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRCO_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRHA2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_AGGRHA2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_DB_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_HEALTH_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_ASIC_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWR2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_HA2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_EVENT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BLI3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI1_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI2_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FABI3_MASK_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL1_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL2_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_BL3_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB1_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB2_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAB3_PWR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CHAS_TYPE_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD2_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD2_TLEFT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD3_TMR_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_WD3_TLEFT_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD1_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD2_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD3_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CPLD4_MVER_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWM1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO3_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO4_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO5_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO6_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO7_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO8_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO9_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO10_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO11_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO12_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO13_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO14_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_CAP1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_CAP2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_FAN_DRW_CAP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_TACHO_SPEED_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_PSU_I2C_CAP_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CONFIG1_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_CONFIG2_OFFSET:
	case MLXPLAT_CPLD_LPC_REG_UFM_VERSION_OFFSET:
		return true;
	}
	return false;
}

static const struct reg_default mlxplat_mlxcpld_regmap_default[] = {
	{ MLXPLAT_CPLD_LPC_REG_WP1_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_WP2_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_AGGRCO_MASK_OFFSET, GENMASK(4, 0) },
	{ MLXPLAT_CPLD_LPC_REG_MASTER_WP_OFFSET, GENMASK(3, 1) },
	{ MLXPLAT_CPLD_LPC_REG_BLI1_MASK_OFFSET, GENMASK(7, 0) },
	{ MLXPLAT_CPLD_LPC_REG_BLI2_MASK_OFFSET, GENMASK(7, 0) },
	{ MLXPLAT_CPLD_LPC_REG_BLI3_MASK_OFFSET, GENMASK(7, 0) },
	{ MLXPLAT_CPLD_LPC_REG_FABI1_MASK_OFFSET, GENMASK(7, 0) },
	{ MLXPLAT_CPLD_LPC_REG_FABI2_MASK_OFFSET, GENMASK(7, 0) },
	{ MLXPLAT_CPLD_LPC_REG_FABI3_MASK_OFFSET, GENMASK(7, 0) },
	{ MLXPLAT_CPLD_LPC_REG_BLI1_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_BLI2_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_BLI3_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_FABI1_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_FABI2_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_FABI3_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_PG_EVENT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_BL1_EVENT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_BL2_EVENT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_BL3_EVENT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_FAB1_EVENT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_FAB2_EVENT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_FAB3_EVENT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_AGGRHA2_MASK_OFFSET, GENMASK(4, 0) },
	{ MLXPLAT_CPLD_LPC_REG_WD_CLEAR_WP_OFFSET, 0x00 },
};

static const struct reg_default mlxplat_mlxcpld_regmap_ng[] = {
	{ MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_WD_CLEAR_WP_OFFSET, 0x00 },
};

static const struct reg_default mlxplat_mlxcpld_regmap_comex_default[] = {
	{ MLXPLAT_CPLD_LPC_REG_AGGRCX_MASK_OFFSET,
	  MLXPLAT_CPLD_LOW_AGGRCX_MASK },
	{ MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET, 0x00 },
};

static const struct reg_default mlxplat_mlxcpld_regmap_ng400[] = {
	{ MLXPLAT_CPLD_LPC_REG_PWM_CONTROL_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_WD1_ACT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_WD2_ACT_OFFSET, 0x00 },
	{ MLXPLAT_CPLD_LPC_REG_WD3_ACT_OFFSET, 0x00 },
};

struct mlxplat_mlxcpld_regmap_context {
	void __iomem *base;
};

static struct mlxplat_mlxcpld_regmap_context mlxplat_mlxcpld_regmap_ctx;

void __iomem *mlxreg_core_get_io_context(void)
{
	return mlxplat_mlxcpld_regmap_ctx.base;
}
EXPORT_SYMBOL_GPL(mlxreg_core_get_io_context);

static int
mlxplat_mlxcpld_reg_read(void *context, unsigned int reg, unsigned int *val)
{
	struct mlxplat_mlxcpld_regmap_context *ctx = context;

	*val = ioread8(ctx->base + reg);
	return 0;
}

static int
mlxplat_mlxcpld_reg_write(void *context, unsigned int reg, unsigned int val)
{
	struct mlxplat_mlxcpld_regmap_context *ctx = context;

	iowrite8(val, ctx->base + reg);
	return 0;
}

static const struct regmap_config mlxplat_mlxcpld_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 255,
	.cache_type = REGCACHE_FLAT,
	.writeable_reg = mlxplat_mlxcpld_writeable_reg,
	.readable_reg = mlxplat_mlxcpld_readable_reg,
	.volatile_reg = mlxplat_mlxcpld_volatile_reg,
	.reg_defaults = mlxplat_mlxcpld_regmap_default,
	.num_reg_defaults = ARRAY_SIZE(mlxplat_mlxcpld_regmap_default),
	.reg_read = mlxplat_mlxcpld_reg_read,
	.reg_write = mlxplat_mlxcpld_reg_write,
};

static const struct regmap_config mlxplat_mlxcpld_regmap_config_ng = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 255,
	.cache_type = REGCACHE_FLAT,
	.writeable_reg = mlxplat_mlxcpld_writeable_reg,
	.readable_reg = mlxplat_mlxcpld_readable_reg,
	.volatile_reg = mlxplat_mlxcpld_volatile_reg,
	.reg_defaults = mlxplat_mlxcpld_regmap_ng,
	.num_reg_defaults = ARRAY_SIZE(mlxplat_mlxcpld_regmap_ng),
	.reg_read = mlxplat_mlxcpld_reg_read,
	.reg_write = mlxplat_mlxcpld_reg_write,
};

static const struct regmap_config mlxplat_mlxcpld_regmap_config_comex = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 255,
	.cache_type = REGCACHE_FLAT,
	.writeable_reg = mlxplat_mlxcpld_writeable_reg,
	.readable_reg = mlxplat_mlxcpld_readable_reg,
	.volatile_reg = mlxplat_mlxcpld_volatile_reg,
	.reg_defaults = mlxplat_mlxcpld_regmap_comex_default,
	.num_reg_defaults = ARRAY_SIZE(mlxplat_mlxcpld_regmap_comex_default),
	.reg_read = mlxplat_mlxcpld_reg_read,
	.reg_write = mlxplat_mlxcpld_reg_write,
};

static const struct regmap_config mlxplat_mlxcpld_regmap_config_ng400 = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 255,
	.cache_type = REGCACHE_FLAT,
	.writeable_reg = mlxplat_mlxcpld_writeable_reg,
	.readable_reg = mlxplat_mlxcpld_readable_reg,
	.volatile_reg = mlxplat_mlxcpld_volatile_reg,
	.reg_defaults = mlxplat_mlxcpld_regmap_ng400,
	.num_reg_defaults = ARRAY_SIZE(mlxplat_mlxcpld_regmap_ng400),
	.reg_read = mlxplat_mlxcpld_reg_read,
	.reg_write = mlxplat_mlxcpld_reg_write,
};

static struct resource mlxplat_mlxcpld_resources[] = {
	[0] = DEFINE_RES_IRQ_NAMED(17, "mlxreg-hotplug"),
};

static struct platform_device *mlxplat_dev;
static struct mlxreg_core_hotplug_platform_data *mlxplat_i2c;
static struct mlxreg_core_hotplug_platform_data *mlxplat_hotplug;
static struct mlxreg_core_platform_data *mlxplat_led;
static struct mlxreg_core_platform_data *mlxplat_regs_io;
static struct mlxreg_core_platform_data *mlxplat_fan;
static struct mlxreg_core_platform_data
	*mlxplat_wd_data[MLXPLAT_CPLD_WD_MAX_DEVS];
static const struct regmap_config *mlxplat_regmap_config;

static int __init mlxplat_dmi_default_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_default_mux_data);
	mlxplat_mux_data = mlxplat_default_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_default_channels[i];
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_default_channels[i]);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_default_data;
	mlxplat_hotplug->deferred_nr =
		mlxplat_default_channels[i - 1][MLXPLAT_CPLD_GRP_CHNL_NUM - 1];
	mlxplat_led = &mlxplat_default_led_data;
	mlxplat_regs_io = &mlxplat_default_regs_io_data;
	mlxplat_wd_data[0] = &mlxplat_mlxcpld_wd_set_type1[0];

	return 1;
}

static int __init mlxplat_dmi_msn21xx_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_default_mux_data);
	mlxplat_mux_data = mlxplat_default_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_msn21xx_channels;
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_msn21xx_channels);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_msn21xx_data;
	mlxplat_hotplug->deferred_nr =
		mlxplat_msn21xx_channels[MLXPLAT_CPLD_GRP_CHNL_NUM - 1];
	mlxplat_led = &mlxplat_msn21xx_led_data;
	mlxplat_regs_io = &mlxplat_msn21xx_regs_io_data;
	mlxplat_wd_data[0] = &mlxplat_mlxcpld_wd_set_type1[0];

	return 1;
}

static int __init mlxplat_dmi_msn274x_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_default_mux_data);
	mlxplat_mux_data = mlxplat_default_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_msn21xx_channels;
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_msn21xx_channels);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_msn274x_data;
	mlxplat_hotplug->deferred_nr =
		mlxplat_msn21xx_channels[MLXPLAT_CPLD_GRP_CHNL_NUM - 1];
	mlxplat_led = &mlxplat_default_led_data;
	mlxplat_regs_io = &mlxplat_msn21xx_regs_io_data;
	mlxplat_wd_data[0] = &mlxplat_mlxcpld_wd_set_type1[0];

	return 1;
}

static int __init mlxplat_dmi_msn201x_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_default_mux_data);
	mlxplat_mux_data = mlxplat_default_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_msn21xx_channels;
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_msn21xx_channels);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_msn201x_data;
	mlxplat_hotplug->deferred_nr =
		mlxplat_default_channels[i - 1][MLXPLAT_CPLD_GRP_CHNL_NUM - 1];
	mlxplat_led = &mlxplat_msn21xx_led_data;
	mlxplat_regs_io = &mlxplat_msn21xx_regs_io_data;
	mlxplat_wd_data[0] = &mlxplat_mlxcpld_wd_set_type1[0];

	return 1;
}

static int __init mlxplat_dmi_qmb7xx_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_default_mux_data);
	mlxplat_mux_data = mlxplat_default_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_msn21xx_channels;
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_msn21xx_channels);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_default_ng_data;
	mlxplat_hotplug->deferred_nr =
		mlxplat_msn21xx_channels[MLXPLAT_CPLD_GRP_CHNL_NUM - 1];
	mlxplat_led = &mlxplat_default_ng_led_data;
	mlxplat_regs_io = &mlxplat_default_ng_regs_io_data;
	mlxplat_fan = &mlxplat_default_fan_data;
	for (i = 0; i < ARRAY_SIZE(mlxplat_mlxcpld_wd_set_type2); i++)
		mlxplat_wd_data[i] = &mlxplat_mlxcpld_wd_set_type2[i];
	mlxplat_i2c = &mlxplat_mlxcpld_i2c_ng_data;
	mlxplat_regmap_config = &mlxplat_mlxcpld_regmap_config_ng;

	return 1;
}

static int __init mlxplat_dmi_modular_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_modular_mux_data);
	mlxplat_mux_data = mlxplat_modular_mux_data;

	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_modular_channels[i];
		mlxplat_mux_data[i].n_values = mlxplat_modular_chan_num[i];
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_modular_data;
	mlxplat_hotplug->deferred_nr = MLXPLAT_CPLD_MODULAR_MUX_LAST,
	mlxplat_led = &mlxplat_modular_led_data;
	mlxplat_regs_io = &mlxplat_modular_regs_io_data;

	return 1;
};

static int __init
mlxplat_dmi_modular200_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_modular_mux_data);
	mlxplat_mux_data = mlxplat_modular_mux_data;

	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_modular200_channels[i];
		mlxplat_mux_data[i].n_values = mlxplat_modular_chan_num[i];
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_modular_data;
	mlxplat_hotplug->deferred_nr = MLXPLAT_CPLD_MODULAR_MUX_LAST,
	mlxplat_led = &mlxplat_modular_led_data;
	mlxplat_regs_io = &mlxplat_modular_regs_io_data;

	return 1;
};

static int __init mlxplat_dmi_undefined_matched(const struct dmi_system_id *dmi)
{
	unsigned char geo_val;

	switch (boot_cpu_data.x86_model) {
	case INTEL_FAM6_IVYBRIDGE:
		geo_val = inb(MLXPLAT_CPLD_LPC_REG_BASE_ADRR +
			      MLXPLAT_CPLD_LPC_REG_GEO_ADDR_OFFSET);
		if (geo_val == MLXPLAT_CPLD_LPC_REG_GEO_ADDR_VALUE1 ||
		    geo_val == MLXPLAT_CPLD_LPC_REG_GEO_ADDR_VALUE2)
			return mlxplat_dmi_modular_matched(dmi);
		else
			return mlxplat_dmi_default_matched(dmi);

	case INTEL_FAM6_ATOM_SILVERMONT2:
			return mlxplat_dmi_msn21xx_matched(dmi);

	case INTEL_FAM6_BROADWELL_XEON_D:
	case INTEL_FAM6_BROADWELL_CORE:
		return mlxplat_dmi_qmb7xx_matched(dmi);

	default:
		return 1;
	}
}

static int __init mlxplat_dmi_comex_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_EXT_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_extended_mux_data);
	mlxplat_mux_data = mlxplat_extended_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_msn21xx_channels;
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_msn21xx_channels);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_comex_data;
	mlxplat_hotplug->deferred_nr = MLXPLAT_CPLD_MAX_PHYS_EXT_ADAPTER_NUM;
	mlxplat_led = &mlxplat_comex_100G_led_data;
	mlxplat_regs_io = &mlxplat_default_ng_regs_io_data;
	mlxplat_fan = &mlxplat_default_fan_data;
	for (i = 0; i < ARRAY_SIZE(mlxplat_mlxcpld_wd_set_type2); i++)
		mlxplat_wd_data[i] = &mlxplat_mlxcpld_wd_set_type2[i];
	mlxplat_regmap_config = &mlxplat_mlxcpld_regmap_config_comex;

	return 1;
}

static int __init mlxplat_dmi_ng400_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_default_mux_data);
	mlxplat_mux_data = mlxplat_default_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_msn21xx_channels;
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_msn21xx_channels);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_ext_data;
	mlxplat_hotplug->deferred_nr =
		mlxplat_msn21xx_channels[MLXPLAT_CPLD_GRP_CHNL_NUM - 1];
	mlxplat_led = &mlxplat_default_ng_led_data;
	mlxplat_regs_io = &mlxplat_default_ng_regs_io_data;
	mlxplat_fan = &mlxplat_default_fan_data;
	for (i = 0; i < ARRAY_SIZE(mlxplat_mlxcpld_wd_set_type2); i++)
		mlxplat_wd_data[i] = &mlxplat_mlxcpld_wd_set_type2[i];
	mlxplat_i2c = &mlxplat_mlxcpld_i2c_ng_data;
	mlxplat_regmap_config = &mlxplat_mlxcpld_regmap_config_ng400;

	return 1;
}

static int __init mlxplat_dmi_qmb8700_matched(const struct dmi_system_id *dmi)
{
	int i;

	mlxplat_max_adap_num = MLXPLAT_CPLD_MAX_PHYS_ADAPTER_NUM;
	mlxplat_mux_num = ARRAY_SIZE(mlxplat_default_mux_data);
	mlxplat_mux_data = mlxplat_default_mux_data;
	for (i = 0; i < mlxplat_mux_num; i++) {
		mlxplat_mux_data[i].values = mlxplat_msn21xx_channels;
		mlxplat_mux_data[i].n_values =
				ARRAY_SIZE(mlxplat_msn21xx_channels);
	}
	mlxplat_hotplug = &mlxplat_mlxcpld_qmb8700_data;
	mlxplat_hotplug->deferred_nr =
		mlxplat_msn21xx_channels[MLXPLAT_CPLD_GRP_CHNL_NUM - 1];
	mlxplat_led = &mlxplat_qmb8700_led_data;
	mlxplat_regs_io = &mlxplat_default_ng_regs_io_data;
	mlxplat_fan = &mlxplat_qmb8700_fan_data;
	for (i = 0; i < ARRAY_SIZE(mlxplat_mlxcpld_wd_set_type2); i++)
		mlxplat_wd_data[i] = &mlxplat_mlxcpld_wd_set_type2[i];
	mlxplat_i2c = &mlxplat_mlxcpld_i2c_ng_data;
	mlxplat_regmap_config = &mlxplat_mlxcpld_regmap_config_ng;

	return 1;
}


static const struct dmi_system_id mlxplat_dmi_table[] __initconst = {
	{
		.callback = mlxplat_dmi_default_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0001"),
		},
	},
	{
		.callback = mlxplat_dmi_msn21xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0002"),
		},
	},
	{
		.callback = mlxplat_dmi_msn274x_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0003"),
		},
	},
	{
		.callback = mlxplat_dmi_msn201x_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0004"),
		},
	},
	{
		.callback = mlxplat_dmi_qmb8700_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MQM8700"),
		},
	},
	{
		.callback = mlxplat_dmi_qmb7xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0005"),
		},
	},
	{
		.callback = mlxplat_dmi_modular200_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0006"),
		},
	},
	{
		.callback = mlxplat_dmi_qmb7xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0007"),
		},
	},
	{
		.callback = mlxplat_dmi_comex_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0009"),
		},
	},
	{
		.callback = mlxplat_dmi_ng400_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0010"),
		},
	},
	{
		.callback = mlxplat_dmi_ng400_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "VMOD0012"),
		},
	},
	{
		.callback = mlxplat_dmi_msn274x_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN274"),
		},
	},
	{
		.callback = mlxplat_dmi_default_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN24"),
		},
	},
	{
		.callback = mlxplat_dmi_default_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN27"),
		},
	},
	{
		.callback = mlxplat_dmi_default_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSB"),
		},
	},
	{
		.callback = mlxplat_dmi_default_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSX"),
		},
	},
	{
		.callback = mlxplat_dmi_msn21xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN21"),
		},
	},
	{
		.callback = mlxplat_dmi_msn201x_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN201"),
		},
	},
	{
		.callback = mlxplat_dmi_qmb7xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MQM87"),
		},
	},
	{
		.callback = mlxplat_dmi_qmb7xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN37"),
		},
	},
	{
		.callback = mlxplat_dmi_qmb7xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN34"),
		},
	},
	{
		.callback = mlxplat_dmi_qmb7xx_matched,
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Mellanox Technologies"),
			DMI_MATCH(DMI_PRODUCT_NAME, "MSN38"),
		},
	},
	/* This tuple must be last */
	{
		.callback = mlxplat_dmi_undefined_matched,
		.matches = {
			DMI_MATCH(DMI_BIOS_VENDOR, "American Megatrends Inc."),
		},
	},
	{ }
};

MODULE_DEVICE_TABLE(dmi, mlxplat_dmi_table);

static int mlxplat_mlxcpld_verify_bus_topology(int *nr)
{
	struct i2c_adapter *search_adap;
	int shift, i;

	/* Scan adapters from expected id to verify it is free. */
	*nr = MLXPLAT_CPLD_PHYS_ADAPTER_DEF_NR;
	for (i = MLXPLAT_CPLD_PHYS_ADAPTER_DEF_NR; i <
	     mlxplat_max_adap_num; i++) {
		search_adap = i2c_get_adapter(i);
		if (search_adap) {
			i2c_put_adapter(search_adap);
			continue;
		}

		/* Return if expected parent adapter is free. */
		if (i == MLXPLAT_CPLD_PHYS_ADAPTER_DEF_NR)
			return 0;
		break;
	}

	/* Return with error if free id for adapter is not found. */
	if (i == mlxplat_max_adap_num)
		return -ENODEV;

	/* Shift adapter ids, since expected parent adapter is not free. */
	*nr = i;
	for (i = 0; i < mlxplat_mux_num; i++) {
		shift = *nr - mlxplat_mux_data[i].parent;
		mlxplat_mux_data[i].parent = *nr;
		mlxplat_mux_data[i].base_nr += shift;
		if (shift > 0)
			mlxplat_hotplug->shift_nr = shift;
	}

	return 0;
}

static int __init mlxplat_init(void)
{
	struct mlxplat_priv *priv;
	int i, j, nr, err;

	if (!dmi_check_system(mlxplat_dmi_table))
		return -ENODEV;

	mlxplat_dev = platform_device_register_simple(MLX_PLAT_DEVICE_NAME, -1,
					mlxplat_lpc_resources,
					ARRAY_SIZE(mlxplat_lpc_resources));

	if (IS_ERR(mlxplat_dev))
		return PTR_ERR(mlxplat_dev);

	priv = devm_kzalloc(&mlxplat_dev->dev, sizeof(struct mlxplat_priv),
			    GFP_KERNEL);
	if (!priv) {
		err = -ENOMEM;
		goto fail_alloc;
	}
	platform_set_drvdata(mlxplat_dev, priv);

	mlxplat_mlxcpld_regmap_ctx.base = devm_ioport_map(&mlxplat_dev->dev,
			       mlxplat_lpc_resources[1].start, 1);
	if (!mlxplat_mlxcpld_regmap_ctx.base) {
		err = -ENOMEM;
		goto fail_alloc;
	}

	if (!mlxplat_regmap_config)
		mlxplat_regmap_config = &mlxplat_mlxcpld_regmap_config;

	priv->regmap = devm_regmap_init(&mlxplat_dev->dev, NULL,
					&mlxplat_mlxcpld_regmap_ctx,
					mlxplat_regmap_config);
	if (IS_ERR(priv->regmap)) {
		err = PTR_ERR(priv->regmap);
		goto fail_alloc;
	}

	err = mlxplat_mlxcpld_verify_bus_topology(&nr);
	if (nr < 0)
		goto fail_alloc;

	nr = (nr == mlxplat_max_adap_num) ? -1 : nr;
	if (mlxplat_i2c)
		mlxplat_i2c->regmap = priv->regmap;
	priv->pdev_i2c = platform_device_register_resndata(
					&mlxplat_dev->dev, "i2c_mlxcpld",
					nr, mlxplat_mlxcpld_resources,
					ARRAY_SIZE(mlxplat_mlxcpld_resources),
					mlxplat_i2c, sizeof(*mlxplat_i2c));
	if (IS_ERR(priv->pdev_i2c)) {
		err = PTR_ERR(priv->pdev_i2c);
		goto fail_alloc;
	}

	for (i = 0; i < mlxplat_mux_num; i++) {
		priv->pdev_mux[i] = platform_device_register_resndata(
						&priv->pdev_i2c->dev,
						"i2c-mux-reg", i, NULL,
						0, &mlxplat_mux_data[i],
						sizeof(mlxplat_mux_data[i]));
		if (IS_ERR(priv->pdev_mux[i])) {
			err = PTR_ERR(priv->pdev_mux[i]);
			goto fail_platform_mux_register;
		}
	}

	/* Add hotplug driver */
	mlxplat_hotplug->regmap = priv->regmap;
	priv->pdev_hotplug = platform_device_register_resndata(
				&mlxplat_dev->dev, "mlxreg-hotplug",
				PLATFORM_DEVID_NONE,
				mlxplat_mlxcpld_resources,
				ARRAY_SIZE(mlxplat_mlxcpld_resources),
				mlxplat_hotplug, sizeof(*mlxplat_hotplug));
	if (IS_ERR(priv->pdev_hotplug)) {
		err = PTR_ERR(priv->pdev_hotplug);
		goto fail_platform_mux_register;
	}

	/* Set default registers. */
	for (j = 0; j <  mlxplat_regmap_config->num_reg_defaults; j++) {
		err = regmap_write(priv->regmap,
				   mlxplat_regmap_config->reg_defaults[j].reg,
				   mlxplat_regmap_config->reg_defaults[j].def);
		if (err)
			goto fail_platform_mux_register;
	}

	/* Add LED driver. */
	mlxplat_led->regmap = priv->regmap;
	priv->pdev_led = platform_device_register_resndata(
				&mlxplat_dev->dev, "leds-mlxreg",
				PLATFORM_DEVID_NONE, NULL, 0,
				mlxplat_led, sizeof(*mlxplat_led));
	if (IS_ERR(priv->pdev_led)) {
		err = PTR_ERR(priv->pdev_led);
		goto fail_platform_hotplug_register;
	}

	/* Add registers io access driver. */
	if (mlxplat_regs_io) {
		mlxplat_regs_io->regmap = priv->regmap;
		priv->pdev_io_regs = platform_device_register_resndata(
					&mlxplat_dev->dev, "mlxreg-io",
					PLATFORM_DEVID_NONE, NULL, 0,
					mlxplat_regs_io,
					sizeof(*mlxplat_regs_io));
		if (IS_ERR(priv->pdev_io_regs)) {
			err = PTR_ERR(priv->pdev_io_regs);
			goto fail_platform_led_register;
		}
	}

	/* Add FAN driver. */
	if (mlxplat_fan) {
		mlxplat_fan->regmap = priv->regmap;
		priv->pdev_fan = platform_device_register_resndata(
					&mlxplat_dev->dev, "mlxreg-fan",
					PLATFORM_DEVID_NONE, NULL, 0,
					mlxplat_fan,
					sizeof(*mlxplat_fan));
		if (IS_ERR(priv->pdev_fan)) {
			err = PTR_ERR(priv->pdev_fan);
			goto fail_platform_io_regs_register;
		}
	}

	/* Add WD drivers. */
	for (j = 0; j < MLXPLAT_CPLD_WD_MAX_DEVS; j++) {
		if (mlxplat_wd_data[j]) {
			mlxplat_wd_data[j]->regmap = priv->regmap;
			priv->pdev_wd[j] = platform_device_register_resndata(
						&mlxplat_dev->dev, "mlx-wdt",
						j, NULL, 0,
						mlxplat_wd_data[j],
						sizeof(*mlxplat_wd_data[j]));
			if (IS_ERR(priv->pdev_wd[j])) {
				err = PTR_ERR(priv->pdev_wd[j]);
				goto fail_platform_wd_register;
			}
		}
	}

	/* Sync registers with hardware. */
	regcache_mark_dirty(priv->regmap);
	err = regcache_sync(priv->regmap);
	if (err)
		goto fail_platform_wd_register;

	return 0;

fail_platform_wd_register:
	while (--j >= 0)
		platform_device_unregister(priv->pdev_wd[j]);
	if (mlxplat_fan)
		platform_device_unregister(priv->pdev_fan);
fail_platform_io_regs_register:
	if (mlxplat_regs_io)
		platform_device_unregister(priv->pdev_io_regs);
fail_platform_led_register:
	platform_device_unregister(priv->pdev_led);
fail_platform_hotplug_register:
	platform_device_unregister(priv->pdev_hotplug);
fail_platform_mux_register:
	while (--i >= 0)
		platform_device_unregister(priv->pdev_mux[i]);
	platform_device_unregister(priv->pdev_i2c);
fail_alloc:
	platform_device_unregister(mlxplat_dev);

	return err;
}
module_init(mlxplat_init);

static void __exit mlxplat_exit(void)
{
	struct mlxplat_priv *priv = platform_get_drvdata(mlxplat_dev);
	int i;

	for (i = MLXPLAT_CPLD_WD_MAX_DEVS - 1; i >= 0 ; i--)
		platform_device_unregister(priv->pdev_wd[i]);
	if (priv->pdev_fan)
		platform_device_unregister(priv->pdev_fan);
	if (priv->pdev_io_regs)
		platform_device_unregister(priv->pdev_io_regs);
	platform_device_unregister(priv->pdev_led);
	platform_device_unregister(priv->pdev_hotplug);

	for (i = mlxplat_mux_num - 1; i >= 0 ; i--)
		platform_device_unregister(priv->pdev_mux[i]);

	platform_device_unregister(priv->pdev_i2c);
	platform_device_unregister(mlxplat_dev);
}
module_exit(mlxplat_exit);

MODULE_AUTHOR("Vadim Pasternak (vadimp@mellanox.com)");
MODULE_DESCRIPTION("Mellanox platform driver");
MODULE_LICENSE("Dual BSD/GPL");
