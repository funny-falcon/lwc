/******************************************************************************
	Dynamic Binary Search Tree.
	Check dbstree.tex for info.
******************************************************************************/


#ifndef HAVE_DBSTREE
#define HAVE_DBSTREE
#include "global.h"

// *#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*
//	Interface of dbstree template
// *#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*

template dbsTreeTemplate (dbsNode, dbsTree) {

class dbsTree
{
	dbsNode **FoundSlot;
	int FoundDepth;
	void tree_to_array	(dbsNode);
	void walk_tree		(dbsNode*, void (*)(dbsNode*));
	void walk_tree_io	(dbsNode*, void (*)(dbsNode*));
	void walk_tree_d	(dbsNode*, void (*)(dbsNode*));
   public:
	dbsNode *root;
	int nnodes;
	dbsTree		();
	void		dbsBalance	();
	dbsNode*	dbsFind		();
	void		dbsRemove	(dbsNode);
	void		copytree	(void (*)(dbsNode*));
	void		foreach		(void (*)(dbsNode*));
	void		deltree		(void (*)(dbsNode*));
	dbsNode*	parentOf	(dbsNode);
	void		addself		(dbsNode);
};

}

// *#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*
//	Implementation of dbstree template
// *#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*

template dbsTreeTemplate_implementation (dbsNode, dbsTree, DBS_MAGIC) {

dbsTree.dbsTree ()
{
	nnodes = 0;
	root = NULL;
	FoundSlot = NULL;
}

/*
 * Balancing a binary tree. This happens whenever the height reaches 32.
 */
void dbsTree.tree_to_array (dbsNode n)
{
	if (n.less) tree_to_array (n.less);
	*FoundSlot++ = &n;
	if (n.more) tree_to_array (n.more);
}

void dbsTree.dbsBalance ()		// O(n)
{
	dbsNode **npp;
	unsigned long long i, j, k, D, Y, k2, k3;

	if (!root) return;

	npp = FoundSlot = (dbsNode**) alloca (sizeof (dbsNode*) * nnodes);
	tree_to_array (root);

	root = npp [nnodes / 2];
	for (D = nnodes + 1, i = 4; i <= D; i *= 2)
		for (j = 2; j < i; j += 4)
		{
			k3 = nnodes * j / i;
			npp [k3]->less = npp [nnodes * (j - 1) / i],
			npp [k3]->more = npp [nnodes * (j + 1) / i];
		}
	k = nnodes + 1 - (Y = i / 2);
	if (k == 0)
	{
		for (i /=2, j = 1; j < i; j += 2)
			k3 = nnodes * j / i,
			npp [k3]->less = npp [k3]->more = NULL;
		return;
	}

	for (j = 2; j < i; j += 4)
	{
		k3 = nnodes * j / i;
		D = (k2 = (j - 1) * nnodes / i) * Y % nnodes;
		if (D >= k || D == 0)
			npp [k3]->less = NULL;
		else
		{
			npp [k3]->less = npp [k2];
			npp [k2]->less = npp [k2]->more = NULL;
		}
		D = (k2 = (j + 1) * nnodes / i) * Y % nnodes;
		if (D >= k || D == 0)
			npp [k3]->more = NULL;
		else
		{
			npp [k3]->more = npp [k2];
			npp [k2]->less = npp [k2]->more = NULL;
		}
	}

	dbsNode *np;
	for (np = root; np->less; np = np->less);

	np->less = npp [0];
	npp [0]->less = npp [0]->more = NULL;
}

void dbsTree.addself (dbsNode t)
{
	t.less = t.more = 0;
	*FoundSlot = &t;
	++nnodes;

	if (FoundDepth >= DBS_MAGIC)
		dbsBalance ();
	FoundSlot = NULL;	// Bug traper
}

/*
 */
void dbsTree.dbsRemove (dbsNode t)		// O(log n)
{
	dbsNode *np, *nl, *nr, *nlp, *nrp;
	int isroot;
	unsigned int i, j;

	isroot = (np = parentOf (t)) == NULL;

	--nnodes;

	if (!(t.less && t.more))
	{
		if (isroot)
			root = (t.less) ? t.less : t.more;
		else
			if (np->less == &t)
				np->less = (t.less) ? t.less : t.more;
			else
				np->more = (t.less) ? t.less : t.more;
		return;
	}

	for (i = 0, nlp = NULL, nl = t.less; nl->more; i++)
		nlp = nl, nl = nl->more;
	for (j = 0, nrp = NULL, nr = t.more; nr->less; j++)
		nrp = nr, nr = nr->less;

	if (i >= j)		// the smallest from bigger ones
	{
		if (isroot) root = nl;
		else
			if (np->less == &t) np->less = nl;
			else np->more = nl;
		if (nlp)
		{
			nlp->more = nl->less;
			nl->less = t.less;
		}
		nl->more = t.more;
	}
	else	// Mirror situation
	{
		if (isroot) root = nr;
		else
			if (np->less == &t) np->less = nr;
			else np->more = nr;
		if (nrp)
		{
			nrp->less = nr->more;
			nr->more = t.more;
		}
		nr->less = t.less;
	}
}

dbsNode *dbsTree.parentOf (dbsNode t)		// O(log n)
{
	dbsNode *np;

	if ((np = root) == &t)
		return NULL;

	while (np)
		if (t.compare (np) < 0)
			if (np->less == &t) break;
			else np = np->less;
		else
			if (np->more == &t) break;
			else np = np->more;

	assert (np);

	return np;
}

dbsNode *dbsTree.dbsFind ()		// O (log n)
{
	dbsNode *d;
	int i;

	FoundDepth = 0;

	if (!(d = root)) {
		FoundSlot = &root;
		return NULL;
	}

	++FoundDepth;

	for (;; ++FoundDepth) {
		if ((i = d->compare ()) == 0) {
			FoundSlot = NULL;
			return d;
		}
		if (i < 0)
			if (d->more) d = d->more;
			else {
				FoundSlot = &d->more;
				return NULL;
			}
		else
			if (d->less) d = d->less;
			else {
				FoundSlot = &d->less;
				return NULL;
			}
	}
}

void dbsTree.walk_tree (dbsNode *n, void (*foo)(dbsNode*))
{
	foo (n);
	if (n->less) walk_tree (n->less, foo);
	if (n->more) walk_tree (n->more, foo);
}
void dbsTree.walk_tree_io (dbsNode *n, void (*foo)(dbsNode*))
{
	if (n->less) walk_tree_io (n->less, foo);
	foo (n);
	if (n->more) walk_tree_io (n->more, foo);
}
void dbsTree.walk_tree_d (dbsNode *n, void (*foo)(dbsNode*))
{
	if (n->less) walk_tree_d (n->less, foo);
	if (n->more) walk_tree_d (n->more, foo);
	foo (n);
}


void dbsTree.copytree (void (*f)(dbsNode*))
{
	if (root) walk_tree (root, f);
}

void dbsTree.foreach (void (*f)(dbsNode*))
{
	if (root) walk_tree_io (root, f);
}

void dbsTree.deltree (void (*f)(dbsNode*))
{
	if (root) walk_tree_d (root, f);
}
}

// *#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*
//	Let's instantiate for a tree of case
//	sensitive strings
// *#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*

extern char *DBS_STRQUERY;

class dbsNodeStr {
   public:
	int compare (dbsNodeStr);
	int compare ();
	dbsNodeStr ();
	char *Name;
	~dbsNodeStr ();
};

class symboltmp : dbsNodeStr
{
   public:
	symboltmp	*less, *more;
	symboltmp (int);
	unsigned int ID;
};

class stringtmp : dbsNodeStr
{
   public:
	stringtmp	*less, *more;
	stringtmp (int);
	unsigned int ID;
};

struct utype
{
	utype *less, *more;
	utype ();
	typeID ID;
	int compare (utype*);
	int compare ();
};

class argNode
{
   public:
	argNode *less, *more;
	argNode ();
	int compare (argNode*);
	int compare ();
	ArglPtr ID;
};

dbsTreeTemplate (symboltmp, symboltmpTree)
dbsTreeTemplate (stringtmp, stringtmpTree)
dbsTreeTemplate (utype, utypeTree)
dbsTreeTemplate (argNode, argNodeTree)

#endif
