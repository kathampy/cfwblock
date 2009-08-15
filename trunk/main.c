#include <pspkernel.h>
#include <systemctrl.h>
#include <pspinit.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAXFDS 16
#define MAXLEN 264

PSP_MODULE_INFO("CFWB", 0x1006, 1, 1);

SceUID fds[MAXFDS];

char *stristr(const char *str1, const char *str2)
{
	char temp1[MAXLEN+1], temp2[MAXLEN+1];
	temp1[MAXLEN] = 0;
	temp2[MAXLEN] = 0;

	strncpy(temp1, str1, MAXLEN);
	strncpy(temp2, str2, MAXLEN);

	int i;
	for (i = 0; i < MAXLEN && (temp1[i] != 0); i++)
	{
		temp1[i] = tolower(temp1[i]);
	}
	for (i = 0; i < MAXLEN && (temp2[i] != 0); i++)
	{
		temp2[i] = tolower(temp2[i]);
	}

	const char *pos = strstr(temp1, temp2);
	if (pos)
	{
		return (char *)(pos - temp1 + str1);
	}
	return 0;
}

SceUID sceIoOpenPatched(const char *file, int flags, SceMode mode)
{
	if (stristr(file, "ms0:/iso") || stristr(file, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoOpen(file, flags, mode);
	return res;
}

int sceIoRenamePatched(const char *oldname, const char *newname)
{
	if (stristr(oldname, "ms0:/iso") || stristr(oldname, "ms0:/seplugins") || stristr(newname, "ms0:/iso") || stristr(newname, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoRename(oldname, newname);
	return res;
}

int sceIoRemovePatched(const char *file)
{
	if (stristr(file, "ms0:/iso") || stristr(file, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoRemove(file);
	return res;
}

int sceIoGetstatPatched(const char *file, SceIoStat *stat)
{
	if (stristr(file, "ms0:/iso") || stristr(file, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoGetstat(file, stat);
	return res;
}

int sceIoChstatPatched(const char *file, SceIoStat *stat, int bits)
{
	if (stristr(file, "ms0:/iso") || stristr(file, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoChstat(file, stat, bits);
	return res;
}

SceUID sceIoDopenPatched(const char *dirname)
{
	if (stristr(dirname, "ms0:/iso") || stristr(dirname, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res;
	if ((res = sceIoDopen(dirname)) >= 0)
	{
		unsigned char i = 0;
		while ((i < MAXFDS) && (fds[i] != res))
		{
			i++;
		}
		if (i < MAXFDS)
		{
			while (i < MAXFDS - 1)
			{
				fds[i] = fds[i + 1];
				i++;
			}
			fds[MAXFDS - 1] = 0;
		}
		if (((dirname[0] == 'm') || (dirname[0] == 'M')) && ((dirname[1] == 's') || (dirname[1] == 'S')) && (dirname[2] == '0') && (dirname[3] == ':') && ((dirname[4] == 0) || ((dirname[4] == '/') && (dirname[5] == 0))))
		{
			i = 0;
			while ((i < MAXFDS) && (fds[i] != 0))
			{
				i++;
			}
			if (i < MAXFDS)
			{
				fds[i] = res;
				if (i == MAXFDS-1)
				{
					fds[0] = 0;
				}
				else
				{
					fds[i + 1] = 0;
				}
			}
		}
	}

	return res;
}

int sceIoDreadPatched(SceUID fd, SceIoDirent *dir)
{
	int res;
	unsigned char i;
nextDread:
	res = sceIoDread(fd, dir);

	if (res >= 0)
	{
		i = 0;
		while ((i < MAXFDS) && (fds[i] != fd))
		{
			i++;
		}
		if ((i < MAXFDS) && ((stristr((*dir).d_name, "iso") == (*dir).d_name) || (stristr((*dir).d_name, "seplugins") == (*dir).d_name)))
		{
			memset(dir, 0, sizeof(SceIoDirent));
			if (res == 0)
			{
				return 0x80020142;
			}
			goto nextDread;
		}
	}

	return res;
}

int sceIoChdirPatched(const char *path)
{
	if (stristr(path, "ms0:/iso") || stristr(path, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoChdir(path);
	return res;
}

int sceIoMkdirPatched(const char *dir, SceMode mode)
{
	if (stristr(dir, "ms0:/iso") || stristr(dir, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoMkdir(dir, mode);
	return res;
}

int sceIoRmdirPatched(const char *path)
{
	if (stristr(path, "ms0:/iso") || stristr(path, "ms0:/seplugins"))
	{
		return 0x80020142;
	}

	int res = sceIoRmdir(path);
	return res;
}

int module_start(SceSize args, void *argp)
{
	if (sceKernelBootFrom() == PSP_BOOT_MS)
	{
		char *eboot = sceKernelInitFileName();

		SceUID fd = sceIoOpen(eboot, PSP_O_RDONLY, 0);
		if (fd >= 0)
		{
			u32 header[10];
			if (sceIoRead(fd, header, sizeof(header)) == sizeof(header))
			{
				sceIoLseek(fd, header[8], PSP_SEEK_SET);
				if (sceIoRead(fd, header, sizeof(u32)) == sizeof(u32))
				{
					if (header[0] == 0x464C457F)
					{
						sceIoClose(fd);
						return 1;
					}
					else if (header[0] == 0x5053507E)
					{
						sceIoLseek(fd, header[8]+336, PSP_SEEK_SET);
						if (sceIoRead(fd, header, sizeof(u32)) == sizeof(u32))
						{
							header[0] &= 0x0000FFFF;
							if (header[0] == 0x00008B1F)
							{
								sceIoClose(fd);
								return 1;
							}
						}
					}
				}
			}
			sceIoClose(fd);
		}
	}

	memset(fds, 0, sizeof(fds));

	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x06A70004), sceIoMkdirPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x109F50BC), sceIoOpenPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x1117C65F), sceIoRmdirPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x55F4717D), sceIoChdirPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x779103A0), sceIoRenamePatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xACE946E8), sceIoGetstatPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xB29DDF9C), sceIoDopenPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xB8A740F4), sceIoChstatPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xE3EB004C), sceIoDreadPatched);
	sctrlHENPatchSyscall(sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xF27A9C51), sceIoRemovePatched);

	sceKernelDcacheWritebackAll();
	sceKernelIcacheClearAll();

	return 0;
}

int module_stop(SceSize args, void *argp)
{
	sctrlHENPatchSyscall((u32)sceIoMkdirPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x06A70004));
	sctrlHENPatchSyscall((u32)sceIoOpenPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x109F50BC));
	sctrlHENPatchSyscall((u32)sceIoRmdirPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x1117C65F));
	sctrlHENPatchSyscall((u32)sceIoChdirPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x55F4717D));
	sctrlHENPatchSyscall((u32)sceIoRenamePatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0x779103A0));
	sctrlHENPatchSyscall((u32)sceIoGetstatPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xACE946E8));
	sctrlHENPatchSyscall((u32)sceIoDopenPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xB29DDF9C));
	sctrlHENPatchSyscall((u32)sceIoChstatPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xB8A740F4));
	sctrlHENPatchSyscall((u32)sceIoDreadPatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xE3EB004C));
	sctrlHENPatchSyscall((u32)sceIoRemovePatched, (void *)sctrlHENFindFunction("sceIOFileManager", "IoFileMgrForUser", 0xF27A9C51));

	sceKernelDcacheWritebackAll();
	sceKernelIcacheClearAll();

	return 0;
}
