/**
 * @file btree.cpp
 * @author Francisco Alcaraz
 * @brief Management of auto-balanced B Trees, allowing search, insertion, deletion and walk-over  
 * @version 1.0
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <string.h>
#include <stdlib.h>

#include "btree.hpp"

int BTree::Found;

/**
 * @brief Insert an item in the BTree
 * 
 * @param Item the item to be inserted
 * @param Func Compare function (must return 1, 0 or -1)
 * @param ... Additional static params to be passed to the compare function
 * @return void** The pointer to where the object was inserted in the BTree (If may be replaced)
 */
void **BTree::Insert(void *Item, BTCompareFunc Func, ...)
{
   va_list List;
   void ** ItemDir;

   va_start (List, Func);
   ItemDir = InsertList(Item, Func, List);
   va_end(List);

   return ItemDir;
}

/**
 * @brief Insert an item in the BTree (version va_list)
 * 
 * @param Item the item to be inserted
 * @param Func Compare function (must return -1, 0 or 1)
 * @param List va_list with the additional static params to be passed to the compare function
 * @return void** The pointer to where the object was inserted in the BTree (If may be replaced)
 */

void **BTree::InsertList(void *Item, BTCompareFunc Func, va_list List)
{
   #ifdef DEBUG
      cout << "I";
   #endif

   bool MoveUp;
   BTNode *NewRight;
   void *NewItem;
   void **BTItemPos;
   BTNode **RootNodePos;
   int numkeys = numKeys();

   RootNodePos = &Root;
   for (int nk=0; nk<numkeys; nk++)
   {
      Found = false;
      BTItemPos = NULL;
      MoveUp = (*RootNodePos)->PushDown(Item, NewItem, NewRight, BTItemPos, &Found, KeysMgr, nk);

      if (MoveUp)   // create a new root node
      {
         *RootNodePos = new BTNode(NewItem, *RootNodePos, NewRight);
         if (Item == NewItem)
            BTItemPos = (void **)(*RootNodePos)->keyPointer(0);
      }

      if (!Found) *BTItemPos = NULL; // Create a null BTNode (empty tree)
      RootNodePos = (BTNode **)BTItemPos;
   }

   Found = false;
   BTItemPos = NULL;
   MoveUp = (*RootNodePos)->PushDown(Item, NewItem, NewRight, BTItemPos, &Found, Func, List);

   if (MoveUp)   // create a new root node
   {
      *RootNodePos = new BTNode(NewItem, *RootNodePos, NewRight);
      if (Item == NewItem)
         BTItemPos = (void **)(*RootNodePos)->keyPointer(0);
   }

   if (!Found) NumItems++;

   return BTItemPos;
}

/**
 * @brief Remove an Item from a BTree
 * 
 * @param Item the item to be removed
 * @param Func Compare Function. Must return -1, 0 or 1
 * @param ... Additional static params to be passed to the compare function
 * @return void* The object that was found in the BTree.
 */
void *BTree::Delete(void *Item, BTCompareFunc Func, ...)
{
   va_list List;
   void *ItemFound;

   va_start (List, Func);
   ItemFound = DeleteList(Item, Func, List);
   va_end(List);

   return ItemFound;
}

/**
 * @brief Remove an Item from a BTree (va_list version)
 * 
 * @param Item the item to be removed
 * @param Func Compare Function. Must return -1, 0 or 1
 * @param List va_list with the additional static params to be passed to the compare function
 * @return void* The object that was found in the BTree.
 */
void *BTree::DeleteList(void *Item, BTCompareFunc Func, va_list List)
{
   #ifdef DEBUG
     cout << "R";
   #endif

   void *ItemFound = NULL;

   if (Root== NULL) return NULL;
   Root->Delete(Item, &ItemFound, Func, List, KeysMgr);

   if (Root->Empty())
   {
      delete Root;
      Root = NULL;
   }
   if (ItemFound!= NULL) NumItems--;
   return ItemFound;
}

/**
 * @brief Find an Item in a BTree
 * 
 * @param Item the item to be found
 * @param Func Compare Function. Must return -1, 0 or 1
 * @param ... Additional static params to be passed to the compare function
 * @return void* The object that was found in the BTree.
 */
const void *BTree::Find(const void *Item, BTCompareFunc Func, ...) const
{
   va_list List;
   const void *ItemFound;

   va_start (List, Func);
   ItemFound = FindList(Item, Func, List);
   va_end(List);

   return ItemFound;
}

