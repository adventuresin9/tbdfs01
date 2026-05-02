#include <u.h>
#include <libc.h>
#include <auth.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"



TBDnode*
tbdmakenewroot(TBDnode *iattr)
{
	TBDnode *new;
	TBDne *ne;
	u64int ktype;

//!! still not liking
	ne = iattr->nodes;
	ktype = GETKTYPE(ne[0].type);

//	ktype = GETKTYPE(iattr->type);
	new = emalloc9p(sizeof(TBDnode));
	new->type = ℚnode|PUTKTYPE(ktype);
	new->parent = iattr;
	tbdstrcpy(new->attr, iattr->attr, Tsz);
	iattr->tree = new;

	return(new);
}


void
tbdnodesplit(TBDnode *node)
{
	TBDnode *parent, *child, *less, *more;
	TBDne *ne, up;
	int i, s;
	int split = Maxtree / 2;
	int mls = split + 1; /* more, without the split pointer */
	int attr;
	u64int ktype;


	ktype = GETKTYPE(node->type);
	parent = node->parent;
	less = emalloc9p(sizeof(TBDnode));
	more = emalloc9p(sizeof(TBDnode));

/* check if root */
	attr = parent->type & TBDNMASK;
	if(attr == ℚattr){
		parent = tbdmakenewroot(parent);
	}

/* setup link to move up */

	ne = node->nodes;
	up.type = ne[split].type;
	up.take = less;
	tbdnecpy(&up, &ne[split]);

/* fill new nodes */

	less->type = ℚnode|PUTKTYPE(ktype);
	less->parent = parent;
	less->more = node->nodes[split].take;
	for(i = 0; i < (Maxtree / 2); i++){
		less->nodes[i].type = ne[i].type;
		less->nodes[i].take = ne[i].take;
		tbdnecpy(&less->nodes[i], &ne[i]);
		child = less->nodes[i].take;
		child->parent = less;
	}
	child = less->more;
	child->parent = less;

	more->type = ℚnode|PUTKTYPE(ktype);
	more->parent = parent;
	more->more = node->more;
	for(i = 0; i < (Maxtree / 2); i++){
		more->nodes[i].type = ne[i+mls].type;
		more->nodes[i].take = ne[i+mls].take;
		tbdnecpy(&more->nodes[i], &ne[i+mls]);
		if(more->nodes[i].type != 0){
			child = more->nodes[i].take;
			child->parent = more;
		}
	}
	child = more->more;
	child->parent = more;

/* update parent node pointers */
/* insert *up */

	ne = parent->nodes;

	for(i = 0; i < Maxtree; i++){
		if(ne[i].type == 0){
			ne[i].type = up.type;
			ne[i].take = up.take;
			tbdnecpy(&ne[i], &up);
			break;
		}
		if(tbdnecmp(&up, &ne[i]) < 0){
			for(s = Maxtree; s > i; s--){
				ne[s].type = ne[s-1].type;
				ne[s].take = ne[s-1].take;
				tbdnecpy(&ne[s], &ne[s-1]);
			}
			ne[i].type = up.type;
			ne[i].take = up.take;
			tbdnecpy(&ne[i], &up);
			break;
		}
	}

/* edit ne after up to point to more */
	i++;
	if(ne[i].type == 0){
		parent->more = more;
	}else{
		ne[i].take = more;
	}

	if(tbddebug){
		tbdnedump(node->nodes);
		tbdnedump(parent->nodes);
		tbdnedump(less->nodes);
		tbdnedump(more->nodes);
	}

/* free old node */

	free(node);
}


