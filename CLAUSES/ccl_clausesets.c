/*-----------------------------------------------------------------------

File  : ccl_clausesets.c

Author: Stephan Schulz

Contents
 
  Implementation of clause sets based on AVL trees.

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Sun May 10 03:03:20 MET DST 1998
    New

-----------------------------------------------------------------------*/

#include "ccl_clausesets.h"



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
// Function: print_var_pattern()
//
//   Print a template for a function/predicate symbol.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

static void print_var_pattern(FILE* out, char* symbol, int arity, char*
			      var, char* alt_var, int exception)
{
   int i;
   char* prefix = "";

   fprintf(out, "%s(", symbol);
   
   for(i=1; i<= arity; i++)
   {
      fprintf(out, prefix);
      if(i==exception)
      {
	 fprintf(out, alt_var);
      }
      else
      {
	 fprintf(out, "%s%d", var, i);
      }
      prefix = ",";
   }
   fprintf(out, ")");
}


/*-----------------------------------------------------------------------
//
// Function:  eq_func_axiom_print()
//
//   Print the LOP substitutivity axiom(s) for a function symbol.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

static void eq_func_axiom_print(FILE* out, char* symbol, int arity,
				bool single_subst)
{
   int i;
   char *prefix = "";
   
   if(single_subst)
   {
      for(i=1; i<=arity; i++)
      {
	 fprintf(out, "equal(");
	 print_var_pattern(out, symbol, arity, "X", "Y", i);
	 fprintf(out, ",");
	 print_var_pattern(out, symbol, arity, "X", "Z", i);	 
	 fprintf(out, ") <- ");
	 fprintf(out, "equal(Y,Z).\n");
      }
   }
   else
   {
      fprintf(out, "equal(");
      print_var_pattern(out, symbol, arity, "X", NULL,0);
      fprintf(out, ",");
      print_var_pattern(out, symbol, arity, "Y", NULL,0);
      fprintf(out, ") <- ");
      for(i=1; i<=arity; i++)
      {
	 fprintf(out, "%sequal(X%d,Y%d)", prefix, i, i);
	 prefix = ",";
      }
      fprintf(out, ".\n");
   }
}

/*-----------------------------------------------------------------------
//
// Function:  eq_pred_axiom_print()
//
//   Print the LOP substitutivity axiom(s) for a predicate symbol.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

static void eq_pred_axiom_print(FILE* out, char* symbol, int arity,
				bool single_subst)
{
   int i;
   
   if(single_subst)
   {
      for(i=1; i<=arity; i++)
      {
	 print_var_pattern(out, symbol, arity, "X", "Y", i);
	 fprintf(out, " <- ");
	 print_var_pattern(out, symbol, arity, "X", "Z", i);
	 fprintf(out, ", equal(Y,Z).\n");
      }
   }
   else
   {
      print_var_pattern(out, symbol, arity, "X", NULL,0);
      fprintf(out, " <- ");
      print_var_pattern(out, symbol, arity, "Y", NULL,0);
      for(i=1; i<=arity; i++)
      {
	 fprintf(out, ",equal(X%d,Y%d)", i, i);
      }
      fprintf(out, ".\n");
   }
}


/*-----------------------------------------------------------------------
//
// Function:  tptp_eq_func_axiom_print()
//
//   Print the TPTP substitutivity axiom(s) for a function symbol.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

static void tptp_eq_func_axiom_print(FILE* out, char* symbol, int arity,
				     bool single_subst)
{
   int i;
   
   if(single_subst)
   {
      for(i=1; i<=arity; i++)
      {	 
	 fprintf(out, "input_clause(eq_subst_%s%d, axiom, [++equal(",
		 symbol, i);
	 print_var_pattern(out, symbol, arity, "X", "Y", i);
	 fprintf(out, ",");
	 print_var_pattern(out, symbol, arity, "X", "Z", i);	 
	 fprintf(out, "),");
	 fprintf(out, "--equal(Y,Z)]).\n");
      }
   }
   else
   {
      fprintf(out, "input_clause(eq_subst_%s, axiom, [++equal(",
	      symbol);
      print_var_pattern(out, symbol, arity, "X", NULL,0);
      fprintf(out, ",");
      print_var_pattern(out, symbol, arity, "Y", NULL,0);
      fprintf(out, ")");
      for(i=1; i<=arity; i++)
      {
	 fprintf(out, ",--equal(X%d,Y%d)", i, i);
      }
      fprintf(out, "]).\n");
   }
}

/*-----------------------------------------------------------------------
//
// Function:  tptp_eq_pred_axiom_print()
//
//   Print the TPTP substitutivity axiom(s) for a predicate symbol.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

static void tptp_eq_pred_axiom_print(FILE* out, char* symbol, int arity,
				     bool single_subst)
{
   int i;
   
   if(single_subst)
   {
      for(i=1; i<=arity; i++)
      {
	 fprintf(out, "input_clause(eq_subst_%s%d, axiom, [++",
		 symbol, i);
	 print_var_pattern(out, symbol, arity, "X", "Y", i);
	 fprintf(out, ",--");
	 print_var_pattern(out, symbol, arity, "X", "Z", i);	 
	 fprintf(out, ",--equal(Y,Z)]).\n");
      }
   }
   else
   {
      fprintf(out, "input_clause(eq_subst_%s, axiom, [++",
	      symbol);
      print_var_pattern(out, symbol, arity, "X", NULL,0);
      fprintf(out, ",--");
      print_var_pattern(out, symbol, arity, "Y", NULL,0);
      for(i=1; i<=arity; i++)
      {
	 fprintf(out, ",--equal(X%d,Y%d)", i, i);
      }
      fprintf(out, "]).\n");
   }
}


/*-----------------------------------------------------------------------
//
// Function: clause_set_extract_entry()
//
//   Remove a plain clause from a plain clause set.
//
// Global Variables: -
//
// Side Effects    : Changes set
//
/----------------------------------------------------------------------*/

