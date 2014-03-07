/*
**********************************************************************
*   Copyright (c) 2002-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*/
//
//  rbbitblb.cpp
//


#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unistr.h"
#include "rbbitblb.h"
#include "rbbirb.h"
#include "rbbisetb.h"
#include "rbbidata.h"
#include "cstring.h"
#include "uassert.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

RBBITableBuilder::RBBITableBuilder(RBBIRuleBuilder *rb, RBBINode **rootNode) :
 fTree(*rootNode) {
    fRB                 = rb;
    fStatus             = fRB->fStatus;
    UErrorCode status   = U_ZERO_ERROR;
    fDStates            = new UVector(status);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    if (U_FAILURE(status)) {
        *fStatus = status;
        return;
    }
    if (fDStates == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;;
    }
}



RBBITableBuilder::~RBBITableBuilder() {
    int i;
    for (i=0; i<fDStates->size(); i++) {
        delete (RBBIStateDescriptor *)fDStates->elementAt(i);
    }
    delete   fDStates;
}


//-----------------------------------------------------------------------------
//
//   RBBITableBuilder::build  -  This is the main function for building the DFA state transtion
//                               table from the RBBI rules parse tree.
//
//-----------------------------------------------------------------------------
void  RBBITableBuilder::build() {

    if (U_FAILURE(*fStatus)) {
        return;
    }

    // If there were no rules, just return.  This situation can easily arise
    //   for the reverse rules.
    if (fTree==NULL) {
        return;
    }

    //
    // Walk through the tree, replacing any references to $variables with a copy of the
    //   parse tree for the substition expression.
    //
    fTree = fTree->flattenVariables();
#ifdef RBBI_DEBUG
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "ftree")) {
        RBBIDebugPuts("Parse tree after flattening variable references.");
        fTree->printTree(TRUE);
    }
#endif

    //
    // If the rules contained any references to {bof} 
    //   add a {bof} <cat> <former root of tree> to the
    //   tree.  Means that all matches must start out with the 
    //   {bof} fake character.
    // 
    if (fRB->fSetBuilder->sawBOF()) {
        RBBINode *bofTop    = new RBBINode(RBBINode::opCat);
        RBBINode *bofLeaf   = new RBBINode(RBBINode::leafChar);
        // Delete and exit if memory allocation failed.
        if (bofTop == NULL || bofLeaf == NULL) {
            *fStatus = U_MEMORY_ALLOCATION_ERROR;
            delete bofTop;
            delete bofLeaf;
            return;
        }
        bofTop->fLeftChild  = bofLeaf;
        bofTop->fRightChild = fTree;
        bofLeaf->fParent    = bofTop;
        bofLeaf->fVal       = 2;      // Reserved value for {bof}.
        fTree               = bofTop;
    }

    //
    // Add a unique right-end marker to the expression.
    //   Appears as a cat-node, left child being the original tree,
    //   right child being the end marker.
    //
    RBBINode *cn = new RBBINode(RBBINode::opCat);
    // Exit if memory allocation failed.
    if (cn == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    cn->fLeftChild = fTree;
    fTree->fParent = cn;
    cn->fRightChild = new RBBINode(RBBINode::endMark);
    // Delete and exit if memory allocation failed.
    if (cn->fRightChild == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        delete cn;
        return;
    }
    cn->fRightChild->fParent = cn;
    fTree = cn;

    //
    //  Replace all references to UnicodeSets with the tree for the equivalent
    //      expression.
    //
    fTree->flattenSets();
#ifdef RBBI_DEBUG
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "stree")) {
        RBBIDebugPuts("Parse tree after flattening Unicode Set references.");
        fTree->printTree(TRUE);
    }
#endif


    //
    // calculate the functions nullable, firstpos, lastpos and followpos on
    // nodes in the parse tree.
    //    See the alogrithm description in Aho.
    //    Understanding how this works by looking at the code alone will be
    //       nearly impossible.
    //
    calcNullable(fTree);
    calcFirstPos(fTree);
    calcLastPos(fTree);
    calcFollowPos(fTree);
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "pos")) {
        RBBIDebugPuts("\n");
        printPosSets(fTree);
    }

    //
    //  For "chained" rules, modify the followPos sets
    //
    if (fRB->fChainRules) {
        calcChainedFollowPos(fTree);
    }

    //
    //  BOF (start of input) test fixup.
    //
    if (fRB->fSetBuilder->sawBOF()) {
        bofFixup();
    }

    //
    // Build the DFA state transition tables.
    //
    buildStateTable();
    flagAcceptingStates();
    flagLookAheadStates();
    flagTaggedStates();

    //
    // Update the global table of rule status {tag} values
    // The rule builder has a global vector of status values that are common
    //    for all tables.  Merge the ones from this table into the global set.
    //
    mergeRuleStatusVals();

    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "states")) {printStates();};
}



