/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> // !!TB
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h> //!!TB
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <wlutils.h>

#include "shutils.h"
#include "shared.h"

#if 0
#define _dprintf	cprintf
#else
#define _dprintf(args...)	do { } while(0)
#endif


int get_wan_proto(void)
{
	const char *names[] = {	// order must be synced with def at shared.h
		"static",
		"dhcp",
//	hbobs		"heartbeat",
		"l2tp",
		"pppoe",
		"pptp",
		NULL
	};
	int i;
	const char *p;

	p = nvram_safe_get("wan_proto");
	for (i = 0; names[i] != NULL; ++i) {
		if (strcmp(p, names[i]) == 0) return i + 1;
	}
	return WP_DISABLED;
}

int using_dhcpc(void)
{
#if 0	// hbobs
	const char *proto = nvram_safe_get("wan_proto");
	return (strcmp(proto, "dhcp") == 0) || (strcmp(proto, "l2tp") == 0) || (strcmp(proto, "heartbeat") == 0);
#endif

	switch (get_wan_proto()) {
	case WP_DHCP:
	case WP_L2TP:
		return 1;
	}
	return 0;
}

int wl_client(void)
{
	return ((nvram_match("wl_mode", "sta")) || (nvram_match("wl_mode", "wet")));
}

void notice_set(const char *path, const char *format, ...)
{
	char p[256];
	char buf[2048];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	mkdir("/var/notice", 0755);
	snprintf(p, sizeof(p), "/var/notice/%s", path);
	f_write_string(p, buf, 0, 0);
	if (buf[0]) syslog(LOG_INFO, "notice[%s]: %s", path, buf);
}


//	#define _x_dprintf(args...)	syslog(LOG_DEBUG, args);
#define _x_dprintf(args...)	do { } while (0);