static void clause_set_extract_entry(Clause_p clause)
{
   int     i;
   Eval_p  handle, test;

   assert(clause);
   assert(clause->set);
   assert(clause->set->members);
   /* assert(ClauseSetFind(clause->set, clause));  */
   
   /* printf("ClauseSetExtractEntry: %d\n", (int)clause); */

   i=0;
   for(handle = clause->evaluations; handle; handle = handle->next)
   {
      /* EvalListPrint(stdout, handle);printf("\n"); */
      test = EvalTreeExtractEntry((Eval_p*)
				  &PDArrayElementP(clause->set->eval_indices, i), 
				  handle); 
      assert(test);
      assert(test->object == clause);
      i++;
   }
   clause->pred->succ = clause->succ;
   clause->succ->pred = clause->pred;
   clause->set->literals-=ClauseLiteralNumber(clause);
   clause->set->members--;
   clause->set = NULL;
   clause->succ = NULL;
   clause->pred = NULL;
}


/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: ClauseSetAlloc()
//
//   Allocate an empty clause set that uses setclock for time-keeping.
//
// Global Variables: -
//
// Side Effects    : Memory management
//
/----------------------------------------------------------------------*/

ClauseSet_p ClauseSetAlloc(void)
{
   ClauseSet_p handle;

   handle = ClauseSetCellAlloc();
   
   handle->members = 0;
   handle->literals = 0;
   handle->anchor = ClauseCellAlloc();
   handle->anchor->literals = NULL;
   handle->anchor->pred = handle->anchor->succ = handle->anchor;
   handle->date = SysDateCreationTime();
   SysDateInc(&handle->date);
   handle->demod_index = NULL;
   handle->fvindex = NULL;
   
   handle->eval_indices = PDArrayAlloc(4,4);
   handle->eval_no = 0;

   return handle;
}

/*-----------------------------------------------------------------------
//
// Function: ClauseSetFree()
//
//   Delete a clauseset.
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

void ClauseSetFree(ClauseSet_p junk)
{
   Clause_p handle;
   
   assert(junk);

   handle = junk->anchor->succ;
   while(handle!=junk->anchor)
   {
      handle = handle->succ;
      handle->pred->set = NULL;
      ClauseFree(handle->pred);
   }
   if(junk->demod_index)
   {
      PDTreeFree(junk->demod_index);
   }
   if(junk->fvindex)
   {
      FVIAnchorFree(junk->fvindex);
   }
   PDArrayFree(junk->eval_indices);
   ClauseCellFree(junk->anchor);
   ClauseSetCellFree(junk);
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetGCMarkTerms()
//
//   Mark all terms in the clause set for the garbage collection.
//
// Global Variables: 
//
// Side Effects    : 
//
/----------------------------------------------------------------------*/

void ClauseSetGCMarkTerms(ClauseSet_p set)
{
   Clause_p handle;
   
   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      ClauseGCMarkTerms(handle);
   }
}

/*-----------------------------------------------------------------------
//
// Function: ClauseSetInsert() 
//
//   Insert a clause as the last clause into the clauseset.
//
// Global Variables: -
//
// Side Effects    : Changes set
//
/----------------------------------------------------------------------*/

