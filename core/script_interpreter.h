//
//  script_interpreter.h
//  SLiM
//
//  Created by Ben Haller on 4/4/15.
//  Copyright (c) 2015 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of SLiM.
//
//	SLiM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	SLiM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with SLiM.  If not, see <http://www.gnu.org/licenses/>.

/*
 
 The class ScriptInterpreter embodies an interpreter for a script, and handles symbol lookup, operation execution,
 etc., using classes that are, for simplicity, also defined in this header.
 
 */

#ifndef __SLiM__script_interpreter__
#define __SLiM__script_interpreter__

#include <vector>
#include <string>
#include <map>

#include "script.h"
#include "script_value.h"
#include "mutation.h"


// A class representing a script interpretation context with all associated symbol table state
class ScriptInterpreter
{
	//	This class has its copy constructor and assignment operator disabled, to prevent accidental copying.
	
private:
	const Script &script_;							// not owned
	SymbolTable *global_symbols_ = nullptr;			// OWNED POINTERS: identifiers to ScriptValues
	
	// flags to handle next/break statements in do...while, while, and for loops
	bool next_statement_hit_ = false;
	bool break_statement_hit_ = false;
	
	// flags and streams for execution logging – a trace of the DFS of the parse tree
	bool logging_execution_ = false;
	int execution_log_indent_ = 0;
	std::ostringstream execution_log_;
	
	// an output stream for output from executed nodes and functions; this goes into the user's console
	std::ostringstream execution_output_;
	
public:
	
	ScriptInterpreter(const ScriptInterpreter&) = delete;					// no copying
	ScriptInterpreter& operator=(const ScriptInterpreter&) = delete;		// no copying
	ScriptInterpreter(void) = delete;										// no null construction
	ScriptInterpreter(const Script &p_script);
	ScriptInterpreter(const Script &p_script, SymbolTable *p_symbols);		// the receiver takes ownership of the passed symbol table
	
	~ScriptInterpreter(void);												// destructor
	
	inline std::string IndentString(int p_indent_level) { return std::string(p_indent_level * 2, ' '); };
	
	void SetShouldLogExecution(bool p_log);
	bool ShouldLogExecution(void);
	std::string ExecutionLog(void);
	
	std::string ExecutionOutput(void);
	
	SymbolTable &BorrowSymbolTable(void);			// the returned pointer is owned by the interpreter, borrowed by the caller
	SymbolTable *YieldSymbolTable(void);			// the returned pointer is owned by the caller, and the receiver nulls out its symbol table pointer
	
	// Evaluation methods; the caller owns the returned ScriptValue object
	ScriptValue *EvaluateScriptBlock(void);			// the starting point for script blocks in SLiM simulations, which require braces
	ScriptValue *EvaluateInterpreterBlock(void);	// the starting point for executed blocks in SLiMscript, which do not require braces
	
	LValueReference *Evaluate_LValueReference(const ScriptASTNode *p_node);
	ScriptValue *EvaluateNode(const ScriptASTNode *p_node);
	
	ScriptValue *Evaluate_NullStatement(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_CompoundStatement(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_RangeExpr(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_FunctionCall(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Subset(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_MemberRef(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Plus(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Minus(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Mod(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Mult(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Div(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Exp(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_And(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Or(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Assign(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Eq(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Lt(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_LtEq(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Gt(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_GtEq(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Not(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_NotEq(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Number(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_String(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Identifier(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_If(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Do(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_While(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_For(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Next(const ScriptASTNode *p_node);
	ScriptValue *Evaluate_Break(const ScriptASTNode *p_node);
};


#endif /* defined(__SLiM__script_interpreter__) */


































































