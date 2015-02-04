/* Written By Onur Demiralay
* Github: @odemiral
* MIT License Copyright(c) 2015 Onur Demiralay
* C++11 implementation of QuadTree. Currently supports insertion, removal (with tree reduction)
* You can read more about what each function do by reading function definition header.

* Tree will stop dividing after given depth and will insert extra nodes to max depth instead.
* By design, I decided not to support duplicates, but you may easily include them by changing insertion and removal.
* QNode is meant to be a guide to show you how to integrate your object.
*/

#pragma once
#include <memory>
#include <iostream>
#include <unordered_set>
#include <iterator>
#include <typeinfo>
#include <algorithm> //find_if
#include "QNode.h"

template<class T>
class QuadTree : public enable_shared_from_this<QuadTree<T>>
{
public:
	QuadTree(const QuadTree&) = delete;				//forbid copy constructor
	QuadTree& operator=(QuadTree const&) = delete;	//forbid copy assignment operator
	QuadTree() = delete;							//forbid default constructor
	QuadTree& operator=(QuadTree&&) = delete;       //forbid default move assignment op
	QuadTree(QuadTree&&) = delete;					//forbid default move const


	//x1,y1 x2,y2. initially 0,0 to screen width, screen height.
	//bucket capacity is the amount of node can be represent by each quadtree before it splits.
	//@depth specifies how many times a tree can split, default value will be INT32_MAX
	explicit QuadTree(float x1, float y1, float x2, float y2, int bucketCapacity, int depth = INT32_MAX)
	{
		m_x = x1;
		m_y = y1;
		m_width = x2;
		m_height = y2;
		m_bucketCapacity = bucketCapacity;
		m_maxDepth = depth;
		m_currentBucketSize = 0;
		m_curDepth = 0;
		m_isLeaf = true;
		m_nodes.reserve(bucketCapacity + 1); //for each vector, we need up to bucketcapacity + 1 space (+1 because we'll push the next node onto vector before we subdivde)
		m_trees.reserve(4); //each subtree.
	}


	/* First check if there is a space within the tree, if the quadrant is a leaf node, and there is a space, insert it into it
	* If the bucket is full, then divide it to quadrants and then insert every element in the current tree to appropriate sbutrees.
	* allocation is done inside this node
	* you must have allocated node object the ptr points to before using this funct
	*/
	void insert(QNode<T> *node)
	{
		/* Can't insert nullptr */
		if (node == nullptr) {
			return;
		}

		std::shared_ptr<QNode<T>> newNode(make_shared<QNode<T>>(node));
		insertHelper(shared_from_this(), newNode);
	}


	/* instead of passing QNode, you can pass necessary info to create a QNode
	* @param x:		x-coordiante of the node
	* @param y:		y-coordinate of the node
	* @param data:		data the node holds
	*/
	void insert(float x, float y, const T& data)
	{
		std::shared_ptr<QNode<T>> newNode(make_shared<QNode<T>>(x, y, data));
		insertHelper(shared_from_this(), newNode);

	}

	/*
	* param @node to update its location.
	* param @x new x coordinates of @node
	* param @y new y coordinates of @node
	* Given @node, moves it to given x,y coordinates. 
	* WARNING: Haven't finished implemented this yet, it's still buggy use it with caution.
	* TODO: Calculate complexity.
	* TODO: this will be the most used function, try to optimize it as much as you can.
	*/
	inline void update(QNode<T>& node, float x, float y)
	{
		/* qTree can only be parent IF there is only 1 node in the tree. */
		shared_ptr <QuadTree<T>> qTree = findHelper(node);
		int count = qTree->m_nodes.count(make_shared<QNode<T>>(node)); //returns false!
		//node doesn't exist in the tree.
		if (count == 0) {
			//cout << "couldn't find the node in the tree!" << endl;
			return;
		}
		
		shared_ptr <QNode<T>> nodeptr = make_shared<QNode<T>>(node);

		/* could fit in the current quadrant, no need to move. */
		if (couldFit(qTree, x, y)) {
			nodeptr->x = x;
			nodeptr->y = y;
			qTree->m_nodes.insert(std::move(nodeptr));
			qTree->m_nodes.erase(make_shared<QNode<T>>(node));
			//only if you want to update the node obj as well. 
			node.x = x;
			node.y = y;
		}
		else {
			shared_ptr <QuadTree<T>> root = shared_from_this();
			while (qTree != root) {
				qTree = qTree->m_parent;
				if (couldFit(qTree, x, y)) {
					int quadrant = checkQuadrant(qTree, x, y);
					nodeptr->x = x;
					nodeptr->y = y;
					insertHelper(qTree, std::move(nodeptr)); //bug: removes them from the tree (qTree->m_trees[quadrant])
					remove(node); //need to call remove to get rid of node
					//node.x = x; 
					//node.y = y;
				}
			}
		}
	}

