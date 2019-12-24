/*
 * Copyright (c) 2016-2018 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2016-2018 Vadim Pasternak <vadimp@mellanox.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <net/genetlink.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_data/mlxreg.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/refcount.h>
#include <linux/regmap.h>
#include <linux/workqueue.h>
#include <uapi/linux/mlxreg.h>

/* Offset of event and mask registers from status register. */
#define MLXREG_HOTPLUG_EVENT_OFF	1
#define MLXREG_HOTPLUG_MASK_OFF		2
#define MLXREG_HOTPLUG_AGGR_MASK_OFF	1

/* ASIC good health mask. */
#define MLXREG_HOTPLUG_GOOD_HEALTH_MASK	0x02

#define MLXREG_HOTPLUG_ATTRS_MAX	100
#define MLXREG_HOTPLUG_NOT_ASSERT	3

/**
 * struct mlxreg_hotplug_priv_data - platform private data:
 * @list: list of objects;
 * @irq: platform device interrupt number;
 * @dev: basic device;
 * @pdev: platform device;
 * @plat: platform data;
 * @regmap: register map handle;
 * @dwork_irq: delayed work template;
 * @lock: spin lock;
 * @hwmon: hwmon device;
 * @mlxreg_hotplug_attr: sysfs attributes array;
 * @mlxreg_hotplug_dev_attr: sysfs sensor device attribute array;
 * @group: sysfs attribute group;
 * @groups: list of sysfs attribute group for hwmon registration;
 * @cell: location of top aggregation interrupt register;
 * @mask: top aggregation interrupt common mask;
 * @aggr_cache: last value of aggregation register status;
 * @after_probe: flag indication probing completion;
 * @not_asserted: number of entries in workqueue with no signal assertion;
 * @sk: netlink socket;
 * @id: socket listener process id;
 */
struct mlxreg_hotplug_priv_data {
	struct list_head list;
	int irq;
	struct device *dev;
	struct platform_device *pdev;
	struct mlxreg_hotplug_platform_data *plat;
	struct regmap *regmap;
	struct delayed_work dwork_irq;
	spinlock_t lock; /* sync with interrupt */
	struct device *hwmon;
	struct attribute *mlxreg_hotplug_attr[MLXREG_HOTPLUG_ATTRS_MAX + 1];
	struct sensor_device_attribute_2
			mlxreg_hotplug_dev_attr[MLXREG_HOTPLUG_ATTRS_MAX];
	struct attribute_group group;
	const struct attribute_group *groups[2];
	u32 cell;
	u32 mask;
	u32 aggr_cache;
	bool after_probe;
	u8 not_asserted;
	struct sock *sk;
	unsigned int id;
};

static struct mlxreg_hotplug_list {
	struct list_head list;
} mlxreg_hotplug_list;

/**
 * struct mlxreg_hotplug_netlink_data - netlink control data:
 * @sk: netlink socket;
 * @pid: socket listener process id;
 * @seq: event sequence number;
 * @refcnt: reference counter;
 */
struct mlxreg_hotplug_netlink_data {
	struct sock *sk;
	unsigned int pid;
	unsigned int seq;
	refcount_t refcnt;
};

static struct mlxreg_hotplug_netlink_data mlxreg_hotplug_nl = {
	.refcnt = REFCOUNT_INIT(0),
};

static int mlxreg_hotplug_set_irq(struct mlxreg_hotplug_priv_data *priv);
static void mlxreg_hotplug_unset_irq(struct mlxreg_hotplug_priv_data *priv);