/**
 * @brief Find an Item from a BTree (va_list version)
 * 
 * @param Item the item to be found
 * @param Func Compare Function. Must return -1, 0 or 1
 * @param List va_list with the additional static params to be passed to the compare function
 * @return void* The object that was found in the BTree.
 */
const void *BTree::FindList(const void *Item, BTCompareFunc Func, va_list List) const
{
   #ifdef DEBUG
     cout << "R";
   #endif

   BTNode *RootNode;
   int numkeys = numKeys();

   RootNode = Root;
   for (int nk=0; RootNode && nk<numkeys; nk++)
      RootNode = (BTNode *)RootNode->Find(Item, KeysMgr, nk);

   if (RootNode == NULL) return NULL;
   return RootNode->Find(Item, Func, List);
}

/**
 * @brief Find the address where an item is stored in a BTree
 * 
 * @param Item the item to be found
 * @param Func Compare Function. Must return -1, 0 or 1
 * @param ... Additional static params to be passed to the compare function
 * @return void* The object that was found in the BTree.
 */
void * const *BTree::FindDir(const void *Item, BTCompareFunc Func, ...) const
{
   va_list List;
   void * const *ItemDir;

   va_start (List, Func);
   ItemDir = FindDirList(Item, Func, List);
   va_end(List);

   return ItemDir;
}

/**
 * @brief Find the address where an item is stored in a BTree (va_list version)
 * 
 * @param Item the item to be found
 * @param Func Compare Function. Must return -1, 0 or 1
 * @param List va_list with the additional static params to be passed to the compare function
 * @return void* The object that was found in the BTree.
 */
void * const *BTree::FindDirList(const void *Item, BTCompareFunc Func, va_list List) const
{

   #ifdef DEBUG
     cout << "R";
   #endif

   BTNode *RootNode;
   int numkeys = numKeys();

   RootNode = Root;
   for (int nk=0; RootNode && nk<numkeys; nk++)
      RootNode = (BTNode *)RootNode->Find(Item, KeysMgr, nk);

   if (RootNode == NULL) return NULL;
   return RootNode->FindDir(Item, Func, List);
}

/**
 * @brief Find an Item in a BTree returning a BTState 
 * 
 * @param Item the item to be found
 * @return BTState (advanced information about where the item is)
 */
BTState BTree::FindByKeys(const void *Item) const
{
   #ifdef DEBUG
     cout << "R";
   #endif

   BTState result;
   BTNode *RootNode;
   int numkeys = numKeys();

   RootNode = Root;
   for (int nk=0; RootNode && nk<numkeys; nk++)
      RootNode = (BTNode *)RootNode->Find(Item, KeysMgr, nk);

   if (RootNode) result.Push(RootNode);
   return result;
}

/**
 * @brief Walk through the BTree maintaining the walk state. It will returns the next object at the next call in a iteractive way
 * 
 * @param state BTState where the walk is going on to make it iterative through the subsequent calls
 * @return const void* Item found
 */
// Static
const void *BTree::Walk(BTState &state)
{
   return BTNode::Walk(state);
}

/**
 * @brief Walk through a BTree calling to a func with every object
 * 
 * @param Func Function to be call with every item (first parameter to be passed)
 * @param ... List of arguments to be passed to the function after the item
 */
// No static
void BTree::WalkBy(BTSimpleConstFunc Func, ...) const
{
   va_list List;
   const void *item;
   BTState iterator = getIterator();

   va_start (List, Func);
   while (item = Walk(iterator))
      (*Func)(item, List);
   va_end(List);
}

/**
 * @brief Return the state positioned on the first item in the tree that is bigger that the passed
 * For this state the BTree may be iterated
 * 
 * @param Item The item to find
 * @param Func the compare function. Must return -1, 0 or 1
 * @param ... List of additional params to be passed to the function
 * @return BTState 
 */
BTState BTree::FindBiggerThan(const void *Item, BTCompareFunc Func, ...)
{
   BTState result;
   BTNode *RootNode;
   int numkeys = numKeys();

   va_list List;
   va_start (List, Func);

   RootNode = Root;
   for (int nk=0; RootNode && nk<numkeys; nk++)
      RootNode = (BTNode *)RootNode->Find(Item, KeysMgr, nk);

   if (RootNode) RootNode->SearchNodeBiggerThan(Item, Func, List, result);
   
   va_end(List);
   return result;
}

/**
 * @brief Basic compare function based on its addresses
 * 
 * @param node1 
 * @param node2 
 * @return int (-1: node1 < node2, 0: node1 == node 2, 1 node1 > node 2)
 */