void
tbdleafsplit(TBDnode *leaf)
{
	TBDnode *node, *child, *less, *more;
	TBDne *ne, up;
	int i, s;
	int split = Maxtree / 2;
	u64int ktype;


	ktype = GETKTYPE(leaf->type);
	node = leaf->parent;
	less = emalloc9p(sizeof(TBDnode));
	more = emalloc9p(sizeof(TBDnode));

/* fill new leafs */

	ne = leaf->nodes;

	less->type = ℚleaf|PUTKTYPE(ktype);
	less->parent = leaf->parent;
	for(i = 0; i < (Maxtree / 2); i++){
		less->nodes[i].type = ne[i].type;
		less->nodes[i].take = ne[i].take;
		tbdnecpy(&less->nodes[i], &ne[i]);
		child = ne[i].take;
		child->parent = less;
	}

	more->type = ℚleaf|PUTKTYPE(ktype);
	more->parent = leaf->parent;
	for(i = 0; i < (Maxtree / 2); i++){
		more->nodes[i].type = ne[i+split].type;
		more->nodes[i].take = ne[i+split].take;
		tbdnecpy(&more->nodes[i], &ne[i+split]);
		child = ne[i].take;
		child->parent = more;
	}

	less->less = leaf->less;
	more->more = leaf->more;
	less->more = more;
	more->less = less;

/* set pointer to send up */

	up.type = ne[split].type;
	up.take = less;
	tbdnecpy(&up, &ne[split]);

/* update parent node pointers */
/* insert *up */

	ne = node->nodes;

	for(i = 0; i < Maxtree; i++){
		if(ne[i].type == 0){
			ne[i].type = up.type;
			ne[i].take = up.take;
			tbdnecpy(&ne[i], &up);
			break;
		}
		if(tbdnecmp(&up, &ne[i]) < 0){
			for(s = Maxtree; s > i; s--){
				ne[s].type = ne[s-1].type;
				ne[s].take = ne[s-1].take;
				tbdnecpy(&ne[s], &ne[s-1]);
			}
			ne[i].type = up.type;
			ne[i].take = up.take;
			tbdnecpy(&ne[i], &up);
			break;
		}
	}

/* edit ne after up to point to more */
	i++;
	if(ne[i].type == 0){
		node->more = more;
	}else{
		ne[i].take = more;
	}

	if(tbddebug){
		tbdnedump(node->nodes);
		tbdnedump(less->nodes);
		tbdnedump(more->nodes);
	}

/* free old leaf */

	free(leaf);
}



// might want to rethink this group
void
tbdstringinsert(TBDnode *tree, TBDnode *key)
{
	int i, s;
	TBDne *ne;
//	u64int ntype = GETNTYPE(tree->type);

	ne = tree->nodes;

	if(GETNTYPE(tree->type) == ℚnode){
		for(i = 0; i < Maxtree; i++){
			if(ne[i].type == 0){
				tbdstringinsert(tree->more, key);
				break;
			}
			if(strncmp(key->s, ne[i].s, Ssz) < 0){
				tbdstringinsert(ne[i].take, key);
				break;
			}
		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdnodesplit(tree);
		}
		return;
	}


	if(GETNTYPE(tree->type) == ℚleaf){
		for(i = 0; i < Maxtree; i++){
			if(strncmp(key->s, ne[i].s, Ssz) == 0){
//				sysfatal("tree dup: %s %s in %s", ne[i].s, key->s, key->attr);
				print("string tree dup: %s %s in %s", ne[i].s, key->s, key->attr);
				return;
			}
			if(ne[i].type == 0){
				ne[i].type = key->type;
				ne[i].take = key;
				tbdstrcpy(ne[i].s, key->s, Ssz);
				break;
			}
			if(strncmp(key->s, ne[i].s, Ssz) < 0){

				for(s = Maxtree; s > i; s--){
					if(ne[s-1].type == 0)
						continue;
					ne[s].type = ne[s-1].type;
					ne[s].take = ne[s-1].take;
					tbdstrcpy(ne[s].s, ne[s-1].s, Ssz);
				}
				ne[i].type = key->type;
				ne[i].take = key;
				tbdstrcpy(ne[i].s, key->s, Ssz);
				break;
			}


		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdleafsplit(tree);
		}

		return;
	}


	print("stringinsert: fallthrough %s %llud %llud\n", key->s, tree->type, key->type);
	return;
//	sysfatal("stringinsert: fallthrough %s %llud %llud", key->s, tree->type, key->type);
}


