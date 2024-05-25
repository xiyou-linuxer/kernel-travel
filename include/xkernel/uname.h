#ifndef __UNAME_H
#define __UNAME_H

struct utsname {
	char sysname[30];
	char nodename[30];
	char release[30];
	char version[30];
	char machine[30];
};

int sys_uname(struct utsname* uts);


#endif