int
BTree::compareEq (const void *node1, const void *node2, va_list)
{
   return (long int) node1 - (long int) node2;
}

/**
 * @brief Basic compare function to be used when storing strings
 * 
 * @param node1 
 * @param node2 
 * @return int (-1: node1 < node2, 0: node1 == node 2, 1 node1 > node 2)
 */
int
BTree::compareStr (const void *node1, const void *node2, va_list)
{
   return strcmp((char*) (node1), (char*) (node2));
}

/**
 * @brief Basic compare function to be used to get the first node.
 * 
 * @param node1 
 * @param node2 
 * @return int : always 0 (what means equality)
 */
int
BTree::EQUALITY_FUNC (const void * /*node1 */ , const void *node2, va_list)
{
      return (0);
}

/**
 * @brief free the full BTree, calling to a function with every item
 * 
 * @param func Function to be called with every object in the BTree
 * @param ... Additional params to be passed to teh function
 */
void
BTree::Free (BTSimpleFunc func, ...)
{

   va_list list;

   va_start (list, func);
   FreeList (func, list);
   va_end(list);
}

/**
 * @brief free the full BTree, calling to a function with every item
 * 
 * @param func Function to be called with every object in the BTree
 * @param list List of additional params to be passed to teh function
 */
void
BTree::FreeList (BTSimpleFunc func, va_list list)
{
   void *found;
   while ((Root) != NULL)
   {
      found = DeleteList(NULL, EQUALITY_FUNC, list);
      (*func)(found, list);
   }
}

/**
 * BTNode class. Implement the nodes of the Btree
 * Each node contains N keys and N+1 Branches to other nodes.
 * - Each Key store (points to) a single object
 * - Each Branch points to another BTnode (subtree)
 * - The branch[i+1] is the subtree that contains the nodes with the items between those stored in key[i] and key[i+1].
 * - The branch[0] is the subtree with those nodes lower than whose is stored at key[0]
 * 
 * It covers the case of multikeys. That means that the top level tree stores trees, that stores trees, ...,  that store final items. 
 * Each level orders the items by the value of a different attribute of the item
 * e.g. if ordered by attributes A and B, the first tree will strore trees, each of them with the items 
 *      having all the same value of A, and ordered by the values of B 
 * 
 */

/**
 * @brief Construct a new BTNode::BTNode object. Create a new Node with just one key stored
 * 
 * @param item to be stored in that node at key[0]
 * @param left the node to be stored at Branch[0]
 * @param right the node to be stored al Branch[1] 
 */
BTNode::BTNode(void * item, BTNode *left, BTNode*right)
{
   Count = 1;
   Key[0] = item;
   Branch[0] = left;
   Branch[1] = right;
}

/**
 * @brief Recursive dump of the Btree. Useful while debuging
 * 
 * @param Func Function to print the content. When null the items are considered strings
 * @param nk Number of existing keys at that level. nk > 0 means that a BTNode (root of a subtree) is stored instead of a final item
 * @param offset Number of tabs to be printed before value. It will increase while deeping into the BTree
 */
void BTNode::Dump(BTPrintFunc Func, int nk, int offset) const
{ 
   if (Count > 0)
   {
      if (Branch[0]) Branch[0]->Dump(Func, nk, offset+1);
      for (int p = 0; p < Count; p++)
      {
         if (nk>0)
         {
            BTNode *node = (BTNode *)Key[p];
            node->Dump(Func, nk-1, offset);
         }
         else {
            char *text;
            if (Func)
               text = (*Func)(Key[p]);
            else
               text = (char *)(Key[p]);

            for (int k = 0; k < offset; k++) cout << "\t";
            cout << text << endl;
         }
         if (Branch[p+1]) Branch[p+1]->Dump(Func, nk, offset+1);
      }
   }
}

/**
 * @brief Recursive dump of the Btree using functions to print the hierarchy and the items. Useful while debuging
 * 
 * @param Func Function to print the hierarchy f(BTPrintFunc func, void *item_of_tree).
 * @param Func2 Function to print the items. f(void *item_of_tree)
 * @param nk Number of existing keys at that level. nk > 0 means that a BTNode (root of a subtree) is stored instead of a final item
 */
void BTNode::DumpKeys(BTPrinterFunc Func, BTPrintFunc Func2, int nk) const
{
   if (Count > 0)
   {
      if (Branch[0]) Branch[0]->DumpKeys(Func, Func2, nk);
		for (int p = 0; p < Count; p++)
      {
         if (nk>1)
         {
            BTNode *node = (BTNode *)Key[p];
            node->DumpKeys(Func, Func2, nk-1);
         }
         else (*Func)(Func2, Key[p]);

         if (Branch[p+1]) Branch[p+1]->DumpKeys(Func, Func2, nk);
      }
		
   }
}



