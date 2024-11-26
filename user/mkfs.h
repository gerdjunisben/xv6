#include "kernel/types.h"

void        balloc(int);
void        wsect(uint, void*);
void        winode(uint, struct dinode*);
void        rinode(uint inum, struct dinode *ip);
void        rsect(uint sec, void *buf);
uint        ialloc(ushort type);
void        iappend(uint inum, void *p, int n);