int check_wanup(void)
{
	int up = 0;
	int proto;
	char buf1[64];
	char buf2[64];
	const char *name;
    int f;
    struct ifreq ifr;

	proto = get_wan_proto();
	if (proto == WP_DISABLED) return 0;

	if ((proto == WP_PPTP) || (proto == WP_L2TP) || (proto == WP_PPPOE)) {
		if (f_read_string("/tmp/ppp/link", buf1, sizeof(buf1)) > 0) {
				// contains the base name of a file in /var/run/ containing pid of a daemon
				snprintf(buf2, sizeof(buf2), "/var/run/%s.pid", buf1);
				if (f_read_string(buf2, buf1, sizeof(buf1)) > 0) {
					name = psname(atoi(buf1), buf2, sizeof(buf2));
					if (proto == WP_PPPOE) {
						if (strcmp(name, "pppoecd") == 0) up = 1;
					}
					else {
						if (strcmp(name, "pppd") == 0) up = 1;
					}
				}
				else {
					_dprintf("%s: error reading %s\n", __FUNCTION__, buf2);
				}
			if (!up) {
				unlink("/tmp/ppp/link");
				_x_dprintf("required daemon not found, assuming link is dead\n");
			}
		}
		else {
			_x_dprintf("%s: error reading %s\n", __FUNCTION__, "/tmp/ppp/link");
		}
	}
	else if (!nvram_match("wan_ipaddr", "0.0.0.0")) {
		up = 1;
	}
	else {
		_x_dprintf("%s: default !up\n", __FUNCTION__);
	}

	if ((up) && ((f = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)) {
		strlcpy(ifr.ifr_name, nvram_safe_get("wan_iface"), sizeof(ifr.ifr_name));
		if (ioctl(f, SIOCGIFFLAGS, &ifr) < 0) {
			up = 0;
			_x_dprintf("%s: SIOCGIFFLAGS\n", __FUNCTION__);
		}
		close(f);
		if ((ifr.ifr_flags & IFF_UP) == 0) {
			up = 0;
			_x_dprintf("%s: !IFF_UP\n", __FUNCTION__);
		}
	}

	return up;
}


const dns_list_t *get_dns(void)
{
	static dns_list_t dns;
	char s[512];
	int n;
	int i, j;
	struct in_addr ia;
	char d[4][16];

	dns.count = 0;

	strlcpy(s, nvram_safe_get("wan_dns"), sizeof(s));
	if ((nvram_match("dns_addget", "1")) || (s[0] == 0)) {
		n = strlen(s);
		snprintf(s + n, sizeof(s) - n, " %s", nvram_safe_get("wan_get_dns"));
	}

	n = sscanf(s, "%15s %15s %15s %15s", d[0], d[1], d[2], d[3]);
	for (i = 0; i < n; ++i) {
		if (inet_pton(AF_INET, d[i], &ia) > 0) {
			for (j = dns.count - 1; j >= 0; --j) {
				if (dns.dns[j].s_addr == ia.s_addr) break;
			}
			if (j < 0) {
				dns.dns[dns.count++].s_addr = ia.s_addr;
				if (dns.count == 3) break;
			}
		}
	}

	return &dns;
}

// -----------------------------------------------------------------------------

void set_action(int a)
{
	int r = 3;
	while (f_write("/var/lock/action", &a, sizeof(a), 0, 0) != sizeof(a)) {
		sleep(1);
		if (--r == 0) return;
	}
	if (a != ACT_IDLE) sleep(2);
}

int check_action(void)
{
	int a;
	int r = 3;

	while (f_read("/var/lock/action", &a, sizeof(a)) != sizeof(a)) {
		sleep(1);
		if (--r == 0) return ACT_UNKNOWN;
	}
	return a;
}

int wait_action_idle(int n)
{
	while (n-- > 0) {
		if (check_action() == ACT_IDLE) return 1;
		sleep(1);
	}
	return 0;
}

// -----------------------------------------------------------------------------

const char *get_wanip(void)
{
	const char *p;

	if (!check_wanup()) return "0.0.0.0";
	switch (get_wan_proto()) {
	case WP_DISABLED:
		return "0.0.0.0";
	case WP_PPTP:
		p = "pptp_get_ip";
		break;
	case WP_L2TP:
		p = "l2tp_get_ip";
		break;
	default:
		p = "wan_ipaddr";
		break;
	}
	return nvram_safe_get(p);
}

long get_uptime(void)
{
	struct sysinfo si;
	sysinfo(&si);
	return si.uptime;
}

int get_radio(void)
{
	uint32 n;

	return (wl_ioctl(nvram_safe_get("wl_ifname"), WLC_GET_RADIO, &n, sizeof(n)) == 0) &&
		((n & WL_RADIO_SW_DISABLE)  == 0);
}

void set_radio(int on)
{
	uint32 n;

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
	n = on ? (WL_RADIO_SW_DISABLE << 16) : ((WL_RADIO_SW_DISABLE << 16) | 1);
	wl_ioctl(nvram_safe_get("wl_ifname"), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_WLAN, 0);
		led(LED_DIAG, 0);
	}
#else
	n = on ? 0 : WL_RADIO_SW_DISABLE;
	wl_ioctl(nvram_safe_get("wl_ifname"), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_DIAG, 0);
	}
#endif
}

int nvram_get_int(const char *key)
{
	return atoi(nvram_safe_get(key));
}

/*
long nvram_xget_long(const char *name, long min, long max, long def)
{
	const char *p;
	char *e;
	long n;

	p = nvram_get(name);
	if ((p != NULL) && (*p != 0)) {
		n = strtol(p, &e, 0);
		if ((e != p) && ((*e == 0) || (*e == ' ')) && (n > min) && (n < max)) {
			return n;
		}
	}
	return def;
}
*/

int nvram_get_file(const char *key, const char *fname, int max)
{
	char b[2048];
	int n;
	char *p;

	p = nvram_safe_get(key);
	n = strlen(p);
	if (n <= max) {
		n = base64_decode(p, b, n);
		if (n > 0) return (f_write(fname, b, n, 0, 0700) == n);
	}
	return 0;
}

int nvram_set_file(const char *key, const char *fname, int max)
{
	char a[2048];
	char b[4096];
	int n;

	if (((n = f_read(fname, &a, sizeof(a))) > 0) && (n <= max)) {
		n = base64_encode(a, b, n);
		b[n] = 0;
		nvram_set(key, b);
		return 1;
	}
	return 0;
}

int nvram_contains_word(const char *key, const char *word)
{
	return (find_word(nvram_safe_get(key), word) != NULL);
}