static void mlxreg_hotplug_receive_nl_msg(struct sk_buff *skb)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	struct mlxreg_hotplug_priv_data *priv;
	struct nlmsghdr *nlh;
	u16 nlmsg_type;
	u32 devnum;
	u32 *p_mod_id;
	int nr;

	nlh = nlmsg_hdr(skb);
	if (!NLMSG_OK(nlh, skb->len)) {
		pr_err("Received corrupted netlink message len = %d\n",
		       skb->len);
		return;
	}

	nlmsg_type = MLXREG_HOTPLUG_GET_MSG_TYPE(nlh->nlmsg_type);
	nr = MLXREG_HOTPLUG_GET_DEVID(nlh->nlmsg_type);
	p_mod_id = (u32 *)NLMSG_DATA(nlh);
	if (p_mod_id)
		devnum = *p_mod_id;

	switch (nlmsg_type) {
	case MLXREG_NL_REGISTER:
		if (mlxreg_hotplug_nl.pid)
			return;
		mlxreg_hotplug_nl.pid = nlh->nlmsg_pid;
		break;
	case MLXREG_NL_UNREGISTER:
		if (refcount_read(&mlxreg_hotplug_nl.refcnt) > 1)
			return;
		mlxreg_hotplug_nl.pid = 0;
		break;
	case MLXREG_NL_ENABLE:
	case MLXREG_NL_DISABLE:
		list_for_each_entry(priv, &mlxreg_hotplug_list.list, list) {
			if (nr == priv->pdev->id) {
				if (nlmsg_type == MLXREG_NL_ENABLE) {
					pdata = dev_get_platdata(&priv->pdev->dev);
					pdata->devnum = devnum;
					mlxreg_hotplug_set_irq(priv);
					priv->after_probe = true;
				} else {
					mlxreg_hotplug_unset_irq(priv);
					priv->after_probe = false;
				}
				break;
			}
		}
		break;
	default:
		pr_err("Received unknown netlink message type %d\n",
		       nlh->nlmsg_type);
		break;
	}
}

static int mlxreg_hotplug_nl_init_once(void)
{
	struct netlink_kernel_cfg cfg = {
		.input	= mlxreg_hotplug_receive_nl_msg,
	};

	if (IS_REACHABLE(CONFIG_NET) &&
	    !refcount_read(&mlxreg_hotplug_nl.refcnt)) {
		mlxreg_hotplug_nl.sk = netlink_kernel_create(&init_net,
							     NETLINK_USERSOCK,
							     &cfg);
		if (!mlxreg_hotplug_nl.sk)
			return -ENOMEM;

		INIT_LIST_HEAD(&mlxreg_hotplug_list.list);
	}

	refcount_inc(&mlxreg_hotplug_nl.refcnt);

	return 0;
}

static void mlxreg_hotplug_nl_release_once(void)
{
	refcount_dec(&mlxreg_hotplug_nl.refcnt);

	if (IS_REACHABLE(CONFIG_NET) &&
	    !refcount_read(&mlxreg_hotplug_nl.refcnt)) {
		netlink_kernel_release(mlxreg_hotplug_nl.sk);
	}
}

int mlxreg_hotplug_generate_netlink_event_sim(int nr, bool event,
					      unsigned int simulated,
					      char *label)
{
	struct mlxreg_hotplug_event *mlxreg_hotplug_event;
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	int size;
	int res;

	if (!mlxreg_hotplug_nl.pid)
		return 0;

	/* Allocate memory for netlink message. */
	size = nla_total_size(sizeof(*mlxreg_hotplug_event)) +
				     nla_total_size(0);
	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	nlh = nlmsg_put(skb, mlxreg_hotplug_nl.pid, mlxreg_hotplug_nl.seq++,
			MLXREG_NL_EVENT, sizeof(*mlxreg_hotplug_event), 0);
	if (!nlh)
		return -ENOMEM;
	mlxreg_hotplug_event = nlmsg_data(nlh);
	memset(mlxreg_hotplug_event, 0, sizeof(*mlxreg_hotplug_event));
	sprintf(mlxreg_hotplug_event->label, "%s", label);
	mlxreg_hotplug_event->nr = nr;
	mlxreg_hotplug_event->event = event;
	mlxreg_hotplug_event->simulated = simulated;
	nlmsg_end(skb, nlh);
	res = netlink_unicast(mlxreg_hotplug_nl.sk, skb, mlxreg_hotplug_nl.pid,
			      MSG_DONTWAIT);

	return res;
}
EXPORT_SYMBOL(mlxreg_hotplug_generate_netlink_event_sim);

