/*
 * phc -- the open source PHP compiler
 * See doc/license/README.license for licensing information
 *
* */

#ifndef PHC_STAT_COLLECTOR
#define PHC_STAT_COLLECTOR

class Whole_program;
class Index_node;

class Stat_collector : public CFG_visitor
{
public:
	Whole_program* wp;
	Stat_collector (Whole_program*);
	Stat_collector ();

	void run (CFG* cfg);

	// for passing between visit_assign_var and visit_expr
	Index_node* lhs;
	bool assign_var_is_ref;

	void visit_entry_block (Entry_block* bb);
	void visit_basic_block (Basic_block* bb);
	void visit_statement_block (Statement_block* bb);

	DECLARE_STATEMENT_CFG_VISITORS
	DECLARE_EXPR_CFG_VISITORS

	void get_number_of_statements (CFG* cfg, string beforeafter);
	void collect_uninit_var_stats (Basic_block* bb);
	void collect_def_use_stats (CFG* cfg);
	void collect_type_stats (Basic_block* bb, MIR::Rvalue* rval,string statname );
	void collect_method_stats ();
private:
	String last_assignment_lhs;
};

class Get_var_name : public MIR::Visitor, virtual public GC_obj
{  
	public: 
		Set<string> var_names;

		void pre_variable_name (MIR::VARIABLE_NAME* in)
		{
			var_names.insert (*in->value);
		}
		

};  



#endif // PHC_STAT_COLLECTOR
