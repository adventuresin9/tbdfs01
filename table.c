#include <u.h>
#include <libc.h>
#include <auth.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"



TBDpool*
tbdgettable(u64int ℚid)
{
	u64int mod;
	int i;


	mod = abs(ℚid % tabsz);

	for(i = 0; 1 < 10; i++){
		if(tbdtab[mod].ℚid == ℚid)
			break;
		mod = (mod + 1) % tabsz;
	}

	return(tbdtab[mod].file);
}


u64int
tbdgetmod(u64int ℚid)
{
	u64int mod;
	int i;

	mod = abs(ℚid % tabsz);

	for(i = 0; 1 < 10; i++){
		if(tbdtab[mod].ℚid == ℚid)
			break;
		mod = (mod + 1) % tabsz;
	}

	return mod;;
}



TBDpool*
tbdmetatable(u64int ℚid)
{
	u64int mod;
	int i;


	mod = abs(ℚid % tabsz);

	for(i = 0; 1 < 10; i++){
		if(tbdtab[mod].ℚid == ℚid)
			break;
		mod = (mod + 1) % tabsz;
	}

	return(tbdtab[mod].meta);
}


u64int
tbdputtable(char *name)
{
	int i;
	u64int ℚid, mod;

	ℚid = tbd0hash(name);

//print("given %uX\n", ℚid);

	/* hash is unsigned, can produce negative numbers */
	mod = abs(ℚid % tabsz);

	i = 0;
	
	while(i < 10){
		if(tbdtab[mod].ℚid == 0){
			tbdtab[mod].ℚid = ℚid;
			return mod;
		}
		if(tbddebug)
			print("colli %lluX at %ulld\n", ℚid, mod);

		mod = (mod + 1) % tabsz;
		i++;
	}

	sysfatal("hash col: %s", name);
}
