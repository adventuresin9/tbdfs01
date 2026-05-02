#include <u.h>
#include <libc.h>
#include <auth.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"


#define TBDPATH(r, d, i)  ((r) | ((d) << 8) | ((i) << 16))


enum{
	Qoops,
	Qroot,
	Qindex,
	Qmeta,
	Qpool,
	Qtree,
	Qquery,
};


void
tabread9p(Req *r, Dirgen *gen, void *aux)
{
	int start, z;
	uchar *p, *ep;
	uint rv;
	Dir d;

	if(r->ifcall.offset == 0)
		start = 0;
	else
		start = r->fid->dirindex;

	p = (uchar*)r->ofcall.data;
	ep = p+r->ifcall.count;

	while(p < ep){
		memset(&d, 0, sizeof d);
		z = (*gen)(start, &d, aux);
		start++;
		if(z > 0)
			continue;
		if(z < 0)
			break;
		rv = convD2M(&d, p, ep-p);
		free(d.name);
		free(d.muid);
		free(d.uid);
		free(d.gid);
		if(rv <= BIT16SZ)
			break;
		p += rv;

	}
	r->fid->dirindex = start;
	r->ofcall.count = p - (uchar*)r->ofcall.data;
}


void
tbdmkqid(Qid *q, TBDfid *f)
{
	TBDnode *n;


	n = f->node;
	q->type = 0;
	q->vers = 0;
	switch(f->root){
	case(Qindex):
	case(Qtree):
	case(Qquery):
		switch(GETNTYPE(n->type)){
		case(ℚroot):
//		case(ℚlib):
		case(ℚattr):
		case(ℚnode):
		case(ℚleaf):
			q->type = QTDIR;
			q->path = TBDPATH(f->root, f->depth, 0);
			break;
		case(ℚkey):
			break;
		}
	}

}


void
tbdfilestat(Dir *d, u64int ℚid)
{
	TBDpool *p;
	Dir *pd;


	p = tbdgettable(ℚid);
	pd = p->d;
	d->name = estrdup9p(pd->name);
	d->length = pd->length;
	d->mode = pd->mode;
	d->atime = pd->atime;
	d->mtime = pd->mtime;
	d->uid = estrdup9p(pd->uid);
	d->gid = estrdup9p(pd->gid);
	d->muid = estrdup9p(pd->muid);
	d->qid.path = ℚid;
	d->qid.vers = 0;
	d->qid.type = QTFILE;

}

	
void
tbddirstat(Dir *d, void *aux)
{
	TBDfid *f;
	TBDnode *n;
	char name[Ssz];

//	char *qroot[] = {
//		"index",
//		"meta",
//		"pool",
//		"tree",
//		"ℚ",
//	};


	memset(d, 0, sizeof(*d));
	f = aux;

	if(f->root == Qroot){
		d->name = estrdup9p("/");
	}else{
		n = f->node;
		tbdgetnodename(name, n);
		d->name = estrdup9p(name);
	}
	d->qid.path = TBDPATH(f->root, f->depth, 0);
	d->qid.vers = 0;
	d->qid.type = QTDIR;
	d->atime = d->mtime = time0;
	d->uid = estrdup9p(user);
	d->gid = estrdup9p(user);
	d->muid = estrdup9p(user);

	d->mode = 0555 | DMDIR;
}


int
rootgen(int i, Dir *d, void*)
{
	char *qroot[] = {
		"index",
		"meta",
		"pool",
		"tree",
		"ℚ",
	};

	if (i >= nelem(qroot))
		return -1;

	d->name = estrdup9p(qroot[i]);
	d->qid.vers = 0;
	d->qid.type = QTDIR;
	d->qid.path = TBDPATH(Qroot, 0, i);
	d->uid = estrdup9p(user);
	d->gid = estrdup9p(user);
	d->muid = estrdup9p(user);
	d->mtime = time0;
	return 0;
}


int
tablegen(int i, Dir *d, void *aux)
{
//	TBDfid *f;
	TBDpool *p;
	Dir *pd;


	if(i >=tabsz)
		return -1;

	if(tbdtab[i].ℚid == 0)
		return 1;

//	f = aux;

	if(tbdtab[i].ℚid != 0){
		p = tbdgettable(tbdtab[i].ℚid);
		pd = p->d;

		d->mode = pd->mode;
		d->name = estrdup9p(pd->name);
		d->qid.vers = 0;
		d->qid.type = QTFILE;
		d->qid.path = p->ℚid;
		d->uid = estrdup9p(pd->uid);
		d->gid = estrdup9p(pd->gid);
		d->muid = estrdup9p(pd->muid);
		d->mtime = pd->mtime;
		d->atime = pd->atime;
	}
	return 0;
}


