/**
 * @file keys.cpp
 * @author Francisco Alcaraz
 * @brief This module provides with the functions related to keyManager for the Multilevel BTrees applied to MetaObjs
 *      Every KeyManager store several keys with the following format
 *          TTPP PPPP PAAA AAAA Appp pppp aaaa aaaa 
 *                  T: Type of the data to be compared
 *                  P: Position of the object at left
 *                  A: Attribute number of the object at left 
 *                  p: Position of the object at right
 *                  a: Attribute number of the object at right 
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <cstring>
#include "keys.hpp"
#include "metaobj.hpp"
#include "single.hpp"
#include "set.hpp"

/**
 * @brief Return the value of the attribute that is used by the BTree to order the 
 *        MetaObjs that arrive an INTER node by a certain side
 * 
 * @param obj The complete MetaObj
 * @param keydata Information about the keys
 * @param side LEFT_MEM or RIGHT MEM (side of the node)
 * @return const Value* 
 */
const Value *
KeyManager::getObjInfo(MetaObj *obj, ULong keydata, ULong side)
{
    MetaObj *meta;
    Value *result;
    ULong pos, attr;

    if (side == LEFT_MEM)
        keydata = (keydata >> 15);
    keydata &= 0x7FFF;
    pos = (keydata >> 8);
    attr = (keydata & 0xFF);
    meta = (*obj)[pos];


    if (meta->class_type() == SINGLE)
        result = &(meta->single()->obj()->attr[attr]);
    else
        // SET   !! SETS of Singles
        result = &(meta->set()->first_item_of_set()->single()->obj()->attr[attr]);
    return result;
}
    
/**
 * @brief Compares a MetaObj target with an item in the BTree (internal)
 *      When _counters_mode and side == LEFT The objects stored in the BTree are MatchCount.
 *      This is to ease the asymmetric nodes (see nodes.cpp). 
 * 
 * @param target Clean object to comparewith the elements of the tree
 * @param internal Element in the Tree. Can be a MetaObj or a MatchCount
 * @param numkey Number of key
 * @return int comparison result
 */
int
KeyManager::compareKey(const void *target, const void *internal, int numkey)
{
    const Value *attrib_tgt, *attrib_int;
    ULong type;

    // In the search of keys the target is always a normal MetaObj, never goes a MatchCount
    // The comparison is made always between the Object (target) an a MatchCounter (internal to the tree)
    // See check_nand and check_oand. FindByKeys by left and when storing in ASYM nodes always the MetaObj is cleanly 
    // passed as parameter

    attrib_tgt = getObjInfo((MetaObj *&)target, _keys[numkey], _tgt_side);

    if (_counters_mode && _int_side == LEFT_MEM)
        attrib_int = getObjInfo(((MatchCount *&)internal)->item, _keys[numkey], _int_side);
    else
        attrib_int = getObjInfo((MetaObj *&)internal, _keys[numkey], _int_side);

    type = (_keys[numkey]>>30) & 0x3;

    switch(type)
    {
        case TYPE_NUM:
            return (int)(attrib_tgt->num - attrib_int->num);
        case TYPE_STR:
            return (attrib_tgt->str.str_p == attrib_int->str.str_p ? 0 :
                        (attrib_tgt->str.str_p == NULL | attrib_int->str.str_p == NULL ? (long int)attrib_tgt->str.str_p - (long int)attrib_int->str.str_p :
                            (strcmp(attrib_tgt->str.str_p, attrib_int->str.str_p))));
        case TYPE_FLO:
            if (attrib_tgt->flo<attrib_int->flo)
                return -1;
            else if (attrib_tgt->flo == attrib_int->flo)
                return 0;
            else
                return 1;

    }
    return -1;
}

/**
 * @brief Checks if the modification of an object has affected to the keys so it must be indexed again
 * 
 * @param objpos Position of the object (in the MetaObjs that arrive at this level)
 * @param obj Object modified
 * @param old_obj Object before being modified
 * @return true it has affected
 * @return false it has not affected
 */
bool 
KeyManager::keysModified(int objpos, ObjectType *obj, ObjectType *old_obj)
{
    for (int nk=0; nk<numKeys(); nk++)
    {
        ULong keydata, pos, attr, type;

        keydata = _keys[nk];
        if (_int_side == LEFT_MEM) keydata = (keydata >> 15);
        keydata &= 0x7FFF;
        pos = (keydata >> 8);
        attr = (keydata & 0xFF);
        type = (_keys[nk]>>30) & 0x3;

        if (pos == objpos)
        {
            switch (type)
            {
                case TYPE_NUM: 
                    if (old_obj->attr[attr].num != obj->attr[attr].num) return true;
                    break;
                case TYPE_FLO: 
                    if (old_obj->attr[attr].flo != obj->attr[attr].flo) return true;
                    break;
                case TYPE_STR:
                    if (old_obj->attr[attr].str.str_p != obj->attr[attr].str.str_p &&
                                (old_obj->attr[attr].str.str_p == NULL || obj->attr[attr].str.str_p == NULL ||
                                        strcmp(old_obj->attr[attr].str.str_p, obj->attr[attr].str.str_p) != 0))
                        return true;
                    break;
            }
        }
    }
    return false;
}