//-----------------------------------------------------------------------------
//
//   calcNullable.    Impossible to explain succinctly.  See Aho, section 3.9
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::calcNullable(RBBINode *n) {
    if (n == NULL) {
        return;
    }
    if (n->fType == RBBINode::setRef ||
        n->fType == RBBINode::endMark ) {
        // These are non-empty leaf node types.
        n->fNullable = FALSE;
        return;
    }

    if (n->fType == RBBINode::lookAhead || n->fType == RBBINode::tag) {
        // Lookahead marker node.  It's a leaf, so no recursion on children.
        // It's nullable because it does not match any literal text from the input stream.
        n->fNullable = TRUE;
        return;
    }


    // The node is not a leaf.
    //  Calculate nullable on its children.
    calcNullable(n->fLeftChild);
    calcNullable(n->fRightChild);

    // Apply functions from table 3.40 in Aho
    if (n->fType == RBBINode::opOr) {
        n->fNullable = n->fLeftChild->fNullable || n->fRightChild->fNullable;
    }
    else if (n->fType == RBBINode::opCat) {
        n->fNullable = n->fLeftChild->fNullable && n->fRightChild->fNullable;
    }
    else if (n->fType == RBBINode::opStar || n->fType == RBBINode::opQuestion) {
        n->fNullable = TRUE;
    }
    else {
        n->fNullable = FALSE;
    }
}




//-----------------------------------------------------------------------------
//
//   calcFirstPos.    Impossible to explain succinctly.  See Aho, section 3.9
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::calcFirstPos(RBBINode *n) {
    if (n == NULL) {
        return;
    }
    if (n->fType == RBBINode::leafChar  ||
        n->fType == RBBINode::endMark   ||
        n->fType == RBBINode::lookAhead ||
        n->fType == RBBINode::tag) {
        // These are non-empty leaf node types.
        // Note: In order to maintain the sort invariant on the set,
        // this function should only be called on a node whose set is
        // empty to start with.
        n->fFirstPosSet->addElement(n, *fStatus);
        return;
    }

    // The node is not a leaf.
    //  Calculate firstPos on its children.
    calcFirstPos(n->fLeftChild);
    calcFirstPos(n->fRightChild);

    // Apply functions from table 3.40 in Aho
    if (n->fType == RBBINode::opOr) {
        setAdd(n->fFirstPosSet, n->fLeftChild->fFirstPosSet);
        setAdd(n->fFirstPosSet, n->fRightChild->fFirstPosSet);
    }
    else if (n->fType == RBBINode::opCat) {
        setAdd(n->fFirstPosSet, n->fLeftChild->fFirstPosSet);
        if (n->fLeftChild->fNullable) {
            setAdd(n->fFirstPosSet, n->fRightChild->fFirstPosSet);
        }
    }
    else if (n->fType == RBBINode::opStar ||
             n->fType == RBBINode::opQuestion ||
             n->fType == RBBINode::opPlus) {
        setAdd(n->fFirstPosSet, n->fLeftChild->fFirstPosSet);
    }
}



//-----------------------------------------------------------------------------
//
//   calcLastPos.    Impossible to explain succinctly.  See Aho, section 3.9
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::calcLastPos(RBBINode *n) {
    if (n == NULL) {
        return;
    }
    if (n->fType == RBBINode::leafChar  ||
        n->fType == RBBINode::endMark   ||
        n->fType == RBBINode::lookAhead ||
        n->fType == RBBINode::tag) {
        // These are non-empty leaf node types.
        // Note: In order to maintain the sort invariant on the set,
        // this function should only be called on a node whose set is
        // empty to start with.
        n->fLastPosSet->addElement(n, *fStatus);
        return;
    }

    // The node is not a leaf.
    //  Calculate lastPos on its children.
    calcLastPos(n->fLeftChild);
    calcLastPos(n->fRightChild);

    // Apply functions from table 3.40 in Aho
    if (n->fType == RBBINode::opOr) {
        setAdd(n->fLastPosSet, n->fLeftChild->fLastPosSet);
        setAdd(n->fLastPosSet, n->fRightChild->fLastPosSet);
    }
    else if (n->fType == RBBINode::opCat) {
        setAdd(n->fLastPosSet, n->fRightChild->fLastPosSet);
        if (n->fRightChild->fNullable) {
            setAdd(n->fLastPosSet, n->fLeftChild->fLastPosSet);
        }
    }
    else if (n->fType == RBBINode::opStar     ||
             n->fType == RBBINode::opQuestion ||
             n->fType == RBBINode::opPlus) {
        setAdd(n->fLastPosSet, n->fLeftChild->fLastPosSet);
    }
}



