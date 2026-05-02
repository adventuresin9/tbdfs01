#include <u.h>
#include <libc.h>
#include <auth.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"


TBDbase tbdbase;
TBDtable *tbdtab;
TBDnode	*tbdroot;
TBDnode *nametree;
int tabsz;
long time0;
char *user;
int tbddebug;

void
tbdmakebase(void)
{
	int n, i, w, l;
	char buf[Ssz * 2];
	int fd;
	Dir *rd;

	n = dirreadall(tbdbase.bfd, &rd);

	tbdbase.mfd = create(tbdbase.meta, ORDWR, 0777|DMDIR);
//!!!add all standard metadata, use ℙname, ℙmtine, etc.
	for(i = 0; i < n; i++){
		memset(buf, 0, Ssz);
		snprint(buf, Ssz, "%s/%s.meta", tbdbase.meta, rd[i].name);
		fd = create(buf, ORDWR, 0666);
		memset(buf, 0, Ssz);
//		snprint(buf, Ssz, "ℙname/text/%s\nℙlength/integer/%lld\nℙmtime/number/%lud\n", 
//			rd[i].name, rd[i].length, rd[i].mtime);
/* 9P doesn't import mtime, ctime, or atime, all files copied together get same mtime */
		snprint(buf, Ssz, "ℙname/text/%s\nℙlength/integer/%lld\n", 
			rd[i].name, rd[i].length);
		l = strlen(buf);
		w = write(fd, buf, l);
		if(w < 0)
			sysfatal("write: %s", rd[i].name);
		close(fd);
	}

	print("wrote %d files\n", n);
}


Srv tbdsrv = {
	.start = tbdfsstart,
	.attach = tbdfsattach,
	.clone = tbdfsclone,
	.destroyfid = tbdfsdestroyfid,
	.open = tbdfsopen,
	.read = tbdfsread,
	.stat = tbdfsstat,
	.walk1 = tbdfswalk1,
};


static void
usage(void)
{
	fprint(2, "usage: %s [-b base] [-s base] [-c]\n", argv0);
	exits("usage");
}


void
main(int argc, char **argv)
{

	char *base = nil;
	char *meta = nil;
	char *srvname = "tbdfs";
	char *mtpt = "/n/tbd";
	int start = 0;


	tbddebug = 0;

	ARGBEGIN{
	case 'b':
		base = EARGF(usage());
		break;
	case 's':
		base = EARGF(usage());
		start = 1;
		break;
	case 'c':
		chatty9p = 1;
		break;
	case 'd':
		tbddebug++;
		break;
	default:
		usage();
		break;
	}ARGEND;
	if(base == nil)
		usage();


	if((tbdbase.bfd = open(base, ORDWR)) == -1)
		sysfatal("can't access base directory");

	user = getuser();
	time0 = time(0);

	meta = mallocz(256, 0);

	sprint(meta, "%s/meta", base);

	print("meta = %s\n", meta);

	tbdbase.base = base;
	tbdbase.meta = meta;

	if(start == 1){
		tbdmakebase();
		exits nil;
	}

	if((tbdbase.mfd = open(meta, ORDWR)) == -1)
		sysfatal("can't access meta directory");

	postmountsrv(&tbdsrv, srvname, mtpt, MBEFORE);

	exits(nil);
}
