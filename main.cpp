
#include <iostream>
#include "Quadtree.hpp"
#include <ctime>
#include <vector>

using namespace std;

#ifdef _DEBUG 
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif


template <typename T>
void insertionTest(shared_ptr<QuadTree<T>>& tree)
{
	cout << "insertion test" << endl;
	QNode<T> node1(750, 500);
	QNode<T> node2(150, 200);
	QNode<T> node3(10, 10);
	tree->insert(&node1);
	cout << "inserting node (" << node1.x << ", " << node1.y << ")" << endl;
	tree->insert(&node2);
	cout << "inserting node (" << node2.x << ", " << node2.y << ")" << endl;
	tree->insert(&node3);
	cout << "inserting node (" << node3.x << ", " << node3.y << ")" << endl;

	tree->insert(350, 500, 1);
	cout << "inserting node (350, 500)" << endl;

	tree->insert(500, 100, 1);
	cout << "inserting node (500, 100)" << endl;

	tree->insert(300, 100, 1);
	cout << "inserting node (300, 100)" << endl;

	tree->insert(900, 600, 1);
	cout << "inserting node (900, 600)" << endl;

}
int myrandom(int i) { return std::rand() % i; }


template <typename T>
void findTest(shared_ptr<QuadTree<T>>& tree)
{
	cout << "find test" << endl;
	QNode<T> node1(10, 10);
	QNode<T> node2(900, 600);
	QNode<T> node3(1234, 123);
	cout << "trying to find a node that doesn't exist" << endl;
	cout << "found? " << tree->find(node3) << endl;
	cout << "trying to find nodes in the tree" << endl;
	cout << "found? " << tree->find(node2) << endl;
	cout << "found? " << tree->find(node1) << endl;
}

template <typename T>
void removalTest(shared_ptr<QuadTree<T>>& tree)
{
	cout << "removal test" << endl;
	QNode<T> node1(1000000, 100000);
	QNode<T> node2(10, 10);
	cout << "removing a node that doesn't exist" << endl;
	tree->remove(node1);

	cout << "removing a node in the tree" << endl;
	tree->remove(node2);
}

template <typename T>
void updateTest(shared_ptr<QuadTree<T>>& tree)
{
	cout << "update test" << endl;
	QNode<T> node1(300, 100); 
	QNode<T> node2(350, 500);
	QNode<T> node3(900, 600);
	QNode<T> node4(10, 10);
	QNode<T> node5(999, 999);
	cout << "updating nodes in the tree" << endl;
	tree->update(node1, 1000, 800);
	tree->update(node2, 1050, 700);
	tree->update(node3, 1300, 600);
	tree->update(node4, 33, 999);

	cout << "updating a node that doesn't exist" << endl;
	tree->update(node5, 1111, 1111);

	cout << "updating a node that has been updated before" << endl;
	tree->update(node4, 333, 999); 

	cout << "updating a node that has set to outside of the tree" << endl;
	//tree->update(node3, 1333, 666);

	//tree->update(node4, 1, 2);

}


template <typename T>
void clearTest(shared_ptr<QuadTree<T>>& tree)
{
	cout << "removing all the contents of the tree" << endl;
	tree->clear();
}


void smallTreeTest()
{
	shared_ptr<QuadTree<float>> qTree(new QuadTree<float>(0, 0, 1920, 1080, 1, 3));
	insertionTest(qTree);
	system("pause");
	findTest(qTree);
	system("pause");
	updateTest(qTree);
	system("pause");
	removalTest(qTree);
	system("pause");
	clearTest(qTree);
}

void largeTreeTest()
{
	float width = 1920;
	float height = 1080;
	shared_ptr<QuadTree<float>> qTree(new QuadTree<float>(0, 0, width, height, 1, 10));

	vector<QNode<float>> vec;
	//randomly skip too.
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			vec.push_back(QNode<float>(i, j));
		}
	}
	cout << "shuffling the vec" << endl;
	std::random_shuffle(vec.begin(), vec.end(), myrandom);

	QNode<float> n1 = vec.at(vec.size() - 100);
	QNode<float> n2 = vec.at(vec.size() - 200);
	QNode<float> n3 = vec.at(vec.size() - 300);
	QNode<float> n4 = vec.at(vec.size() - 400);
	QNode<float> n5 = vec.at(vec.size() - 500);
	QNode<float> n6(width * 3 - 5, height * 5 - 3);
	cout << n1.x << ", " << n1.y << endl;
	cout << n2.x << ", " << n2.y << endl;
	cout << n3.x << ", " << n3.y << endl;
	cout << n4.x << ", " << n4.y << endl;
	cout << n5.x << ", " << n5.y << endl;

	cout << "inserting elements to Quadtree" << endl;
	system("pause");
	for (int i = 0; i < vec.size() / 2; ++i) {
		qTree->insert(&vec.back());
		vec.pop_back();
	}
	cout << "finding nodes" << endl;
	system("pause");
	cout << "found? " << qTree->find(n1) << endl;
	cout << "found? " << qTree->find(n2) << endl;
	cout << "found? " << qTree->find(n3) << endl;
	cout << "found? " << qTree->find(n4) << endl;
	cout << "found? " << qTree->find(n5) << endl;
	cout << "found? " << qTree->find(n6) << endl;

	cout << "updating nodes" << endl;
	qTree->update(n1, 1000,100);
	qTree->update(n2, height / 2, height / 2);
	qTree->update(n3, width/2, width/2);
	qTree->update(n4, 123, 456);
	qTree->update(n5, 1234, 654);
	qTree->update(n6, 654321, 12345678);

	cout << "deleting nodes" << endl;
	system("pause");
	qTree->remove(n1);
	qTree->remove(n2);
	qTree->remove(n3);
	qTree->remove(n4);
	qTree->remove(n5);
	qTree->remove(n6);

	cout << "clearing tree" << endl;
	system("pause");
	qTree->clear();
}

int main()
{
	std::srand(unsigned(std::time(0)));
	largeTreeTest();
	smallTreeTest();
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
	system("pause");
}