static int
mlxreg_hotplug_generate_netlink_event(struct mlxreg_hotplug_priv_data *priv,
				      struct mlxreg_core_data *data,
				      bool event)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	struct mlxreg_hotplug_event *mlxreg_hotplug_event;
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	int size;
	int res;

	if (!priv->after_probe)
		return 0;

	if (!data)
		return -EINVAL;

	pdata = dev_get_platdata(&priv->pdev->dev);
	if (!pdata)
		return -EINVAL;

	if (!mlxreg_hotplug_nl.pid)
		return 0;

	/* Allocate memory for netlink message. */
	size = nla_total_size(sizeof(*mlxreg_hotplug_event)) +
	       nla_total_size(0);
	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	nlh = nlmsg_put(skb, mlxreg_hotplug_nl.pid, mlxreg_hotplug_nl.seq++,
			MLXREG_NL_EVENT, sizeof(*mlxreg_hotplug_event), 0);
	if (!nlh)
		return -ENOMEM;
	mlxreg_hotplug_event = nlmsg_data(nlh);
	memset(mlxreg_hotplug_event, 0, sizeof(*mlxreg_hotplug_event));
	sprintf(mlxreg_hotplug_event->label, "%s", data->label);
	mlxreg_hotplug_event->nr = priv->pdev->id;
	mlxreg_hotplug_event->event = event;
	nlmsg_end(skb, nlh);

	res = netlink_unicast(mlxreg_hotplug_nl.sk, skb, mlxreg_hotplug_nl.pid,
			      MSG_DONTWAIT);

	return res;
}

static int mlxreg_hotplug_device_create(struct mlxreg_hotplug_priv_data *priv,
					struct mlxreg_core_data *data)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	int res;

	pdata = dev_get_platdata(&priv->pdev->dev);

	/* Send hotplug event. */
	if (IS_REACHABLE(CONFIG_NET) && priv->after_probe) {
		if (pdata->cell || (pdata->wakeup_signal &&
		    pdata->wakeup_signal(pdata))) {
			res = mlxreg_hotplug_generate_netlink_event(priv, data,
								    true);
			if (res < 0)
			dev_err(priv->dev, "Failed to send netlink event:%d",
				res);
		}
	}

	/* Notify user by sending hwmon uevent. */
	kobject_uevent(&priv->hwmon->kobj, KOBJ_CHANGE);

	/*
	 * Return if adapter number is negative. It could be in case hotplug
	 * event is not associated with hotplug device.
	 */
	if (data->hpdev.nr < 0)
		return 0;

	data->hpdev.adapter = i2c_get_adapter(data->hpdev.nr +
					      pdata->shift_nr);
	if (!data->hpdev.adapter) {
		dev_err(priv->dev, "Failed to get adapter for bus %d\n",
			data->hpdev.nr + pdata->shift_nr);
		return -EFAULT;
	}

	data->hpdev.client = i2c_new_device(data->hpdev.adapter,
					    data->hpdev.brdinfo);
	if (!data->hpdev.client) {
		dev_err(priv->dev, "Failed to create client %s at bus %d at addr 0x%02x\n",
			data->hpdev.brdinfo->type, data->hpdev.nr +
			pdata->shift_nr, data->hpdev.brdinfo->addr);

		i2c_put_adapter(data->hpdev.adapter);
		data->hpdev.adapter = NULL;
		return -EFAULT;
	}

	return 0;
}

static void
mlxreg_hotplug_device_destroy(struct mlxreg_hotplug_priv_data *priv,
			      struct mlxreg_core_data *data)
{
	int res;
	struct mlxreg_core_hotplug_platform_data *pdata;

	pdata = dev_get_platdata(&priv->pdev->dev);

	/* Send hotplug event. */
	if (IS_REACHABLE(CONFIG_NET) && priv->after_probe) {
		if (pdata->cell || (pdata->wakeup_signal &&
		    pdata->wakeup_signal(pdata))) {
			res = mlxreg_hotplug_generate_netlink_event(priv, data,
								    false);
			if (res < 0)
				dev_err(priv->dev, "Failed to send netlink event:%d",
					res);
		}
	}

	/* Notify user by sending hwmon uevent. */
	kobject_uevent(&priv->hwmon->kobj, KOBJ_CHANGE);