void ClauseSetInsert(ClauseSet_p set, Clause_p new)
{
   int    i;
   Eval_p handle, test;

   assert(!new->set);

   new->succ = set->anchor;
   new->pred = set->anchor->pred;
   set->anchor->pred->succ = new;
   set->anchor->pred = new;
   new->set = set;
   set->members++;
   set->literals+=ClauseLiteralNumber(new);

   i=0;
   for(handle = new->evaluations; handle; handle = handle->next)
   {
      test = EvalTreeInsert((Eval_p*)
			    &PDArrayElementP(new->set->eval_indices,
					     i),
			    handle);
      assert(!test);
      i++;
   }
   set->eval_no = MAX(i, set->eval_no);
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetIndexedInsert()
//
//   Insert a demodulator into the set and the sets index.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ClauseSetIndexedInsert(ClauseSet_p set, Clause_p new)
{
   ClausePos_p pos;

   assert(set->demod_index);
   assert(ClauseIsUnit(new));
   
   ClauseSetInsert(set, new);
   pos          = ClausePosCellAlloc();
   pos->clause  = new;
   pos->literal = new->literals;
   pos->side    = LeftSide;
   pos->pos     = NULL;
   PDTreeInsert(set->demod_index, pos);
   if(!EqnIsOriented(new->literals))
   {
      pos          = ClausePosCellAlloc();
      pos->clause  = new;
      pos->literal = new->literals;
      pos->side    = RightSide;
      pos->pos     = NULL;
      PDTreeInsert(set->demod_index, pos);
   }
   ClauseSetProp(new, CPIsDIndexed);
}




/*-----------------------------------------------------------------------
//
// Function: ClauseSetExtractEntry()
//
//   Remove a (possibly indexed) clause from a clause set.
//
// Global Variables: -
//
// Side Effects    : Changes set
//
/----------------------------------------------------------------------*/

Clause_p ClauseSetExtractEntry(Clause_p clause)
{
   assert(clause && clause->set);

   if(ClauseQueryProp(clause, CPIsDIndexed))
   {
      assert(clause->set->demod_index);
      if(clause->set->demod_index)
      {
	 assert(ClauseIsUnit(clause));	 
	 PDTreeDelete(clause->set->demod_index, clause->literals->lterm,
		      clause); 
	 if(!EqnIsOriented(clause->literals))
	 {
	    PDTreeDelete(clause->set->demod_index,
			 clause->literals->rterm, clause);
	 }
	 ClauseDelProp(clause, CPIsDIndexed);
      }
   }
   if(ClauseQueryProp(clause, CPIsSIndexed))
   {
      FVIndexDelete(clause->set->fvindex, clause);
   }
   clause_set_extract_entry(clause);
   return clause;
}  


/*-----------------------------------------------------------------------
//
// Function: ClauseSetExtractFirst()
//
//   Extract the first element of the set and return it. Return NULL
//   if set is empty.
//
// Global Variables: -
//
// Side Effects    : As above
//
/----------------------------------------------------------------------*/

Clause_p ClauseSetExtractFirst(ClauseSet_p set)
{
   Clause_p handle;

   if(ClauseSetEmpty(set))
   {
      return NULL;
   }
   handle = set->anchor->succ;
   assert(handle->set == set);
   ClauseSetExtractEntry(handle);
   
   return handle;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetDeleteEntry()
//
//   Delete a clause from the clause set. 
//
// Global Variables: -
//
// Side Effects    : Changes tree, memory operations
//
/----------------------------------------------------------------------*/

void ClauseSetDeleteEntry(Clause_p clause)
{
   ClauseSetExtractEntry(clause);
   ClauseFree(clause);
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFindBest()
//
//   Find the best clause (i.e. the clause with the smallest
//   evaluation).
//
// Global Variables: -
//
// Side Effects    : Changes clause set
//
/----------------------------------------------------------------------*/

Clause_p ClauseSetFindBest(ClauseSet_p set, int idx)
{
   Clause_p clause;
   Eval_p   evaluation;

   evaluation =
      EvalTreeFindSmallest(PDArrayElementP(set->eval_indices, idx));
   if(!evaluation)
   {
      assert(set->anchor->succ == set->anchor);
      return NULL;
   }
   assert(evaluation->object);
   clause = evaluation->object;
   assert(clause->set == set);
   return clause;
}



/*-----------------------------------------------------------------------
//
// Function: ClauseSetPrint()
//
//   Print the clause set to the given stream.
//
// Global Variables: Only for term output
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

void ClauseSetPrint(FILE* out, ClauseSet_p set, bool fullterms)
{
   Clause_p handle;

   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      ClausePrint(out, handle, fullterms);
      fputc('\n', out);
   }
}

/*-----------------------------------------------------------------------
//
// Function: ClauseSetPrintPrefix()
//
//   Print the clause set, one clause per line, with prefix prefix on
//   each line.
//
// Global Variables: For format (read only)
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

void ClauseSetPrintPrefix(FILE* out, char* prefix, ClauseSet_p set)
{
   Clause_p handle;

   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      fputs(prefix, out);
      ClausePrint(out, handle, true);
      fputc('\n', out);
   }
}




/*-----------------------------------------------------------------------
//
// Function: ClauseSetSort()
//
//   Sort a clause set according to the comparison function
//   given. Note: This is unnecssarily inefficient for evaluated
//   clauses! Reimplement if you need to use it for large evaluatied
//   sets!
//
// Global Variables: -
//
// Side Effects    : Memory operations, will reorganize evaluation
//                   tree
//
/----------------------------------------------------------------------*/

void ClauseSetSort(ClauseSet_p set, ClauseCmpFunType cmp_fun)
{
   PStack_p stack = PStackAlloc();
   Clause_p clause;
   PStackPointer i;

   while((clause = ClauseSetExtractFirst(set)))
   {
      clause->weight = ClauseStandardWeight(clause);
      PStackPushP(stack, clause);
   }
   assert(ClauseSetEmpty(set));

   qsort(stack->stack, PStackGetSP(stack), sizeof(IntOrP),
	 (ComparisonFunctionType)cmp_fun);

   for(i=0; i<PStackGetSP(stack); i++)
   {
      clause = PStackElementP(stack,i);
      ClauseSetInsert(set, clause);
   }
   /* ClauseSetPrint(GlobalOut, set, true); */
   PStackFree(stack);
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetSetProp()
//
//   Set prop in all clauses in set.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ClauseSetSetProp(ClauseSet_p set, ClauseProperties prop)
{
   Clause_p handle;

   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      ClauseSetProp(handle, prop);
   }
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetDelProp()
//
//   Delete prop in all clauses in set.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ClauseSetDelProp(ClauseSet_p set, ClauseProperties prop)
{
   Clause_p handle;

   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      ClauseDelProp(handle, prop);
   }
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetMarkCopies()
//
//   Mark clauses that are equivalent (modulo ClauseCompareFun) to
//   clauses that occur earlier in set. Returns number of marked
//   clauses. 
//
// Global Variables: -
//
// Side Effects    : Sets property, memory operations, deletes parents
//                   of unmarked copyes.
//
/----------------------------------------------------------------------*/

long ClauseSetMarkCopies(ClauseSet_p set)
{
   long res = 0;
   Clause_p handle, exists;
   PTree_p  store = NULL;

   assert(set);

   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      if((exists = PTreeObjStore(&store, handle,
			       (ComparisonFunctionType)ClauseCompareFun)))
      {
	 if(!ClauseParentsAreSubset(exists, handle))
	 {
	    ClauseDetachParents(exists);
	 }
	 ClauseSetProp(handle, CPDeleteClause);
	 res++;
      }
   }
   PTreeFree(store);
   
   return res;
}



/*-----------------------------------------------------------------------
//
// Function: ClauseSetDeleteMarkedEntries()
//
//   Remove all clauses with property CPDeleteClause set. Returns
//   number of deleted clauses.
//
// Global Variables: -
//
// Side Effects    : Changes set.
//
/----------------------------------------------------------------------*/

long ClauseSetDeleteMarkedEntries(ClauseSet_p set)
{
   long deleted = 0;
   Clause_p clause, handle;
   
   assert(set);

   handle = set->anchor->succ;
   
   while(handle != set->anchor)
   {
      clause = handle;
      handle = handle->succ;

      if(ClauseQueryProp(clause, CPDeleteClause))
      {
	 deleted++;
	 ClauseDetachParents(clause); /* If no parents, nothing should
					 happen! */
	 ClauseSetDeleteEntry(clause);
      }
   }
   return deleted;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetDeleteCopies()
//
//   Delete all but one occurence of a clause in set.
//
// Global Variables: -
//
// Side Effects    : Changes clause set, changes parent-rlation of
//                   kept copies.
//
/----------------------------------------------------------------------*/

long ClauseSetDeleteCopies(ClauseSet_p set)
{
   long res1, res2;

   assert(set);

   res1 = ClauseSetMarkCopies(set);
   res2 = ClauseSetDeleteMarkedEntries(set);
   assert(res1==res2);

   return res1;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetDeleteNonUnits()
//
//   Remove all non-empty-non-unit-clauses from set, return number of
//   clauses eleminated. 
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

long ClauseSetDeleteNonUnits(ClauseSet_p set)
{
   Clause_p handle;

   assert(set);
   assert(!set->demod_index);
   
   handle = set->anchor->succ;
   while(handle != set->anchor)
   {      
      if(ClauseLiteralNumber(handle)>1)
      {
	 ClauseSetProp(handle,CPDeleteClause);
      }
      else
      {
	 ClauseDelProp(handle,CPDeleteClause);	 
      }
      handle = handle->succ;
   }
   return ClauseSetDeleteMarkedEntries(set);
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetGetTermNodes()
//
//   Count the nodes of terms in the clauses of set as though they
//   were unshared.
//
// Global Variables: -
//
// Side Effects    : Potentially via ClauseWeight()
//
/----------------------------------------------------------------------*/

long ClauseSetGetTermNodes(ClauseSet_p set)
{
   long     res = 0;
   Clause_p handle;
   
   for(handle = set->anchor->succ; handle != set->anchor; handle =
	  handle->succ)
   {
      res += ClauseWeight(handle, 1, 1, 1, 1, 1, true);
   }
   return res;
}

/*-----------------------------------------------------------------------
//
// Function: ClauseSetMarkSOS()
//
//   Mark Set-of-Support clauses in set with CPIsSOS. Return size of
//   SOS. 
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

long ClauseSetMarkSOS(ClauseSet_p set, bool tptp_types)
{
   long     res = 0;
   Clause_p handle;
   
   for(handle = set->anchor->succ; handle != set->anchor; handle =
	  handle->succ)
   {
      if((tptp_types && (ClauseQueryTPTPType(handle) == CPTypeConjecture))||
	 (!tptp_types && (ClauseIsGoal(handle))))
      {
	 ClauseSetProp(handle, CPIsSOS);
	 res++;	    
      }
      else
      {
	 ClauseDelProp(handle, CPIsSOS);
      }
   }
   return res;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetTermSetProp()
//
//   Set prop in all term nodes in clause set.
//
// Global Variables: -
//
// Side Effects    : No serious ones
//
/----------------------------------------------------------------------*/

void ClauseSetTermSetProp(ClauseSet_p set, TermProperties prop)
{
   Clause_p handle;
   
   for(handle = set->anchor->succ; handle != set->anchor; handle =
	  handle->succ)
   {
      ClauseTermSetProp(handle, prop);
   }
}

/*-----------------------------------------------------------------------
//
// Function: ClauseSetTBTermPropDelCount()
//
//   Delete prop in all term cells, return number of props encountered
//
// Global Variables: -
//
// Side Effects    : No serious ones
//
/----------------------------------------------------------------------*/

long ClauseSetTBTermPropDelCount(ClauseSet_p set, TermProperties prop)
{ 
   Clause_p handle;
   long res = 0;
   
   for(handle = set->anchor->succ; handle != set->anchor; handle =
	  handle->succ)
   {
      res += ClauseTBTermDelPropCount(handle, prop);
   }
   return res;
}  
   
/*-----------------------------------------------------------------------
//
// Function: ClauseSetGetSharedTermNodes()
//
//   Return the number of shared term nodes used by set.
//
// Global Variables: -
//
// Side Effects    : Changes TPOpFlag
//
/----------------------------------------------------------------------*/

long ClauseSetGetSharedTermNodes(ClauseSet_p set)
{
   long res;

   ClauseSetTermSetProp(set, TPOpFlag);
   res = ClauseSetTBTermPropDelCount(set, TPOpFlag);

   return res;
}



/*-----------------------------------------------------------------------
//
// Function: ClauseSetParseList()
//
//   Parse a list of clauses into the set. Clauses are not
//   evaluated. Returns number of clauses parsed.
//
// Global Variables: Only for parsing of terms.
//
// Side Effects    : Input, memory operations, changes set.
//
/----------------------------------------------------------------------*/

long ClauseSetParseList(Scanner_p in, ClauseSet_p set, TB_p bank)
{
   long     count = 0;
   Clause_p clause;
   
   while(ClauseStartsMaybe(in))
   {
      clause = ClauseParse(in, bank);
      count++;
      ClauseSetInsert(set, clause);
   }
   return count;
}



/*-----------------------------------------------------------------------
//
// Function: ClauseSetMarkMaximalTerms()
//
//   Orient all literals and mark all maximal terms and literals in
//   the set.
//
// Global Variables: -
//
// Side Effects    : Orients and marks literals
//
/----------------------------------------------------------------------*/

void ClauseSetMarkMaximalTerms(OCB_p ocb, ClauseSet_p set)
{
   Clause_p handle;

   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      ClauseMarkMaximalTerms(ocb, handle);
   }
}



/*-----------------------------------------------------------------------
//
// Function: ClauseSetListGetMaxDate()
//
//   Return the oldest date of a first limit elements from set of
//   demodulators in the array demodulators. 
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

SysDate ClauseSetListGetMaxDate(ClauseSet_p *demodulators, int limit)
{
   int i;
   SysDate res = SysDateCreationTime();
   
   for(i=0; i<limit; i++)
   {
      assert(demodulators[i]);
      res = SysDateMaximum(res, demodulators[i]->date);
   }
   return res;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFind()
//
//   Given a clause and a clause set, try to find the clause in the
//   set. This is only useful for debugging, as usually clause should
//   know about the set it is in!
//
// Global Variables: -
//
// Side Effects    : - 
//
/----------------------------------------------------------------------*/

Clause_p ClauseSetFind(ClauseSet_p set, Clause_p clause)
{
   Clause_p handle, res = NULL;
   
   assert(set);
   assert(clause);

   for(handle = set->anchor->succ; handle!=set->anchor;
       handle=handle->succ)
   {
      if(handle == clause)
      {
	 res = handle;
	 assert(clause->set == set);
	 break;
      }
   }
   return res;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFindById()
//
//   Given a clause ident and a clause set, try to find the clause in
//   the set. 
//
// Global Variables: -
//
// Side Effects    : - 
//
/----------------------------------------------------------------------*/

Clause_p ClauseSetFindById(ClauseSet_p set, long ident)
{
   Clause_p handle, res = NULL;
   
   assert(set);

   for(handle = set->anchor->succ; handle!=set->anchor;
       handle=handle->succ)
   {
      if(handle->ident == ident)
      {
	 res = handle;
	 assert(handle->set == set);
	 break;
      }
   }
   return res;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetRemoveEvaluations()
//
//   Remove all evaluations from the clauses in set.
//
// Global Variables: -
//
// Side Effects    : Memory operations...
//
/----------------------------------------------------------------------*/

void ClauseSetRemoveEvaluations(ClauseSet_p set)
{
   int i;
   Clause_p handle;
   for(i=0; i<set->eval_indices->size; i++)
   {
      EvalTreeFree(PDArrayElementP(set->eval_indices, i));
      PDArrayAssignP(set->eval_indices, i, NULL);
   }
   for(handle = set->anchor->succ; handle!=set->anchor;
       handle=handle->succ)
   {
      handle->evaluations = NULL;
   }
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFilterTrivial()
//
//   Given a clause set, remove all trivial tautologies from
//   it. Return number of clauses removed.
//
// Global Variables: -
//
// Side Effects    : Changes set.
//
/----------------------------------------------------------------------*/

long ClauseSetFilterTrivial(ClauseSet_p set)
{
   Clause_p handle, next;
   long count = 0;

   assert(set);
   assert(!set->demod_index);
   
   handle = set->anchor->succ;
   while(handle != set->anchor)
   {      
      next = handle->succ;
      
      assert(handle);

      if(ClauseIsTrivial(handle))
      {
	 ClauseDetachParents(handle);	 
	 ClauseSetDeleteEntry(handle);
	 count++;
      }
      handle = next;
   }
   return count;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFilterTautologies()
//
//   Given a clause set, remove all tautologies from it. Return number
//   of clauses removed.
//
// Global Variables: -
//
// Side Effects    : Changes set.
//
/----------------------------------------------------------------------*/

long ClauseSetFilterTautologies(ClauseSet_p set, TB_p work_bank)
{
   Clause_p handle, next;
   long count = 0;

   assert(set);
   assert(!set->demod_index);
   
   handle = set->anchor->succ;
   while(handle != set->anchor)
   {      
      next = handle->succ;
      
      assert(handle);

      if(ClauseIsTautology(work_bank, handle))
      {
	 ClauseDetachParents(handle);	 
	 ClauseSetDeleteEntry(handle);
	 count++;
      }
      handle = next;
   }
   return count;
}

/*-----------------------------------------------------------------------
//
// Function: ClauseSetFindMaxStandardWeight()
//
//   Return a pointer to a clause with the largest standard weight
//   among clauses in set (or NULL if set is empty).
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

Clause_p ClauseSetFindMaxStandardWeight(ClauseSet_p set)
{
   long max_weight = 0, weight;
   Clause_p handle, res = NULL;
   
   assert(set);
   
   handle = set->anchor->succ;
   while(handle != set->anchor)
   {  
      weight = ClauseStandardWeight(handle);
      if(weight > max_weight)
      {
	 res = handle;
	 max_weight = weight;
      }
      handle = handle->succ;
   }
   return res;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFindEqDefinition()
//
//   If set contains an equality definition, return the potential
//   matching side (as a reduced clause position), otherwise NULL.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

ClausePos_p ClauseSetFindEqDefinition(ClauseSet_p set, int min_arity)
{
   Clause_p handle;
   ClausePos_p res = NULL;
   EqnSide side;

   for(handle = set->anchor->succ;
       handle!=set->anchor;
       handle = handle->succ)
   {
      side = ClauseIsEqDefinition(handle, min_arity);
      if(side!=NoSide)
      {
	 res = ClausePosCellAlloc();
	 res->clause = handle;
	 res->literal = handle->literals;
	 res->side = side;
	 res->pos = NULL;
	 break;
      }
   }
   /* if(res)
   {
      printf("# EqDef found: ");
      ClausePrint(stdout, res->clause, true);
      printf(" Side %d\n", res->side);
      } */
   return res;
}

/*-----------------------------------------------------------------------
//
// Function: ClausesSetDocInital()
//
//   If level >= 2, print all clauses as axioms.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

void ClauseSetDocInital(FILE* out, long level, ClauseSet_p set)
{
   Clause_p handle;
   
   if(level>=2)
   {
      for(handle = set->anchor->succ; handle!=set->anchor; handle =
	     handle->succ)
      {
	 DocClauseCreationDefault(handle, inf_initial, NULL, NULL);
      }
   }   
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetDocQuote()
//
//   Quote all clauses in set.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

void ClauseSetDocQuote(FILE* out, long level, ClauseSet_p set,
			      char* comment)
{
   Clause_p handle;
   
   if(level>=2)
   {
      for(handle = set->anchor->succ; handle!=set->anchor; handle =
	     handle->succ)
      {
	 DocClauseQuote(out, level, 2, handle, "final" , NULL);
      }
   }      
}

#ifndef NDBUG

/*-----------------------------------------------------------------------
//
// Function: ClauseSetVerifyDemod()
//
//   Return true if pos->clause is in clause set, is a demodulator,
//   and if pos describes a potential maximal side in clause.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

bool ClauseSetVerifyDemod(ClauseSet_p demods, ClausePos_p pos)
{
   assert(demods);
   assert(pos);
   assert(TermPosIsTopPos(pos->pos));

   if(!ClauseSetFind(demods, pos->clause))
   {
      return false;
   }
   if(!ClauseIsDemodulator(pos->clause))
   {
      return false;
   }
   if((pos->side == RightSide) && EqnIsOriented(pos->clause->literals))
   {
      return false;
   }
   return true;      
}


/*-----------------------------------------------------------------------
//
// Function: PDTreeVerifyIndex()
//
//   Check if all clauses in index are in demod as well.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

bool PDTreeVerifyIndex(PDTree_p tree, ClauseSet_p demods)
{
   PStack_p stack = PStackAlloc(), trav;
   PDTNode_p handle;
   int              i;
   ClausePos_p      pos;
   PTree_p          entry;
   bool             res = true;   
      
   PStackPushP(stack, tree->tree);
   
   while(!PStackEmpty(stack))
   {
      handle = PStackPopP(stack);
      
      if(!handle->entries)
      {
	 for(i=1; i<handle->max_fun; i++)
	 {
	    if(PDArrayElementP(handle->f_alternatives, i))
	    {
	       PStackPushP(stack, PDArrayElementP(handle->f_alternatives, i));
	    }
	 }
	 for(i=1; i<=handle->max_var; i++)
	 {
	    if(PDArrayElementP(handle->v_alternatives, i))
	    {
	       PStackPushP(stack, PDArrayElementP(handle->v_alternatives, i));
	    }
	 }
      }
      else
      {
	 trav = PTreeTraverseInit(handle->entries);
	 while((entry = PTreeTraverseNext(trav)))
	 {
	    pos = entry->key;
	    if(!ClauseSetFind(demods, pos->clause))
	    {
	       res = false;
	       /* printf("Fehler: %d\n", (int)pos->clause);
		  ClausePrint(stdout, pos->clause, true);
		  printf("\n"); */
	    }
	 }
	 PTreeTraverseExit(trav);
	 for(i=0; i<handle->max_fun; i++)
	 {
	    assert(!PDArrayElementP(handle->f_alternatives, i));
	 }
	 for(i=0; i<handle->max_var; i++)
	 {
	    assert(!PDArrayElementP(handle->v_alternatives, i));
	 }
      }
   }
   PStackFree(stack);

   return res;
}

#endif

/*-----------------------------------------------------------------------
//
// Function: EqAxiomsPrint()
//
//   Print the equality axioms (symmetry, transitivity, refexivity,
//   substitutivity) for the given signature.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void EqAxiomsPrint(FILE* out, Sig_p sig, bool single_subst)
{
   FunCode i;
   int arity;

   if(TPTPFormatPrint)      
   {
      fprintf(out, 
	      "input_clause(eq_reflexive, axiom, [++equal(X,X)]).\n"
	      "input_clause(eq_symmetric, axiom,"
	      " [++equal(X,Y),--equal(Y,X)]).\n"
	      "input_clause(eq_transitive, axiom,"
	      " [++equal(X,Z),--equal(X,Y),--equal(Y,Z)]).\n");
      for(i=sig->internal_symbols+1; i<=sig->f_count; i++)
      {
	 if((arity=SigFindArity(sig, i)))
	 {
	    if(SigIsPredicate(sig,i))
	    {
	       tptp_eq_pred_axiom_print(out, SigFindName(sig,i),
					arity, single_subst);
	    }
	    else
	    {
	       tptp_eq_func_axiom_print(out, SigFindName(sig,i),
					arity, single_subst);
	    }
	 }
      }      
   }
   else
   {
      fprintf(out, 
	      "equal(X,X) <- .\n"
	      "equal(X,Y) <- equal(Y,X).\n"
	      "equal(X,Z) <- equal(X,Y), equal(Y,Z).\n");
      for(i=sig->internal_symbols+1; i<=sig->f_count; i++)
      {
	 if((arity=SigFindArity(sig, i)))
	 {
	    if(SigIsPredicate(sig,i))
	    {
	       eq_pred_axiom_print(out, SigFindName(sig,i), arity,
				   single_subst);
	    }
	    else
	    {
	       eq_func_axiom_print(out, SigFindName(sig,i), arity,
				   single_subst);
	    }
	 }
      }
   }
}



/*-----------------------------------------------------------------------
//
// Function: ClauseSetAddSymbolDistribution()
//
//   Count the occurrences of function symbols in set.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ClauseSetAddSymbolDistribution(ClauseSet_p set, long *dist_array)
{
   Clause_p handle;
   
   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      ClauseAddSymbolDistribution(handle, dist_array);
   }
}



/*-----------------------------------------------------------------------
//
// Function: ClauseSetComputeFunctionRanks()
//
//   Assign to each function symbol a uniq number based on its
//   position in the clause set.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ClauseSetComputeFunctionRanks(ClauseSet_p set, long *rank_array, 
				    long* count)
{
   Clause_p handle;
   
   for(handle = set->anchor->succ; handle!=set->anchor; handle =
	  handle->succ)
   {
      ClauseComputeFunctionRanks(handle, rank_array, count);
   }
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFindFreqSymbol()
//
//   Find the most/least frequent non-special, non-predicate symbol of the
//   given arity in the clause set.
//
// Global Variables: - 
//
// Side Effects    : Memory Operations
//
/----------------------------------------------------------------------*/

FunCode ClauseSetFindFreqSymbol(ClauseSet_p set, Sig_p sig, int arity,
				bool least)
{
   FunCode       i, selected=0;
   long          *dist_array,freq=least?LONG_MAX:0;
   
   dist_array = SizeMalloc((sig->f_count+1) * sizeof(long));

   for(i=0; i<=sig->f_count; i++)
   {
      dist_array[i] = 0;
   }      
   ClauseSetAddSymbolDistribution(set, dist_array);
   
   for(i=sig->internal_symbols+1; i<= sig->f_count; i++)
   {
      if((SigFindArity(sig,i)==arity) && !SigIsPredicate(sig,i) &&
	 !SigIsSpecial(sig,i))
      {
	 if((least && (dist_array[i]<=freq)) ||
	    (dist_array[i]>=freq))
	 {
	    freq = dist_array[i];
	    selected = i;
	 }
      }
   }
   SizeFree(dist_array, (sig->f_count+1)*sizeof(long));      

   return selected;
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetMaxVarNumber()
//
//   Return the largest number of variables occurring in any clause.
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

long ClauseSetMaxVarNumber(ClauseSet_p set)
{
   long res=0, tmp;
   PTree_p tree;
   Clause_p handle;

   for(handle = set->anchor->succ; handle!= set->anchor; handle =
	  handle->succ)
   {
      tree = NULL;
      tmp = ClauseCollectVariables(handle, &tree);
      PTreeFree(tree);
      res = MAX(res, tmp);
   }
   return res;   
}


/*-----------------------------------------------------------------------
//
// Function: ClauseSetFindCharFreqVectors()
//
//   Compute the characteristic frequency vectors for set. Vectors are
//   re-initialized. Returns number of clauses in set.
//
// Global Variables: -
//
// Side Effects    : Memory operations.
//
/----------------------------------------------------------------------*/

long ClauseSetFindCharFreqVectors(ClauseSet_p set, FreqVector_p fsum,
				  FreqVector_p fmax, FreqVector_p fmin, 
				  long symbol_size)
{
   Clause_p handle;
   FreqVector_p current;

   assert(set && fsum && fmax && fmin);
 
   FreqVectorInitialize(fsum, 0);
   FreqVectorInitialize(fmax, 0);
   FreqVectorInitialize(fmin, LONG_MAX);
   
   for(handle = set->anchor->succ;
       handle!= set->anchor;
       handle = handle->succ)
   {
      current = StandardFreqVectorCompute(handle, symbol_size);
      FreqVectorAdd(fsum, fsum, current);
      FreqVectorMax(fmax, fmax, current);
      FreqVectorMin(fmin, fmin, current);
      FreqVectorFree(current);
   }   
   return set->members;
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/