	/* Recursively removes all the elements in the quadtree (and its subtrees) 
	 * TODO: it's way too slow. try iterative, see if that would help.
	*/
	void clear()
	{
		m_nodes.clear();
		for (auto& tree : m_trees) {
			tree->clear(); //m_nodes.clear()? 
		}
		m_trees.clear();
		m_currentBucketSize = 0;
		m_curDepth = 0;
	}

	/* TODO: Implement this
	* Given node returns all the nodes that might collide with @node (currently that means return all the nodes in the same quadrant,
	* but for AABBs, this will mean every node that's in AABB
	*/
	vector<shared_ptr<QNode<T>>> getPossibleCollisions(const QNode<T>& node)
	{
		return nullptr;
	}

	/* given node find if it's in the quadtree, true if it is, false otherwise
	* O(log4(N)) to find the tree
	* O(1) time to find the node.
	*/
	bool find(const QNode<T>& node)
	{
		shared_ptr <QuadTree<T>> qTree = findHelper(node);
		/* Find the node in the unsorted_set, takes O(1) */
		int count = qTree->m_nodes.count(make_shared<QNode<T>>(node));
		return count != 0;
	}

	/*
	Find the node and remove it, then call removeSubtree which will find and remove all the redundant subtrees after removing the node.
	* O(log4(N)) to find the tree,
	* O(1) to remove the node,
	* O(K + log4(N)) to reduce the tree (if the reduction needed, O(!) otherwise)  K = num of nodes you have to move while reducing trees.
	* You can treat overall complexity as O(log4(N)) K should never be larger than m_maxBucketSize
	*/
	void remove(const QNode<T>& node)
	{
		shared_ptr <QuadTree<T>>  qTree = findHelper(node);
		int removed = qTree->m_nodes.erase(make_shared<QNode<T>>(node));
		if (removed != 0) {
			qTree->m_currentBucketSize--; //update the bucketsize of the quadrant MIGHT WANNA MOVE THIS TO removeSubtree() instead.
			removeSubtree(qTree);
		}
	}

	/* Getters */
	inline float getX() const { return m_x; }
	inline float getY() const { return m_y; }
	inline float getWidth() const { return m_width; }
	inline float getHeight() const { return m_height; }
	inline int getDepth() const { return m_curDepth; }


	/* Keep track of the parent ptr to decrease amount of calc done to reinsert a node. */
	QuadTree(const shared_ptr<QuadTree<T>>& parent, float x1, float y1, float x2, float y2)
	{
		m_parent = parent;
		m_x = x1;
		m_y = y1;
		m_width = x2;
		m_height = y2;
		m_currentBucketSize = 0;
		m_isLeaf = true;
		m_nodes.reserve(m_bucketCapacity + 1);
	}


private:

