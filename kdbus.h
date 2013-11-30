/*
 * Copyright (C) 2013 Kay Sievers
 * Copyright (C) 2013 Greg Kroah-Hartman <gregkh@linuxfoundation.org>
 * Copyright (C) 2013 Linux Foundation
 * Copyright (C) 2013 Lennart Poettering
 * Copyright (C) 2013 Daniel Mack <daniel@zonque.org>
 *
 * kdbus is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

#ifndef _KDBUS_H_
#define _KDBUS_H_

#ifndef __KERNEL__
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/types.h>
#endif

#define KDBUS_IOC_MAGIC			0x95
#define KDBUS_SRC_ID_KERNEL		(0)
#define KDBUS_DST_ID_WELL_KNOWN_NAME	(0)
#define KDBUS_MATCH_SRC_ID_ANY		(~0ULL)
#define KDBUS_DST_ID_BROADCAST		(~0ULL)

/* Common first elements in a structure which are used to iterate over
 * a list of elements. */
#define KDBUS_PART_HEADER \
	struct {							\
		__u64 size;						\
		__u64 type;						\
	}

/* Message sent from kernel to userspace, when the owner or starter of
 * a well-known name changes */
struct kdbus_notify_name_change {
	__u64 old_id;
	__u64 new_id;
	__u64 flags;			/* 0 or (possibly?) KDBUS_NAME_IN_QUEUE */
	char name[0];
};

struct kdbus_notify_id_change {
	__u64 id;
	__u64 flags;			/* The kernel flags field from KDBUS_HELLO */
};

struct kdbus_creds {
	__u64 uid;
	__u64 gid;
	__u64 pid;
	__u64 tid;

	/* The starttime of the process PID. This is useful to detect
	PID overruns from the client side. i.e. if you use the PID to
	look something up in /proc/$PID/ you can afterwards check the
	starttime field of it to ensure you didn't run into a PID
	ovretun. */
	__u64 starttime;
};

struct kdbus_audit {
	__u64 sessionid;
	__u64 loginuid;
};

struct kdbus_timestamp {
	__u64 monotonic_ns;
	__u64 realtime_ns;
};

struct kdbus_vec {
	__u64 size;
	union {
		__u64 address;
		__u64 offset;
	};
};

struct kdbus_memfd {
	__u64 size;
	int fd;
	__u32 __pad;
};

/* Message Item Types */
enum {
	_KDBUS_ITEM_NULL,

	/* Filled in by userspace */
	_KDBUS_ITEM_USER_BASE	= 1,
	KDBUS_ITEM_PAYLOAD_VEC	= 1,	/* .data_vec, reference to memory area */
	KDBUS_ITEM_PAYLOAD_OFF,		/* .data_vec, reference to memory area */
	KDBUS_ITEM_PAYLOAD_MEMFD,	/* file descriptor of a special data file */
	KDBUS_ITEM_FDS,			/* .data_fds of file descriptors */
	KDBUS_ITEM_BLOOM,		/* for broadcasts, carries bloom filter blob in .data */
	KDBUS_ITEM_DST_NAME,		/* destination's well-known name, in .str */
	KDBUS_ITEM_PRIORITY,		/* queue priority for message */

	/* Filled in by kernelspace */
	_KDBUS_ITEM_ATTACH_BASE	= 0x400,
	KDBUS_ITEM_NAMES	= 0x400,/* NUL separated string list with well-known names of source */
	KDBUS_ITEM_STARTER_NAME,	/* Only used in HELLO for starter connection */
	KDBUS_ITEM_TIMESTAMP,		/* .timestamp */

	/* when appended to a message, the following items refer to the sender */
	KDBUS_ITEM_CREDS,		/* .creds */
	KDBUS_ITEM_PID_COMM,		/* optional, in .str */
	KDBUS_ITEM_TID_COMM,		/* optional, in .str */
	KDBUS_ITEM_EXE,			/* optional, in .str */
	KDBUS_ITEM_CMDLINE,		/* optional, in .str (a chain of NUL str) */
	KDBUS_ITEM_CGROUP,		/* optional, in .str */
	KDBUS_ITEM_CAPS,		/* caps data blob, in .data */
	KDBUS_ITEM_SECLABEL,		/* NUL terminated string, in .str */
	KDBUS_ITEM_AUDIT,		/* .audit */

