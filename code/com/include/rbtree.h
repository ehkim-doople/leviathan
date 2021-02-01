#ifndef	__JINRBTREE_H__
#define	__JINRBTREE_H__

typedef unsigned int HASH_CODE;
enum color { RED, BLACK };

struct rb_value
{
	void *value;	// Data
	void *next;		// 중복된 key 값을 갖는 next data
};


typedef struct rbtree_node
{
    enum color color;
    void *key;
    void *value;
    rbtree_node *left, *right, *parent;
}*node;

 
typedef struct rbtree_t
{
    node root;
}*rbtree;
 
/*
 * Class RBTree Declaration
 */
class RBTree
{
    public:
        typedef int (*compare_func)(void* left, void* right);
        rbtree rbtree_create();
        void* rbtree_lookup(rbtree t, void* , compare_func compare);
		node new_node(void* key, void* , node newNode, color=RED,  node left=NULL, node right=NULL);
        void rbtree_insert(rbtree t, node inserted_node , compare_func compare);
        void rbtree_delete(rbtree t, void* , compare_func compare);
        node grandparent(node n);
        node sibling(node n);
        node uncle(node n);

private:
        void verify_properties(rbtree t);
        void verify_property_1(node root);
        void verify_property_2(node root);
        color node_color(node n);
        void verify_property_4(node root);
        void verify_property_5(node root);
        void verify_property_5_helper(node n, int , int*);        
        node lookup_node(rbtree t, void* , compare_func compare);
        void rotate_left(rbtree t, node n);
        void rotate_right(rbtree t, node n);
        void replace_node(rbtree t, node oldn, node newn);
        void insert_case1(rbtree t, node n);
        void insert_case2(rbtree t, node n);
        void insert_case3(rbtree t, node n);
        void insert_case4(rbtree t, node n);
        void insert_case5(rbtree t, node n);
        node maximum_node(node root);
        void delete_case1(rbtree t, node n);
        void delete_case2(rbtree t, node n);
        void delete_case3(rbtree t, node n);
        void delete_case4(rbtree t, node n);
        void delete_case5(rbtree t, node n);
        void delete_case6(rbtree t, node n);
};

static int compare_int(void* leftp, void* rightp);


#endif	//__JINRBTREE_H__