	/* removeSubTree should always be called on parent, that way we can always assume subquadtrant exist.
	* O(log4(N)) to reduce the tree.
	* TODO: Explain how this work in detail.
	*/
	void removeSubtree(shared_ptr <QuadTree<T>> tree) //pass by copy because we'll be manipulating @tree
	{
		tree = tree->m_parent; // go to its parent
		tree->m_currentBucketSize--; //update its first parent.

		shared_ptr <QuadTree<T>> root = shared_from_this();
		shared_ptr <QuadTree<T>> prev = tree;

		//check if you need to perform reduction at all.
		if (tree->m_currentBucketSize > m_bucketCapacity) {
			//still need to update bucket sizes
			while (tree != root) {
				tree->m_parent->m_currentBucketSize--;
				tree = tree->m_parent;
			}
			return;
		}



		/* MOVE all the nodes in each quadrant to tempSet, which will be used to move the nodes of reduced tree to new one. */
		unordered_set<shared_ptr<QNode<T>>, NodeHashFunc<QNode<T>>, EqualTo<QNode<T>>> tempSet;
		/* Move all the nodes of each quadrant into tempSet */
		std::move(tree->m_trees[NW_QUADRANT]->m_nodes.begin(), tree->m_trees[NW_QUADRANT]->m_nodes.end(), std::inserter(tempSet, tempSet.begin()));
		std::move(tree->m_trees[SW_QUADRANT]->m_nodes.begin(), tree->m_trees[SW_QUADRANT]->m_nodes.end(), std::inserter(tempSet, tempSet.begin()));
		std::move(tree->m_trees[NE_QUADRANT]->m_nodes.begin(), tree->m_trees[NE_QUADRANT]->m_nodes.end(), std::inserter(tempSet, tempSet.begin()));
		std::move(tree->m_trees[SE_QUADRANT]->m_nodes.begin(), tree->m_trees[SE_QUADRANT]->m_nodes.end(), std::inserter(tempSet, tempSet.begin()));


		//while tree is not root, we can traverse to parent.
		while (tree != root) {
			tree->m_parent->m_currentBucketSize--; //decrease one, then check. (since we removed a node, parents trees bucketsize haven't updated yet.
			if (tree->m_currentBucketSize <= m_bucketCapacity) { //if bucket size is less than capacity, can reduce that tree, go to its parent.
				prev = tree;
				tree = tree->m_parent;
			}
			else {
				//we're on a tree that can't be reduced, move all the nodes to the correct quadrant of this tree and remove all the subtrees.
				int quadrant = checkQuadrant(tree, prev);
				prev->m_trees.clear(); //clear all the data of the quadrant.
				//move all the nodes from tempset to the new, reduced quadrant
				std::move(tempSet.begin(), tempSet.end(), std::inserter(tree->m_trees[quadrant]->m_nodes, tree->m_trees[quadrant]->m_nodes.begin()));

				//update currentBucketSize of rest of the parent trees including root. 
				tree = tree->m_parent;
				while (tree != root) {
					tree->m_parent->m_currentBucketSize--;
					tree = tree->m_parent;
				}
			}
		}
	}

	/* Given quadtree and x,y check if x,y is within the boundaries of @tree */
	bool couldFit(const shared_ptr<QuadTree<T>>& tree, float x, float y)
	{
		return (tree->m_x <= x && x <= tree->m_width) && (tree->m_y <= y && y <= tree->m_height);
	}

	/* Helper function, used by find() and remove(), given node finds the quadrant, the node *would* be in if it exist
	* it then checks the quadrant, if the node exist, returns the quadtrant, if it doesn't returns the last leaf node with node child.
	* takes O(log4(N)) time to find the quadtree that contains the node.
	*/
	shared_ptr <QuadTree<T>> findHelper(const QNode<T>& node)
	{
		shared_ptr <QuadTree<T>> currentHead = shared_from_this();
		/* if m_trees size is > 0, then child exist, */
		while (currentHead->m_trees.size() != 0) {
			int quadrant = checkQuadrant(currentHead, node);
			switch (quadrant) {
			case NW_QUADRANT:
				currentHead = currentHead->m_trees[NW_QUADRANT];
				break;
			case NE_QUADRANT:
				currentHead = currentHead->m_trees[NE_QUADRANT];
				break;
			case SW_QUADRANT:
				currentHead = currentHead->m_trees[SW_QUADRANT];
				break;
			case SE_QUADRANT:
				currentHead = currentHead->m_trees[SE_QUADRANT];
				break;
			default: //throw a runtime error instead
				std::cout << "ERROR: UNDEFINED QUADRANT! THIS SHOULD NEVER HAVE HAPPENED!" << std::endl;
				exit(-1);
				break;
			}
		}
		return currentHead;
	}