	if (data->hpdev.client) {
		i2c_unregister_device(data->hpdev.client);
		data->hpdev.client = NULL;
	}

	if (data->hpdev.adapter) {
		i2c_put_adapter(data->hpdev.adapter);
		data->hpdev.adapter = NULL;
	}
}

static ssize_t mlxreg_hotplug_attr_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct mlxreg_hotplug_priv_data *priv = dev_get_drvdata(dev);
	struct mlxreg_core_hotplug_platform_data *pdata;
	int index = to_sensor_dev_attr_2(attr)->index;
	int nr = to_sensor_dev_attr_2(attr)->nr;
	struct mlxreg_core_item *item;
	struct mlxreg_core_data *data;
	u32 regval;
	int ret;

	pdata = dev_get_platdata(&priv->pdev->dev);
	item = pdata->items + nr;
	data = item->data + index;

	ret = regmap_read(priv->regmap, data->reg, &regval);
	if (ret)
		return ret;

	if (item->health) {
		if (data->bit)
			regval = ror32(regval & data->mask, (data->bit - 1));
		else
			regval &= data->mask;
	} else {
		/* Bit = 0 : functional if item->inversed is true. */
		if (item->inversed)
			regval = !(regval & data->mask);
		else
			regval = !!(regval & data->mask);
	}

	return sprintf(buf, "%u\n", regval);
}

#define PRIV_ATTR(i) priv->mlxreg_hotplug_attr[i]
#define PRIV_DEV_ATTR(i) priv->mlxreg_hotplug_dev_attr[i]

static int mlxreg_hotplug_attr_init(struct mlxreg_hotplug_priv_data *priv)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	struct mlxreg_core_item *item;
	struct mlxreg_core_data *data;
	int num_attrs = 0, id = 0, i, j;

	pdata = dev_get_platdata(&priv->pdev->dev);
	item = pdata->items;

	/* Go over all kinds of items - psu, pwr, fan. */
	for (i = 0; i < pdata->counter; i++, item++) {
		num_attrs += item->count;
		data = item->data;

		/* Go over all units within the item. */
		for (j = 0; j < item->count; j++, data++, id++) {
			PRIV_ATTR(id) = &PRIV_DEV_ATTR(id).dev_attr.attr;
			PRIV_ATTR(id)->name = devm_kasprintf(&priv->pdev->dev,
							     GFP_KERNEL,
							     data->label);

			if (!PRIV_ATTR(id)->name) {
				dev_err(priv->dev, "Memory allocation failed for attr %d.\n",
					id);
				return -ENOMEM;
			}

			PRIV_DEV_ATTR(id).dev_attr.attr.name =
							PRIV_ATTR(id)->name;
			PRIV_DEV_ATTR(id).dev_attr.attr.mode = 0444;
			PRIV_DEV_ATTR(id).dev_attr.show =
						mlxreg_hotplug_attr_show;
			PRIV_DEV_ATTR(id).nr = i;
			PRIV_DEV_ATTR(id).index = j;
			sysfs_attr_init(&PRIV_DEV_ATTR(id).dev_attr.attr);
		}
	}

	priv->group.attrs = devm_kcalloc(&priv->pdev->dev,
					 num_attrs,
					 sizeof(struct attribute *),
					 GFP_KERNEL);
	if (!priv->group.attrs)
		return -ENOMEM;

	priv->group.attrs = priv->mlxreg_hotplug_attr;
	priv->groups[0] = &priv->group;
	priv->groups[1] = NULL;

	return 0;
}

static void
mlxreg_hotplug_work_helper(struct mlxreg_hotplug_priv_data *priv,
			   struct mlxreg_core_item *item, bool *changed)
{
	struct mlxreg_core_data *data;
	unsigned long asserted;
	u32 regval, bit;
	int ret;

	/*
	 * Validate if item related to received signal type is valid.
	 * It should never happen, excepted the situation when some
	 * piece of hardware is broken. In such situation just produce
	 * error message and return. Caller must continue to handle the
	 * signals from other devices if any.
	 */
	if (unlikely(!item)) {
		dev_err(priv->dev, "False signal: at offset:mask 0x%02x:0x%02x.\n",
			item->reg, item->mask);

		return;
	}

