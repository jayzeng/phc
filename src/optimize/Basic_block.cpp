#include "process_ir/General.h"
#include "process_mir/MIR_unparser.h"
#include "MIR.h"

#include "Basic_block.h"
#include "Def_use.h"

using namespace std;
using namespace MIR;

/* Constructors */

Basic_block::Basic_block(CFG* cfg)
: cfg(cfg)
, vertex (NULL)
, defs (NULL)
, uses (NULL)
, live_in (NULL)
, live_out (NULL)
, changed (false)
, aliases (NULL)
{
	phi_lhss = new VARIABLE_NAME_list;
}

Branch_block::Branch_block (CFG* cfg, MIR::Branch* b)
: Basic_block (cfg)
, branch (b) 
{
}

Statement_block::Statement_block (CFG* cfg, MIR::Statement* s) 
: Basic_block (cfg)
, statement(s)
{
}

Empty_block::Empty_block (CFG* cfg)
: Basic_block (cfg)
{
}

Entry_block::Entry_block (CFG* cfg, Method* method)
: Basic_block (cfg)
, method (method)
{
}

Exit_block::Exit_block (CFG* cfg, Method* method)
: Basic_block (cfg)
, method (method)
{
}

list<pair<String*,String*> >*
Basic_block::get_graphviz_properties ()
{
	list<pair<String*,String*> >* result = new list<pair<String*,String*> >;
	result->push_back (make_pair (s("shape"), s("box")));
	return result;
}


/*
 * Labels for graphviz
 */
String*
Entry_block::get_graphviz_label ()
{
	return s("ENTRY");
}

String* Exit_block::get_graphviz_label ()
{
	return s("EXIT");
}

String* Empty_block::get_graphviz_label ()
{
	return s("");
}

String* Branch_block::get_graphviz_label ()
{
	return branch->variable_name->get_ssa_var_name ();
}

String*
Statement_block::get_graphviz_label ()
{
	stringstream ss;
	MIR_unparser mup(ss, true);
	mup.unparse(statement);
	return s(ss.str());
}

list<pair<String*,String*> >*
Branch_block::get_graphviz_properties ()
{
	list<pair<String*,String*> >* result =
		Basic_block::get_graphviz_properties ();

	result->push_back (make_pair (s("shape"), s("diamond")));

	return result;
}


list<pair<String*,list<String*> > >*
Basic_block::get_graphviz_bb_properties ()
{
	list<pair<String*,list<String*> > >* result = new list<pair<String*,list<String*> > >;
//	if (defs)
//		result->push_back (pair<String*, Set*> (s("defs"), defs));
//	if (uses)
//		result->push_back (pair<String*, Set*> (s("uses"), uses));
//	if (aliases && dynamic_cast<Entry_block*> (this))
//		result->push_back (pair<String*, Set*> (s("aliases"), aliases));
	return result;
}

list<pair<String*,list<String*> > >*
Basic_block::get_graphviz_head_properties ()
{
	list<pair<String*,list<String*> > >* result = new list<pair<String*,list<String*> > >;
	// Phi nodes
	foreach (VARIABLE_NAME* phi_lhs, *get_phi_lhss ())
	{
		list<String*> list;
		foreach (Edge* edge, *get_predecessor_edges ())
		{
			Rvalue* arg = get_phi_arg_for_edge (edge, phi_lhs);

			stringstream ss;
			ss << "(" << cfg->index[edge->get_source()->vertex] << "): ";
			if (isa<VARIABLE_NAME> (arg))
				ss << *dyc<VARIABLE_NAME> (arg)->get_ssa_var_name ();
			else
				ss << *dyc<Literal> (arg)->get_value_as_string ();

			list.push_back (s (ss.str()));
		}

		result->push_back (make_pair (phi_lhs->get_ssa_var_name (), list));
	}

//	if (live_in)
//		result->push_back (make_pair (s("IN"), live_in));
	return result;
}

list<pair<String*,list<String*> > >*
Basic_block::get_graphviz_tail_properties ()
{
	list<pair<String*,list<String*> > >* result = new list<pair<String*,list<String*> > >;
//	if (live_out)
//		result->push_back (make_pair (s("OUT"), live_out));
	return result;
}


/*
 * SSA
 */

/* Merge the phi nodes from OTHER into this BB. */
void
Basic_block::copy_phi_nodes (Basic_block* other)
{
	// Since a phi is an assignment, and variables can only be assigned to once,
	// OTHER's PHIs cant exist in THIS.
	foreach (VARIABLE_NAME* phi_lhs, *other->get_phi_lhss ())
		assert (!has_phi_node (phi_lhs));

	phi_lhss->push_back_all (other->get_phi_lhss());
}

bool
Basic_block::has_phi_node (VARIABLE_NAME* phi_lhs)
{

	foreach (VARIABLE_NAME* lhs, *phi_lhss)
	{
		// Only operator< is defined, not >= or ==
		if (!(*lhs < *phi_lhs) && !(*phi_lhs < *lhs))
			return true;
	}

	return false;
}

