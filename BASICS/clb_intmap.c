/*-----------------------------------------------------------------------

File  : clb_intmap.c

Author: Stephan Schulz (schulz@eprover.org)

Contents
 
  Functions implementing the multi-representation N_0->void* mapping
  data type.

  Copyright 2004 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Mon Dec 27 17:34:48 CET 2004
    New

-----------------------------------------------------------------------*/

#include "clb_intmap.h"



/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/


/*-----------------------------------------------------------------------
//
// Function: switch_to_array()
//
//   Return true if representation should switch to array (because of
//   high density)
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static bool switch_to_array(long old_max, long new_key, long entries)
{
   long max_key = MAX(old_max, new_key);

   if((entries * MAX_TREE_DENSITY) > max_key)
   {
      return true;
   }
   return false;
}


/*-----------------------------------------------------------------------
//
// Function: switch_to_tree()
//
//   Return true if representation should switch to tree (because of
//   low density)
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static bool switch_to_tree(long old_max, long new_key, long entries)
{
   long max_key = MAX(old_max, new_key);

   if((entries * MIN_TREE_DENSITY) < max_key)
   {
      return true;
   }      
   return false;
}

/*-----------------------------------------------------------------------
//
// Function: add_new_tree_node()
//
//   Add a *new* key node to a IntMap in tree form and return its
//   address. Assertion fail, if key is not new. Increases element
//   count! 
//
// Global Variables: -
//
// Side Effects    : Changes tree
//
/----------------------------------------------------------------------*/

static NumTree_p add_new_tree_node(IntMap_p map, long key, void* val)
{
   NumTree_p handle, check;
   assert(map->type == IMTree);

   handle = NumTreeCellAlloc();
   handle->key = key;
   handle->val1.p_val = val;
   check = NumTreeInsert(&(map->values.tree), handle);
   assert(!check);
   map->entry_no++;

   return handle;
}


/*-----------------------------------------------------------------------
//
// Function: array_to_tree()
//
//   Convert a IntMap in array form to an equivalent one in tree
//   form. 
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

static void array_to_tree(IntMap_p map)
{
   PDArray_p     tmp_arr;
   IntOrP        tmp_val;
   long          i;
   long          max_key = 0;

   assert(map->type == IMArray);

   tmp_arr = map->values.array;
   map->values.tree = NULL;
   map->type = IMTree;
   map->entry_no = 0;

   for(i=0; i<=map->max_key; i++)
   {
      tmp_val.p_val = PDArrayElementP(tmp_arr, i);
      if(tmp_val.p_val)
      {
         max_key = i;
         NumTreeStore(&(map->values.tree), i, tmp_val, tmp_val);
         map->entry_no++;
      }
   }
   map->max_key = max_key;
   PDArrayFree(tmp_arr);
}


/*-----------------------------------------------------------------------
//
// Function: tree_to_array()
//
//   Convert a IntMap in tree form to an equivalent one in array
//   form. 
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

static void tree_to_array(IntMap_p map)
{
   PDArray_p     tmp_arr;
   long          max_key = 0;
   PStack_p      tree_iterator;
   NumTree_p     handle;

   assert(map->type == IMTree);

   map->entry_no = 0;
   tmp_arr = PDArrayAlloc(IM_ARRAY_SIZE,IM_ARRAY_SIZE);
   tree_iterator = NumTreeTraverseInit(map->values.tree);
   while((handle = NumTreeTraverseNext(tree_iterator)))
   {
      if(handle->val1.p_val)
      {
         PDArrayAssignP(tmp_arr, handle->key, handle->val1.p_val);
         map->entry_no++;
         max_key = handle->key;
      }
   }
   NumTreeTraverseExit(tree_iterator);
   NumTreeFree(map->values.tree);
   map->values.array = tmp_arr;
   map->type = IMArray;   
}



/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: IntMapAlloc() 
//
//   Allocate an empty int mapper.
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

IntMap_p IntMapAlloc()
{
   IntMap_p handle = IntMapCellAlloc();

   handle->type = IMEmpty;
   
   return handle;
}


/*-----------------------------------------------------------------------
//
// Function: IntMapFree()
//
//   Free an int mapper (does _not_ free pointed-to elements).
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

void IntMapFree(IntMap_p map)
{
   assert(map);

   switch(map->type)
   {
   case IMEmpty:
   case IMSingle:
         break;
   case IMArray:
         PDArrayFree(map->values.array);
         break;
   case IMTree:
         NumTreeFree(map->values.tree);
         break;
   default:
         assert(false && "Unknown IntMap type.");
   }
   IntMapCellFree(map);
}


/*-----------------------------------------------------------------------
//
// Function: IntMapGetVal()
//
//   Given a key, return the associated value or NULL, if no suitable
//   key/value pair exists.
//
// Global Variables: 
//
// Side Effects    : 
//
/----------------------------------------------------------------------*/