int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, int timeout)
{
	fd_set fds;
	struct timeval tv;
	int flags;
	int n;
	int r;

	if (((flags = fcntl(fd, F_GETFL, 0)) < 0) ||
		(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)) {
		_dprintf("%s: error in F_*ETFL %d\n", __FUNCTION__, fd);
		return -1;
	}

	if (connect(fd, addr, len) < 0) {
//		_dprintf("%s: connect %d = <0\n", __FUNCTION__, fd);

		if (errno != EINPROGRESS) {
			_dprintf("%s: error in connect %d errno=%d\n", __FUNCTION__, fd, errno);
			return -1;
		}
		
		while (1) {
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			r = select(fd + 1, NULL, &fds, NULL, &tv);
			if (r == 0) {
				_dprintf("%s: timeout in select %d\n", __FUNCTION__, fd);
				return -1;
			}
			else if (r < 0) {
				if (errno != EINTR) {
					_dprintf("%s: error in select %d\n", __FUNCTION__, fd);
					return -1;
				}
				// loop
			}
			else {
				r = 0;
				n = sizeof(r);
				if ((getsockopt(fd, SOL_SOCKET, SO_ERROR, &r, &n) < 0) || (r != 0)) {
					_dprintf("%s: error in SO_ERROR %d\n", __FUNCTION__, fd);
					return -1;
				}
				break;
			}
		}
	}
	
	if (fcntl(fd, F_SETFL, flags) < 0) {
		_dprintf("%s: error in F_*ETFL %d\n", __FUNCTION__, fd);
		return -1;
	}	

//	_dprintf("%s: OK %d\n", __FUNCTION__, fd);
	return 0;
}

/*
int time_ok(void)
{
	return time(0) > Y2K;
}
*/

// -----------------------------------------------------------------------------
//!!TB - USB Support

char *detect_fs_type(char *device)
{
	int fd;
	unsigned char buf[4096];
	
	if ((fd = open(device, O_RDONLY)) < 0)
		return NULL;
		
	if (read(fd, buf, sizeof(buf)) != sizeof(buf))
	{
		close(fd);
		return NULL;
	}
	
	close(fd);
	
	/* first check for mbr */
	if (*device && device[strlen(device) - 1] > '9' &&
		buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
		((buf[0x1be] | buf[0x1ce] | buf[0x1de] | buf[0x1ee]) & 0x7f) == 0) /* boot flags */ 
	{
		return "mbr";
	} 
	/* detect swap */
	else if (memcmp(buf + 4086, "SWAPSPACE2", 10) == 0 ||
		memcmp(buf + 4086, "SWAP-SPACE", 10) == 0)
	{
		return "swap";
	}
	/* detect ext2/3 */
	else if (buf[0x438] == 0x53 && buf[0x439] == 0xEF)
	{
		return ((buf[0x460] & 0x0008 /* JOURNAL_DEV */) != 0 ||
			(buf[0x45c] & 0x0004 /* HAS_JOURNAL */) != 0) ? "ext3" : "ext2";
	}
	/* detect ntfs */
	else if (buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
		memcmp(buf + 3, "NTFS    ", 8) == 0)
	{
		return "ntfs";
	}
	/* detect vfat */
	else if (buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
		buf[11] == 0 && buf[12] >= 1 && buf[12] <= 8 /* sector size 512 - 4096 */ &&
		buf[13] != 0 && (buf[13] & (buf[13] - 1)) == 0) /* sectors per cluster */
	{
		return "vfat";
	}

	return NULL;
}