	/* Mask event. */
	ret = regmap_write(priv->regmap, item->reg + MLXREG_HOTPLUG_MASK_OFF,
			   0);
	if (ret)
		goto out;

	/* Read status. */
	ret = regmap_read(priv->regmap, item->reg, &regval);
	if (ret)
		goto out;

	/* Set asserted bits and save last status. */
	regval &= item->mask;
	asserted = item->cache ^ regval;
	item->cache = regval;

	if (asserted)
		*changed = true;
	else
		goto ack_event;

	for_each_set_bit(bit, (unsigned long *)&asserted, 8) {
		data = item->data + bit;
		if (regval & BIT(bit)) {
			if (item->inversed)
				mlxreg_hotplug_device_destroy(priv, data);
			else
				mlxreg_hotplug_device_create(priv, data);
		} else {
			if (item->inversed)
				mlxreg_hotplug_device_create(priv, data);
			else
				mlxreg_hotplug_device_destroy(priv, data);
		}
	}
ack_event:
	/* Acknowledge event. */
	ret = regmap_write(priv->regmap, item->reg + MLXREG_HOTPLUG_EVENT_OFF,
			   0);
	if (ret)
		goto out;

	/* Unmask event. */
	ret = regmap_write(priv->regmap, item->reg + MLXREG_HOTPLUG_MASK_OFF,
			   item->mask);
 out:
	if (ret)
		dev_err(priv->dev, "%s:Failed to complete workqueue.\n",
			__func__);
}

static void
mlxreg_hotplug_health_work_helper(struct mlxreg_hotplug_priv_data *priv,
				  struct mlxreg_core_item *item, bool *changed)
{
	struct mlxreg_core_data *data = item->data;
	u32 regval, health;
	int i, ret = 0;

	/* Mask event. */
	ret = regmap_write(priv->regmap, item->reg + MLXREG_HOTPLUG_MASK_OFF,
			   0);
	if (ret)
		goto out;

	/* Read status. */
	ret = regmap_read(priv->regmap, item->reg, &regval);
	if (ret)
		goto out;

	regval &= item->mask;

	if (item->cache == regval)
		goto ack_event;
	else
		*changed = true;

	for (i = 0; i < item->count; i++, data++) {
		/*
		 * ASIC health indication is provided through two bits. Bits
		 * value 0x2 indicates that ASIC reached the good health, value
		 * 0x0 indicates ASIC the bad health or dormant state and value
		 * 0x3 indicates the booting state. During ASIC reset it should
		 * pass the following states: dormant -> booting -> good.
		 */
		if (data->bit)
			health = ror32(regval & data->mask, (data->bit - 1));
		else
			health = regval;

		if (health == MLXREG_HOTPLUG_GOOD_HEALTH_MASK) {
			if (!data->attached) {
				/*
				 * ASIC is in steady state. Connect associated
				 * device, if configured.
				 */
				mlxreg_hotplug_device_create(priv, data);
				data->attached = true;
			}
		} else {
			if (data->attached) {
				/*
				 * ASIC health is failed after ASIC has been
				 * in steady state. Disconnect associated
				 * device, if it has been connected.
				 */
				mlxreg_hotplug_device_destroy(priv, data);
				data->attached = false;
				data->health_cntr = 0;
			}
		}
	}
	item->cache = regval;
ack_event:
	/* Acknowledge event. */
	ret = regmap_write(priv->regmap, item->reg + MLXREG_HOTPLUG_EVENT_OFF,
			   0);
	if (ret) {
		dev_err(priv->dev, "Failed to acknowledge health event at offset 0x%08x.\n",
			item->reg + MLXREG_HOTPLUG_EVENT_OFF);
		goto out;
	}

	/* Unmask event. */
	ret = regmap_write(priv->regmap, item->reg + MLXREG_HOTPLUG_MASK_OFF,
			   item->mask);
	if (ret)
		goto out;

 out:
	if (ret)
		dev_err(priv->dev, "%s:Failed to complete workqueue.\n",
			__func__);
}

