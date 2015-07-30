//
//  eidos_ast_node.cpp
//  Eidos
//
//  Created by Ben Haller on 7/27/15.
//  Copyright (c) 2015 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of Eidos.
//
//	Eidos is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	Eidos is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with Eidos.  If not, see <http://www.gnu.org/licenses/>.


#include "eidos_ast_node.h"

#include "eidos_interpreter.h"


using std::string;


EidosASTNode::EidosASTNode(EidosToken *p_token, bool p_token_is_owned) :
token_(p_token), token_is_owned_(p_token_is_owned)
{
}

EidosASTNode::EidosASTNode(EidosToken *p_token, EidosASTNode *p_child_node) :
token_(p_token)
{
	this->AddChild(p_child_node);
}

EidosASTNode::~EidosASTNode(void)
{
	for (auto child : children_)
		delete child;
	
	if (cached_value_)
	{
		if (cached_value_is_owned_)
			delete cached_value_;
		
		cached_value_ = nullptr;
	}
	
	if (token_is_owned_)
	{
		delete token_;
		token_ = nullptr;
	}
}

void EidosASTNode::AddChild(EidosASTNode *p_child_node)
{
	children_.push_back(p_child_node);
}

void EidosASTNode::ReplaceTokenWithToken(EidosToken *p_token)
{
	// dispose of our previous token
	if (token_is_owned_)
	{
		delete token_;
		token_ = nullptr;
		token_is_owned_ = false;
	}
	
	// used to fix virtual token to encompass their children; takes ownership
	token_ = p_token;
	token_is_owned_ = true;
}

void EidosASTNode::OptimizeTree(void) const
{
	_OptimizeConstants();
	_OptimizeIdentifiers();
}

void EidosASTNode::_OptimizeConstants(void) const
{
	// recurse down the tree; determine our children, then ourselves
	for (const EidosASTNode *child : children_)
		child->_OptimizeConstants();
	
	// now find constant expressions and make EidosValues for them
	EidosTokenType token_type = token_->token_type_;
	
	if (token_type == EidosTokenType::kTokenNumber)
	{
		const std::string &number_string = token_->token_string_;
		
		// This is taken from EidosInterpreter::Evaluate_Number and needs to match exactly!
		EidosValue *result = nullptr;
		
		if ((number_string.find('.') != string::npos) || (number_string.find('-') != string::npos))
			result = new EidosValue_Float_singleton_const(strtod(number_string.c_str(), nullptr));							// requires a float
		else if ((number_string.find('e') != string::npos) || (number_string.find('E') != string::npos))
			result = new EidosValue_Int_singleton_const(static_cast<int64_t>(strtod(number_string.c_str(), nullptr)));		// has an exponent
		else
			result = new EidosValue_Int_singleton_const(strtoll(number_string.c_str(), nullptr, 10));						// plain integer
		
		// OK, so this is a weird thing.  We don't call SetExternalPermanent(), because that guarantees that the object set will
		// live longer than the symbol table it might end up in, and we cannot make that guarantee here; the tree we are setting
		// this value in might be short-lived, whereas the symbol table might be long-lived (in an interactive interpreter
		// context, for example).  Instead, we call SetExternalTemporary().  Basically, we are pretending that we ourselves
		// (meaning the AST) are a symbol table, and that we "own" this object.  That will make the real symbol table make
		// its own copy of the object, rather than using ours.  It will also prevent the object from being regarded as a
		// temporary object and deleted by somebody, though.  So we get the best of both worlds; external ownership, but without
		// having to make the usual guarantee about the lifetime of the object.  The downside of this is that the value will
		// be copied if it is put into a symbol table, even though it is constant and could be shared in most circumstances.
		result->SetExternalTemporary();
		
		cached_value_ = result;
		cached_value_is_owned_ = true;
	}
	else if (token_type == EidosTokenType::kTokenString)
	{
		// This is taken from EidosInterpreter::Evaluate_String and needs to match exactly!
		EidosValue *result = new EidosValue_String(token_->token_string_);
		
		// See the above comment that begins "OK, so this is a weird thing".  It is still a weird thing down here, too.
		result->SetExternalTemporary();
		
		cached_value_ = result;
		cached_value_is_owned_ = true;
	}
	else if ((token_type == EidosTokenType::kTokenReturn) || (token_type == EidosTokenType::kTokenLBrace))
	{
		// These are node types which can propagate a single constant value upward.  Note that this is not strictly
		// true; both return and compound statements have side effects on the flow of execution.  It would therefore
		// be inappropriate for their execution to be short-circuited in favor of a constant value in general; but
		// that is not what this optimization means.  Rather, it means that these nodes are saying "I've got just a
		// constant value inside me, so *if* nothing else is going on around me, I can be taken as equal to that
		// constant."  We honor that conditional statement by only checking for the cached constant in specific places.
		if (children_.size() == 1)
		{
			const EidosASTNode *child = children_[0];
			EidosValue *cached_value = child->cached_value_;
			
			if (cached_value)
			{
				cached_value_ = cached_value;
				cached_value_is_owned_ = false;	// somebody below us owns the value
			}
		}
	}
}

