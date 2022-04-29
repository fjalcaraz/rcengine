/**
 * @file keys.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the keys module. Definition of the Class KeyManager
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "engine.h"
#include "btree.hpp"
#include "eng.hpp"

class KeyManager : public BTKeyManager
{
    private:
        int _int_side;  // Node Memory Side where the tree is
        int _tgt_side;  // Side of the object to match to
        ULong *_keys;   // Keys array
        bool _counters_mode; // If at the left side will be MatchCounters (in asymmetric nodes)
    public:
    inline KeyManager(int int_side, int tgt_side, int nkeys, ULong *keys, bool counters_mode=false) 
    : BTKeyManager(nkeys) 
    { _int_side=int_side; _keys=keys; _tgt_side = tgt_side; _counters_mode= counters_mode; };
    inline void setTargetSide(int tgt_side) { _tgt_side = tgt_side; };

    static const Value * getObjInfo(MetaObj *obj, ULong data, ULong side);
    bool keysModified(int objpos, ObjectType *obj, ObjectType *old_obj);
    int compareKey(const void *target, const void *internal, int numkey);
};

