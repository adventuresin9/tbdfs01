#define TBDMMASK 0xFFFFFFFF00000000ULL
#define TBDNMASK 0xFF
#define TBDTMASK 0xFF00
#define PUTKTYPE(x)	((x) << 8)
#define GETKTYPE(x) ((x) >> 8)
#define GETNTYPE(x) ((x) & 0xFF)

typedef struct TBDbase TBDbase;
typedef struct TBDtable TBDtable;
typedef struct TBDgethash TBDgethash;
typedef struct TBDmeta TBDmeta;
typedef struct TBDnode TBDnode;
typedef struct TBDne TBDne;
typedef struct TBDle TBDle;
typedef struct TBDkeys TBDkeys;
typedef struct TBDpool TBDpool;
typedef struct TBDfid TBDfid;


enum{
	Mbsz	=	3584,	/* 4096 - 512 */

	Ssz		=	256,
	Tsz		=	32,		/* all strings are broken up into tokens if seperated by space or hyphen */
	Csz		= 5,	/* holds utf comp operator, with null byte */

	Maxtree	=	8,
	Maxnode	=	13,
	Maxleaf	=	224,
	Maxquery	=	16,
};
	

enum{
	ℚnil	=	0,
	ℚroot,
	ℚlib,
	ℚattr,
	ℚnode,
	ℚleaf,
	ℚkey,
	ℚfile,
/* internal use */
	ℚtoken,
	ℚdot3,	//token?
	ℚnumber,	//base u64int
	ℙname,
	ℙsize,	//s64int
	ℙowner,
	ℙgroup,
	ℙmode,	//u32int
	ℙmtime,	//u32int
	ℙatime,	//u32int
	ℙℚid,	//u64int
/* general use */
	ℚstring,
	ℚinteger,	//s64int
	ℚdecimal,	//double
	ℚtime,
	ℚdate,
	ℚpointer,	//u64int
};


//keytype magic[4](node/leaf), pad[3], type[1]

/* magics */
enum{
	Mroot	=	0xAAAAAAAA00000000ULL,
	Mlib	=	0xBBBBBBBB00000000ULL,
	Mattr	=	0xCCCCCCCC00000000ULL,
	Mkey	=	0xDDDDDDDD00000000ULL,
	Mnode	=	0xEEEEEEEE00000000ULL,
	Mleaf	=	0xFFFFFFFF00000000ULL,
};


struct TBDfid{
	int root;
	int depth;
//	char comp[Maxquery][Csz];
	char path[Ssz];
	TBDnode *node;
//	TBDtable *qtab[Maxquery];
	u64int	*qtab[Maxquery];
};


struct TBDbase{
	char	*base;
	char	*meta;
	int		bfd;
	int		mfd;
};


struct TBDtable{
	u64int	ℚid;
	TBDpool		*file;
	TBDpool		*meta;
};


struct TBDgethash{
	char	name[Ssz];
	vlong	length;
	vlong	mtime;
};


struct TBDpool{
	u64int	ℚid;
	int		fd;
	char	realpath[Ssz];
	Dir		*d;
};


struct TBDmeta{
	char	*lib;
	char	*attr;
	char	*type;
	char	*key;
};


struct TBDkeys{
	union{
		char	s[Ssz];
		char	t[Tsz];
		s64int	i;
		double	d;
		u64int	n;
	};
};


struct TBDne{			/* 13 per node.elements */
	u64int	type;
	TBDnode	*take;
	TBDkeys;
};


struct TBDle{			/* can fit 224 per leaf */
	u64int	type;
	u64int	link;
};


struct TBDnode{
	u64int	type;
	char	attr[Tsz];
	TBDnode *tree;
	TBDnode	*parent;
	TBDnode	*less;
	TBDnode	*more;

	TBDkeys;

	union{
		TBDne nodes[Maxnode];
		TBDle leafs[Maxleaf];
	};
};