void
tbdtokeninsert(TBDnode *tree, TBDnode *key)
{
	int i, s;
	TBDne *ne;


	ne = tree->nodes;

	if(tree->type == ℚnode){
		for(i = 0; i < Maxtree; i++){
			if(ne[i].type == 0){
				tbdtokeninsert(tree->more, key);
				break;
			}
			if(strncmp(key->t, ne[i].t, Tsz) < 0){
				tbdtokeninsert(ne[i].take, key);
				break;
			}
		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdnodesplit(tree);
		}
		return;
	}


	if(tree->type == ℚleaf){
		for(i = 0; i < Maxtree; i++){
			if(strncmp(key->t, ne[i].t, Tsz) == 0){
				sysfatal("token tree dup: %s %s in %s", ne[i].t, key->t, key->attr);
			}
			if(ne[i].type == 0){
				ne[i].type = key->type;
				ne[i].take = key;
				tbdstrcpy(ne[i].t, key->t, Tsz);
				break;
			}
			if(strncmp(key->t, ne[i].t, Tsz) < 0){

				for(s = Maxtree; s > i; s--){
					if(ne[s-1].type == 0)
						continue;
					ne[s].type = ne[s-1].type;
					ne[s].take = ne[s-1].take;
					tbdstrcpy(ne[s].t, ne[s-1].t, Tsz);
				}
				ne[i].type = key->type;
				ne[i].take = key;
				tbdstrcpy(ne[i].t, key->t, Tsz);
				break;
			}


		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdleafsplit(tree);
		}

		return;
	}

	sysfatal("stringinsert: fallthrough %s", key->t);
}


void
tbdintegerinsert(TBDnode *tree, TBDnode *key)
{
	int i, s;
	TBDne *ne;

	ne = tree->nodes;

	if(GETNTYPE(tree->type) == ℚnode){
		for(i = 0; i < Maxtree; i++){
			if(ne[i].type == 0){
				tbdintegerinsert(tree->more, key);
				break;
			}
			if(key->i < ne[i].i){
				tbdintegerinsert(ne[i].take, key);
				break;
			}
		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdnodesplit(tree);
		}
		return;
	}


	if(GETNTYPE(tree->type) == ℚleaf){
		for(i = 0; i < Maxtree; i++){
			if(key->i == ne[i].i){
				sysfatal("integer tree dup: %s %lld in %s", ne[i].i, key->i, key->attr);
			}
			if(ne[i].type == 0){
				ne[i].type = key->type;
				ne[i].take = key;
				ne[i].i = key->i;
				break;
			}
			if(key->i < ne[i].i){

				for(s = Maxtree; s > i; s--){
					if(ne[s-1].type == 0)
						continue;
					ne[s].type = ne[s-1].type;
					ne[s].take = ne[s-1].take;
					ne[s].i = ne[s-1].i;
				}
				ne[i].type = key->type;
				ne[i].take = key;
				ne[i].i = key->i;
				break;
			}


		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdleafsplit(tree);
		}

		return;
	}

tbdnedump(ne);

	print("integerinsert: fallthrough %lld @ %lld = %s\n", key->i, tree->type, tree->attr);
	return;
//	sysfatal("integerinsert: fallthrough %lld @ %lld = %s", key->i, tree->type, tree->attr);
}


void
tbddecimalinsert(TBDnode *tree, TBDnode *key)
{
	int i, s;
	TBDne *ne;


	ne = tree->nodes;

	if(tree->type == ℚnode){
		for(i = 0; i < Maxtree; i++){
			if(ne[i].type == 0){
				tbddecimalinsert(tree->more, key);
				break;
			}
			if(key->i < ne[i].i){
				tbddecimalinsert(ne[i].take, key);
				break;
			}
		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdnodesplit(tree);
		}
		return;
	}


	if(tree->type == ℚleaf){
		for(i = 0; i < Maxtree; i++){
			if(key->d == ne[i].d){
				sysfatal("decimal tree dup: %f @ %f in %s", ne[i].d, key->d, key->attr);
			}
			if(ne[i].type == 0){
				ne[i].type = key->type;
				ne[i].take = key;
				ne[i].d = key->d;
				break;
			}
			if(key->d < ne[i].d){

				for(s = Maxtree; s > i; s--){
					if(ne[s-1].type == 0)
						continue;
					ne[s].type = ne[s-1].type;
					ne[s].take = ne[s-1].take;
					ne[s].d = ne[s-1].d;
				}
				ne[i].type = key->type;
				ne[i].take = key;
				ne[i].d = key->d;
				break;
			}


		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdleafsplit(tree);
		}

		return;
	}

	sysfatal("decimalinsert: fallthrough %f", key->d);
}


