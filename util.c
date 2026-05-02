#include <u.h>
#include <libc.h>
#include <auth.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"


/*
 *	a strncpy that always null terminates
 */
char*
tbdstrcpy(char *s1, char *s2, int n)
{
	int i;
	char *os1;


	n--;
	os1 = s1;
	for(i = 0; i < n; i++)
		if((*s1++ = *s2++) == 0) {
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	os1[n] = 0;
	return os1;
}



u64int
tbdgetkeytype(char *x)
{

	if(strcmp(x, "text") == 0)
		return ℚstring;
	if(strcmp(x, "integer") == 0)
		return ℚinteger;
	if(strcmp(x, "decimal") == 0)
		return ℚdecimal;
	if(strcmp(x, "number") == 0)
		return ℚnumber;


	return 0;
}


/*
 *	compare various types to see if it is equal, reater, less than
 */
int
tbdnecmp(TBDne *ne1, TBDne *ne2)
{

	u64int ktype = GETKTYPE(ne2->type);
	u64int ntype = GETNTYPE(ne2->type);


	if(ntype == ℚattr)
		ktype = ℚtoken;

	switch(ktype){
	case(ℚstring):
		return strncmp(ne1->s, ne2->s, Ssz);
	case(ℚtoken):
		return strncmp(ne1->t, ne2->t, Tsz);
	case(ℚinteger):
		return ne1->i - ne2->i;
	case(ℚdecimal):
		return ne1->d - ne2->d;
	case(ℚnumber):
		return ne1->n - ne2->n;
	}

	sysfatal("tbdnecmp fall %llud", ne2->type);
}


int
tbdtreecmp(TBDnode *n1, TBDne *ne2)
{
	u64int ktype = GETKTYPE(ne2->type);


	switch(ktype){
	case(ℚstring):
		return strncmp(n1->s, ne2->s, Ssz);
	case(ℚtoken):
		return strncmp(n1->t, ne2->t, Tsz);
	case(ℚinteger):
		return n1->i - ne2->i;
	case(ℚdecimal):
		return n1->d - ne2->d;
	case(ℚnumber):
		return n1->n - ne2->n;
	}

	sysfatal("tbdnecmp fall %llud", ne2->type);
}


void
tbdnecpy(TBDne *ne1, TBDne *ne2)
{
	u64int ktype = GETKTYPE(ne2->type);
	u64int ntype = GETNTYPE(ne2->type);


	if((ntype == ℚlib) || (ntype == ℚattr))
		ktype = ℚtoken;

	ne1->type = ne2->type;

	switch(ktype){
	case(ℚstring):
		tbdstrcpy(ne1->s, ne2->s, Ssz);
		break;
	case(ℚlib):
	case(ℚattr):
	case(ℚtoken):
		tbdstrcpy(ne1->t, ne2->t, Tsz);
		break;
	case(ℚinteger):
		ne1->i = ne2->i;
		break;
	case(ℚdecimal):
		ne1->d = ne2->d;
		break;
	case(ℚnumber):
		ne1->n = ne2->n;
		break;
	}

}


void
tbdfillne(TBDne *ne, char *x, u64int type)
{
	u64int ktype = GETKTYPE(type);
	u64int ntype = GETNTYPE(type);


	if((ntype == ℚlib) || (ntype == ℚattr) || (ntype == ℚroot))
		ktype = ℚtoken;

	ne->type = type;

	switch(ktype){
	case(ℚtoken):
		tbdstrcpy(ne->t, x, Tsz);
		break;
	case(ℚstring):
		tbdstrcpy(ne->s, x, Ssz);
		break;
	case(ℚinteger):
		ne->i = atoll(x);
		break;
	case(ℚdecimal):
		ne->d = atof(x);
		break;
	case(ℚnumber):
		ne->n = strtoull(x, nil, 0);
		break;
	default:
		sysfatal("tbdfillne: fell x=%s type=%llud", x, ktype);
	}

}


void
tbdnedump(TBDne *ne)
{
	int e;

print("DUMP\n");

	for(e = 0; e < Maxnode; e++){
		switch(GETKTYPE(ne[e].type)){
		case(ℚstring):
		case(ℚtoken):
			print("%d %llud %s | ", e, ne[e].type, ne[e].s);
			break;
		case(ℚinteger):
			print("%d %llud %lld | ", e, ne[e].type, ne[e].i);
			break;
		}
	}

	print("\n");
}


int
tbdgetlastne(TBDne *ne)
{
	int e;

	for(e = 0; e < Maxnode; e++){
		if(ne[e].type == 0)
			break;
	}

	return(e);
}


TBDnode*
tbdfetchnode(TBDnode *in, char *x)
{
	TBDne *ne, new;
	TBDnode *root;
	int i = 0;
	u64int ktype;



	if(strcmp(x, "ℚmore") == 0)
		return in->more;

	root = in;
	ne = in->nodes;
/* get type in the ne's */
//!! fix
	ktype = ne[0].type;

	tbdfillne(&new, x, ktype);
	root = in;
	ne = in->nodes;

	while(ne[i].type != 0){
		if((ne[i].type == ktype) && (tbdnecmp(&new, &ne[i]) == 0)){
			return ne[i].take;
		}
		i++;
		if(i == Maxnode){
			if(root->more == nil){
				return nil;
			}
			root = root->more;
			i = 0;
			ne = root->nodes;
		}
	}

	return nil;
}


void
tbdgetnodename(char *name, TBDnode *n)
{
	u64int ktype = GETKTYPE(n->type);
	u64int ntype = GETNTYPE(n->type);


	if((ntype == ℚlib) || (ntype == ℚattr))
		ktype = ℚtoken;

	switch(ktype){
	case(ℚtoken):
		tbdstrcpy(name, n->t, Tsz);
		break;
	case(ℚstring):
		tbdstrcpy(name, n->s, Ssz);
		break;
	case(ℚinteger):
		snprint(name, Ssz, "%lld", n->i);
		break;
	case(ℚdecimal):
		snprint(name, Ssz, "%f", n->d);
		break;
	case(ℚnumber):
		snprint(name, Ssz, "%llud", n->n);
		break;
	}

//print("getnodename: %s\n", name);
}


void
tbdgetnename(char *name, TBDne *ne)
{
	u64int ktype = GETKTYPE(ne->type);
	u64int ntype = GETNTYPE(ne->type);

//print("nename: %llud %s\n", ne->type, ne->s);

	if((ntype == ℚlib) || (ntype == ℚattr))
		ktype = ℚtoken;

	switch(ktype){
	case(ℚtoken):
		tbdstrcpy(name, ne->t, Tsz);
		break;
	case(ℚstring):
		tbdstrcpy(name, ne->s, Ssz);
		break;
	case(ℚinteger):
		snprint(name, Ssz, "%lld", ne->i);
		break;
	case(ℚdecimal):
		snprint(name, Ssz, "%f", ne->d);
		break;
	case(ℚnumber):
		snprint(name, Ssz, "%llud", ne->n);
		break;
	}

}


TBDnode*
tbdrunquery(TBDnode *root, char *x)
{
	TBDnode *tree;
	TBDne *ne, qe;
	int i;



	tree = root;
	tbdfillne(&qe, x, tree->type);

	while(GETNTYPE(tree->type) == ℚnode){
//print("query node: %llud\n", tree->type);
		ne = tree->nodes;
		for(i = 0; i < Maxtree; i++){
			if(ne[i].type == 0){
				tree = tree->more;
				break;
			}
			if(tbdnecmp(&qe, &ne[i]) < 0){
				tree = ne[i].take;
				break;
			}
		}
	}


	/* should be on a leaf node */
	if(GETNTYPE(tree->type) == ℚleaf){
//print("query leaf: %llud\n", tree->type);
		ne = tree->nodes;
		for(i = 0; i < Maxtree; i++){
			if(ne[i].type == 0)
				return nil;
			if(tbdnecmp(&qe, &ne[i]) == 0){
				tree = ne[i].take;
				break;
			}
		}
	}

//print("query key: %llud\n", tree->type);
	if(GETNTYPE(tree->type) == ℚkey)
		return tree;

	sysfatal("tbdrunquery: fall on %llud", tree->type);
}


u64int
tbdfilefromkey(TBDnode *key, char *x)
{
	TBDnode *n;
	TBDle *le;
	TBDpool *p;
	Dir *pd;
	int i = 0;


	n = key;
	le = key->leafs;

//!! fix for key->more
	while(le[i].type != 0){
		p = tbdgettable(le[i].link);
		pd = p->d;
		if(strncmp(x, pd->name, Ssz) == 0)
			return le[i].link;
		i++;
	}

	return 0;
}


void
tbdsanistring(char *x, char *n, char *y)
{
	int i, l;
//	char n[1] = "/";
//	char y[1] = " ";


/* cap all strings at 255 */
//	x[255] = 0;  doesn't work?  cuts string early

	l = strlen(x);

	for(i = 0; i < l; i++){
		if(x[i] == n[0])
			x[i] = y[0];
	}
}
