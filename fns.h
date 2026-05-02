extern Srv tbdsrv;
extern TBDbase tbdbase;
extern TBDtable *tbdtab;
extern int tabsz;
extern TBDnode *tbdroot;
extern TBDnode *nametree;
extern long time0;
extern char *user;
extern int tbddebug;


/* fs.c */
void	tbdfsattach(Req*);
void	tbdfsstart(Srv*);
void	tbdfsattach(Req*);
void	tbdfsread(Req*);
char*	tbdfswalk1(Fid*, char*, Qid*);
char*	tbdfsclone(Fid*, Fid*);
void	tbdfsdestroyfid(Fid*);
void	tbdfsstat(Req*);
void	tbdfsopen(Req*);

/* hash.c */
u64int	tbd0hash(char *src);
int		tbdfetchprime(int);

/* table.c */
TBDpool*	tbdgettable(u64int);
TBDpool*	tbdmetatable(u64int);
void	tbdstarttable(int);
u64int		tbdputtable(char*);
u64int	tbdgetmod(u64int);

/* index.c */
void	tbdbuildindex(void);
void	addnode(char*, u64int);
TBDnode*	tbdfindnode(TBDnode*, char*, int);

/* tree.c */
void	tbdbuildtree(void);

/* util.c */
u64int	tbdgetkeytype(char*);
void	tbdnedump(TBDne*);
int		tbdgetlastne(TBDne*);
void	tbdnecpy(TBDne*, TBDne*);
int		tbdnecmp(TBDne*, TBDne*);
void	tbdfillne(TBDne*, char*, u64int);
TBDnode*	tbdfetchnode(TBDnode*, char*);
void	tbdgetnodename(char*, TBDnode*);
void	tbdgetnename(char*, TBDne*);
TBDnode*	tbdrunquery(TBDnode*, char*);
u64int	tbdfilefromkey(TBDnode*, char*);
void	tbdsanistring(char*, char*, char*);
char*	tbdstrcpy(char*, char*, int);
int		tbdtreecmp(TBDnode*, TBDne*);