void
tbdnumberinsert(TBDnode *tree, TBDnode *key)
{
	int i, s;
	TBDne *ne;


	ne = tree->nodes;

	if(GETNTYPE(tree->type) == ℚnode){
		for(i = 0; i < Maxtree; i++){
			if(ne[i].type == 0){
				tbdnumberinsert(tree->more, key);
				break;
			}
			if(key->i < ne[i].i){
				tbdnumberinsert(ne[i].take, key);
				break;
			}
		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdnodesplit(tree);
		}
		return;
	}


	if(GETNTYPE(tree->type) == ℚleaf){
		for(i = 0; i < Maxtree; i++){
			if(key->n == ne[i].n){
//				sysfatal("number tree dup: %llud @ %llud in %s", ne[i].n, key->n, key->attr);
				print("number tree dup: %llud @ %llud in %s\n", ne[i].n, key->n, key->attr);
				return;
			}
			if(ne[i].type == 0){
				ne[i].type = key->type;
				ne[i].take = key;
				ne[i].n = key->n;
				break;
			}
			if(key->n < ne[i].n){

				for(s = Maxtree; s > i; s--){
					if(ne[s-1].type == 0)
						continue;
					ne[s].type = ne[s-1].type;
					ne[s].take = ne[s-1].take;
					ne[s].n = ne[s-1].n;
				}
				ne[i].type = key->type;
				ne[i].take = key;
				ne[i].n = key->n;
				break;
			}


		}

		if(tbdgetlastne(ne) >= Maxtree){
			tbdleafsplit(tree);
		}

		return;
	}

	print("tbdnumberinsert: error %llud\n", key->n);
	return;
//	sysfatal("numberinsert: fallthrough %lluX", key->n);
}
// rethink above


void
tbdtreeinsert(TBDnode *attr, TBDnode *key)
{
	TBDnode *tree;


	tree = attr->tree;
	switch(GETKTYPE(key->type)){
	case(ℚstring):
		tbdstringinsert(tree, key);
		break;
	case(ℚtoken):
		tbdtokeninsert(tree, key);
		break;
	case(ℚinteger):
		tbdintegerinsert(tree, key);
		break;
	case(ℚdecimal):
		tbddecimalinsert(tree, key);
		break;
	case(ℚnumber):
		tbdnumberinsert(tree, key);
		break;
	default:
		sysfatal("tbdtreeinsert fall");
	}
}


void
tbdattrtree(TBDnode *iattr)
{
	int i = 0;
	TBDne *ne;
	TBDnode *tree, *root, *more, *less;
	u64int ktype = GETKTYPE(iattr->type);


//!! I still don't like this
	ne = iattr->nodes;
	ktype = GETKTYPE(ne[0].type);

/* start the tree on a ℚnode */
	tree = emalloc9p(sizeof(TBDnode));
	tree->type = ℚnode|PUTKTYPE(ktype);
	tree->parent = iattr;
	tbdstrcpy(tree->attr, iattr->attr, Tsz);

	iattr->tree = tree;
	root = iattr;

/* set up the first leafs */
	less = emalloc9p(sizeof(TBDnode));
	more = emalloc9p(sizeof(TBDnode));
	more->type = ℚleaf|PUTKTYPE(ktype);
	more->parent = tree;
	less->type = ℚleaf|PUTKTYPE(ktype);
	less->parent = tree;
	less->more = more;
	more->less = less;

/* set up first sort */
	tree->nodes[0].type = iattr->nodes[0].type;
	tree->nodes[0].take = less;
	tbdnecpy(&tree->nodes[0], &iattr->nodes[0]);
	tree->more = more;

	ne = root->nodes;

	while(ne[i].type != 0){
		tbdtreeinsert(iattr, ne[i].take);
		i++;
		if((i == Maxnode) && (root->more != nil)){
			root = root->more;
			ne = root->nodes;
			i = 0;
		}
	}

}


void
tbdbuildtree(void)
{
	int i = 0;
	TBDnode *iroot;
	TBDne *ne;


	iroot = tbdroot;
	ne  = iroot->nodes;
	while(ne[i].type != 0){
		tbdattrtree(ne[i].take);
		i++;
		if((i == Maxnode) && (iroot->more != nil)){
			iroot = iroot->more;
			ne = iroot->nodes;
			i = 0;
		}
	}
}