void* IntMapGetVal(IntMap_p map, long key)
{
   assert(map);
   assert(key >=0);
   void* res = NULL;

   switch(map->type)
   {
   case IMEmpty:         
         break;
   case IMSingle:
         if(map->max_key == key)
         {
            res = map->values.value;
         }
         break;
   case IMArray:
         if(key <= map->max_key)
         {
            res = PDArrayElementP(map->values.array, key);            
         }
         break;
   case IMTree:
         if(key <= map->max_key)
         {
            NumTree_p entry = NumTreeFind(&(map->values.tree), key);
            if(entry)
            {
               res = entry->val1.p_val;
            }
         }         
         break;
   default:
         assert(false && "Unknown IntMap type.");
   }
   return res;   
}


/*-----------------------------------------------------------------------
//
// Function: IntMapGetRef()
//
//   Get a reference to the address of the value of a key/value
//   pair. Note that this always creates the key value pair (with
//   empty value) if it does not exist yet.
//
// Global Variables: -
//
// Side Effects    : May reorganize the map.
//
/----------------------------------------------------------------------*/


void** IntMapGetRef(IntMap_p map, long key)
{
   void      **res = NULL;
   void      *val;
   NumTree_p handle;
   IntOrP tmp;  

   assert(map);
   assert(key >= 0);

   switch(map->type)
   {
   case IMEmpty:        
         map->type = IMSingle;
         map->max_key = key;
         map->values.value = NULL;
         res = &(map->values.value);
         map->entry_no = 1;
         break;
   case IMSingle:
         if(key == map->max_key)
         {
            res = &(map->values.value);
         }
         else if(switch_to_array(key, map->max_key, 2))
         {
            map->type = IMArray;
            val = map->values.value;
            map->values.array = PDArrayAlloc(IM_ARRAY_SIZE,IM_ARRAY_SIZE);
            PDArrayAssignP(map->values.array, map->max_key, val);
            PDArrayAssignP(map->values.array, key, NULL);
            res = &(PDArrayElementP(map->values.array, key));
            map->entry_no = 2;                     
         }
         else
         {
            map->type = IMTree;
            val = map->values.value;
            map->values.tree = NULL;
            tmp.p_val = val;
            NumTreeStore(&(map->values.tree),map->max_key, tmp, tmp);
            handle = add_new_tree_node(map, key, NULL);
            res = &(handle->val1.p_val);
         }
         map->max_key = MAX(key, map->max_key);
         break;
   case IMArray:
         if((key > map->max_key) && 
            switch_to_tree(map->max_key, key, map->entry_no+1))
         {
            array_to_tree(map);
            res = IntMapGetRef(map, key);
         }
         else
         {
            res = &(PDArrayElementP(map->values.array, key));
            if(!(*res))
            {
               map->entry_no++;
            }
         }
         map->max_key=MAX(map->max_key, key);
         break;
   case IMTree:
         handle = NumTreeFind(&(map->values.tree), key);
         if(handle)
         {
            res = &(handle->val1.p_val);
         }
         else
         {
            if(switch_to_array(map->max_key, key, map->entry_no+1))
            {
               tree_to_array(map);
               res = IntMapGetRef(map, key);
            }
            else
            {
               handle = add_new_tree_node(map, key, NULL);
               map->max_key=MAX(map->max_key, key);
               res = &(handle->val1.p_val);            
            }
         }
         break;
   default:
         assert(false && "Unknown IntMap type.");
   }

   assert(res);
   return res;
}


/*-----------------------------------------------------------------------
//
// Function: IntMapAssign()
//
//   Add key/value pair to map, overriding any previous association. 
//
// Global Variables: -
//
// Side Effects    : Changes map, may trigger reorganization
//
/----------------------------------------------------------------------*/

void IntMapAssign(IntMap_p map, long key, void* value)
{
   void** ref;
   
   assert(map);
   assert(key > 0);

   ref  = IntMapGetRef(map, key);
   *ref = value;
}


/*-----------------------------------------------------------------------
//
// Function: IntMapDelKey()
//
//   Delete a key/value association. If there was one, return the
//   value, otherwise return NULL.
//
//   **Currently, arrays never shrink. This might be worth
//   **changing (unlikely, though).
//
// Global Variables: -
//
// Side Effects    : May reorganize map
//
/----------------------------------------------------------------------*/