/* Execute a function for each disc partition on the specified controller.
 *
 * Directory /dev/discs/ looks like this:
 * disc0 -> ../scsi/host0/bus0/target0/lun0/
 * disc1 -> ../scsi/host1/bus0/target0/lun0/
 * disc2 -> ../scsi/host2/bus0/target0/lun0/
 * disc3 -> ../scsi/host2/bus0/target0/lun1/
 *
 * Scsi host 2 supports multiple drives.
 * Scsi host 0 & 1 support one drive.
 *
 * For attached drives, like this.  If not attached, there is no "part#" item.
 * Here, only one drive, with 2 partitions, is plugged in.
 * /dev/discs/disc0/disc
 * /dev/discs/disc0/part1
 * /dev/discs/disc0/part2
 * /dev/discs/disc1/disc
 * /dev/discs/disc2/disc
 *
 * Which is the same as:
 * /dev/scsi/host0/bus0/target0/lun0/disc
 * /dev/scsi/host0/bus0/target0/lun0/part1
 * /dev/scsi/host0/bus0/target0/lun0/part2
 * /dev/scsi/host1/bus0/target0/lun0/disc
 * /dev/scsi/host2/bus0/target0/lun0/disc
 * /dev/scsi/host2/bus0/target0/lun1/disc
 *
 * Implementation notes:
 * Various mucking about with a disc that just got plugged in or unplugged
 * will make the scsi subsystem try a re-validate, and read the partition table of the disc.
 * This will make sure the partitions show up.
 *
 * It appears to try to do the revalidate and re-read & update the partition
 * information when this code does the "readdir of /dev/discs/disc0/?".  If the
 * disc has any mounted partitions the revalidate will be rejected.  So the
 * current partition info will remain.  On an unplug event, when it is doing the
 * readdir's, it will try to do the revalidate as we are doing the readdir's.
 * But luckily they'll be rejected, otherwise the later partitions will disappear as
 * soon as we get the first one.
 * But be very careful!  If something goes not exactly right, the partition entries
 * will disappear before we've had a chance to unmount from them.
 *
 * To avoid this automatic revalidation, we go through /proc/partitions looking for the partitions
 * that /dev/discs point to.  That will avoid the implicit revalidate attempt.
 * Which means that we had better do it ourselves.  An ioctl BLKRRPART does just that.
 * 
 * 
 * If host < 0, do all hosts.   If >= 0, it is the host number to do.
 * When_to_update, flags:
 *	0x01 = before reading partition info
 *	0x02 = after reading partition info
 *
 */

/* So as not to include linux/fs.h, let's explicitly do this here. */
#ifndef BLKRRPART
#define BLKRRPART	_IO(0x12,95)	/* re-read partition table */
#endif

int exec_for_host(int host, int when_to_update, uint flags, host_exec func)
{
	DIR *usb_dev_disc;
	char bfr[64];	/* Will be: /dev/discs/disc#					*/
	char link[256];	/* Will be: ../scsi/host#/bus0/target0/lun#  that bfr links to. */
			/* When calling the func, will be: /dev/discs/disc#/part#	*/
	char bfr2[64];	/* Will be: /dev/discs/disc#/disc     for the BLKRRPART.	*/
	int fd;
	char *cp;
	int len;
	int host_no;	/* SCSI controller/host # */
	int disc_num;	/* Disc # */
	int part_num;	/* Parition # */
	struct dirent *dp;
	FILE *prt_fp;
	char *mp;	/* Ptr to after any leading ../ path */
	int siz;
	char line[256];
	int result = 0;

	flags |= EFH_1ST_HOST;
	if ((usb_dev_disc = opendir(DEV_DISCS_ROOT))) {
		while ((dp = readdir(usb_dev_disc))) {
			sprintf(bfr, "%s/%s", DEV_DISCS_ROOT, dp->d_name);
			if (strncmp(dp->d_name, "disc", 4) != 0)
				continue;

			disc_num = atoi(dp->d_name + 4);
			len = readlink(bfr, link, sizeof(link) - 1);
			if (len < 0)
				continue;

			link[len] = 0;
			cp = strstr(link, "/scsi/host");
			if (!cp)
				continue;

			host_no = atoi(cp + 10);
			if (host >= 0 && host_no != host)
				continue;

			/* We have found a disc that is on this controller.
			 * Loop thru all the partitions on this disc.
			 * The new way, reading thru /proc/partitions.
			 */
			mp = link;
			if ((cp = strstr(link, "../")) != NULL)
				mp = cp + 3;
			strcat(mp, "/part");
			siz = strlen(mp);

			sprintf(bfr2, "%s/disc", bfr);	/* Prepare for BLKRRPART */
			if (when_to_update & 0x01) {
				if ((fd = open(bfr2, O_RDONLY | O_NONBLOCK)) >= 0) {
					ioctl(fd, BLKRRPART);
					close(fd);
				}
			}

			flags |= EFH_1ST_DISC;
			if (func && (prt_fp = fopen("/proc/partitions", "r"))) {
				while (fgets(line, sizeof(line) - 2, prt_fp)) {
					if (line[0] != ' ')
						continue;
					for (cp = line; isdigit(*cp) || isblank(*cp); ++cp)
						;
					if (strncmp(cp, mp, siz) == 0) {
						part_num = atoi(cp + siz);
						sprintf(line, "%s/part%d", bfr, part_num);
						result = (*func)(line, host_no, disc_num, part_num, flags) || result;
						flags &= ~(EFH_1ST_HOST | EFH_1ST_DISC);
					}
				}
				fclose(prt_fp);
			}

			if (when_to_update & 0x02) {
				if ((fd = open(bfr2, O_RDONLY | O_NONBLOCK)) >= 0) {
					ioctl(fd, BLKRRPART);
					close(fd);
				}
			}
		}
		closedir(usb_dev_disc);
	}
	return result;
}