/*
 * mlxreg_hotplug_work_handler - performs traversing of device interrupt
 * registers according to the below hierarchy schema:
 *
 *				Aggregation registers (status/mask)
 * PSU registers:		*---*
 * *-----------------*		|   |
 * |status/event/mask|----->    | * |
 * *-----------------*		|   |
 * Power registers:		|   |
 * *-----------------*		|   |
 * |status/event/mask|----->    | * |
 * *-----------------*		|   |
 * FAN registers:		|   |--> CPU
 * *-----------------*		|   |
 * |status/event/mask|----->    | * |
 * *-----------------*		|   |
 * ASIC registers:		|   |
 * *-----------------*		|   |
 * |status/event/mask|----->    | * |
 * *-----------------*		|   |
 *				*---*
 *
 * In case some system changed are detected: FAN in/out, PSU in/out, power
 * cable attached/detached, ASIC health good/bad, relevant device is created
 * or destroyed.
 */
static void mlxreg_hotplug_work_handler(struct work_struct *work)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	struct mlxreg_hotplug_priv_data *priv;
	struct mlxreg_core_item *item;
	u32 regval, aggr_asserted = 0;
	unsigned long flags;
	bool changed = false;
	int i, ret;
	ret = 0;

	priv = container_of(work, struct mlxreg_hotplug_priv_data,
			    dwork_irq.work);
	pdata = dev_get_platdata(&priv->pdev->dev);
	item = pdata->items;

	/* Mask aggregation event. */
	if (pdata->cell) {
		ret = regmap_write(priv->regmap, pdata->cell +
				   MLXREG_HOTPLUG_AGGR_MASK_OFF, 0);
		if (ret < 0)
			goto out;

		/* Read aggregation status. */
		ret = regmap_read(priv->regmap, pdata->cell, &regval);
		if (ret)
			goto out;

		regval &= pdata->mask;
		aggr_asserted = priv->aggr_cache ^ regval;
		priv->aggr_cache = regval;

		/*
		 * Handler is invoked, but no assertion is detected at top
		 * aggregation status level. Set aggr_asserted to mask value
		 * to allow handler extra run over all relevant signals to
		 * recover any missed signal.
		 */
		if (priv->not_asserted == MLXREG_HOTPLUG_NOT_ASSERT) {
			priv->not_asserted = 0;
			aggr_asserted = pdata->mask;
		}
		if (!aggr_asserted)
			goto unmask_event;
	} else {
		if ((priv->not_asserted == MLXREG_HOTPLUG_NOT_ASSERT) ||
		    (pdata->presence && !pdata->presence(pdata)) ||
		    (pdata->wakeup_signal && !pdata->wakeup_signal(pdata) &&
		     priv->after_probe)) {
			priv->not_asserted = 0;
			goto unmask_event;
		}
	}

	/* Handle topology and health configuration changes. */
	for (i = 0; i < pdata->counter; i++, item++) {
		if (aggr_asserted & item->aggr_mask || !pdata->cell) {
			if (item->health)
				mlxreg_hotplug_health_work_helper(priv, item,
								  &changed);
			else
				mlxreg_hotplug_work_helper(priv, item,
							   &changed);

			if (pdata->wakeup_signal_clear)
				pdata->wakeup_signal_clear(pdata);
		}
	}

	if (!priv->after_probe)
		goto unmask_event;

	if (!pdata->cell && !changed)
		priv->not_asserted++;

	spin_lock_irqsave(&priv->lock, flags);

	/*
	 * It is possible, that some signals have been inserted, while
	 * interrupt has been masked by mlxreg_hotplug_work_handler. In this
	 * case such signals will be missed. In order to handle these signals
	 * delayed work is canceled and work task re-scheduled for immediate
	 * execution. It allows to handle missed signals, if any. In other case
	 * work handler just validates that no new signals have been received
	 * during masking.
	 */
	cancel_delayed_work(&priv->dwork_irq);
	schedule_delayed_work(&priv->dwork_irq, 0);

	spin_unlock_irqrestore(&priv->lock, flags);

	return;