/**
 * @brief Search for an item in a node. A binary search is performed comparing the target with the key in the middle 
 *    of the search interval over the keys, moving the interval margins according to the comparison result
 * 
 * @param Target the item serached for
 * @param Func Comapare function. Must return -1, 0, or 1
 * @param List Variable list of additional arguments passed to Func
 * @param Location the offset tested. It will store the final position when found
 * @return true when found, false elsewhere and the search must continue in the branch between last two (consecutive) keys
 */
bool BTNode::SearchNode(const void * Target, BTCompareFunc Func, va_list List, int & Location) const
{
   bool Found;
   int i,j;
   int result;

   Found = false;
   i=-1; j=Count; result = 1; Location=-1;
   while (result != 0 && (j-i)>1)
   {
       Location = i + (j-i)/2;
       result = (* Func)(Target, Key[Location], List);
       if (result>0)
           i=Location;
       else if (result<0)
           j=Location--;
   }

   Found = (result == 0);

   return Found;
}

/**
 * @brief Search for an item in a node using a keyManager. The peer to compare to is taken going down the lower level BTrees 
 *    until reaching the last one. Then the item at key[0] is chosen
 *    A binary search is performed comparing the target with the key in the middle 
 *    of the search interval over the keys, moving the interval margins according to the comparison result
 * 
 * @param Target the item serached for
 * @param KeysMgr contains the number of keys and the Comapare function
 * @param numkey current key number (form 0: top level to KeysMgr->numKeys(): lower level)
 * @param Location the offset tested. It will store the final position when found
 * @return true when found, false elsewhere and the search must continue in the branch between last two (consecutive) keys
 */
bool BTNode::SearchNode(const void * Target, BTKeyManager *KeysMgr, int numkey, int & Location) const
{
   bool Found;
   int i,j;
   int result;

   Found = false;
   i=-1; j=Count; result = 1; Location=-1;
   while (result != 0 && (j-i)>1)
   {
       void *Internal;

       Location = i + (j-i)/2;

       Internal = Key[Location];
       for (int nk=numkey; nk < KeysMgr->numKeys(); nk++)
           Internal = ((BTNode *)Internal)->Key[0];

       result = KeysMgr->compareKey(Target, Internal, numkey);
       if (result>0)
           i=Location;
       else if (result<0)
           j=Location--;
   }

   Found = (result == 0);

   return Found;
}

/**
 * @brief Make space in the current Keys and Branch arrays at Location offset and then set there a new key value and the branch to its right
 *    if Location is -1 it means the last key
 * 
 * @param NewItem Item to be set al position Location
 * @param NewRight Branch to be set al Location +1 position (Its right)
 * @param Location the position
 */
void BTNode::AddItemToRight(void *NewItem, BTNode *NewRight, int Location)
{
   int j;

   if (Location == -1) Location = Count;

   for (j = Count; j > Location; j--)
   {
      Key[j] = Key[j - 1];
      Branch[j + 1] = Branch[j];
   }

   Key[Location] = NewItem;
   Branch[Location + 1] = NewRight;
   Count++;
}

/**
 * @brief Make space in the current Keys and Branch arrays at Location offset and then set there a new key value and the branch to its left
 * 
 * @param NewItem Item to be set al position Location
 * @param NewLeft Branch to be set al Location position (Its left)
 * @param Location the position
 */
void BTNode::AddItemToLeft(void *NewItem, BTNode *NewLeft, int Location)
{
   int j;

   for (j = Count; j > Location; j--)
   {
      Key[j] = Key[j - 1];
      Branch[j + 1] = Branch[j];
   }
   Branch[Location + 1] = Branch[Location];

   Key[Location] = NewItem;
   Branch[Location] = NewLeft;
   Count++;
}

/**
 * @brief Delete the key of the Key array at the Location position and the branch to its right inside Branch array
 *    if location is -1, it means at the end
 * 
 * @param Location 
 */
void BTNode::DeleteItemToRight(int Location)
{
   int j;

   if (Location == -1) Location = Count - 1;

   for (j=Location; j<(Count-1); j++)
   {
      Key[j] = Key[j + 1];
      Branch[j+1] = Branch[j+2];
   }
   //Key[j] = Branch[j+1] = NULL;
   Count--;
}