	/* private insert funct, should only be used internally. */
	inline void insert(const shared_ptr<QNode<T>>& node)
	{
		insertHelper(shared_from_this(), node);
	}





	/* Helper func used by all the insert functions.
	* if the node we're leaf
	*	1) if current size is less than the max, insert the element onto that node.
	*	2) if current depth is less than the max, subdivide the tree, insert them onto current node, and rearrange the node.
	*	3) otherwise we reached max depth and max capacity, we can't divide any more but we can still override max capacity and insert them onto the current node
	* else
	*   we're at max capacity, insert them onto the first node you find (root) and call rearrange to find the correct spot.
	* When it's called by insert functions, tree will always be shared_from_this() (shared ptr to "this")
	*/
	inline void insertHelper(shared_ptr<QuadTree<T>>& tree, const shared_ptr<QNode<T>>& node)
	{
		if (m_isLeaf) {
			if (m_currentBucketSize < m_bucketCapacity) {
				m_nodes.emplace(std::move(node)); 
				m_currentBucketSize++;
			}
			else if (m_curDepth < m_maxDepth) {
				subdivide();
				m_nodes.emplace(std::move(node));
				m_currentBucketSize++;
				reArrangeNodes(tree);
			}
			else {
				m_nodes.emplace(std::move(node));
				m_currentBucketSize++;
			}
		}
		else { /* This will only triggered when depth is more than max,
			   remove this block if you wish to stop adding elements to tree after depth >= maxDepth and bucketSize >= maxBucketSize */
			m_nodes.emplace(std::move(node));
			m_currentBucketSize++;
			reArrangeNodes(tree); //must rearrange them, since we'd be adding them to root.
		}
	}

	/* divides quadtree to 4 quadtrants
	* updates current depth
	* updates m_isLeaf for the current tree.
	*/
	void subdivide()
	{
		float newXRegion = m_width / 2.0f;
		float newYRegion = m_height / 2.0f;

		//NW, NE, SW, SE
		m_trees.emplace_back(make_shared<QuadTree<T>>(shared_from_this(), m_x, m_y, newXRegion, newYRegion));
		m_trees.emplace_back(make_shared<QuadTree<T>>(shared_from_this(), m_x + newXRegion, m_y, m_width, newYRegion));
		m_trees.emplace_back(make_shared<QuadTree<T>>(shared_from_this(), m_x, m_y + newYRegion, newXRegion, m_height));
		m_trees.emplace_back(make_shared<QuadTree<T>>(shared_from_this(), m_x + newXRegion, m_y + newYRegion, m_width, m_height));


		//set depth of the current tree
		m_trees[NW_QUADRANT]->setDepth(m_curDepth + 1);
		m_trees[NE_QUADRANT]->setDepth(m_curDepth + 1);
		m_trees[SW_QUADRANT]->setDepth(m_curDepth + 1);
		m_trees[SE_QUADRANT]->setDepth(m_curDepth + 1);

		m_isLeaf = false; //tree is divided to quadrants, it's no longer a leaf node.

	}

	/* private setter, used by subdivision */
	inline void setDepth(int depth) {
		m_curDepth = depth;
	}


	/* Pushes nodes currently located on @tree to the appropriate subtrees. (treats @tree as the root)
	* called after subtrees are generated.
	* NOTE: this funct uses move semantics to prevent unnecessary copies and increase performance.
	* O(1) time to erase
	* O(1) time to move
	* O(log4(N)) time to recursively insert
	*/
	inline void reArrangeNodes(shared_ptr <QuadTree<T>>& tree)
	{
		for (auto it = tree->m_nodes.begin(); it != tree->m_nodes.end();) { //++it) {
			int quadrant = checkQuadrant((**it));
			//move the node into appropriate quadrant
			switch (quadrant) {
				case NW_QUADRANT:
					tree->m_trees[NW_QUADRANT]->insert(std::move(*it));
					tree->m_nodes.erase(it++);
					break;
				case NE_QUADRANT:
					tree->m_trees[NE_QUADRANT]->insert(std::move(*it));
					tree->m_nodes.erase(it++);
					break;
				case SW_QUADRANT:
					tree->m_trees[SW_QUADRANT]->insert(std::move(*it));
					tree->m_nodes.erase(it++);
					break;
				case SE_QUADRANT:
					tree->m_trees[SE_QUADRANT]->insert(std::move(*it));
					tree->m_nodes.erase(it++);
					break;
				default: //throw a runtime error instead
					std::cout << "ERROR: UNDEFINED QUADRANT! THIS SHOULD NEVER HAVE HAPPENED!" << std::endl;
					exit(-1);
					break;
			}
		}
	}

