/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note) */
#ifndef _UAPI_LINUX_MLXREG_H
#define _UAPI_LINUX_MLXREG_H

#define MLXREG_GENL_NAME_LENGTH		42
#define MLXREG_NL_CONTROL		(NLMSG_MIN_TYPE)
#define MLXREG_NL_REGISTER		(NLMSG_MIN_TYPE + 1)
#define MLXREG_NL_UNREGISTER		(NLMSG_MIN_TYPE + 2)
#define MLXREG_NL_EVENT			(NLMSG_MIN_TYPE + 3)
#define MLXREG_NL_ENABLE		(NLMSG_MIN_TYPE + 4)
#define MLXREG_NL_DISABLE		(NLMSG_MIN_TYPE + 5)

#define MLXREG_HOTPLUG_GET_MSG_TYPE(t)	(t & GENMASK(7, 0))
#define MLXREG_HOTPLUG_GET_DEVID(d)	((d & GENMASK(15, 8)) >> 8)

/**
 * struct mlxreg_hotplug_event - mlxreg netlink hotplug event:
 *
 * @id: user process id;
 * @label: generic mlxreg netlink family event label;
 * @nr: I2C device adapter number, to which device is to be attached;
 * @event: generic mlxreg netlink family event value;
 */
struct mlxreg_hotplug_event {
	unsigned int id;
	char label[MLXREG_GENL_NAME_LENGTH];
	int nr;
	char event;
	char simulated;
};

#endif /* _UAPI_LINUX_MLXREG_H */