/**
 * @brief Delete the key of the Key array at the Location position and the branch to its left inside Branch array
 * 
 * @param Location 
 */
void BTNode::DeleteItemToLeft(int Location)
{
   int j;

   for (j=Location; j<(Count-1); j++)
   {
      Key[j] = Key[j + 1];
      Branch[j] = Branch[j+1];
   }
   Branch[j] = Branch[j+1];
   //Key[j] = Branch[j+1] = NULL;
   Count--;
}

/**
 * @brief Divide the node in two. 
 * 
 * @param CurrItem is the node to be moved lo location Location+1. 
 *    It will go to current node or to the new node depending on whether Location is lower that MinKeys or higher
 *    this node will remain with MinKeys + 1 keys, the rest are copied to the new node
 *    CurItem at the function return is the element to be put as key at the higher level node (the element between this an the new created) 
 * @param CurrRight the branch at the right of CurrItem
 * @param Location where to store CurrItem
 */
void BTNode::Split(void *&CurrItem, BTNode *&CurrRight, int Location)
{
   int j, Median;
   BTNode *NewRight;

   #ifdef DEBUG
      cout << "S";
   #endif

   if (Location < MinKeys)
      Median = MinKeys;     // The first node will remain always with MinKeys + 1, due in the first case the CurItem is finally inserted in current node!!
   else
      Median = MinKeys + 1;

   NewRight = new BTNode;

   for (j = Median; j < MaxKeys; j++)
   {  // move half of the items to the NewRight
      NewRight->Key[j - Median] = Key[j];
      NewRight->Branch[j - Median + 1] = Branch[j + 1];
   }

   NewRight->Count = MaxKeys - Median;
   Count = Median;   // is then incremented by AddItemToRight

   // put CurrentItem in place
   if (Location < MinKeys)
      this->AddItemToRight(CurrItem, CurrRight, Location + 1);
   else
      NewRight->AddItemToRight(CurrItem, CurrRight, Location - Median + 1);

   NewRight->Branch[0] = Branch[Count];

   CurrRight = NewRight;
   CurrItem = Key[Count - 1];
   Count--;
}

/**
 * @brief Add a new object to the tree rebalancing the nodes maintaining the number of keys between MaxKeys and MinKeys
 * 
 * @param CurrentItem The item to add
 * @param NewItem The item that is being moved 
 * @param NewRight The new node at the right branch of NewItem in the superior node
 * @param BTItemPos Pointer to where the currentItem is stored
 * @param Found where to store if the object (CurrentItem) was found or not
 * @param Func Compare function. Must return -1, 0, or 1
 * @param List va_list of additional parameters to be passed to the compare function
 * @return true when NewItem/NewRight must me moved to the superior node
 * @return false when that is not needed
 */
bool  BTNode::PushDown(void *CurrentItem, void *&NewItem, BTNode *&NewRight, void **&BTItemPos, int *Found,
                       BTCompareFunc Func, va_list List)
{
   int Location;
   bool MoveUp, isTheObject;

   #ifdef DEBUG
      cout << "P ";
   #endif

   if (this == NULL)   // stopping case
   {  // cannot insert into empty tree
      MoveUp = true;
      NewItem = CurrentItem;
      NewRight = NULL;
   }
   else  // recursive case
   {
      if (SearchNode(CurrentItem, Func, List, Location))
      {
         // Communicate the element found and return with false
         // to end the process
         BTItemPos = &Key[Location];
         *Found = true;
         return false;
      }

      MoveUp = Branch[Location + 1]->PushDown(CurrentItem, NewItem, NewRight, BTItemPos, Found, Func, List);

      isTheObject = (NewItem == CurrentItem);

      if (MoveUp)
      {
         if (Count < MaxKeys)
         {
            MoveUp = false;
            AddItemToRight(NewItem, NewRight, Location + 1);
            if (isTheObject) BTItemPos = &Key[Location+1];
         }
         else
         {
            MoveUp = true;
            Split(NewItem, NewRight, Location);

            if (isTheObject)
            {
               if (Location < MinKeys) BTItemPos = &Key[Location+1];
               else BTItemPos = &(NewRight->Key[Location-MinKeys]);
            }

         }
      }
   }
   return MoveUp;
}
/**
 * @brief Add a new object to the tree rebalancing the nodes maintaining the number of keys between MaxKeys and MinKeys. Multikey version
 * 
 * @param CurrentItem The item to add
 * @param NewItem The item that is being moved 
 * @param NewRight The new node at the right branch of NewItem in the superior node
 * @param BTItemPos Pointer to where the currentItem is stored
 * @param Found where to store if the object (CurrentItem) was found or not
 * @param KeysMgr Keys Manager that includes a compare function and the number of keys
 * @param numkey current numkey
 * @return true when NewItem/NewRight must me moved to the superior node
 * @return false when that is not needed
 */