//-----------------------------------------------------------------------------
//
//   calcFollowPos.    Impossible to explain succinctly.  See Aho, section 3.9
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::calcFollowPos(RBBINode *n) {
    if (n == NULL ||
        n->fType == RBBINode::leafChar ||
        n->fType == RBBINode::endMark) {
        return;
    }

    calcFollowPos(n->fLeftChild);
    calcFollowPos(n->fRightChild);

    // Aho rule #1
    if (n->fType == RBBINode::opCat) {
        RBBINode *i;   // is 'i' in Aho's description
        uint32_t     ix;

        UVector *LastPosOfLeftChild = n->fLeftChild->fLastPosSet;

        for (ix=0; ix<(uint32_t)LastPosOfLeftChild->size(); ix++) {
            i = (RBBINode *)LastPosOfLeftChild->elementAt(ix);
            setAdd(i->fFollowPos, n->fRightChild->fFirstPosSet);
        }
    }

    // Aho rule #2
    if (n->fType == RBBINode::opStar ||
        n->fType == RBBINode::opPlus) {
        RBBINode   *i;  // again, n and i are the names from Aho's description.
        uint32_t    ix;

        for (ix=0; ix<(uint32_t)n->fLastPosSet->size(); ix++) {
            i = (RBBINode *)n->fLastPosSet->elementAt(ix);
            setAdd(i->fFollowPos, n->fFirstPosSet);
        }
    }



}


//-----------------------------------------------------------------------------
//
//   calcChainedFollowPos.    Modify the previously calculated followPos sets
//                            to implement rule chaining.  NOT described by Aho
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::calcChainedFollowPos(RBBINode *tree) {

    UVector         endMarkerNodes(*fStatus);
    UVector         leafNodes(*fStatus);
    int32_t         i;

    if (U_FAILURE(*fStatus)) {
        return;
    }

    // get a list of all endmarker nodes.
    tree->findNodes(&endMarkerNodes, RBBINode::endMark, *fStatus);

    // get a list all leaf nodes
    tree->findNodes(&leafNodes, RBBINode::leafChar, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }

    // Get all nodes that can be the start a match, which is FirstPosition()
    // of the portion of the tree corresponding to user-written rules.
    // See the tree description in bofFixup().
    RBBINode *userRuleRoot = tree;
    if (fRB->fSetBuilder->sawBOF()) {
        userRuleRoot = tree->fLeftChild->fRightChild;
    }
    U_ASSERT(userRuleRoot != NULL);
    UVector *matchStartNodes = userRuleRoot->fFirstPosSet;


    // Iteratate over all leaf nodes,
    //
    int32_t  endNodeIx;
    int32_t  startNodeIx;

    for (endNodeIx=0; endNodeIx<leafNodes.size(); endNodeIx++) {
        RBBINode *tNode   = (RBBINode *)leafNodes.elementAt(endNodeIx);
        RBBINode *endNode = NULL;

        // Identify leaf nodes that correspond to overall rule match positions.
        //   These include an endMarkerNode in their followPos sets.
        for (i=0; i<endMarkerNodes.size(); i++) {
            if (tNode->fFollowPos->contains(endMarkerNodes.elementAt(i))) {
                endNode = tNode;
                break;
            }
        }
        if (endNode == NULL) {
            // node wasn't an end node.  Try again with the next.
            continue;
        }

        // We've got a node that can end a match.

        // Line Break Specific hack:  If this node's val correspond to the $CM char class,
        //                            don't chain from it.
        // TODO:  Add rule syntax for this behavior, get specifics out of here and
        //        into the rule file.
        if (fRB->fLBCMNoChain) {
            UChar32 c = this->fRB->fSetBuilder->getFirstChar(endNode->fVal);
            if (c != -1) {
                // c == -1 occurs with sets containing only the {eof} marker string.
                ULineBreak cLBProp = (ULineBreak)u_getIntPropertyValue(c, UCHAR_LINE_BREAK);
                if (cLBProp == U_LB_COMBINING_MARK) {
                    continue;
                }
            }
        }


        // Now iterate over the nodes that can start a match, looking for ones
        //   with the same char class as our ending node.
        RBBINode *startNode;
        for (startNodeIx = 0; startNodeIx<matchStartNodes->size(); startNodeIx++) {
            startNode = (RBBINode *)matchStartNodes->elementAt(startNodeIx);
            if (startNode->fType != RBBINode::leafChar) {
                continue;
            }

            if (endNode->fVal == startNode->fVal) {
                // The end val (character class) of one possible match is the
                //   same as the start of another.

                // Add all nodes from the followPos of the start node to the
                //  followPos set of the end node, which will have the effect of
                //  letting matches transition from a match state at endNode
                //  to the second char of a match starting with startNode.
                setAdd(endNode->fFollowPos, startNode->fFollowPos);
            }
        }
    }
}