	/* Special messages from kernel, consisting of one and only one of these data blocks */
	_KDBUS_ITEM_KERNEL_BASE	= 0x800,
	KDBUS_ITEM_NAME_ADD	= 0x800,/* .name_change */
	KDBUS_ITEM_NAME_REMOVE,		/* .name_change */
	KDBUS_ITEM_NAME_CHANGE,		/* .name_change */
	KDBUS_ITEM_ID_ADD,		/* .id_change */
	KDBUS_ITEM_ID_REMOVE,		/* .id_change */
	KDBUS_ITEM_REPLY_TIMEOUT,	/* empty, but .reply_cookie in .kdbus_msg is filled in */
	KDBUS_ITEM_REPLY_DEAD,		/* dito */
};

/**
 * struct  kdbus_item - chain of data blocks
 *
 * size: overall data record size
 * type: kdbus_item type of data
 */
struct kdbus_item {
	KDBUS_PART_HEADER;
	union {
		/* inline data */
		__u8 data[0];
		__u32 data32[0];
		__u64 data64[0];
		char str[0];

		/* connection */
		__u64 id;

		/* data vector */
		struct kdbus_vec vec;

		/* process credentials and properties*/
		struct kdbus_creds creds;
		struct kdbus_audit audit;
		struct kdbus_timestamp timestamp;

		/* specific fields */
		struct kdbus_memfd memfd;
		int fds[0];
		struct kdbus_notify_name_change name_change;
		struct kdbus_notify_id_change id_change;
	};
};

enum {
	KDBUS_MSG_FLAGS_EXPECT_REPLY	= 1 << 0,
	KDBUS_MSG_FLAGS_NO_AUTO_START	= 1 << 1,
};

enum {
	KDBUS_PAYLOAD_KERNEL,
	KDBUS_PAYLOAD_DBUS1	= 0x4442757356657231ULL, /* 'DBusVer1' */
	KDBUS_PAYLOAD_GVARIANT	= 0x4756617269616e74ULL, /* 'GVariant' */
};

struct kdbus_msg {
	__u64 size;
	__u64 flags;
	__u64 dst_id;			/* connection, 0 == name in data, ~0 broadcast */
	__u64 src_id;			/* connection, 0 == kernel */
	__u64 payload_type;		/* 'DBusVer1', 'GVariant', ... */
	__u64 cookie;			/* userspace-supplied cookie */
	union {
		__u64 cookie_reply;	/* cookie we reply to */
		__u64 timeout_ns;	/* timespan to wait for reply */
	};
	struct kdbus_item items[0];
};

enum {
	_KDBUS_POLICY_NULL,
	KDBUS_POLICY_NAME,
	KDBUS_POLICY_ACCESS,
};

enum {
	_KDBUS_POLICY_ACCESS_NULL,
	KDBUS_POLICY_ACCESS_USER,
	KDBUS_POLICY_ACCESS_GROUP,
	KDBUS_POLICY_ACCESS_WORLD,
};

enum {
	KDBUS_POLICY_RECV		= 1 <<  2,
	KDBUS_POLICY_SEND		= 1 <<  1,
	KDBUS_POLICY_OWN		= 1 <<  0,
};

struct kdbus_policy_access {
	__u64 type;			/* USER, GROUP, WORLD */
	__u64 bits;			/* RECV, SEND, OWN */
	__u64 id;			/* uid, gid, 0 */
};

struct kdbus_policy {
	KDBUS_PART_HEADER;
	union {
		char name[0];
		struct kdbus_policy_access access;
	};
};

/* A series of KDBUS_POLICY_NAME, plus one or more KDBUS_POLICY_ACCESS */
struct kdbus_cmd_policy {
	__u64 size;
	struct kdbus_policy policies[0];
};

/* Flags for struct kdbus_cmd_hello */
enum {
	KDBUS_HELLO_STARTER		=  1 <<  0,
	KDBUS_HELLO_ACCEPT_FD		=  1 <<  1,
};

/* Flags for message attachments */
enum {
	KDBUS_ATTACH_TIMESTAMP		=  1 <<  0,
	KDBUS_ATTACH_CREDS		=  1 <<  1,
	KDBUS_ATTACH_NAMES		=  1 <<  2,
	KDBUS_ATTACH_COMM		=  1 <<  3,
	KDBUS_ATTACH_EXE		=  1 <<  4,
	KDBUS_ATTACH_CMDLINE		=  1 <<  5,
	KDBUS_ATTACH_CGROUP		=  1 <<  6,
	KDBUS_ATTACH_CAPS		=  1 <<  7,
	KDBUS_ATTACH_SECLABEL		=  1 <<  8,
	KDBUS_ATTACH_AUDIT		=  1 <<  9,
};

/* KDBUS_CMD_HELLO */
struct kdbus_cmd_hello {
	__u64 size;