bool  BTNode::PushDown(void *CurrentItem, void *&NewItem, BTNode *&NewRight, void **&BTItemPos, int *Found,
                       BTKeyManager *KeysMgr, int numkey)
{
   int Location;
   bool MoveUp, isTheObject;

   #ifdef DEBUG
      cout << "P ";
   #endif

   if (this == NULL)   // stopping case
   {  // cannot insert into empty tree
      MoveUp = true;
      NewItem = CurrentItem;
      NewRight = NULL;
   }
   else  // recursive case
   {
      if (SearchNode(CurrentItem, KeysMgr, numkey, Location))
      {
         // Communicate the element found and return with false
         // to end the process
         BTItemPos = &Key[Location];
         *Found = true;
         return false;
      }

      MoveUp = Branch[Location + 1]->PushDown(CurrentItem, NewItem, NewRight, BTItemPos, Found, KeysMgr, numkey);

      isTheObject = (NewItem == CurrentItem);

      if (MoveUp)
      {
         if (Count < MaxKeys)
         {
            MoveUp = false;
            AddItemToRight(NewItem, NewRight, Location + 1);
            if (isTheObject) BTItemPos = &Key[Location+1];
         }
         else
         {
            MoveUp = true;
            Split(NewItem, NewRight, Location);

            if (isTheObject)
            {
               if (Location < MinKeys) BTItemPos = &Key[Location+1];
               else BTItemPos = &(NewRight->Key[Location-MinKeys]);
            }

         }
      }
   }
   return MoveUp;
}

/**
 * @brief The delete algorithm to maintain the tree balanced is as follows:
 *    - first a search is done to locate the item
 *    - starting by the branch to its right and going deeper in the tree by branches[0] the next object in order is found
 *    - this next element is removed from where it is and will replace the position where the item found is
 *    - With this procedure only leave nodes are being reduced their keys
 *    - if The leave node gets less keys than minKeys, this node can increase their keys taking the next key 
 *      of the parent node that will be replaced by the first item of the brach to its right
 *    - sometimes we will have to get from the left side, so we will put at position 0 the previous key of the parent  
 *      and that will be substituted by the last item of the previous branch  
 *    - This operations are known as right/left rotations
 * 
 * @param Item The item to remove from the tree
 * @param ItemFound The pointer to object found that is considered equal to the item passed (but may be physically different object)
 * @param Func Function to compare items. Must return -1, 0 or 1
 * @param List variable arguments that will be passed to the compare function
 * @param KeysMgr Key manager of the tree. May be null if that tree is simple
 * @param numkey number of keys (multikeys -> tres of tress)
 * @param nextItemSearch If is being searched the next object in the tree
 * @return true Id the node has less keys than minKeys and require rotations or joins
 * @return false otherwise
 */