void* IntMapDelKey(IntMap_p map, long key)
{
   void* res = NULL;
   NumTree_p   handle;

   assert(map);

   switch(map->type)
   {
   case IMEmpty:
         res = NULL;
         break;
   case IMSingle:
         if(key == map->max_key)
         {
            res = map->values.value;
            map->type = IMEmpty;
            map->entry_no = 0;
         }
         break;
   case IMArray:
         if(key > map->max_key)
         {
            res = NULL;
         }
         /* if key == map->max_key optionally do something (shrink
          * array, recompute map->max_key - likely unnecessary at
          * least for my current applications */ 
         else if((res = PDArrayElementP(map->values.array, key)))
         {
            PDArrayAssignP(map->values.array, key, NULL);
            map->entry_no--;
            if(switch_to_tree(map->max_key, map->max_key, map->entry_no))
            {
               array_to_tree(map);
            }
         }
         break;
   case IMTree:
         handle = NumTreeExtractEntry(&(map->values.tree), key);
         if(handle)
         {
            map->entry_no--;
            res = handle->val1.p_val;
            if(handle->key == map->max_key)
            {
               map->max_key = NumTreeMaxKey(map->values.tree);               
               if(switch_to_array(map->max_key, map->max_key, map->entry_no))
               {
                  tree_to_array(map);
               }
            }
            NumTreeCellFree(handle);
         }
         break;
   default:
         assert(false && "Unknown IntMap type.");                  
         break;
   }
   return res;
}

/*-----------------------------------------------------------------------
//
// Function: IntMapIterAlloc()
//
//   Allocate an iterator object iterating over key range lower_key to
//   upper_key (both inclusive) in map. This is only valid as long as
//   no new key is introduced or old key is deleted.
//
// Global Variables: -
//
// Side Effects    : Memory operations.
//
/----------------------------------------------------------------------*/

IntMapIter_p IntMapIterAlloc(IntMap_p map, long lower_key, long upper_key)
{
   IntMapIter_p handle = IntMapIterCellAlloc();

   handle->map = map;
   handle->lower_key = lower_key;
   handle->upper_key = MIN(upper_key, map->max_key);

   switch(map->type)
   {
   case IMEmpty:
         break;
   case IMSingle:
         handle->admin_data.seen = true;
         if((map->max_key >= lower_key) && (map->max_key <= upper_key))
         {
            handle->admin_data.seen = false;
         }
         break;
   case IMArray:
         handle->admin_data.current = lower_key;
         break;
   case IMTree:
         handle->admin_data.tree_iter = 
            NumTreeLimitedTraverseInit(map->values.tree, lower_key);
         break;
   default:
         assert(false && "Unknown IntMap type.");
         break;
   }
   return handle;
}

/*-----------------------------------------------------------------------
//
// Function: IntMapIterFree()
//
//   Free an IntMapIterator.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void IntMapIterFree(IntMapIter_p junk)
{
   assert(junk);

   switch(junk->map->type)
   {
   case IMEmpty:
   case IMSingle:
   case IMArray:
         break;
   case IMTree:
         PStackFree(junk->admin_data.tree_iter);
         break;
   default:
         assert(false && "Unknown IntMap type.");
         break;
   }
   IntMapIterCellFree(junk);
}


/*-----------------------------------------------------------------------
//
// Function: IntMapIterNext()
//
//   Return the next value/key pair in the map (or NULL/ndef) if the
//   iterator is exhausted.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void* IntMapIterNext(IntMapIter_p iter, long *key)
{
   assert(iter);
   assert(key);
   void* res = NULL;
   long  i;
   NumTree_p handle;

   switch(iter->map->type)
   {
   case IMEmpty:
         break;
   case IMSingle:
         if(!iter->admin_data.seen)
         {
            iter->admin_data.seen = true;
            *key = iter->map->max_key;
            res = iter->map->values.value;
         }
         break;
   case IMArray: 
         for(i=iter->admin_data.current; i<= iter->upper_key; i++)
         {
            res = PDArrayElementP(iter->map->values.array, i);
            if(res)
            {
               *key = i;
               break;
            }
         }
         iter->admin_data.current = i+1;
         break;
   case IMTree:
         handle = NumTreeTraverseNext(iter->admin_data.tree_iter);
         if(handle && (handle->key <= iter->upper_key))
         {
            *key = handle->key;
            res = handle->val1.p_val;
         }
         break;
   default:
         assert(false && "Unknown IntMap type.");
         break;
   }
   return res;
}


/*-----------------------------------------------------------------------
//
// Function: IntMapDebugPrint()
//
//   Print an intmap datatype as a list of key:value pairs.
//
// Global Variables: -
//
// Side Effects    : Output, memory operations
//
/----------------------------------------------------------------------*/

void IntMapDebugPrint(FILE* out, IntMap_p map)
{
   IntMapIter_p iter = IntMapIterAlloc(map,0, LONG_MAX);
   void* val;
   long  key;

   fprintf(out, "# ==== IntMapType %d\n", map->type);
   for(val=IntMapIterNext(iter, &key); val; val=IntMapIterNext(iter, &key))
   {
      fprintf(out, "# %5ld : %p\n", key, val);
   }
   fprintf(out, "# ==== IntMap End\n");   
   
   IntMapIterFree(iter);
}


long     IntMapRecFree(IntMap_p map, IntMapFreeFunc free_func);


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/