	/* userspace → kernel, kernel → userspace */
	__u64 conn_flags;	/* userspace specifies its
				 * capabilities and more, kernel
				 * returns its capabilites and
				 * more. Kernel might refuse client's
				 * capabilities by returning an error
				 * from KDBUS_HELLO */
	__u64 attach_flags;	/* userspace specifies the metadata
				 * attachments it wishes to receive with
				 * every message. */

	/* kernel → userspace */
	__u64 bus_flags;	/* this is .flags copied verbatim from
				 * from original KDBUS_CMD_BUS_MAKE
				 * ioctl. It's intended to be useful
				 * to do negotiation of features of
				 * the payload that is transfreted. */
	__u64 id;		/* id assigned to this connection */
	__u64 bloom_size;	/* The bloom filter size chosen by the
				 * bus owner */
	__u64 pool_size;	/* maximum size of pool buffer */
	__u8 id128[16];		/* the unique id of the bus */

	struct kdbus_item items[0];
};

/* Flags for kdbus_cmd_{bus,ep,ns}_make */
enum {
	KDBUS_MAKE_ACCESS_GROUP		= 1 <<  0,
	KDBUS_MAKE_ACCESS_WORLD		= 1 <<  1,
	KDBUS_MAKE_POLICY_OPEN		= 1 <<  2,
};

/* Items to append to kdbus_cmd_{bus,ep,ns}_make */
enum {
	_KDBUS_MAKE_NULL,
	KDBUS_MAKE_NAME,
	KDBUS_MAKE_CRED,	/* allow translator services which connect
				 * to the bus on behalf of somebody else,
				 * allow specifying the credentials of the
				 * client to connect on behalf on. Needs
				 * privileges */
};

struct kdbus_cmd_bus_make {
	__u64 size;
	__u64 flags;		/* userspace → kernel, kernel → userspace
				 * When creating a bus feature
				 * kernel negotiation. */
	__u64 bus_flags;	/* userspace → kernel
				 * When a bus is created this value is
				 * copied verbatim into the bus
				 * structure and returned from
				 * KDBUS_CMD_HELLO, later */
	__u64 bloom_size;	/* size of the bloom filter for this bus */
	struct kdbus_item items[0];
};

struct kdbus_cmd_ep_make {
	__u64 size;
	__u64 flags;		/* userspace → kernel, kernel → userspace
				 * When creating an entry point
				 * feature kernel negotiation done the
				 * same way as for
				 * KDBUS_CMD_BUS_MAKE. Unused for
				 * now. */
	struct kdbus_item items[0];
};

struct kdbus_cmd_ns_make {
	__u64 size;
	__u64 flags;		/* userspace → kernel, kernel → userspace
				 * When creating an entry point
				 * feature kernel negotiation done the
				 * same way as for
				 * KDBUS_CMD_BUS_MAKE. Unused for
				 * now. */
	struct kdbus_item items[0];
};

enum {
	/* userspace → kernel */
	KDBUS_NAME_REPLACE_EXISTING		= 1 <<  0,
	KDBUS_NAME_QUEUE			= 1 <<  1,
	KDBUS_NAME_ALLOW_REPLACEMENT		= 1 <<  2,

	/* kernel → userspace */
	KDBUS_NAME_IN_QUEUE			= 1 << 16,
};

/* KDBUS_CMD_NAME_ACQUIRE */
struct kdbus_cmd_name {
	__u64 size;
	__u64 flags;
	__u64 id;		/* we allow (de)registration of names of other peers */
	__u64 conn_flags;
	char name[0];
};

/* KDBUS_CMD_NAME_LIST */
enum {
	KDBUS_NAME_LIST_UNIQUE_NAMES		= 1 <<  0,
};

struct kdbus_cmd_name_list {
	__u64 flags;
	__u64 offset;		/* returned offset in the caller's buffer */
};

struct kdbus_name_list {
	__u64 size;
	struct kdbus_cmd_name names[0];
};

/* KDBUS_CMD_NAME_INFO */
struct kdbus_cmd_name_info {
	__u64 size;
	__u64 flags;			/* query flags */
	__u64 id;			/* either ID, or 0 and name follows */
	__u64 offset;			/* returned offset in the caller's buffer */
	char name[0];
};

struct kdbus_name_info {
	__u64 size;
	__u64 id;
	__u64 flags;			/* connection flags */
	struct kdbus_item items[0];	/* list of item records */
};