/* Stolen from the e2fsprogs/ismounted.c.
 * Find wherever 'file' (actually: device) is mounted.
 * Either the exact same device-name, or another device-name.
 * The latter is detected by comparing the rdev or dev&inode.
 * So aliasing won't fool us---we'll still find if it's mounted.
 * Return its mnt entry.
 * In particular, the caller would look at the mnt->mountpoint.
 */
struct mntent *findmntent(char *file)
{
	struct mntent 	*mnt;
	struct stat	st_buf;
	dev_t		file_dev=0, file_rdev=0;
	ino_t		file_ino=0;
	FILE 		*f;
	
	if ((f = setmntent("/proc/mounts", "r")) == NULL)
		return NULL;

	if (stat(file, &st_buf) == 0) {
		if (S_ISBLK(st_buf.st_mode)) {
			file_rdev = st_buf.st_rdev;
		} else {
			file_dev = st_buf.st_dev;
			file_ino = st_buf.st_ino;
		}
	}
	while ((mnt = getmntent(f)) != NULL) {
		if (strcmp(file, mnt->mnt_fsname) == 0)
			break;

		if (stat(mnt->mnt_fsname, &st_buf) == 0) {
			if (S_ISBLK(st_buf.st_mode)) {
				if (file_rdev && (file_rdev == st_buf.st_rdev))
					break;
			} else {
				if (file_dev && ((file_dev == st_buf.st_dev) &&
						 (file_ino == st_buf.st_ino)))
					break;
			}
		}
	}

	fclose(f);
	return mnt;
}

/* Figure out the volume label. */
/* If we found it, return Non-Zero and store it at the_label. */
#define USE_BB	0	/* Use the actual busybox .a file. */

#if USE_BB
char applet_name[] = "hotplug";	/* Needed to satisfy a reference. */
int find_label(char *mnt_dev, char *the_label)
{
	char *uuid, *label;
	int i;
   
	*the_label = 0;
	/* heuristic: partition name ends in a digit */
	if ( !isdigit(mnt_dev[strlen(mnt_dev) - 1]))
		return(0);

	cprintf("\n\nGetting volume label for '%s'\n", mnt_dev);

	/* it's in get_devname.c  */
	i = get_label_uuid(mnt_dev, &label, &uuid, 0);
	cprintf("get_label_uuid= %d\n", i);
	if (i == 0) {
		cprintf(" label= %s uuid = %s\n", label, uuid);
		strcpy(the_label, label);
		return(1);
	}
	return (0);
}
#else
int find_label(char *mnt_dev, char *the_label)
{
	char *label, *p;
	FILE *mfp;
	char buf[128];
	struct stat st_arg, st_mou;

	*the_label = 0;
	/* heuristic: partition name ends in a digit */
	if ( !isdigit(mnt_dev[strlen(mnt_dev) - 1]))
		return(0);

	mfp = popen("mount LABEL= /no-such-label", "r");
	if (mfp == NULL) {
		_dprintf("Cannot popen mount!!\n");
		return(0);
	}

	while (fgets(buf, sizeof(buf), mfp)) {
		if ((p = strchr(buf, '\n')))
			*p = 0;
		if ((p = strstr(buf, "LABEL "))) {	/* Isolate the label. */
			if ((label=strchr(p+6, '\''))) {
				++label;
				if ((p = strchr(label, '\''))) {
					*p++ = 0;
					if ((p = strchr(p, '/'))) {	/* Isolate the device. */
						/* See if the argument is the same as what mount gives. */
						if (stat(mnt_dev, &st_arg) == 0 && stat(p, &st_mou) == 0) {
							if (S_ISBLK(st_arg.st_mode) && S_ISBLK(st_mou.st_mode) &&
									st_arg.st_rdev == st_mou.st_rdev) {
								strcpy(the_label, label);
								fclose(mfp);
								return(1);
							}
						}
					}
				}
			}
		}
	}
	fclose(mfp);
	return(0);
}
#endif