unmask_event:
	if (pdata->cell) {
		priv->not_asserted++;
		/* Unmask aggregation event (no need acknowledge). */
		ret = regmap_write(priv->regmap, pdata->cell +
				   MLXREG_HOTPLUG_AGGR_MASK_OFF, pdata->mask);
	}

 out:
	if (ret)
		dev_err(priv->dev, "%s:Failed to complete workqueue.\n",
			__func__);
}

static int mlxreg_hotplug_set_irq(struct mlxreg_hotplug_priv_data *priv)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	struct mlxreg_core_item *item;
	struct mlxreg_core_data *data;
	u32 regval;
	int i, j, ret;

	pdata = dev_get_platdata(&priv->pdev->dev);
	item = pdata->items;

	for (i = 0; i < pdata->counter; i++, item++) {
		if (item->capability) {
			/*
			 * Read group capability register to get actual number
			 * of interrupt capable components and set group mask
			 * accordingly.
			 */
			ret = regmap_read(priv->regmap, item->capability,
					  &regval);
			if (ret)
				goto out;

			item->mask = GENMASK((regval & item->mask) - 1, 0);
		}

		/* Clear group presense event. */
		ret = regmap_write(priv->regmap, item->reg +
				   MLXREG_HOTPLUG_EVENT_OFF, 0);
		if (ret)
			goto out;

		/*
		 * Verify if hardware configuration requires to disable
		 * interrupt capability for some of components.
		 */
		data = item->data;
		for (j = 0; j < item->count; j++, data++) {
			/* Verify if the attribute has capability register. */
			if (data->capability) {
				/* Read capability register. */
				ret = regmap_read(priv->regmap,
						  data->capability, &regval);
				if (ret)
					goto out;

				if (!(regval & data->bit))
					item->mask &= ~BIT(j);
			}
		}

		/* Set group initial status as mask and unmask group event. */
		if (item->inversed) {
			item->cache = item->mask;
			ret = regmap_write(priv->regmap, item->reg +
					   MLXREG_HOTPLUG_MASK_OFF,
					   item->mask);
			if (ret)
				goto out;
		}
	}

	/* Keep aggregation initial status as zero and unmask events. */
	if (pdata->cell) {
		ret = regmap_write(priv->regmap, pdata->cell +
				    MLXREG_HOTPLUG_AGGR_MASK_OFF, pdata->mask);
		if (ret)
			goto out;
	}

	/* Keep low aggregation initial status as zero and unmask events. */
	if (pdata->cell_low) {
		ret = regmap_write(priv->regmap, pdata->cell_low +
				   MLXREG_HOTPLUG_AGGR_MASK_OFF,
				   pdata->mask_low);
		if (ret)
			goto out;
	}

	/* Invoke work handler for initializing hot plug devices setting. */
	mlxreg_hotplug_work_handler(&priv->dwork_irq.work);

 out:
	if (ret)
		dev_err(priv->dev, "Failed to set interrupts.\n");

	if (pdata->cell)
		enable_irq(priv->irq);
	return ret;
}

static void mlxreg_hotplug_unset_irq(struct mlxreg_hotplug_priv_data *priv)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	struct mlxreg_core_item *item;
	struct mlxreg_core_data *data;
	int count, i, j;

	pdata = dev_get_platdata(&priv->pdev->dev);
	item = pdata->items;

	if (pdata->cell)
		disable_irq(priv->irq);

	cancel_delayed_work_sync(&priv->dwork_irq);

	/* Mask low aggregation event, if defined. */
	if (pdata->cell_low)
		regmap_write(priv->regmap, pdata->cell_low +
			     MLXREG_HOTPLUG_AGGR_MASK_OFF, 0);

	/* Mask aggregation event. */
	if (pdata->cell) {
		regmap_write(priv->regmap, pdata->cell +
			     MLXREG_HOTPLUG_AGGR_MASK_OFF, 0);
	}

	/* Clear topology configurations. */
	for (i = 0; i < pdata->counter; i++, item++) {
		data = item->data;
		/* Mask group presense event. */
		regmap_write(priv->regmap, data->reg + MLXREG_HOTPLUG_MASK_OFF,
			     0);
		/* Clear group presense event. */
		regmap_write(priv->regmap, data->reg +
			     MLXREG_HOTPLUG_EVENT_OFF, 0);

		/* Remove all the attached devices in group. */
		count = item->count;
		for (j = 0; j < count; j++, data++)
			mlxreg_hotplug_device_destroy(priv, data);
	}
}