/* KDBUS_CMD_MATCH_ADD/REMOVE */
enum {
	_KDBUS_MATCH_NULL,
	KDBUS_MATCH_BLOOM,		/* Matches a mask blob against KDBUS_MSG_BLOOM */
	KDBUS_MATCH_SRC_NAME,		/* Matches a name string against KDBUS_MSG_SRC_NAMES */
	KDBUS_MATCH_NAME_ADD,		/* Matches a name string against KDBUS_MSG_NAME_ADD */
	KDBUS_MATCH_NAME_REMOVE,	/* Matches a name string against KDBUS_MSG_NAME_REMOVE */
	KDBUS_MATCH_NAME_CHANGE,	/* Matches a name string against KDBUS_MSG_NAME_CHANGE */
	KDBUS_MATCH_ID_ADD,		/* Matches an ID against KDBUS_MSG_ID_ADD */
	KDBUS_MATCH_ID_REMOVE,		/* Matches an ID against KDBUS_MSG_ID_REMOVE */
};

struct kdbus_cmd_match {
	__u64 size;
	__u64 id;	/* We allow registration/deregestration of matches for other peers */
	__u64 cookie;	/* userspace supplied cookie; when removing; kernel deletes everything with same cookie */
	__u64 src_id;	/* ~0: any. other: exact unique match */
	struct kdbus_item items[0];
};

/* KDBUS_CMD_MONITOR */
enum {
	KDBUS_MONITOR_ENABLE		= 1 <<  0,
};

struct kdbus_cmd_monitor {
	__u64 id;		/* We allow setting the monitor flag of other peers */
	__u64 flags;
};

enum {
	/* kdbus control node commands: require unset state */
	KDBUS_CMD_BUS_MAKE =		_IOW(KDBUS_IOC_MAGIC, 0x00, struct kdbus_cmd_bus_make),
	KDBUS_CMD_NS_MAKE =		_IOR(KDBUS_IOC_MAGIC, 0x10, struct kdbus_cmd_ns_make),

	/* kdbus ep node commands: require unset state */
	KDBUS_CMD_EP_MAKE =		_IOW(KDBUS_IOC_MAGIC, 0x20, struct kdbus_cmd_ep_make),
	KDBUS_CMD_HELLO =		_IOWR(KDBUS_IOC_MAGIC, 0x30, struct kdbus_cmd_hello),

	/* kdbus ep node commands: require connected state */
	KDBUS_CMD_MSG_SEND =		_IOW(KDBUS_IOC_MAGIC, 0x40, struct kdbus_msg),
	KDBUS_CMD_MSG_RECV =		_IOR(KDBUS_IOC_MAGIC, 0x41, __u64 *),
	KDBUS_CMD_FREE =		_IOW(KDBUS_IOC_MAGIC, 0x42, __u64 *),

	KDBUS_CMD_NAME_ACQUIRE =	_IOWR(KDBUS_IOC_MAGIC, 0x50, struct kdbus_cmd_name),
	KDBUS_CMD_NAME_RELEASE =	_IOW(KDBUS_IOC_MAGIC, 0x51, struct kdbus_cmd_name),
	KDBUS_CMD_NAME_LIST =		_IOWR(KDBUS_IOC_MAGIC, 0x52, struct kdbus_cmd_name_list),
	KDBUS_CMD_NAME_INFO =		_IOWR(KDBUS_IOC_MAGIC, 0x53, struct kdbus_cmd_name_info),

	KDBUS_CMD_MATCH_ADD =		_IOW(KDBUS_IOC_MAGIC, 0x60, struct kdbus_cmd_match),
	KDBUS_CMD_MATCH_REMOVE =	_IOW(KDBUS_IOC_MAGIC, 0x61, struct kdbus_cmd_match),
	KDBUS_CMD_MONITOR =		_IOW(KDBUS_IOC_MAGIC, 0x62, struct kdbus_cmd_monitor),

	/* kdbus ep node commands: require ep owner state */
	KDBUS_CMD_EP_POLICY_SET =	_IOW(KDBUS_IOC_MAGIC, 0x70, struct kdbus_cmd_policy),

	/* kdbus memfd commands: */
	KDBUS_CMD_MEMFD_NEW =		_IOR(KDBUS_IOC_MAGIC, 0x80, int *),
	KDBUS_CMD_MEMFD_SIZE_GET =	_IOR(KDBUS_IOC_MAGIC, 0x81, __u64 *),
	KDBUS_CMD_MEMFD_SIZE_SET =	_IOW(KDBUS_IOC_MAGIC, 0x82, __u64 *),
	KDBUS_CMD_MEMFD_SEAL_GET =	_IOR(KDBUS_IOC_MAGIC, 0x83, int *),
	KDBUS_CMD_MEMFD_SEAL_SET =	_IO(KDBUS_IOC_MAGIC, 0x84),
};
#endif
