#include <u.h>
#include <libc.h>
#include <auth.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"






void
tbdnodeoverflow(TBDnode *old)
{
	TBDnode *new;


	new = emalloc9p(sizeof(TBDnode));

	new->type = old->type;
	tbdstrcpy(new->attr, old->attr, Tsz);
	new->parent = old->parent;
	old->more = new;
	new->less = old;

	switch(GETKTYPE(new->type)){
		case(ℚstring):
			tbdstrcpy(new->s, old->s, Ssz);
			break;
		case(ℚtoken):
			tbdstrcpy(new->t, old->t, Tsz);
			break;
		case(ℚinteger):
		case(ℚtime):
		case(ℚdate):
			new->i = old->i;
			break;
		case(ℚdecimal):
			new->d = old->d;
			break;
		case(ℚnumber):
			new->n = old->n;
			break;
	}
}


void
tbdaddkey(TBDnode *kn, u64int ℚid)
{
	int i = 0;
	TBDle *le;


	le = kn->leafs;

	while(le[i].type != 0){
		if((le[i].type == ℚfile) && (le[i].link == ℚid))
			return;
		i++;

		if(i == Maxleaf){
			if(kn->more == nil){
				tbdnodeoverflow(kn);
			}
			kn = kn->more;
			i = 0;
			le = kn->leafs;
		}
	}

	le[i].type = ℚfile;
	le[i].link = ℚid;
}


TBDnode*
tbdfindnode(TBDnode *in, char *x, int type)
{
	TBDnode *root, *out;
	TBDne *ne, new;
	int i;

//print("findnode; %s %s %d\n", in->attr, x, type);
	tbdfillne(&new, x, type);
	
	root = in;
	ne = root->nodes;
	i = 0;

	while(ne[i].type != 0){
		if((ne[i].type == type) && (tbdnecmp(&new, &ne[i]) == 0)){
			out = ne[i].take;
			return(out);
		}

		i++;

		if(i == Maxnode){
			if(root->more == nil){
				tbdnodeoverflow(root);
			}
			root = root->more;
			i = 0;
			ne = root->nodes;
		}
	}

/* didn't find one, so make one */

	out = emalloc9p(sizeof(TBDnode));

/* add node link to root */
	tbdnecpy(&ne[i], &new);
	ne[i].take = out;


	switch(GETNTYPE(type)){
	case(ℚattr):
		tbdstrcpy(out->attr, x, Tsz);
		tbdstrcpy(out->t, x, Tsz);
		break;
	default:
		tbdstrcpy(out->attr, in->attr, Tsz);
		switch(GETKTYPE(type)){
		case(ℚstring):
			tbdstrcpy(out->s, x, Ssz);
			break;
		case(ℚtoken):
			tbdstrcpy(out->t, x, Tsz);
			break;
		case(ℚinteger):
			out->i = atoll(x);
			break;
		case(ℚdecimal):
			out->d = atof(x);
			break;
		case(ℚnumber):
			out->n = strtoull(x, nil, 0);
			break;
		default:
			sysfatal("addnode: x=%s type=%d", x, type);
		}
	}


	out->type = type;
	out->parent = root;

//print("node made %s\n", x);
	return out;
}


void
tbdstringtotoken(char *attr, char *key, u64int ℚid)
{
	char *t[Ssz];
	char buf[Ssz], out[Ssz];
	int i, nt;

//print("strtotok: %s\n", key);

	tbdstrcpy(buf, key, Ssz);
	nt = getfields(buf, t, 128, 0, " ");

	if(nt <= 1)
		return;

//!!! more string cleaning
	for(i = 0; i < nt; i++){
		memset(out, 0, Ssz);
		tbdsanistring(t[i], ",", "\0");
		tbdsanistring(t[i], ".", "\0");
		tbdsanistring(t[i], ":", "\0");
		tbdsanistring(t[i], ";", "\0");
		snprint(out, Ssz, "%s/text/%s", attr, t[i]);
		addnode(out, ℚid);
	}
}


void
addnode(char *line, u64int ℚid)
{
	char *f[3];
	TBDnode *ln, *an, *kn;
	char *attr, *type, *key;
	int ktype;
	char debug[Tsz];
	TBDpool *errpath;


	f[0] = nil;
	f[1] = nil;
	f[2] = nil;


	getfields(line, f, 3, 0, "/");
	attr = f[0];
	type = f[1];
	key = f[2];

	if(tbddebug)
		sprint(debug, "%s.%s", attr, type);

//!!! error check
	if(f[0] == nil || f[1] == nil || f[2] == nil){
		errpath = tbdgettable(ℚid);
		print("addnode: error %s/%s/%s in %s\n", 
			attr, type, key, errpath->realpath);
		return;
	}

	ktype = tbdgetkeytype(type);

	if(ktype == 0){
		errpath = tbdgettable(ℚid);
		print("addnode: no key type %s in %s\n", type, errpath->realpath);
		return;
	}

/* clean up strings */
	if(tbdgetkeytype(type) == ℚstring){
		tbdsanistring(key, "/", " ");
	}


/* check for existing attr */
	if(tbddebug){
		an = tbdfindnode(tbdroot, debug, ℚattr);
	} else {
		an = tbdfindnode(tbdroot, attr, ℚattr);
	}

/* check for existing key */
	kn = tbdfindnode(an, key, ℚkey|PUTKTYPE(ktype));

	tbdaddkey(kn, ℚid);

/* break strings into tokens */
	if(tbdgetkeytype(type) == ℚstring){
		tbdstringtotoken(attr, key, ℚid);
	}
	
}


void
buildmeta(TBDpool *mf, u64int ℚid)
{
	char *metadata;
	int n, i;
	char *lines[256];

	metadata = emalloc9p(1024*4);
	memset(lines, 0, sizeof(lines));

	mf->fd = open(mf->realpath, OREAD);
	n = read(mf->fd, metadata, 1024*4);
	if(n < 1){
		print("no meta for %s\n", mf->realpath);
		return;
	}

	n = getfields(metadata, lines, 256, 0, "\n");

	for(i = 0; i < n; i++){
//print("buildmeta: %s %lluX\n", lines[i], ℚid);
		if(strlen(lines[i]) > 2)
			addnode(lines[i], ℚid);
	}

	close(mf->fd);
	free(metadata);
}


void
tbdbuildindex(void)
{
	int i;

	tbdroot = emalloc9p(sizeof(TBDnode));
	tbdroot->type = ℚroot;


	for(i = 0; i < tabsz; i++){
		if(tbdtab[i].ℚid == 0)
			continue;
		buildmeta(tbdtab[i].meta, tbdtab[i].ℚid);
	}
}