int
treegen(int i, Dir *dir, void *aux)
{
	TBDnode *n;
	TBDne *ne;
	TBDle *le;
	TBDpool *p;
	Dir *pd;
	int d, r, w = 0;
	u64int ntype, ktype;
	u64int ℚid;
	char name[Ssz];


	n = aux;

	if(n == nil)
		return -1;

	ktype = GETKTYPE(n->type);
	ntype = GETNTYPE(n->type);
//	if((ntype == ℚlib) || (ntype == ℚattr))
	if(ntype == ℚattr)
		ktype = ℚtoken;

//print("treegen: %llud\n", ntype);

	switch(ntype){
	case(ℚroot):
//	case(ℚlib):
	case(ℚattr):
		w = Maxnode;
		break;
	case(ℚnode):
	case(ℚleaf):
		w = Maxtree;
		break;
	case(ℚkey):
		w = Maxleaf;
		break;
	}

	d = i / w;
	r = i % w;

	switch(ntype){
	case(ℚroot):
	case(ℚattr):
//print("treegen: off tree\n");
		while(d > 0){
			if(n->more == nil)
				return -1;
			n = n->more;
			d--;
		}
		ne = n->nodes;
		if(ne[r].type == 0)
			return -1;
		dir->mode = 0555 | DMDIR;
		dir->uid = estrdup9p(user);
		dir->gid = estrdup9p(user);
		dir->muid = estrdup9p(user);
		dir->mtime = time0;
		tbdgetnename(name, &ne[r]);
		dir->name = estrdup9p(name);
		return 0;
	case(ℚnode):
//print("treegen: on node\n");
		ne = n->nodes;
		if(i > Maxtree)
			return -1;
		if((ne[r].type == 0) && (ne[r-1].type == 0))
			return -1;
		dir->mode = 0555 | DMDIR;
		dir->uid = estrdup9p(user);
		dir->gid = estrdup9p(user);
		dir->muid = estrdup9p(user);
		dir->mtime = time0;
		if(ne[r].type == 0){
			sprint(name, "ℚmore");
		}else{
		tbdgetnename(name, &ne[r]);
		}
		dir->name = estrdup9p(name);
		return 0;
	case(ℚleaf):
//print("treegen: on leaf %llud\n", ne[r].type);
		ne = n->nodes;
		if(i > Maxtree)
			return -1;
		if(ne[r].type == 0)
			return -1;
		dir->mode = 0555 | DMDIR;
		dir->uid = estrdup9p(user);
		dir->gid = estrdup9p(user);
		dir->muid = estrdup9p(user);
		dir->mtime = time0;
		tbdgetnename(name, &ne[r]);
		dir->name = estrdup9p(name);
		return 0;
	case(ℚkey):
//print("treegen: on key\n");
		le = n->leafs;
		if(le[r].type == 0)
			return -1;
		ℚid = le[r].link;
		p = tbdgettable(ℚid);
		pd = p->d;
		dir->mode = pd->mode;
		dir->length = pd->length;
		dir->uid = estrdup9p(pd->uid);
		dir->gid = estrdup9p(pd->gid);
		dir->muid = estrdup9p(pd->muid);
		dir->mtime = pd->mtime;
		dir->atime = pd->atime;
		dir->name = estrdup9p(pd->name);
		return 0;
	}

	return -1;
}


int
indexgen(int i, Dir *dir, void *aux)
{
	TBDnode *n;
	TBDne *ne;
	TBDle *le;
	TBDpool *p;
	Dir *pd;
	int d, r, w;
	u64int ntype, ktype;
	u64int ℚid;
	char name[Ssz];


	memset(dir, 0, sizeof(Dir));

	n = aux;

	ktype = GETKTYPE(n->type);
	ntype = GETNTYPE(n->type);
	if(ntype == ℚattr)
		ktype = ℚtoken;

	w = Maxnode;

	if(ntype == ℚkey)
		w = Maxleaf;

	d = i / w;
	r = i % w;

	while(d > 0){
		if(n->more == nil)
			return -1;
		n = n->more;
		d--;
	}

	if(w == Maxnode){
		ne = n->nodes;
		if(ne[r].type == 0)
			return -1;
		dir->mode = 0555 | DMDIR;
		dir->uid = estrdup9p(user);
		dir->gid = estrdup9p(user);
		dir->muid = estrdup9p(user);
		dir->mtime = time0;
		tbdgetnename(name, &ne[r]);
		dir->name = estrdup9p(name);
//print("indexgen: %d %s\n", i, dir->name);
		return 0;
	}

	if(w == Maxleaf){
		le = n->leafs;
		if(le[r].type == 0)
			return -1;
		ℚid = le[r].link;
		p = tbdgettable(ℚid);
		pd = p->d;
		dir->mode = pd->mode;
		dir->length = pd->length;
		dir->uid = estrdup9p(pd->uid);
		dir->gid = estrdup9p(pd->gid);
		dir->muid = estrdup9p(pd->muid);
		dir->mtime = pd->mtime;
		dir->atime = pd->atime;
		dir->name = estrdup9p(pd->name);
		return 0;
	}
	

	return -1;
}