static irqreturn_t mlxreg_hotplug_irq_handler(int irq, void *dev)
{
	struct mlxreg_hotplug_priv_data *priv;

	priv = (struct mlxreg_hotplug_priv_data *)dev;

	/* Schedule work task for immediate execution.*/
	schedule_delayed_work(&priv->dwork_irq, 0);

	return IRQ_HANDLED;
}

static int mlxreg_hotplug_probe(struct platform_device *pdev)
{
	struct mlxreg_core_hotplug_platform_data *pdata;
	struct mlxreg_hotplug_priv_data *priv;
	struct i2c_adapter *deferred_adap;
	int err;

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		dev_err(&pdev->dev, "Failed to get platform data.\n");
		return -EINVAL;
	}

	/* Defer probing if the necessary adapter is not configured yet. */
	deferred_adap = i2c_get_adapter(pdata->deferred_nr);
	if (!deferred_adap)
		return -EPROBE_DEFER;
	i2c_put_adapter(deferred_adap);

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	if (pdata->irq) {
		priv->irq = pdata->irq;
	} else {
		priv->irq = platform_get_irq(pdev, 0);
		if (priv->irq < 0) {
			dev_err(&pdev->dev, "Failed to get platform irq: %d\n",
				priv->irq);
			return priv->irq;
		}
	}

	priv->regmap = pdata->regmap;
	priv->dev = pdev->dev.parent;
	priv->pdev = pdev;

	err = devm_request_irq(&pdev->dev, priv->irq,
			       mlxreg_hotplug_irq_handler, IRQF_TRIGGER_FALLING
			       | IRQF_SHARED, "mlxreg-hotplug", priv);
	if (err) {
		dev_err(&pdev->dev, "Failed to request irq: %d\n", err);
		return err;
	}

	if (pdata->cell)
		disable_irq(priv->irq);

	spin_lock_init(&priv->lock);
	INIT_DELAYED_WORK(&priv->dwork_irq, mlxreg_hotplug_work_handler);
	dev_set_drvdata(&pdev->dev, priv);

	err = mlxreg_hotplug_attr_init(priv);
	if (err) {
		dev_err(&pdev->dev, "Failed to allocate attributes: %d\n",
			err);
		return err;
	}

	priv->hwmon = devm_hwmon_device_register_with_groups(&pdev->dev,
					"mlxreg_hotplug", priv, priv->groups);
	if (IS_ERR(priv->hwmon)) {
		dev_err(&pdev->dev, "Failed to register hwmon device %ld\n",
			PTR_ERR(priv->hwmon));
		return PTR_ERR(priv->hwmon);
	}

	/* Register hotplug genetlink familiy. */
	err = mlxreg_hotplug_nl_init_once();
	if (err)
		return err;

	list_add(&priv->list, &mlxreg_hotplug_list.list);

	/* Verify deferred interrupt setting configuration. */
	if (pdata->deferred_irq_set)
		return 0;

	/* Perform initial interrupts setup. */
	mlxreg_hotplug_set_irq(priv);
	priv->after_probe = true;

	return 0;
}

static int mlxreg_hotplug_remove(struct platform_device *pdev)
{
	struct mlxreg_hotplug_priv_data *priv = dev_get_drvdata(&pdev->dev);

	if (!list_empty(&mlxreg_hotplug_list.list))
		list_del_rcu(&mlxreg_hotplug_list.list);

	mlxreg_hotplug_nl_release_once();

	/* Clean interrupts setup. */
	mlxreg_hotplug_unset_irq(priv);

	return 0;
}

static struct platform_driver mlxreg_hotplug_driver = {
	.driver = {
		.name = "mlxreg-hotplug",
	},
	.probe = mlxreg_hotplug_probe,
	.remove = mlxreg_hotplug_remove,
};

module_platform_driver(mlxreg_hotplug_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@mellanox.com>");
MODULE_DESCRIPTION("Mellanox regmap hotplug platform driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("platform:mlxreg-hotplug");