	/*
	1 (0 << 1)		1 | 0 = 1
	0 (1 << 1)		0 | 2 = 2
	1 (1  << 1)		1 | 2 = 3
	0 (0 << 1)		0 | 0 = 0
	*/

	/*
	Given node, check which quadrant it would fall into.
	optimized so that it only does very few instructions to get quadrant
	TODO: see if you can come up with even faster solution
	DOESNT CHECK FOR NODES FALLS OUTSIDE QuadTree (which should never happen)
	if you want to check if a node is outside of quadtree, check it BEFORE calling this function.
	*/
	inline int checkQuadrant(const QNode<T> &node) const
	{
		float xMid = m_x + (m_width - m_x) / 2.0f;
		float yMid = m_y + (m_height - m_y) / 2.0f;
		int res = (node.x >= xMid) | ((node.y >= yMid) << 1);
		return res;
	}

	/* return the quadrant of where @node would be in given @tree. */
	int checkQuadrant(const shared_ptr<QuadTree<T>>& tree, const QNode<T> &node) const
	{
		float xMid = tree->getX() + (tree->getWidth() - tree->getX()) / 2.0f;
		float yMid = tree->getY() + (tree->getHeight() - tree->getY()) / 2.0f;
		int res = (node.x >= xMid) | ((node.y >= yMid) << 1);
		return res;
	}

	/* return the quadrant of where @child would be in given @parent.*/
	int checkQuadrant(const shared_ptr<QuadTree<T>>& parent, const shared_ptr<QuadTree<T>>& child) const
	{
		float xMid = parent->getX() + (parent->getWidth() - parent->getX()) / 2.0f;
		float yMid = parent->getY() + (parent->getHeight() - parent->getY()) / 2.0f;
		int res = (child->getX() >= xMid) | ((child->getY() >= yMid) << 1);
		return res;
	}

	/* return the quadrant of where x,y would be in given @tree. */
	int checkQuadrant(const shared_ptr<QuadTree<T>>& tree, float x, float y) const
	{
		float xMid = tree->getX() + (tree->getWidth() - tree->getX()) / 2.0f;
		float yMid = tree->getY() + (tree->getHeight() - tree->getY()) / 2.0f;
		int res = (x >= xMid) | ((y >= yMid) << 1);
		return res;
	}

	//Member variables
	float m_height;
	float m_width;
	float m_x;
	float m_y;
	int m_currentBucketSize;	// num of nodes in current bucket. 
	bool m_isLeaf;				//determine whether or not the newly created quadtree is a leaf
	int m_curDepth;				//cur depth of the tree.

	//static member variables, these won't change for subtrees, therefore no need to pass them again in the constructor (mo opcode, mo problems!)
	static int m_bucketCapacity;	//num of nodes per tree before it splitting to subtrees.
	static int m_maxDepth;			//max time tree can split.


	/* holds pointers to subtrees. */
	vector<shared_ptr<QuadTree<T>>> m_trees;

	//represents quadrants
	enum quadrants { NW_QUADRANT = 0, NE_QUADRANT = 1, SW_QUADRANT = 2, SE_QUADRANT = 3 };
	
	shared_ptr<QuadTree<T>> m_parent; //parent node for any given tree.
	unordered_set<shared_ptr<QNode<T>>, NodeHashFunc<QNode<T>>, EqualTo<QNode<T>>> m_nodes; //stores nodes in each quadrant.

};

template <class T> int QuadTree<T>::m_bucketCapacity = 0;
template <class T> int QuadTree<T>::m_maxDepth = 0;