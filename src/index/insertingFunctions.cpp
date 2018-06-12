#include <index/indexManager.h>
#include <index/bplusTree.h>
#include<stack>

namespace minisql
{

namespace __index
{

void IndexManager::cutNode(Node data, int index_id)
{
	Node now = data.son(), tail = now;
	now.count(M >> 1);
	for (int i = 0; i < M / 2; ++i)
		tail = tail.next();
	if (now.son() == nullptr)
	{
		tail.next(createNode(index_id, {
			NullType(),
			(M + 1) >> 1,
			NullType(),
			tail.next()
		}));
		data.next(createNode(index_id, {
			tail.next().next().data(),
			-1,
			tail.next(),
			data.next()
		}));
	}
	else
	{
		data.next(createNode(index_id, {
			tail.next().data(),
			-1,
			tail.next(),
			data.next()
		}));
		tail.next().count((M - 1) >> 1);
		// tail.next().data() = 0;
	}
}

void IndexManager::insertDataToCreateIndex(Item x, int index_id)
{
	std::stack<Node>Stack;
	BPlusTree bplusTree = idToTree[index_id];
	Node root{ bplusTree.root };
	Stack.push(root);
	Node now, data;
	//const ItemValue &value=x[bplusTree.attrno];

	while (true) 
	{
		now = Stack.top();
		if (now == nullptr)
			break;
		data = now;
		for (int i = 0; i < now.count(); ++i) 
		{
			//if (value < xxxxxxxxxxxx)
			if (x[bplusTree.attrno] < data.next().data()[bplusTree.attrno])
				break;
			data = data.next();
		}
		Stack.push(data);
		Stack.push(data.son());
	}
	Stack.pop();

	now = Stack.top();

	now.next(createNode(index_id, {
		x,//just a pointer to real data
		-1,
		NullType(),
		now.next()
	}));
	Stack.pop();
	now = Stack.top();
	now.count(now.count() + 1);
	Stack.pop();

	while (!Stack.empty()) 
	{
		data = Stack.top();
		Stack.pop();
		now = Stack.top();
		Stack.pop();
		if (data.son().count() == M) 
		{
			cutNode(data,index_id);
			now.count(now.count());
		}
	}
	if (root.count() == M) 
	{
		root = createNode(index_id, {
			NullType(),
			1,
			root,
			NullType()
		});
		cutNode(root,index_id);
		BufferManager::registerRoot(root);
	}
}
bool IndexManager::recursivelyInsert(
	int &err,
	const ItemValue &values,
	const vector<int> &queue,
	vector<int>::const_iterator &it)
{
	int index_id = *it;
	std::stack<Node> Stack;
	BPlusTree bplusTree = idToTree[index_id];
	Node root{ bplusTree.root };
	Stack.push(root);
	Node now, data;
	//const ItemValue &value=x[bplusTree.attrno];

	while (true) {
		now = Stack.top();
		if (now == nullptr)
			break;
		data = now;
		for (int i = 0; i < now.count(); ++i) {
			//if (value < xxxxxxxxxxxx)
			auto next = data.next();
			if (values[bplusTree.attrno] < next.data()[bplusTree.attrno].val())
				break;
			data = next;
		}
		Stack.push(data);
		Stack.push(data.son());
	}
	Stack.pop();

	now = Stack.top();
	if (now.haveData() && now.data()[bplusTree.attrno].val() == values[bplusTree.attrno]) {
		//roll back******any thing else?
		err = bplusTree.attrno;
		return false;
	}

	static Item x;
	if (++it == queue.end())
		x = BufferManager::insertItem(treesToTables[index_id], values);
	else
		if (!recursivelyInsert(err, values, queue, it))//recursively insert
			return 0;

	now.next(createNode(index_id, {
		x,//just a pointer to real data
		-1,
		NullType(),
		now.next()
	}));
	BufferManager::addRef(x);	// this node has a reference to x.
	Stack.pop();
	now = Stack.top();
	now.count(now.count() + 1);
	Stack.pop();

	while (!Stack.empty()) {
		data = Stack.top();
		Stack.pop();
		now = Stack.top();
		Stack.pop();
		if (data.son().count() == M) {
			cutNode(data, index_id);
			now.count(now.count());
		}
	}
	if (root.count() == M) {
		root = createNode(index_id, {
			NullType(),
			1,
			root,
			NullType()
		});
		cutNode(root, index_id);

		BufferManager::registerRoot(root);//************************!!!!!!!!FBI WARNING!!!!!!
	}
	return true;
}

bool IndexManager::insertData(int table_id, const ItemValue &values, int &err)
{
	vector<int> &targetTrees = tablesToTrees[table_id];
	vector<int>::const_iterator it = targetTrees.begin();
	return recursivelyInsert( err, values, targetTrees, it);
}

}

}