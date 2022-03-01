
_lwc_config_ {
//	lwcdebug DCL_TRACE;
//	lwcdebug VIRTUALTABLES;
	const_virtual_tables;
};

class mmap_file
{
	int fd;
   public:
	char *data;
	int len;
	bool success;

	ctor (char*);
	dtor ();
};

class intNode;

class intTree
{
	int Query;
	intNode **FoundSlot;
	void addself	(intNode);
   public:
	intNode *root;
	int cnt;
	intTree		();
	intNode*	intFind		(unsigned int);
	void		intRemove	(intNode*);
};

class intNode
{
	intNode *less, *more;
   public:
	unsigned int Key;
	intNode (intTree);
};

class foreach_intNode
{
	inline virtual;
virtual	modular void Do (intNode) = 0;
	void walktree (intNode) const;
   public:
	ctor (intTree);
};

class dbsNode;

class dbsTree
{
	inline virtual;
virtual	int DBS_MAGIC = 31;
virtual	int compare (const dbsNode) const = 0;
virtual	int compare (const dbsNode, const dbsNode) const = 0;

	dbsNode **FoundSlot;
	int FoundDepth;
	void tree_to_array	(dbsNode);

	dbsNode*	parentOf	(dbsNode*) const;
	void		addself		(dbsNode);
   public:
	dbsNode *root;
	int nnodes;
	dbsTree		();
	void		dbsBalance	();
auto
virtual dbsNode*	dbsFind		();
	void		dbsRemove	(dbsNode*);
	dbsNode*	dbsPrev		(dbsNode*);
	dbsNode*	dbsNext		(dbsNode*);
};

class dbsNode
{
	dbsNode *less, *more;
};

class dbsNodeStr : dbsNode
{
	char *str;
};

class dbsTreeStr : dbsTree
{
	int compare (const dbsNode) const;
	int compare (const dbsNode, const dbsNode) const;
   public:
	char *Query;
	dbsNodeStr *dbsFind (char*);
};

class dbsForEach
{
	const inline virtual;
modular
virtual void Do (dbsNode) = 0;
auto
virtual void walktree (dbsNode) = 0;
   public:
	dbsForEach (dbsTree);
};

class dbsForEach_Inorder : dbsForEach
{
	void walktree (dbsNode);
};

class dbsForEach_Preorder : dbsForEach
{
	void walktree (dbsNode);
};

class dbsForEach_Postorder : dbsForEach
{
	void walktree (dbsNode);
};

//
// expanding array template
//

#define I1A(x) (x / (PSEG*NSEG))
#define I2A(x) ((x % (PSEG*NSEG)) / PSEG)
#define I3A(x) ((x % (PSEG*NSEG)) % PSEG)

template
class exp_array
{
   private:
	X ***p;
   public:
	int sp;
	X& operator [] (int i)	{ return p [I1A (i)] [I2A (i)] [I3A (i)]; }
	X& operator ++ ();
	exp_array ();
	X *freeze ();
};

exp_array.exp_array ()
{
	sp = 0;
	p = (X***) malloc (sizeof (X**) * TSEG);
}

X& exp_array.operator ++ ()
{
	if (sp % (PSEG*NSEG) == 0)
		p [I1A (sp)] = (X**) malloc (NSEG * sizeof (X*));
	if (sp % PSEG == 0)
		p [I1A (sp)] [I2A (sp)] = (X*) malloc (PSEG * sizeof (X));
	return (*this) [sp++];
}

X *exp_array.freeze ()
{
	X *ret = (X*) malloc (sizeof (X) * sp);

	int i;

	for (i = 0; i + PSEG < sp ; i += PSEG)
		memcpy (&ret [i], p [I1A (i)] [I2A (i)], PSEG * sizeof (X));
	memcpy (&ret [i], p [I1A (i)] [I2A (i)], (sp - i) * sizeof (X));

	return ret;
}

//
// 
// auto functions
//
dbsNode *dbsTree.dbsFind ()		// O (log n)
{
	dbsNode *d;
	int i;

	FoundDepth = 0;

	if (!(d = root)) {
		FoundSlot = &root;
		return 0;
	}

	++FoundDepth;

	for (;; ++FoundDepth) {
		if ((i = compare (d)) == 0) {
			FoundSlot = 0;
			return d;
		}
		if (i < 0)
			if (d->more) d = d->more;
			else {
				FoundSlot = &d->more;
				return 0;
			}
		else
			if (d->less) d = d->less;
			else {
				FoundSlot = &d->less;
				return 0;
			}
	}
}