void
Basic_block::add_phi_node (VARIABLE_NAME* phi_lhs)
{
	assert (!has_phi_node (phi_lhs));
	phi_lhss->push_back (phi_lhs->clone ());
}

void
Basic_block::add_phi_arg (VARIABLE_NAME* phi_lhs, int version, Edge* edge)
{
//	assert (has_phi_node (phi_lhs));
	// phi_lhs doesnt have to be in SSA, since it will be updated later using
	// update_phi_node, if it is not.
	VARIABLE_NAME* arg = phi_lhs->clone ();
	arg->set_version (version);
	set_phi_arg_for_edge (edge, phi_lhs, arg);
}

void
Basic_block::remove_phi_nodes ()
{
	foreach (Edge* pred, *get_predecessor_edges ())
		pred->pm.clear ();

	phi_lhss->clear ();
}

void
Basic_block::remove_phi_node (VARIABLE_NAME* phi_lhs)
{
	assert (has_phi_node (phi_lhs));
	foreach (Edge* pred, *get_predecessor_edges ())
		pred->pm.erase (phi_lhs);

	// TODO: are we tryingto remove the pointer, when we have a different
	// pointer to the same thing?
	phi_lhss->remove (phi_lhs);
	assert (!has_phi_node (phi_lhs));
}


void
Basic_block::update_phi_node (MIR::VARIABLE_NAME* old_phi_lhs, MIR::VARIABLE_NAME* new_phi_lhs)
{
	// If a phi_lhs changes into SSA form, its indexing will change. So we must
	// re-insert its args with the new index.
	assert (!old_phi_lhs->in_ssa);
	assert (new_phi_lhs->in_ssa);
	assert (*old_phi_lhs->value == *new_phi_lhs->value);
	add_phi_node (new_phi_lhs);

	foreach (Edge* pred, *get_predecessor_edges ())
	{
		// Not all nodes have their phi argument added yet
		if (pred->pm.find (old_phi_lhs) != pred->pm.end ())
			set_phi_arg_for_edge (
					pred,
					new_phi_lhs,
					get_phi_arg_for_edge (pred, old_phi_lhs));
	}

	remove_phi_node (old_phi_lhs);
}

/* Replace any phi node with a single argument with an assignment. */
void
Basic_block::fix_solo_phi_args ()
{
	// TODO: The theory is that each Phi node executes simultaneously. If
	// there are dependencies between the nodes, this could be wrong.

	if (get_predecessor_edges()->size () == 1)
	{
		BB_list* replacements = new BB_list ();
		foreach (VARIABLE_NAME* phi_lhs, *get_phi_lhss ())
		{
			Rvalue* arg = get_phi_args (phi_lhs)->front ();
			replacements->push_back (
					new Statement_block (
						cfg,
						new Assign_var (
							phi_lhs->clone(),
							false,
							arg)));
		}

		remove_phi_nodes ();

		replacements->push_back (this);
		replace (replacements);
	}
}

Rvalue_list*
Basic_block::get_phi_args (MIR::VARIABLE_NAME* phi_lhs)
{
	Rvalue_list* result = new Rvalue_list;

	foreach (Edge* pred, *get_predecessor_edges ())
		result->push_back (get_phi_arg_for_edge (pred, phi_lhs));

	return result;
}

VARIABLE_NAME_list*
Basic_block::get_phi_lhss()
{
	// Return a clone, since we sometimes like to update the list
	VARIABLE_NAME_list* result = new VARIABLE_NAME_list;
	result->push_back_all (phi_lhss);
	return result;
}

Rvalue*
Basic_block::get_phi_arg_for_edge (Edge* edge, VARIABLE_NAME* phi_lhs)
{
	Rvalue* result = edge->pm[phi_lhs];
	assert (result);
	return result;
}

void
Basic_block::set_phi_arg_for_edge (Edge* edge, VARIABLE_NAME* phi_lhs, Rvalue* arg)
{
	assert (arg);
	edge->pm[phi_lhs] = arg->clone ();
}



/*
 * Block manipulation
 */

void
Basic_block::insert_predecessor (Basic_block* bb)
{
	cfg->insert_predecessor_bb (this, bb);
}

BB_list*
Basic_block::get_predecessors ()
{
	return cfg->get_bb_predecessors (this);
}

BB_list*
Basic_block::get_successors ()
{
	return cfg->get_bb_successors (this);
}

Basic_block*
Basic_block::get_successor ()
{
	BB_list* succs = get_successors ();
	assert (succs->size() == 1);
	return succs->front ();
}

Edge_list*
Basic_block::get_predecessor_edges ()
{
	return cfg->get_edge_predecessors (this);
}

Edge_list*
Basic_block::get_successor_edges ()
{
	return cfg->get_edge_successors (this);
}


