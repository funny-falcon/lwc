/******************************************************************************
	Interger Key Binary Tree.
	O(1)
******************************************************************************/

class intNode;

class intTree
{
   public:
	intNode *root;
	int cnt;
	intTree		();
	intNode*	intFind		(unsigned int);
};

class intNode
{
	void addself	(intTree);
	intNode *less, *more;
	void intRemove	(intTree);
   public:
	unsigned int Key;
	intNode (intTree);
};