//-----------------------------------------------------------------------------
//
//   bofFixup.    Fixup for state tables that include {bof} beginning of input testing.
//                Do an swizzle similar to chaining, modifying the followPos set of
//                the bofNode to include the followPos nodes from other {bot} nodes
//                scattered through the tree.
//
//                This function has much in common with calcChainedFollowPos().
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::bofFixup() {

    if (U_FAILURE(*fStatus)) {
        return;
    }

    //   The parse tree looks like this ...
    //         fTree root  --->       <cat>
    //                               /     \       .
    //                            <cat>   <#end node>
    //                           /     \  .
    //                     <bofNode>   rest
    //                               of tree
    //
    //    We will be adding things to the followPos set of the <bofNode>
    //
    RBBINode  *bofNode = fTree->fLeftChild->fLeftChild;
    U_ASSERT(bofNode->fType == RBBINode::leafChar);
    U_ASSERT(bofNode->fVal == 2);

    // Get all nodes that can be the start a match of the user-written rules
    //  (excluding the fake bofNode)
    //  We want the nodes that can start a match in the
    //     part labeled "rest of tree"
    // 
    UVector *matchStartNodes = fTree->fLeftChild->fRightChild->fFirstPosSet;

    RBBINode *startNode;
    int       startNodeIx;
    for (startNodeIx = 0; startNodeIx<matchStartNodes->size(); startNodeIx++) {
        startNode = (RBBINode *)matchStartNodes->elementAt(startNodeIx);
        if (startNode->fType != RBBINode::leafChar) {
            continue;
        }

        if (startNode->fVal == bofNode->fVal) {
            //  We found a leaf node corresponding to a {bof} that was
            //    explicitly written into a rule.
            //  Add everything from the followPos set of this node to the
            //    followPos set of the fake bofNode at the start of the tree.
            //  
            setAdd(bofNode->fFollowPos, startNode->fFollowPos);
        }
    }
}

//-----------------------------------------------------------------------------
//
//   buildStateTable()    Determine the set of runtime DFA states and the
//                        transition tables for these states, by the algorithm
//                        of fig. 3.44 in Aho.
//
//                        Most of the comments are quotes of Aho's psuedo-code.
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::buildStateTable() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    RBBIStateDescriptor *failState;
    // Set it to NULL to avoid uninitialized warning
    RBBIStateDescriptor *initialState = NULL; 
    //
    // Add a dummy state 0 - the stop state.  Not from Aho.
    int      lastInputSymbol = fRB->fSetBuilder->getNumCharCategories() - 1;
    failState = new RBBIStateDescriptor(lastInputSymbol, fStatus);
    if (failState == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        goto ExitBuildSTdeleteall;
    }
    failState->fPositions = new UVector(*fStatus);
    if (failState->fPositions == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
    }
    if (failState->fPositions == NULL || U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }
    fDStates->addElement(failState, *fStatus);
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }

    // initially, the only unmarked state in Dstates is firstpos(root),
    //       where toot is the root of the syntax tree for (r)#;
    initialState = new RBBIStateDescriptor(lastInputSymbol, fStatus);
    if (initialState == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }
    initialState->fPositions = new UVector(*fStatus);
    if (initialState->fPositions == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }
    setAdd(initialState->fPositions, fTree->fFirstPosSet);
    fDStates->addElement(initialState, *fStatus);
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }

    // while there is an unmarked state T in Dstates do begin
    for (;;) {
        RBBIStateDescriptor *T = NULL;
        int32_t              tx;
        for (tx=1; tx<fDStates->size(); tx++) {
            RBBIStateDescriptor *temp;
            temp = (RBBIStateDescriptor *)fDStates->elementAt(tx);
            if (temp->fMarked == FALSE) {
                T = temp;
                break;
            }
        }
        if (T == NULL) {
            break;
        }

        // mark T;
        T->fMarked = TRUE;

        // for each input symbol a do begin
        int32_t  a;
        for (a = 1; a<=lastInputSymbol; a++) {
            // let U be the set of positions that are in followpos(p)
            //    for some position p in T
            //    such that the symbol at position p is a;
            UVector    *U = NULL;
            RBBINode   *p;
            int32_t     px;
            for (px=0; px<T->fPositions->size(); px++) {
                p = (RBBINode *)T->fPositions->elementAt(px);
                if ((p->fType == RBBINode::leafChar) &&  (p->fVal == a)) {
                    if (U == NULL) {
                        U = new UVector(*fStatus);
                        if (U == NULL) {
                        	*fStatus = U_MEMORY_ALLOCATION_ERROR;
                        	goto ExitBuildSTdeleteall;
                        }
                    }
                    setAdd(U, p->fFollowPos);
                }
            }

            // if U is not empty and not in DStates then
            int32_t  ux = 0;
            UBool    UinDstates = FALSE;
            if (U != NULL) {
                U_ASSERT(U->size() > 0);
                int  ix;
                for (ix=0; ix<fDStates->size(); ix++) {
                    RBBIStateDescriptor *temp2;
                    temp2 = (RBBIStateDescriptor *)fDStates->elementAt(ix);
                    if (setEquals(U, temp2->fPositions)) {
                        delete U;
                        U  = temp2->fPositions;
                        ux = ix;
                        UinDstates = TRUE;
                        break;
                    }
                }

                // Add U as an unmarked state to Dstates
                if (!UinDstates)
                {
                    RBBIStateDescriptor *newState = new RBBIStateDescriptor(lastInputSymbol, fStatus);
                    if (newState == NULL) {
                    	*fStatus = U_MEMORY_ALLOCATION_ERROR;
                    }
                    if (U_FAILURE(*fStatus)) {
                        goto ExitBuildSTdeleteall;
                    }
                    newState->fPositions = U;
                    fDStates->addElement(newState, *fStatus);
                    if (U_FAILURE(*fStatus)) {
                        return;
                    }
                    ux = fDStates->size()-1;
                }

                // Dtran[T, a] := U;
                T->fDtran->setElementAt(ux, a);
            }
        }
    }
    return;
    // delete local pointers only if error occured.