void EidosASTNode::_OptimizeIdentifiers(void) const
{
	// recurse down the tree; determine our children, then ourselves
	for (auto child : children_)
		child->_OptimizeIdentifiers();
	
	if (token_->token_type_ == EidosTokenType::kTokenIdentifier)
	{
		const std::string &token_string = token_->token_string_;
		
		// if the identifier's name matches that of a global function, cache the function signature
		EidosFunctionMap *function_map = EidosInterpreter::BuiltInFunctionMap();
		
		auto signature_iter = function_map->find(token_string);
		
		if (signature_iter != function_map->end())
			cached_signature_ = signature_iter->second;
		
		// if the identifier's name matches that of a property or method that we know about, cache an ID for it
		cached_stringID = EidosGlobalStringIDForString(token_string);
	}
	else if (token_->token_type_ == EidosTokenType::kTokenLParen)
	{
		// If we are a function call node, check that our first child, if it is a simple identifier, has cached a signature.
		// If we do not have children, or the first child is not an identifier, that is also problematic, but not our problem.
		// If a function identifier does not have a cached signature, it must at least have a cached string ID; this allows
		// zero-generation functions to pass our check, since they can't be set up before this check occurs.
		int children_count = (int)children_.size();
		
		if (children_count >= 1)
		{
			const EidosASTNode *first_child = children_[0];
			
			if (first_child->token_->token_type_ == EidosTokenType::kTokenIdentifier)
				if ((first_child->cached_signature_ == nullptr) && (first_child->cached_stringID == gID_none))
					EIDOS_TERMINATION << "ERROR (EidosASTNode::_OptimizeIdentifiers): unrecognized function name \"" << first_child->token_->token_string_ << "\"." << eidos_terminate();
		}
	}
	else if (token_->token_type_ == EidosTokenType::kTokenDot)
	{
		// If we are a dot-operator node, check that our second child has cached a string ID for the property or method being invoked.
		// If we do not have children, or the second child is not an identifier, that is also problematic, but not our problem.
		int children_count = (int)children_.size();
		
		if (children_count >= 2)
		{
			const EidosASTNode *second_child = children_[1];
			
			if (second_child->token_->token_type_ == EidosTokenType::kTokenIdentifier)
				if (second_child->cached_stringID == gID_none)
					EIDOS_TERMINATION << "ERROR (EidosASTNode::_OptimizeIdentifiers): unrecognized property or method name \"" << second_child->token_->token_string_ << "\"." << eidos_terminate();
		}
	}
}

void EidosASTNode::PrintToken(std::ostream &p_outstream) const
{
	// We want to print some tokens differently when they are in the context of an AST, for readability
	switch (token_->token_type_)
	{
		case EidosTokenType::kTokenLBrace:		p_outstream << "BLOCK";				break;
		case EidosTokenType::kTokenSemicolon:	p_outstream << "NULL_STATEMENT";	break;
		case EidosTokenType::kTokenLParen:		p_outstream << "CALL";				break;
		case EidosTokenType::kTokenLBracket:		p_outstream << "SUBSET";			break;
		case EidosTokenType::kTokenComma:		p_outstream << "ARG_LIST";			break;
		default:							p_outstream << *token_;				break;
	}
}

void EidosASTNode::PrintTreeWithIndent(std::ostream &p_outstream, int p_indent) const
{
	// If we are indented, start a new line and indent
	if (p_indent > 0)
	{
		p_outstream << "\n  ";
		
		for (int i = 0; i < p_indent - 1; ++i)
			p_outstream << "  ";
	}
	
	if (children_.size() == 0)
	{
		// If we are a leaf, just print our token
		PrintToken(p_outstream);
	}
	else
	{
		// Determine whether we have only leaves as children
		bool childWithChildren = false;
		
		for (auto child : children_)
		{
			if (child->children_.size() > 0)
			{
				childWithChildren = true;
				break;
			}
		}
		
		if (childWithChildren)
		{
			// If we have non-leaf children, then print them with an incremented indent
			p_outstream << "(";
			PrintToken(p_outstream);
			
			for (auto child : children_)
				child->PrintTreeWithIndent(p_outstream, p_indent + 1);
			
			// and then outdent and show our end paren
			p_outstream << "\n";
			
			if (p_indent > 0)
			{
				p_outstream << "  ";
				
				for (int i = 0; i < p_indent - 1; ++i)
					p_outstream << "  ";
			}
			
			p_outstream << ")";
		}
		else
		{
			// If we have only leaves as children, then print everything on one line, for compactness
			p_outstream << "(";
			PrintToken(p_outstream);
			
			for (auto child : children_)
			{
				p_outstream << " ";
				child->PrintToken(p_outstream);
			}
			
			p_outstream << ")";
		}
	}
}
































































