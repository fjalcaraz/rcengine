/**
 * @file BTree.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the BTree module. Definitions of the BTree, BTNode and BTKeyManager
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef BTREE_HH_INCLUDED
#define BTREE_HH_INCLUDED

#include <iostream>
#include <stdarg.h>

using namespace std;

typedef int (*BTCompareFunc)(const void *item_of_tree, const void *item_passed, va_list list);
typedef char *(*BTPrintFunc)(void *item_of_tree);
typedef char *(*BTPrinterFunc)(BTPrintFunc func, void *item_of_tree);
typedef void (*BTSimpleFunc)(void *item_of_tree, va_list list);
typedef void (*BTSimpleConstFunc)(const void *item_of_tree, va_list list);

// The following line is commented off to omit debugging output.
//#define DEBUG
// Remove the // at the beginning of that line and recompile if you want
// to get extensive debugging output.

// max number of keys in a node
// MaxKeys = MinKeys*2 + 1
#define MaxKeys 11
#define MaxKeysPlusOne (MaxKeys + 1)
#define MinKeys 5

void Error(char *msg);

class BTNode;

class BTKeyManager
{
private:
    int NKeys;

public:
    BTKeyManager(int nkeys) { NKeys = nkeys; };
    virtual int compareKey(const void *target, const void *internal, int numkey) = 0;
    int numKeys() { return NKeys; };
};

struct BTState
{
    int level;
    int n_items;
    struct
    {
        const BTNode *node;
        int loc;
    } stack[10]; // 10 = vertical deep : 11^10 =  25.937.424.601 of items

    inline BTState(const BTNode *root = NULL, int num_items = 0)
    {
        level = -1;
        n_items = num_items;
        if (root)
            Push(root);
    };
    inline void Push(const BTNode *node, int location = 0)
    {
        level++;
        stack[level].node = node;
        stack[level].loc = location;
    };
    inline void Pop() { level--; };
    inline int nitems() { return n_items; };
};

class BTNode
{
private:
    int Count;                      // Number of keys stored in the current node
    void *Key[MaxKeys];             // Warning: indexing starts at 0, not 1
    BTNode *Branch[MaxKeysPlusOne]; // Fake pointers to child nodes

    bool SearchNode(const void *Target, BTCompareFunc Func, va_list list, int &location) const;
    bool SearchNode(const void *Target, BTKeyManager *KeysMgr, int numkey, int &location) const;
    void AddItemToLeft(void *NewItem, BTNode *NewRight, int Location);
    void AddItemToRight(void *NewItem, BTNode *NewRight, int Location);
    void DeleteItemToLeft(int location);
    void DeleteItemToRight(int location);
    void Split(void *&CurrItem, BTNode *&CurrRight, int Location);

public:
    BTNode() { Count = 0; };
    BTNode(void *Item, BTNode *left, BTNode *right);

    bool PushDown(void *CurrentItem, void *&NewItem, BTNode *&NewRight, void **&BTItemPos, int *Found, BTCompareFunc Func, va_list list);
    bool PushDown(void *CurrentItem, void *&NewItem, BTNode *&NewRight, void **&BTItemPos, int *Found, BTKeyManager *KeysMgr, int numkey);
    const void *Find(const void *Search, BTCompareFunc Func, va_list list) const;
    const void *Find(const void *Search, BTKeyManager *KeysMgr, int numkey) const;
    void *const *FindDir(const void *Search, BTCompareFunc Func, va_list list) const;
    void *const *FindDir(const void *Search, BTKeyManager *KeysMgr, int numkey) const;
    bool Delete(void *Search, void **Found, BTCompareFunc Func, va_list List, BTKeyManager *KeysMgr, int numkey = 0, bool nextItemSearch = false);
    static const void *Walk(BTState &state);
    void SearchNodeBiggerThan(const void *Target, BTCompareFunc Func, va_list List, BTState &state) const;
    void SearchNodeBiggerThan(const void *Target, BTKeyManager *KeysMgr, int numkey, BTState &state) const;
    void Dump(BTPrintFunc Func, int nk, int ofsset = 0) const;
    void DumpKeys(BTPrinterFunc Func, BTPrintFunc Func2, int nk) const;
    bool Empty(void) const { return Count == 0; };
    void *const *keyPointer(int n) const { return &Key[n]; };
};

class BTree
{
private:
    static int Found;
    BTNode *Root; // fake pointer to the root node
    int NumItems;
    BTKeyManager *KeysMgr;

public:
    inline BTree()
    {
        NumItems = 0;
        Root = NULL;
        KeysMgr = NULL;
    };
    inline BTree(BTKeyManager *manager)
    {
        NumItems = 0;
        Root = NULL;
        KeysMgr = manager;
    };
    inline void setKeyManager(BTKeyManager *manager) { KeysMgr = manager; };
    inline void *getElement() const
    {
        if (!Root)
            return NULL;
        else
            return *Root->keyPointer(0);
    };

    inline int numKeys() const
    {
        if (!KeysMgr)
            return 0;
        else
            return KeysMgr->numKeys();
    };
    inline int numItems() const { return NumItems; };
    static int WasFound() { return Found; };
    void **Insert(void *Item, BTCompareFunc Func, ...);
    void **InsertList(void *Item, BTCompareFunc Func, va_list List);
    void *Delete(void *Search, BTCompareFunc Func, ...);
    void *DeleteList(void *Search, BTCompareFunc Func, va_list List);
    const void *Find(const void *Search, BTCompareFunc Func, ...) const;
    const void *FindList(const void *Search, BTCompareFunc Func, va_list List) const;
    void *const *FindDir(const void *Search, BTCompareFunc Func, ...) const;
    void *const *FindDirList(const void *Search, BTCompareFunc Func, va_list List) const;
    BTState FindByKeys(const void *Item) const;
    BTState getIterator() const
    {
        BTState iterator(Root, NumItems);
        return iterator;
    }
    static const void *Walk(BTState &state);
    void WalkBy(BTSimpleConstFunc func, ...) const;
    BTState FindBiggerThan(const void *item, BTCompareFunc Func, ...);

    bool Empty(void) const { return (Root == NULL); };
    void Dump(BTPrintFunc Func = NULL)
    {
        if (Root)
            Root->Dump(Func, numKeys());
        else
            cout << "Empty\n";
    };                // for debugging only - could be removed
    void Check(void); // for debugging only
    static int compareEq(const void *node1, const void *node2, va_list);
    static int compareStr(const void *node1, const void *node2, va_list);
    static int EQUALITY_FUNC(const void *, const void *, va_list);
    void Free(BTSimpleFunc func, ...);
    void FreeList(BTSimpleFunc func, va_list list);
};

#endif