bool  BTNode::Delete(void *Item, void **ItemFound,
                     BTCompareFunc Func, va_list List,
                     BTKeyManager *KeysMgr, int numkey, 
                     bool nextItemSearch)
{
   int Location;
   int Found;
   int reorder;
   int numkeys;

   numkeys = ((!KeysMgr) ? 0 : KeysMgr->numKeys());

   if (nextItemSearch)
   {
      Found = (Branch[0] == NULL);
      Location = (Found ? 0 : -1);
   }
   else
   {
      if (numkey<numkeys)
         Found = SearchNode(Item, KeysMgr, numkey, Location);
      else
         Found = SearchNode(Item, Func, List, Location);
   }

   if (!nextItemSearch && Found && numkey < numkeys)
   {
      BTNode *root = (BTNode *)Key[Location];
      root->Delete(Item, ItemFound, Func, List, KeysMgr, numkey+1, nextItemSearch);
      if (root->Empty())
         delete root;
      else return false;
   }

   if (Found)
   {
      if (Branch[Location+1])
      {
         void *Next;

         if ((nextItemSearch || numkey == numkeys) && ItemFound) *ItemFound = Key[Location];
         reorder = Branch[Location+1]->Delete(Item, &Next, Func, List, KeysMgr, numkey, true);
         Key[Location] = Next;
      }
      else
      {
         if ((nextItemSearch || numkey == numkeys) && ItemFound) *ItemFound = Key[Location];
         DeleteItemToRight(Location);
         return (Count < MinKeys);
      }
   }
   else if (Branch[Location+1]) reorder =  Branch[Location+1]->Delete(Item, ItemFound, Func, List, KeysMgr, numkey, nextItemSearch);
   else reorder = false;
    
   if (reorder)
   {
      
      // We try a left rotation
      if (Location < (Count-1) && Branch[Location+2]->Count > MinKeys)
      {
         // Left Rotation
         Branch[Location+1]->AddItemToRight(Key[Location+1], Branch[Location+2]->Branch[0], -1);
         Key[Location+1] = Branch[Location+2]->Key[0];
         Branch[Location+2]->DeleteItemToLeft(0);
      }
      // and, if we couldn't, a right rotation
      else if (Location >=0 && Branch[Location]->Count > MinKeys)
      {
         // Right rotation
         Branch[Location+1]->AddItemToLeft(Key[Location], Branch[Location]->Branch[Branch[Location]->Count], 0);
         Key[Location] = Branch[Location]->Key[Branch[Location]->Count - 1];
         Branch[Location]->DeleteItemToRight(-1);
      }

      else
      {
         // Else we join nodes at by-left and by-right branches. 
         // The addition of keys always will be lower than Maxkeys due MaxKey = MinKeys*2 + 1
         if (Count > 1)
         {
            BTNode *toDelete;
            if (Location<0) Location=0;
            Branch[Location]->AddItemToRight(Key[Location], Branch[Location+1]->Branch[0], -1);
            for (int i=0; i<Branch[Location+1]->Count; i++)
               Branch[Location]->AddItemToRight(Branch[Location+1]->Key[i], Branch[Location+1]->Branch[i+1], -1);
            toDelete = Branch[Location+1];
            DeleteItemToRight(Location);
            delete toDelete;
         }
         else
         {
            int i;
            BTNode *left, *right;
            left = Branch[0];
            right = Branch[1];
            for (i=0; i<left->Count; i++)
               AddItemToLeft(left->Key[i], left->Branch[i], i);
            Branch[i] = left->Branch[i];
            Branch[i+1] = right->Branch[0];
            for (i=0; i<right->Count; i++)
               AddItemToRight(right->Key[i], right->Branch[i+1], -1);
            delete left;
            delete right;
         }

      }
   }
   return (Count < MinKeys);
}

/**
 * @brief Find an item in a tree
 * 
 * @param Item to search for 
 * @param Func Compare function. Must return -1, 0, or 1
 * @param List variable list of additional arguments that will be added when calling Func
 * @return The item found or NULL if nor found
 */
const void *BTNode::Find(const void *Item, BTCompareFunc Func, va_list List) const
{
   int Location;
   bool Found;
   const BTNode *CurrentNode;

   Found = false;
   CurrentNode = this;

   while ((CurrentNode != NULL) && (! Found))
   {
      if (CurrentNode->SearchNode(Item, Func, List, Location))
         Found = true;

      else CurrentNode = CurrentNode->Branch[Location + 1];
   }

   if (Found)
      return CurrentNode->Key[Location];
   else 
      return NULL;
}

/**
 * @brief Find an item in a tree. Use KeysManager
 * 
 * @param Item to search for 
 * @param KeysMgr that contains the compare function and the number of keys (levels of trees of trees)
 * @param numkey current key number (level at what we are)
 * @return The item found or NULL if nor found
 */
const void *BTNode::Find(const void *Item, BTKeyManager *KeysMgr, int numkey) const
{
   int Location;
   bool Found;
   const BTNode *CurrentNode;

   Found = false;
   CurrentNode = this;

   while ((CurrentNode != NULL) && (! Found))
   {
      if (CurrentNode->SearchNode(Item, KeysMgr, numkey, Location))
         Found = true;

      else CurrentNode = CurrentNode->Branch[Location + 1];
   }

   if (Found)
      return CurrentNode->Key[Location];
   else 
      return NULL;
}

/**
 * @brief Find an item in a tree. Return a pointer to where the item found is stored
 * 
 * @param Item to search for 
 * @param Func Compare function. Must return -1, 0, or 1
 * @param List variable list of additional arguments that will be added when calling Func
 * @return The pointer to where the item found is stored or NULL if nor found
 */
void * const *BTNode::FindDir(const void *Item, BTCompareFunc Func, va_list List) const
{
   int Location;
   bool Found;
   const BTNode *CurrentNode;

   Found = false;
   CurrentNode = this;

   while ((CurrentNode != NULL) && (! Found))
   {
      if (CurrentNode->SearchNode(Item, Func, List, Location))
         Found = true;

      else CurrentNode = CurrentNode->Branch[Location + 1];
   }

   if (Found)
      return &CurrentNode->Key[Location];
   else 
      return NULL;
}