ExitBuildSTdeleteall:
    delete initialState;
    delete failState;
}



//-----------------------------------------------------------------------------
//
//   flagAcceptingStates    Identify accepting states.
//                          First get a list of all of the end marker nodes.
//                          Then, for each state s,
//                              if s contains one of the end marker nodes in its list of tree positions then
//                                  s is an accepting state.
//
//-----------------------------------------------------------------------------
void     RBBITableBuilder::flagAcceptingStates() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    UVector     endMarkerNodes(*fStatus);
    RBBINode    *endMarker;
    int32_t     i;
    int32_t     n;

    if (U_FAILURE(*fStatus)) {
        return;
    }

    fTree->findNodes(&endMarkerNodes, RBBINode::endMark, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }

    for (i=0; i<endMarkerNodes.size(); i++) {
        endMarker = (RBBINode *)endMarkerNodes.elementAt(i);
        for (n=0; n<fDStates->size(); n++) {
            RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
            if (sd->fPositions->indexOf(endMarker) >= 0) {
                // Any non-zero value for fAccepting means this is an accepting node.
                // The value is what will be returned to the user as the break status.
                // If no other value was specified, force it to -1.

                if (sd->fAccepting==0) {
                    // State hasn't been marked as accepting yet.  Do it now.
                    sd->fAccepting = endMarker->fVal;
                    if (sd->fAccepting == 0) {
                        sd->fAccepting = -1;
                    }
                }
                if (sd->fAccepting==-1 && endMarker->fVal != 0) {
                    // Both lookahead and non-lookahead accepting for this state.
                    // Favor the look-ahead.  Expedient for line break.
                    // TODO:  need a more elegant resolution for conflicting rules.
                    sd->fAccepting = endMarker->fVal;
                }
                // implicit else:
                // if sd->fAccepting already had a value other than 0 or -1, leave it be.

                // If the end marker node is from a look-ahead rule, set
                //   the fLookAhead field or this state also.
                if (endMarker->fLookAheadEnd) {
                    // TODO:  don't change value if already set?
                    // TODO:  allow for more than one active look-ahead rule in engine.
                    //        Make value here an index to a side array in engine?
                    sd->fLookAhead = sd->fAccepting;
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
//
//    flagLookAheadStates   Very similar to flagAcceptingStates, above.
//
//-----------------------------------------------------------------------------
void     RBBITableBuilder::flagLookAheadStates() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    UVector     lookAheadNodes(*fStatus);
    RBBINode    *lookAheadNode;
    int32_t     i;
    int32_t     n;

    fTree->findNodes(&lookAheadNodes, RBBINode::lookAhead, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    for (i=0; i<lookAheadNodes.size(); i++) {
        lookAheadNode = (RBBINode *)lookAheadNodes.elementAt(i);

        for (n=0; n<fDStates->size(); n++) {
            RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
            if (sd->fPositions->indexOf(lookAheadNode) >= 0) {
                sd->fLookAhead = lookAheadNode->fVal;
            }
        }
    }
}




//-----------------------------------------------------------------------------
//
//    flagTaggedStates
//
//-----------------------------------------------------------------------------
void     RBBITableBuilder::flagTaggedStates() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    UVector     tagNodes(*fStatus);
    RBBINode    *tagNode;
    int32_t     i;
    int32_t     n;

    if (U_FAILURE(*fStatus)) {
        return;
    }
    fTree->findNodes(&tagNodes, RBBINode::tag, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    for (i=0; i<tagNodes.size(); i++) {                   // For each tag node t (all of 'em)
        tagNode = (RBBINode *)tagNodes.elementAt(i);

        for (n=0; n<fDStates->size(); n++) {              //    For each state  s (row in the state table)
            RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
            if (sd->fPositions->indexOf(tagNode) >= 0) {  //       if  s include the tag node t
                sortedAdd(&sd->fTagVals, tagNode->fVal);
            }
        }
    }
}




//-----------------------------------------------------------------------------
//
//  mergeRuleStatusVals
//
//      Update the global table of rule status {tag} values
//      The rule builder has a global vector of status values that are common
//      for all tables.  Merge the ones from this table into the global set.
//
//-----------------------------------------------------------------------------
void  RBBITableBuilder::mergeRuleStatusVals() {
    //
    //  The basic outline of what happens here is this...
    //
    //    for each state in this state table
    //       if the status tag list for this state is in the global statuses list
    //           record where and
    //           continue with the next state
    //       else
    //           add the tag list for this state to the global list.
    //
    int i;
    int n;

    // Pre-set a single tag of {0} into the table.
    //   We will need this as a default, for rule sets with no explicit tagging.
    if (fRB->fRuleStatusVals->size() == 0) {
        fRB->fRuleStatusVals->addElement(1, *fStatus);  // Num of statuses in group
        fRB->fRuleStatusVals->addElement((int32_t)0, *fStatus);  //   and our single status of zero
    }

    //    For each state
    for (n=0; n<fDStates->size(); n++) {
        RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
        UVector *thisStatesTagValues = sd->fTagVals;
        if (thisStatesTagValues == NULL) {
            // No tag values are explicitly associated with this state.
            //   Set the default tag value.
            sd->fTagsIdx = 0;
            continue;
        }

        // There are tag(s) associated with this state.
        //   fTagsIdx will be the index into the global tag list for this state's tag values.
        //   Initial value of -1 flags that we haven't got it set yet.
        sd->fTagsIdx = -1;
        int32_t  thisTagGroupStart = 0;   // indexes into the global rule status vals list
        int32_t  nextTagGroupStart = 0;

        // Loop runs once per group of tags in the global list
        while (nextTagGroupStart < fRB->fRuleStatusVals->size()) {
            thisTagGroupStart = nextTagGroupStart;
            nextTagGroupStart += fRB->fRuleStatusVals->elementAti(thisTagGroupStart) + 1;
            if (thisStatesTagValues->size() != fRB->fRuleStatusVals->elementAti(thisTagGroupStart)) {
                // The number of tags for this state is different from
                //    the number of tags in this group from the global list.
                //    Continue with the next group from the global list.
                continue;
            }
            // The lengths match, go ahead and compare the actual tag values
            //    between this state and the group from the global list.
            for (i=0; i<thisStatesTagValues->size(); i++) {
                if (thisStatesTagValues->elementAti(i) !=
                    fRB->fRuleStatusVals->elementAti(thisTagGroupStart + 1 + i) ) {
                    // Mismatch.
                    break;
                }
            }

            if (i == thisStatesTagValues->size()) {
                // We found a set of tag values in the global list that match
                //   those for this state.  Use them.
                sd->fTagsIdx = thisTagGroupStart;
                break;
            }
        }

        if (sd->fTagsIdx == -1) {
            // No suitable entry in the global tag list already.  Add one
            sd->fTagsIdx = fRB->fRuleStatusVals->size();
            fRB->fRuleStatusVals->addElement(thisStatesTagValues->size(), *fStatus);
            for (i=0; i<thisStatesTagValues->size(); i++) {
                fRB->fRuleStatusVals->addElement(thisStatesTagValues->elementAti(i), *fStatus);
            }
        }
    }
}







//-----------------------------------------------------------------------------
//
//  sortedAdd  Add a value to a vector of sorted values (ints).
//             Do not replicate entries; if the value is already there, do not
//                add a second one.
//             Lazily create the vector if it does not already exist.
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::sortedAdd(UVector **vector, int32_t val) {
    int32_t i;

    if (*vector == NULL) {
        *vector = new UVector(*fStatus);
    }
    if (*vector == NULL || U_FAILURE(*fStatus)) {
        return;
    }
    UVector *vec = *vector;
    int32_t  vSize = vec->size();
    for (i=0; i<vSize; i++) {
        int32_t valAtI = vec->elementAti(i);
        if (valAtI == val) {
            // The value is already in the vector.  Don't add it again.
            return;
        }
        if (valAtI > val) {
            break;
        }
    }
    vec->insertElementAt(val, i, *fStatus);
}



//-----------------------------------------------------------------------------
//
//  setAdd     Set operation on UVector
//             dest = dest union source
//             Elements may only appear once and must be sorted.
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::setAdd(UVector *dest, UVector *source) {
    int32_t destOriginalSize = dest->size();
    int32_t sourceSize       = source->size();
    int32_t di           = 0;
    MaybeStackArray<void *, 16> destArray, sourceArray;  // Handle small cases without malloc
    void **destPtr, **sourcePtr;
    void **destLim, **sourceLim;

    if (destOriginalSize > destArray.getCapacity()) {
        if (destArray.resize(destOriginalSize) == NULL) {
            return;
        }
    }
    destPtr = destArray.getAlias();
    destLim = destPtr + destOriginalSize;  // destArray.getArrayLimit()?

    if (sourceSize > sourceArray.getCapacity()) {
        if (sourceArray.resize(sourceSize) == NULL) {
            return;
        }
    }
    sourcePtr = sourceArray.getAlias();
    sourceLim = sourcePtr + sourceSize;  // sourceArray.getArrayLimit()?

    // Avoid multiple "get element" calls by getting the contents into arrays
    (void) dest->toArray(destPtr);
    (void) source->toArray(sourcePtr);

    dest->setSize(sourceSize+destOriginalSize, *fStatus);

    while (sourcePtr < sourceLim && destPtr < destLim) {
        if (*destPtr == *sourcePtr) {
            dest->setElementAt(*sourcePtr++, di++);
            destPtr++;
        }
        // This check is required for machines with segmented memory, like i5/OS.
        // Direct pointer comparison is not recommended.
        else if (uprv_memcmp(destPtr, sourcePtr, sizeof(void *)) < 0) {
            dest->setElementAt(*destPtr++, di++);
        }
        else { /* *sourcePtr < *destPtr */
            dest->setElementAt(*sourcePtr++, di++);
        }
    }

    // At most one of these two cleanup loops will execute
    while (destPtr < destLim) {
        dest->setElementAt(*destPtr++, di++);
    }
    while (sourcePtr < sourceLim) {
        dest->setElementAt(*sourcePtr++, di++);
    }

    dest->setSize(di, *fStatus);
}



//-----------------------------------------------------------------------------
//
//  setEqual    Set operation on UVector.
//              Compare for equality.
//              Elements must be sorted.
//
//-----------------------------------------------------------------------------
UBool RBBITableBuilder::setEquals(UVector *a, UVector *b) {
    return a->equals(*b);
}


//-----------------------------------------------------------------------------
//
//  printPosSets   Debug function.  Dump Nullable, firstpos, lastpos and followpos
//                 for each node in the tree.
//
//-----------------------------------------------------------------------------
#ifdef RBBI_DEBUG
void RBBITableBuilder::printPosSets(RBBINode *n) {
    if (n==NULL) {
        return;
    }
    n->printNode();
    RBBIDebugPrintf("         Nullable:  %s\n", n->fNullable?"TRUE":"FALSE");

    RBBIDebugPrintf("         firstpos:  ");
    printSet(n->fFirstPosSet);

    RBBIDebugPrintf("         lastpos:   ");
    printSet(n->fLastPosSet);

    RBBIDebugPrintf("         followpos: ");
    printSet(n->fFollowPos);

    printPosSets(n->fLeftChild);
    printPosSets(n->fRightChild);
}
#endif



//-----------------------------------------------------------------------------
//
//   getTableSize()    Calculate the size of the runtime form of this
//                     state transition table.
//
//-----------------------------------------------------------------------------
int32_t  RBBITableBuilder::getTableSize() const {
    int32_t    size = 0;
    int32_t    numRows;
    int32_t    numCols;
    int32_t    rowSize;

    if (fTree == NULL) {
        return 0;
    }

    size    = sizeof(RBBIStateTable) - 4;    // The header, with no rows to the table.

    numRows = fDStates->size();
    numCols = fRB->fSetBuilder->getNumCharCategories();

    //  Note  The declaration of RBBIStateTableRow is for a table of two columns.
    //        Therefore we subtract two from numCols when determining
    //        how much storage to add to a row for the total columns.
    rowSize = sizeof(RBBIStateTableRow) + sizeof(uint16_t)*(numCols-2);
    size   += numRows * rowSize;
    return size;
}



//-----------------------------------------------------------------------------
//
//   exportTable()    export the state transition table in the format required
//                    by the runtime engine.  getTableSize() bytes of memory
//                    must be available at the output address "where".
//
//-----------------------------------------------------------------------------
void RBBITableBuilder::exportTable(void *where) {
    RBBIStateTable    *table = (RBBIStateTable *)where;
    uint32_t           state;
    int                col;

    if (U_FAILURE(*fStatus) || fTree == NULL) {
        return;
    }

    if (fRB->fSetBuilder->getNumCharCategories() > 0x7fff ||
        fDStates->size() > 0x7fff) {
        *fStatus = U_BRK_INTERNAL_ERROR;
        return;
    }

    table->fRowLen    = sizeof(RBBIStateTableRow) +
                            sizeof(uint16_t) * (fRB->fSetBuilder->getNumCharCategories() - 2);
    table->fNumStates = fDStates->size();
    table->fFlags     = 0;
    if (fRB->fLookAheadHardBreak) {
        table->fFlags  |= RBBI_LOOKAHEAD_HARD_BREAK;
    }
    if (fRB->fSetBuilder->sawBOF()) {
        table->fFlags  |= RBBI_BOF_REQUIRED;
    }
    table->fReserved  = 0;

    for (state=0; state<table->fNumStates; state++) {
        RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(state);
        RBBIStateTableRow   *row = (RBBIStateTableRow *)(table->fTableData + state*table->fRowLen);
        U_ASSERT (-32768 < sd->fAccepting && sd->fAccepting <= 32767);
        U_ASSERT (-32768 < sd->fLookAhead && sd->fLookAhead <= 32767);
        row->fAccepting = (int16_t)sd->fAccepting;
        row->fLookAhead = (int16_t)sd->fLookAhead;
        row->fTagIdx    = (int16_t)sd->fTagsIdx;
        for (col=0; col<fRB->fSetBuilder->getNumCharCategories(); col++) {
            row->fNextState[col] = (uint16_t)sd->fDtran->elementAti(col);
        }
    }
}



//-----------------------------------------------------------------------------
//
//   printSet    Debug function.   Print the contents of a UVector
//
//-----------------------------------------------------------------------------
#ifdef RBBI_DEBUG
void RBBITableBuilder::printSet(UVector *s) {
    int32_t  i;
    for (i=0; i<s->size(); i++) {
        void *v = s->elementAt(i);
        RBBIDebugPrintf("%10p", v);
    }
    RBBIDebugPrintf("\n");
}
#endif


//-----------------------------------------------------------------------------
//
//   printStates    Debug Function.  Dump the fully constructed state transition table.
//
//-----------------------------------------------------------------------------
#ifdef RBBI_DEBUG
void RBBITableBuilder::printStates() {
    int     c;    // input "character"
    int     n;    // state number

    RBBIDebugPrintf("state |           i n p u t     s y m b o l s \n");
    RBBIDebugPrintf("      | Acc  LA    Tag");
    for (c=0; c<fRB->fSetBuilder->getNumCharCategories(); c++) {
        RBBIDebugPrintf(" %2d", c);
    }
    RBBIDebugPrintf("\n");
    RBBIDebugPrintf("      |---------------");
    for (c=0; c<fRB->fSetBuilder->getNumCharCategories(); c++) {
        RBBIDebugPrintf("---");
    }
    RBBIDebugPrintf("\n");

    for (n=0; n<fDStates->size(); n++) {
        RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
        RBBIDebugPrintf("  %3d | " , n);
        RBBIDebugPrintf("%3d %3d %5d ", sd->fAccepting, sd->fLookAhead, sd->fTagsIdx);
        for (c=0; c<fRB->fSetBuilder->getNumCharCategories(); c++) {
            RBBIDebugPrintf(" %2d", sd->fDtran->elementAti(c));
        }
        RBBIDebugPrintf("\n");
    }
    RBBIDebugPrintf("\n\n");
}
#endif



//-----------------------------------------------------------------------------
//
//   printRuleStatusTable    Debug Function.  Dump the common rule status table
//
//-----------------------------------------------------------------------------
#ifdef RBBI_DEBUG
void RBBITableBuilder::printRuleStatusTable() {
    int32_t  thisRecord = 0;
    int32_t  nextRecord = 0;
    int      i;
    UVector  *tbl = fRB->fRuleStatusVals;

    RBBIDebugPrintf("index |  tags \n");
    RBBIDebugPrintf("-------------------\n");

    while (nextRecord < tbl->size()) {
        thisRecord = nextRecord;
        nextRecord = thisRecord + tbl->elementAti(thisRecord) + 1;
        RBBIDebugPrintf("%4d   ", thisRecord);
        for (i=thisRecord+1; i<nextRecord; i++) {
            RBBIDebugPrintf("  %5d", tbl->elementAti(i));
        }
        RBBIDebugPrintf("\n");
    }
    RBBIDebugPrintf("\n\n");
}
#endif


//-----------------------------------------------------------------------------
//
//   RBBIStateDescriptor     Methods.  This is a very struct-like class
//                           Most access is directly to the fields.
//
//-----------------------------------------------------------------------------

RBBIStateDescriptor::RBBIStateDescriptor(int lastInputSymbol, UErrorCode *fStatus) {
    fMarked    = FALSE;
    fAccepting = 0;
    fLookAhead = 0;
    fTagsIdx   = 0;
    fTagVals   = NULL;
    fPositions = NULL;
    fDtran     = NULL;

    fDtran     = new UVector(lastInputSymbol+1, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    if (fDtran == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    fDtran->setSize(lastInputSymbol+1, *fStatus);    // fDtran needs to be pre-sized.
                                           //   It is indexed by input symbols, and will
                                           //   hold  the next state number for each
                                           //   symbol.
}


RBBIStateDescriptor::~RBBIStateDescriptor() {
    delete       fPositions;
    delete       fDtran;
    delete       fTagVals;
    fPositions = NULL;
    fDtran     = NULL;
    fTagVals   = NULL;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