void
tbdfsstart(Srv *thissrv)
{
	Dir *rd;
	TBDpool *pool, *meta;
	TBDnode *name;
	int i, n;
	u64int h;
	char hname[Ssz * 2];

/* start the hash table */
	n = dirreadall(tbdbase.bfd, &rd);
	tabsz = tbdfetchprime(n * 4);
	tbdtab = emalloc9p(sizeof(TBDtable) * tabsz);

/* fill in the pool and metadata */
	for(i = 0; i < n; i++){
		if(rd[i].mode & DMDIR)
			continue;
		meta = nil;
		pool = nil;
		snprint(hname, Ssz * 2, "%llX%luX%s", rd[i].length, rd[i].mtime, rd[i].name);
		h = tbdputtable(hname);
		pool = emalloc9p(sizeof(TBDpool));
		meta = emalloc9p(sizeof(TBDpool));
		snprint(pool->realpath, Ssz, "%s/%s", tbdbase.base, rd[i].name);
		snprint(meta->realpath, Ssz, "%s/%s.meta", tbdbase.meta, rd[i].name);
		pool->ℚid = tbdtab[h].ℚid;
		meta->ℚid = tbdtab[h].ℚid;
		pool->d = dirstat(pool->realpath);
		if(pool->d == nil)
			print("cant stat %s\n", pool->realpath);
		tbdtab[h].file = pool;
		tbdtab[h].meta = meta;
	}

/* fill index */
	tbdbuildindex();

/* build B+ tree */
	tbdbuildtree();

	if(tbddebug){
		name = tbdfetchnode(tbdroot, "ℙname.text");
	} else {
		name = tbdfetchnode(tbdroot, "ℙname");
	}

	name = name->tree;
	thissrv->aux = name;
	nametree = name;
}


void
tbdfsattach(Req *r)
{
	TBDfid *f;

	if(r->ifcall.aname && r->ifcall.aname[0]){
		respond(r, "invalid attach specifier");
		return;
	}

	f = emalloc9p(sizeof(TBDfid));

	r->ofcall.qid = (Qid){Qroot, 0, QTDIR};
	r->fid->qid = r->ofcall.qid;
	f->root = Qroot;
	r->fid->aux = f;
	respond(r, nil);
}


void
tbdfsopen(Req *r)
{

	respond(r, nil);
}


void
tbdfsread(Req *r)
{
	TBDfid *f;
	TBDpool *pool;
	char *path;
	char *buf;
	int fd;
	int count, n;
	vlong off;

	f = r->fid->aux;

	if(r->fid->qid.type == QTFILE){
		off = r->ifcall.offset;
		count = r->ifcall.count;

		if(f->root == Qmeta){
			pool = tbdmetatable(r->fid->qid.path);
		}else{
			pool = tbdgettable(r->fid->qid.path);
		}

		if(pool == nil){
			respond(r, "no pool entry");
			return;
		}
		
		path = pool->realpath;
		fd = open(path, OREAD);

		if(fd < 0){
			respond(r, "no file");
			return;
		}

		buf = emalloc9p(count);
		n = pread(fd, buf, count, off);

		if(n > 0){
			r->ofcall.count = n;
			memmove(r->ofcall.data, buf, n);
		}else{
			r->ofcall.count = 0;
		}

//print("tbdread: count=%d n=%d off=%lld\n", count, n, off);

		respond(r, nil);

		free(buf);
		close(fd);
		return;
	}


	switch(f->root){
	case(Qroot):
		dirread9p(r, rootgen, nil);
		respond(r, nil);
		return;
	case(Qindex):
	case(Qquery):
		dirread9p(r, indexgen, f->node);
		respond(r, nil);
		return;
	case(Qtree):
		dirread9p(r, treegen, f->node);
		respond(r, nil);
		return;
	case(Qpool):
	case(Qmeta):
		tabread9p(r, tablegen, f);
		respond(r, nil);
		return;
	}

	respond(r, nil);
}


