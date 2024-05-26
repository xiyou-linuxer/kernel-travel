#include <xkernel/uname.h>
#include <xkernel/stdio.h>

static struct utsname utsname = {
	.sysname = "XS",
	.nodename = "Xkernel",
	.release = "0.1",
	.version = "0.1",
	.machine = "loongarch",
};


int sys_uname(struct utsname* uts)
{
	*uts = utsname;
	return 0;
}