/**
 * @brief Find an item in a tree. Use KeysManager. Return a pointer to where the item found is stored
 * 
 * @param Item to search for 
 * @param KeysMgr that contains the compare function and the number of keys (levels of trees of trees)
 * @param numkey current key number (level at what we are)
 * @return The pointer to where the item found is stored or NULL if nor found
 */

void * const *BTNode::FindDir(const void *Item, BTKeyManager *KeysMgr, int numkey) const
{
   int Location;
   bool Found;
   const BTNode *CurrentNode;

   Found = false;
   CurrentNode = this;

   while ((CurrentNode != NULL) && (! Found))
   {
      if (CurrentNode->SearchNode(Item, KeysMgr, numkey, Location))
         Found = true;

      else CurrentNode = CurrentNode->Branch[Location + 1];
   }

   if (Found)
      return &CurrentNode->Key[Location];
   else 
      return NULL;
}

/**
 * @brief Walk through a Tree in an iterative, non recursive way
 *    This is done by means of a BTstate that maintain where the walk proccess is and is able to continue from there
 *    State location will point to the next branch to the key returned
 *    - The search for the next item is done going down in the branch to reach the very next simple item (going by location 0)
 *    - if there is no more keys and no more branches to the right we climb up in the hierarchy until finding new keys to the right
 *    - If a key is found it is returned and the next location is stored in the state. Else where NULL is returned
 * 
 * @param state where the walk state is stored
 * @return const void* the next key found
 */
// static
const void *BTNode::Walk(BTState &state)
{
    const BTNode *CurrentNode;

    if (state.level<0)
        return NULL;

    while (CurrentNode = state.stack[state.level].node->Branch[state.stack[state.level].loc])
        state.Push(CurrentNode);

    while (state.level>=0 && state.stack[state.level].loc >= state.stack[state.level].node->Count)
        state.Pop();
    
    if (state.level>=0)
        return state.stack[state.level].node->Key[state.stack[state.level].loc++];
    else
        return NULL;
}

/**
 * @brief Setup a BTstate from where a given item is to walk from there
 * 
 * @param Target item found 
 * @param Func Compare function. Must return -1, 0, or 1
 * @param List variable list of additional arguments that will be added when calling Func
 * @param state the state that is setup as the result of the search
 */
void
BTNode::SearchNodeBiggerThan(const void * Target, BTCompareFunc Func, va_list List, BTState &state) const
{
    bool Found;
    int i,j;
    int result;
    const BTNode *node = this;

    Found = false;
    while ((node != NULL) && !Found)
    {
        int loc;
        state.Push(node);
       
        result = 1; i=0; j=node->Count-1;

        while (result!=0 && i<=j)
        {
            loc = i + (j-i)/2;
            result = (* Func)(Target, node->Key[loc], List);
            if (result>0)
                i=loc + 1;
            else if (result<0)
                j=loc - 1;
        }

        if (Found = (result == 0)) i = loc+1;
        state.stack[state.level].loc = i;
        node = node->Branch[i];
    }
}

/**
 * @brief Setup a BTstate from where a given item is to walk from there
 * 
 * @param Target item found 
 * @param KeysMgr that contains the compare function and the number of keys (levels of trees of trees)
 * @param numkey current key number (level at what we are)
 * @param state the state that is setup as the result of the search
 */
void
BTNode::SearchNodeBiggerThan(const void * Target, BTKeyManager *KeysMgr, int numkey, BTState &state) const
{
    bool Found;
    int i,j;
    int result;
    const BTNode *node = this;

    Found = false;
    while ((node != NULL) && !Found)
    {
        int loc;
        state.Push(node);
       
        result = 1; i=0; j=node->Count-1;

        while (result!=0 && i<=j)
        {
            const void *Internal;

            loc = i + (j-i)/2;

            Internal = Key[loc];
            for (int nk=numkey; nk < KeysMgr->numKeys(); nk++)
                Internal = ((BTNode *)Internal)->Key[0];

            result = KeysMgr->compareKey(Target, Internal, numkey);

            if (result>0)
                i=loc + 1;
            else if (result<0)
                j=loc - 1;
        }

        if (Found = (result == 0)) i = loc+1;
        state.stack[state.level].loc = i;
        node = node->Branch[i];
    }
}