Edge*
Basic_block::get_successor_edge ()
{
	Edge_list* succs = get_successor_edges ();
	assert (succs->size() == 1);
	return succs->front ();
}


Basic_block*
Branch_block::get_true_successor ()
{
	Edge_list* succs = get_successor_edges ();
	assert (succs->size() == 2);

	foreach (Edge* succ, *succs)
		if (cfg->is_true_edge (succ))
			return succ->get_target ();

	assert (0);
}


Basic_block*
Branch_block::get_false_successor ()
{
	Edge_list* succs = get_successor_edges ();
	assert (succs->size() == 2);

	foreach (Edge* succ, *succs)
		if (not cfg->is_true_edge (succ))
			return succ->get_target ();

	assert (0);
}

Edge*
Branch_block::get_true_successor_edge ()
{
	Edge_list* succs = get_successor_edges ();
	assert (succs->size() == 2);

	foreach (Edge* succ, *succs)
		if (cfg->is_true_edge (succ))
			return succ;

	assert (0);
}

Edge*
Branch_block::get_false_successor_edge ()
{
	Edge_list* succs = get_successor_edges ();
	assert (succs->size() == 2);

	foreach (Edge* succ, *succs)
		if (not cfg->is_true_edge (succ))
			return succ;

	assert (0);
}

void
Branch_block::switch_successors ()
{
	foreach (Edge* succ, *get_successor_edges ())
		succ->direction = !succ->direction;
}

void
Basic_block::remove ()
{
	cfg->remove_bb (this);
}

void
Basic_block::replace (BB_list* replacements)
{
	cfg->replace_bb (this, replacements);
}

void
Branch_block::set_always (bool direction)
{
	cfg->consistency_check ();

	// If this block has phi nodes, and the successors do too, then moving this
	// blocks phis to the successor leaves edges in the successor with no
	// argument for that node. Instead, replace this node with an empty node.
	// Tody will sort out the rest, if needs be.
	Basic_block* new_bb = new Empty_block (cfg);
	cfg->add_bb (new_bb);
	new_bb->copy_phi_nodes (this); // edges updated later


	Edge* true_edge = get_true_successor_edge ();
	Edge* false_edge = get_false_successor_edge ();

	Basic_block* succ = direction ? get_true_successor () : get_false_successor ();
	Edge* succ_edge = direction ? true_edge : false_edge;

	// Add the incoming edges
	foreach (Edge* old_edge, *get_predecessor_edges ())
	{
		Edge* new_edge = cfg->add_edge (old_edge->get_source (), new_bb);

		// copy the properties
		new_edge->direction = old_edge->direction;
		new_edge->copy_phi_map (old_edge);

		cfg->remove_edge (old_edge);
	}

	// Add the outgoing edge
	Edge* new_edge = cfg->add_edge (new_bb, succ);

	// copy the phi properties (no direction)
	new_edge->copy_phi_map (succ_edge);


	// Remove whats left
	cfg->remove_edge (true_edge);
	cfg->remove_edge (false_edge);
	remove ();

	cfg->consistency_check ();
}

BB_list*
Basic_block::get_dominance_frontier ()
{
	return cfg->dominance->get_bb_dominance_frontier (this);
}

Basic_block*
Basic_block::get_immediate_dominator ()
{
	return cfg->dominance->get_bb_immediate_dominator (this);
}

bool
Basic_block::is_in_dominance_frontier (Basic_block* df)
{
	return cfg->dominance->is_bb_in_dominance_frontier (this, df);
}

bool
Basic_block::is_dominated_by (Basic_block* bb)
{
	return cfg->dominance->is_bb_dominated_by (this, bb);
}

void
Basic_block::add_to_dominance_frontier (Basic_block* bb)
{
	cfg->dominance->add_to_bb_dominance_frontier (this, bb);
}

BB_list*
Basic_block::get_dominated_blocks ()
{
	return cfg->dominance->get_blocks_dominated_by_bb (this);
}

VARIABLE_NAME_list*
Basic_block::get_pre_ssa_defs ()
{
	return cfg->duw->get_bb_defs (this);
}

VARIABLE_NAME_list*
Basic_block::get_pre_ssa_uses ()
{
	return cfg->duw->get_bb_uses (this);
}

int
Basic_block::get_index ()
{
	return cfg->index[vertex];
}


/*
 * Debugging
 */

void
Entry_block::dump()
{
	DEBUG ("Entry block");
}

void
Exit_block::dump()
{
	DEBUG ("Exit block");
}

void
Branch_block::dump()
{
	DEBUG ("Branch block (" << cfg->index[vertex] << ")");
}

void
Statement_block::dump()
{
	CHECK_DEBUG ();

	DEBUG ("Statement block (" << cfg->index[vertex] << ")");
}
void
Empty_block::dump()
{
	DEBUG ("Empty block (" << cfg->index[vertex] << ")");
}
