/* Written By Onur Demiralay
* Github: @odemiral
* MIT License Copyright(c) 2015 Onur Demiralay
* Simple class to show how Quadtree functions work, you can implement your own node method instead,
* just make sure it has x,y to represent its location in 2d plane.
* You might also wanna change NodeHashFunc for your custom class.
*/

#pragma once
#include <memory>
#include <iostream>
#include <unordered_set>
#include <iterator>
#include <typeinfo>
#include <algorithm>

using namespace std;
template<class T>
class QNode
{
public:
	float x, y; //coordinates of the node.
	T m_data;
	QNode(float x, float y)
	{
		this->x = x;
		this->y = y;
	}
	QNode(float x, float y, const T& data)
	{
		this->x = x;
		this->y = y;
		m_data = data;
	}
		
	QNode(const QNode *node)
	{
		x = node->x;
		y = node->y;
		m_data = node->m_data;
	}

	bool operator==(const QNode<T>* rhs)
	{
		return rhs->x == x && rhs->y == y;
	}
	bool operator==(const QNode<T>& rhs)
	{
		return rhs.x == x && rhs.y == y;
	}
};

/* EqualTo pred function for QNode class */
template<typename T> struct EqualTo : public std::unary_function<std::shared_ptr<T>, bool>
{
	inline bool operator()(const std::shared_ptr<T>& node1, const std::shared_ptr<T>& node2) const
	{
		return *node1 == *node2;
	}
};

/* Hash funct for QNode class */
template<typename T> struct NodeHashFunc : public std::unary_function<std::shared_ptr<T>, std::size_t>
{
	inline std::size_t operator()(const shared_ptr<T>& node) const
	{
		std::size_t seed = 0;
		hash_combine(seed, node->x);
		hash_combine(seed, node->y);
		return seed;
	}

};

/* Shift-Add-XOR for hashing (using TEA) */
template <typename T>
inline void hash_combine(std::size_t & seed, const T & v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