void
tbdfsstat(Req *r)
{

	if(r->fid->qid.type & QTDIR){
		tbddirstat(&r->d, r->fid->aux);
		respond(r, nil);
		return;
	}

	tbdfilestat(&r->d, r->fid->qid.path);

	respond(r, nil);
}


char*
tbdfswalk1(Fid *fid, char *name, Qid *qid)
{
	TBDfid *f;
	TBDnode *in, *out;
	TBDle *le;
	u64int ℚid;
	char npath[Ssz];


	f = fid->aux;

//print("walk1: try %s @ %d %d\n", name, f->root, f->depth);
	switch(f->root){
	case(Qroot):
		if(strcmp(name, "pool") == 0)
			f->root = Qpool;
		if(strcmp(name, "meta") == 0)
			f->root = Qmeta;
		if(strcmp(name, "index") == 0)
			f->root = Qindex;
		if(strcmp(name, "tree") == 0)
			f->root = Qtree;
		if(strcmp(name, "ℚ") == 0)
			f->root = Qquery;

		if(f->root == ℚroot)
			return "nonstarter";

		f->node = tbdroot;
		break;
	case(Qindex):
		f->depth++;
		in = f->node;
		if(GETNTYPE(in->type) == ℚkey){
			ℚid = tbdfilefromkey(in, name);
			*qid = (Qid){ℚid, 0, QTFILE};
			fid->qid = *qid;
			return nil;
		}
		out = tbdfetchnode(f->node, name);
		if(out == nil)
			return "stopped making sense";
			f->node = out;
		break;
	case(Qtree):
		f->depth++;
		in = f->node;
		if(GETNTYPE(in->type) == ℚkey){
			ℚid = tbdfilefromkey(in, name);
			*qid = (Qid){ℚid, 0, QTFILE};
			fid->qid = *qid;
			return nil;
		}
		out = tbdfetchnode(f->node, name);
		if(out == nil)
			return "not a node";
		if(GETNTYPE(out->type) == ℚattr)
			out = out->tree;
		f->node = out;
		break;
	case(Qquery):
		f->depth++;

		if(f->path[0] == 0){
			tbdstrcpy(f->path, name, Ssz);
		}else{
			snprint(npath, Ssz, "%s/%s", f->path, name);
			tbdstrcpy(f->path, npath, Ssz);
		}

		in = f->node;
		switch(GETNTYPE(in->type)){
		case(ℚroot):
		case(ℚattr):
			out = tbdfetchnode(f->node, name);
			if(out == nil)
				return "here be dragons";
			if(GETNTYPE(out->type) == ℚattr){
				out = out->tree;
			}
			f->node = out;
			break;
		case(ℚnode):
			out = tbdrunquery(f->node, name);
			if(out == nil)
				return "not a name";
			f->node = out;
			break;
		case(ℚkey):
			out = tbdrunquery(nametree, name);
			if(out == nil)
				return "not a node";
			le = out->leafs;
			ℚid = le[0].link;
			*qid = (Qid){ℚid, 0, QTFILE};
			fid->qid = *qid;
			return nil;
		}
		if(out == nil)
			return "no results";

		f->node = out;
//print("walk1: path=%s\n", f->path);
		break;
	case(Qpool):
	case(Qmeta):
		out = tbdrunquery(nametree, name);
		if(out == nil)
			return "not a name";
		le = out->leafs;
		ℚid = le[0].link;
		*qid = (Qid){ℚid, 0, QTFILE};
		fid->qid = *qid;
		return nil;
	}

	if(f->root == 0)
		return "non starter";

	*qid = (Qid){TBDPATH(f->root, f->depth, 0), 0, QTDIR};
	fid->qid = *qid;

	return nil;
}


char*
tbdfsclone(Fid *oldfid, Fid *newfid)
{
	TBDfid *f, *o;

	o = oldfid->aux;
	if(o == nil)
		return "bad fid";
	f = emalloc9p(sizeof(*f));
	memmove(f, o, sizeof(*f));
	newfid->aux = f;
	return nil;
}


void
tbdfsdestroyfid(Fid *fid)
{
	TBDfid *f;
	int i;

	if(f = fid->aux){
		for(i = 0; i < Maxquery; i++){
			if(f->qtab[i])
				free(f->qtab[i]);
		}
		fid->aux = nil;
		free(f);
	}
}

