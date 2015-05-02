//
//  script_functions.cpp
//  SLiM
//
//  Created by Ben Haller on 4/6/15.
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


#include "script_functions.h"
#include "script_pathproxy.h"

#include "math.h"


using std::string;
using std::vector;
using std::map;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::istream;
using std::ostream;


ScriptValue *Execute_c(string p_function_name, vector<ScriptValue*> p_arguments);
ScriptValue *Execute_rep(string p_function_name, vector<ScriptValue*> p_arguments);
ScriptValue *Execute_repEach(string p_function_name, vector<ScriptValue*> p_arguments);
ScriptValue *Execute_seq(string p_function_name, vector<ScriptValue*> p_arguments);


//
//	Construct our built-in function map
//

void RegisterSignature(FunctionMap &p_map, const FunctionSignature *p_signature)
{
	p_map.insert(FunctionMapPair(p_signature->function_name_, p_signature));
}

FunctionMap BuiltInFunctionMap(void)
{
	FunctionMap map;
	
	
	// data construction functions
	
	RegisterSignature(map, (new FunctionSignature("rep",		FunctionIdentifier::repFunction,		kScriptValueMaskAnyBase))->AddAnyBase()->AddInt_S());
	RegisterSignature(map, (new FunctionSignature("repEach",	FunctionIdentifier::repEachFunction,	kScriptValueMaskAnyBase))->AddAnyBase()->AddInt());
	RegisterSignature(map, (new FunctionSignature("seq",		FunctionIdentifier::seqFunction,		kScriptValueMaskNumeric))->AddNumeric_S()->AddNumeric_S()->AddNumeric_OS());
	RegisterSignature(map, (new FunctionSignature("seqAlong",	FunctionIdentifier::seqAlongFunction,	kScriptValueMaskInt))->AddAny());
	RegisterSignature(map, (new FunctionSignature("c",			FunctionIdentifier::cFunction,			kScriptValueMaskAnyBase))->AddEllipsis());
	
	
	// data inspection/manipulation functions
	 
	RegisterSignature(map, (new FunctionSignature("print",		FunctionIdentifier::printFunction,		kScriptValueMaskNULL))->AddAny());
	RegisterSignature(map, (new FunctionSignature("cat",		FunctionIdentifier::catFunction,		kScriptValueMaskNULL))->AddAny());
	RegisterSignature(map, (new FunctionSignature("size",		FunctionIdentifier::sizeFunction,		kScriptValueMaskInt | kScriptValueMaskSingleton))->AddAny());
	
	/*
	strFunction,
	sumFunction,
	prodFunction,
	meanFunction,
	sdFunction,
	*/
	
	RegisterSignature(map, (new FunctionSignature("rev",		FunctionIdentifier::revFunction,		kScriptValueMaskAnyBase))->AddAnyBase());
	
	/*
	sortFunction,
	*/
	
	// data class testing/coercion functions
	 
	RegisterSignature(map, (new FunctionSignature("class",		FunctionIdentifier::classFunction,		kScriptValueMaskString | kScriptValueMaskSingleton))->AddAny());
	/*
	isLogicalFunction,
	isStringFunction,
	isIntegerFunction,
	isFloatFunction,
	asLogicalFunction,
	asStringFunction,
	asIntegerFunction,
	asFloatFunction,
	*/
	
	// math functions
	
	RegisterSignature(map, (new FunctionSignature("acos",		FunctionIdentifier::acosFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("asin",		FunctionIdentifier::asinFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("atan",		FunctionIdentifier::atanFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("cos",		FunctionIdentifier::cosFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("sin",		FunctionIdentifier::sinFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("tan",		FunctionIdentifier::tanFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("exp",		FunctionIdentifier::expFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("log",		FunctionIdentifier::logFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("log10",		FunctionIdentifier::log10Function,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("log2",		FunctionIdentifier::log2Function,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("sqrt",		FunctionIdentifier::sqrtFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("ceil",		FunctionIdentifier::ceilFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("floor",		FunctionIdentifier::floorFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("round",		FunctionIdentifier::roundFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("trunc",		FunctionIdentifier::truncFunction,		kScriptValueMaskFloat))->AddNumeric());
	RegisterSignature(map, (new FunctionSignature("abs",		FunctionIdentifier::absFunction,		kScriptValueMaskNumeric))->AddNumeric());
	
	// bookkeeping functions

	RegisterSignature(map, (new FunctionSignature("stop",		FunctionIdentifier::stopFunction,		kScriptValueMaskNULL))->AddString_OS());
	RegisterSignature(map, (new FunctionSignature("version",	FunctionIdentifier::versionFunction,	kScriptValueMaskString | kScriptValueMaskSingleton)));
	RegisterSignature(map, (new FunctionSignature("license",	FunctionIdentifier::licenseFunction,	kScriptValueMaskNULL)));
	RegisterSignature(map, (new FunctionSignature("help",		FunctionIdentifier::helpFunction,		kScriptValueMaskNULL))->AddString_OS());
	RegisterSignature(map, (new FunctionSignature("ls",			FunctionIdentifier::lsFunction,			kScriptValueMaskNULL)));
	RegisterSignature(map, (new FunctionSignature("function",	FunctionIdentifier::functionFunction,	kScriptValueMaskNULL))->AddString_OS());
	
	// proxy instantiation
	
	RegisterSignature(map, (new FunctionSignature("Path",		FunctionIdentifier::PathFunction,		kScriptValueMaskProxy | kScriptValueMaskSingleton))->AddString_OS());
	
	return map;
}

FunctionMap gBuiltInFunctionMap = BuiltInFunctionMap();


//
//	Executing function calls
//

ScriptValue *Execute_c(string p_function_name, vector<ScriptValue*> p_arguments)
{
#pragma unused(p_function_name)
	ScriptValueType highest_type = ScriptValueType::kValueNULL;
	
	// First figure out our return type, which is the highest-promotion type among all our arguments
	for (ScriptValue *arg_value : p_arguments)
	{
		ScriptValueType arg_type = arg_value->Type();
		
		if (arg_type > highest_type)
			highest_type = arg_type;
	}
	
	// If we've got nothing but NULL, then return NULL
	if (highest_type == ScriptValueType::kValueNULL)
		return new ScriptValue_NULL();
	
	// Create an object of the right return type, concatenate all the arguments together, and return it
	if (highest_type == ScriptValueType::kValueLogical)
	{
		ScriptValue_Logical *result = new ScriptValue_Logical();
		
		for (ScriptValue *arg_value : p_arguments)
			if (arg_value->Type() != ScriptValueType::kValueNULL)
				for (int value_index = 0; value_index < arg_value->Count(); ++value_index)
					result->PushLogical(arg_value->LogicalAtIndex(value_index));
		
		return result;
	}
	else if (highest_type == ScriptValueType::kValueInt)
	{
		ScriptValue_Int *result = new ScriptValue_Int();
		
		for (ScriptValue *arg_value : p_arguments)
			if (arg_value->Type() != ScriptValueType::kValueNULL)
				for (int value_index = 0; value_index < arg_value->Count(); ++value_index)
					result->PushInt(arg_value->IntAtIndex(value_index));
		
		return result;
	}
	else if (highest_type == ScriptValueType::kValueFloat)
	{
		ScriptValue_Float *result = new ScriptValue_Float();
		
		for (ScriptValue *arg_value : p_arguments)
			if (arg_value->Type() != ScriptValueType::kValueNULL)
				for (int value_index = 0; value_index < arg_value->Count(); ++value_index)
					result->PushFloat(arg_value->FloatAtIndex(value_index));
		
		return result;
	}
	else if (highest_type == ScriptValueType::kValueString)
	{
		ScriptValue_String *result = new ScriptValue_String();
		
		for (ScriptValue *arg_value : p_arguments)
			if (arg_value->Type() != ScriptValueType::kValueNULL)
				for (int value_index = 0; value_index < arg_value->Count(); ++value_index)
					result->PushString(arg_value->StringAtIndex(value_index));
		
		return result;
	}
	else
	{
		SLIM_TERMINATION << "ERROR (Execute_c): type '" << highest_type << "' cannot be used with c()." << endl << slim_terminate();
	}
	
	return nullptr;
}

ScriptValue *Execute_rep(string p_function_name, vector<ScriptValue*> p_arguments)
{
#pragma unused(p_function_name)
	ScriptValue *arg1_value = p_arguments[0];
	int arg1_count = arg1_value->Count();
	ScriptValue *arg2_value = p_arguments[1];
	int arg2_count = arg2_value->Count();
	
	// the return type depends on the type of the first argument, which will get replicated
	ScriptValue *result = arg1_value->NewMatchingType();
	
	if (arg2_count == 1)
	{
		int64_t rep_count = arg2_value->IntAtIndex(0);
		
		for (int rep_idx = 0; rep_idx < rep_count; rep_idx++)
			for (int value_idx = 0; value_idx < arg1_count; value_idx++)
				result->PushValueFromIndexOfScriptValue(value_idx, arg1_value);
	}
	
	return result;
}

ScriptValue *Execute_repEach(string p_function_name, vector<ScriptValue*> p_arguments)
{
	ScriptValue *arg1_value = p_arguments[0];
	int arg1_count = arg1_value->Count();
	ScriptValue *arg2_value = p_arguments[1];
	int arg2_count = arg2_value->Count();
	
	// the return type depends on the type of the first argument, which will get replicated
	ScriptValue *result = arg1_value->NewMatchingType();
	
	if (arg2_count == 1)
	{
		int64_t rep_count = arg2_value->IntAtIndex(0);
		
		for (int value_idx = 0; value_idx < arg1_count; value_idx++)
			for (int rep_idx = 0; rep_idx < rep_count; rep_idx++)
				result->PushValueFromIndexOfScriptValue(value_idx, arg1_value);
	}
	else if (arg2_count == arg1_count)
	{
		for (int value_idx = 0; value_idx < arg1_count; value_idx++)
		{
			int64_t rep_count = arg2_value->IntAtIndex(value_idx);
			
			for (int rep_idx = 0; rep_idx < rep_count; rep_idx++)
				result->PushValueFromIndexOfScriptValue(value_idx, arg1_value);
		}
	}
	else
	{
		SLIM_TERMINATION << "ERROR (Execute_repEach): function " << p_function_name << "() requires that its second argument's size() either (1) be equal to 1, or (2) be equal to the size() of its first argument." << endl << slim_terminate();
	}
	
	return result;
}

ScriptValue *Execute_seq(string p_function_name, vector<ScriptValue*> p_arguments)
{
	ScriptValue *result = nullptr;
	ScriptValue *arg1_value = p_arguments[0];
	ScriptValueType arg1_type = arg1_value->Type();
	ScriptValue *arg2_value = p_arguments[1];
	ScriptValueType arg2_type = arg2_value->Type();
	ScriptValue *arg3_value = ((p_arguments.size() == 3) ? p_arguments[2] : nullptr);
	ScriptValueType arg3_type = (arg3_value ? arg3_value->Type() : ScriptValueType::kValueInt);
	
	if ((arg1_type == ScriptValueType::kValueFloat) || (arg2_type == ScriptValueType::kValueFloat) || (arg3_type == ScriptValueType::kValueFloat))
	{
		// float return case
		ScriptValue_Float *float_result = new ScriptValue_Float();
		result = float_result;
		
		double first_value = arg1_value->FloatAtIndex(0);
		double second_value = arg2_value->FloatAtIndex(0);
		double default_by = ((first_value < second_value) ? 1 : -1);
		double by_value = (arg3_value ? arg3_value->FloatAtIndex(0) : default_by);
		
		if (by_value == 0.0)
			SLIM_TERMINATION << "ERROR (Execute_seq): function " << p_function_name << " requires a by argument != 0." << endl << slim_terminate();
		if (((first_value < second_value) && (by_value < 0)) || ((first_value > second_value) && (by_value > 0)))
			SLIM_TERMINATION << "ERROR (Execute_seq): function " << p_function_name << " by argument has incorrect sign." << endl << slim_terminate();
		
		if (by_value > 0)
			for (double seq_value = first_value; seq_value <= second_value; seq_value += by_value)
				float_result->PushFloat(seq_value);
		else
			for (double seq_value = first_value; seq_value >= second_value; seq_value += by_value)
				float_result->PushFloat(seq_value);
	}
	else
	{
		// int return case
		ScriptValue_Int *int_result = new ScriptValue_Int();
		result = int_result;
		
		int64_t first_value = arg1_value->IntAtIndex(0);
		int64_t second_value = arg2_value->IntAtIndex(0);
		int64_t default_by = ((first_value < second_value) ? 1 : -1);
		int64_t by_value = (arg3_value ? arg3_value->IntAtIndex(0) : default_by);
		
		if (by_value == 0)
			SLIM_TERMINATION << "ERROR (Execute_seq): function " << p_function_name << " requires a by argument != 0." << endl << slim_terminate();
		if (((first_value < second_value) && (by_value < 0)) || ((first_value > second_value) && (by_value > 0)))
			SLIM_TERMINATION << "ERROR (Execute_seq): function " << p_function_name << " by argument has incorrect sign." << endl << slim_terminate();
		
		if (by_value > 0)
			for (int64_t seq_value = first_value; seq_value <= second_value; seq_value += by_value)
				int_result->PushInt(seq_value);
		else
			for (int64_t seq_value = first_value; seq_value >= second_value; seq_value += by_value)
				int_result->PushInt(seq_value);
	}
	
	return result;
}

ScriptValue *ExecuteFunctionCall(std::string const &p_function_name, vector<ScriptValue*> const &p_arguments, std::ostream &p_output_stream, ScriptInterpreter &p_interpreter)
{
	ScriptValue *result = nullptr;
	
	// Get the function signature and check our arguments against it
	auto signature_iter = gBuiltInFunctionMap.find(p_function_name);
	
	if (signature_iter == gBuiltInFunctionMap.end())
		SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): unrecognized function name " << p_function_name << "." << endl << slim_terminate();
	
	const FunctionSignature *signature = signature_iter->second;
	
	signature->CheckArguments("function", p_arguments);
	
	// We predefine variables for the return types, and preallocate them here if possible.  This is for code brevity below.
	ScriptValue_NULL *null_result = nullptr;
	ScriptValue_Logical *logical_result = nullptr;
	ScriptValue_Float *float_result = nullptr;
	ScriptValue_Int *int_result = nullptr;
	ScriptValue_String *string_result = nullptr;
	ScriptValueMask return_type_mask = signature->return_mask_ & kScriptValueMaskFlagStrip;
	
	if (return_type_mask == kScriptValueMaskNULL)
	{
		null_result = ScriptValue_NULL::ScriptValue_NULL_Invisible();	// assumed that invisible is correct when the return type is NULL
		result = null_result;
	}
	else if (return_type_mask == kScriptValueMaskLogical)
	{
		logical_result = new ScriptValue_Logical();
		result = logical_result;
	}
	else if (return_type_mask == kScriptValueMaskFloat)
	{
		float_result = new ScriptValue_Float();
		result = float_result;
	}
	else if (return_type_mask == kScriptValueMaskInt)
	{
		int_result = new ScriptValue_Int();
		result = int_result;
	}
	else if (return_type_mask == kScriptValueMaskString)
	{
		string_result = new ScriptValue_String();
		result = string_result;
	}
	// else the return type is not predictable and thus cannot be set up beforehand; the function implementation will have to do it
	
	// Prefetch arguments to allow greater brevity in the code below
	int n_args = (int)p_arguments.size();
	ScriptValue *arg1_value = (n_args >= 1) ? p_arguments[0] : nullptr;
	ScriptValueType arg1_type = (n_args >= 1) ? arg1_value->Type() : ScriptValueType::kValueNULL;
	int arg1_count = (n_args >= 1) ? arg1_value->Count() : 0;
	
	/*
	ScriptValue *arg2_value = (n_args >= 2) ? p_arguments[1] : nullptr;
	ScriptValueType arg2_type = (n_args >= 2) ? arg2_value->Type() : ScriptValueType::kValueNULL;
	int arg2_count = (n_args >= 2) ? arg2_value->Count() : 0;
	
	ScriptValue *arg3_value = (n_args >= 3) ? p_arguments[2] : nullptr;
	ScriptValueType arg3_type = (n_args >= 3) ? arg3_value->Type() : ScriptValueType::kValueNULL;
	int arg3_count = (n_args >= 3) ? arg3_value->Count() : 0;
	*/
	
	// Now we look up the function again and actually execute it
	switch (signature->function_id_)
	{
		case FunctionIdentifier::kNoFunction:
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): internal logic error." << endl << slim_terminate();
			break;
			
			// data construction functions
			
		case FunctionIdentifier::repFunction:			result = Execute_rep(p_function_name, p_arguments);	break;
		case FunctionIdentifier::repEachFunction:		result = Execute_repEach(p_function_name, p_arguments);	break;
		case FunctionIdentifier::seqFunction:			result = Execute_seq(p_function_name, p_arguments);	break;
			
		case FunctionIdentifier::seqAlongFunction:
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): function unimplemented." << endl << slim_terminate();
			break;
			
		case FunctionIdentifier::cFunction:				result = Execute_c(p_function_name, p_arguments); break;
			
			// data inspection/manipulation functions
			
		case FunctionIdentifier::printFunction:
			p_output_stream << *arg1_value << endl;
			break;
			
		case FunctionIdentifier::catFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
			{
				if (value_index > 0)
					p_output_stream << " ";
				
				p_output_stream << arg1_value->StringAtIndex(value_index);
			}
			break;
			
		case FunctionIdentifier::sizeFunction:
			int_result->PushInt(arg1_value->Count());
			break;
			
		case FunctionIdentifier::strFunction:
		case FunctionIdentifier::sumFunction:
		case FunctionIdentifier::prodFunction:
		case FunctionIdentifier::rangeFunction:
		case FunctionIdentifier::minFunction:
		case FunctionIdentifier::maxFunction:
		case FunctionIdentifier::whichMinFunction:
		case FunctionIdentifier::whichMaxFunction:
		case FunctionIdentifier::whichFunction:
		case FunctionIdentifier::meanFunction:
		case FunctionIdentifier::sdFunction:
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): function unimplemented." << endl << slim_terminate();
			break;
			
		case FunctionIdentifier::revFunction:
			result = arg1_value->NewMatchingType();

			for (int value_index = arg1_count - 1; value_index >= 0; --value_index)
				result->PushValueFromIndexOfScriptValue(value_index, arg1_value);
			break;
			
		case FunctionIdentifier::sortFunction:
		case FunctionIdentifier::anyFunction:
		case FunctionIdentifier::allFunction:
		case FunctionIdentifier::strsplitFunction:
		case FunctionIdentifier::pasteFunction:
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): function unimplemented." << endl << slim_terminate();
			break;
			
			// data class testing/coercion functions
			
		case FunctionIdentifier::classFunction:
			string_result->PushString(StringForScriptValueType(arg1_value->Type()));
			break;
			
		case FunctionIdentifier::isLogicalFunction:
		case FunctionIdentifier::isStringFunction:
		case FunctionIdentifier::isIntegerFunction:
		case FunctionIdentifier::isFloatFunction:
		case FunctionIdentifier::isObjectFunction:
		case FunctionIdentifier::asLogicalFunction:
		case FunctionIdentifier::asStringFunction:
		case FunctionIdentifier::asIntegerFunction:
		case FunctionIdentifier::asFloatFunction:
		case FunctionIdentifier::isFiniteFunction:
		case FunctionIdentifier::isNaNFunction:
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): function unimplemented." << endl << slim_terminate();
			break;
			
			// math functions, all implemented using the standard C++ function of the same name
			
		case FunctionIdentifier::acosFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(acos(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::asinFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(asin(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::atanFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(atan(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::atan2Function:
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): function unimplemented." << endl << slim_terminate();
			break;
			
		case FunctionIdentifier::cosFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(cos(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::sinFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(sin(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::tanFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(tan(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::expFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(exp(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::logFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(log(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::log10Function:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(log10(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::log2Function:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(log2(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::sqrtFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(sqrt(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::absFunction:
			if (arg1_type == ScriptValueType::kValueInt)
			{
				int_result = new ScriptValue_Int();
				result = int_result;
				
				for (int value_index = 0; value_index < arg1_count; ++value_index)
					int_result->PushInt(llabs(arg1_value->IntAtIndex(value_index)));
			}
			else if (arg1_type == ScriptValueType::kValueFloat)
			{
				float_result = new ScriptValue_Float();
				result = float_result;
				
				for (int value_index = 0; value_index < arg1_count; ++value_index)
					float_result->PushFloat(fabs(arg1_value->FloatAtIndex(value_index)));
			}
			break;
			
		case FunctionIdentifier::ceilFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(ceil(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::floorFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(floor(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::roundFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(round(arg1_value->FloatAtIndex(value_index)));
			break;
			
		case FunctionIdentifier::truncFunction:
			for (int value_index = 0; value_index < arg1_count; ++value_index)
				float_result->PushFloat(trunc(arg1_value->FloatAtIndex(value_index)));
			break;
			
			// bookkeeping functions
			
		case FunctionIdentifier::stopFunction:
			if (arg1_value)
				p_output_stream << arg1_value->StringAtIndex(0) << endl;
			
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): stop() called by user code." << endl << slim_terminate();
			break;
			
		case FunctionIdentifier::versionFunction:
			string_result->PushString("SLiMscript version 2.0a1");
			break;
			
		case FunctionIdentifier::licenseFunction:
			p_output_stream << "SLiM is free software: you can redistribute it and/or" << endl;
			p_output_stream << "modify it under the terms of the GNU General Public" << endl;
			p_output_stream << "License as published by the Free Software Foundation," << endl;
			p_output_stream << "either version 3 of the License, or (at your option)" << endl;
			p_output_stream << "any later version." << endl << endl;
			
			p_output_stream << "SLiM is distributed in the hope that it will be" << endl;
			p_output_stream << "useful, but WITHOUT ANY WARRANTY; without even the" << endl;
			p_output_stream << "implied warranty of MERCHANTABILITY or FITNESS FOR" << endl;
			p_output_stream << "A PARTICULAR PURPOSE.  See the GNU General Public" << endl;
			p_output_stream << "License for more details." << endl << endl;
			
			p_output_stream << "You should have received a copy of the GNU General" << endl;
			p_output_stream << "Public License along with SLiM.  If not, see" << endl;
			p_output_stream << "<http://www.gnu.org/licenses/>." << endl;
			break;
			
		case FunctionIdentifier::helpFunction:
			p_output_stream << "Help for SLiMscript is currently unimplemented." << endl;
			break;
			
		case FunctionIdentifier::lsFunction:
			p_output_stream << p_interpreter.BorrowSymbolTable();
			break;
		
		case FunctionIdentifier::functionFunction:
		{
			string match_string = (arg1_value ? arg1_value->StringAtIndex(0) : "");
			bool signature_found = false;
			
			for (auto functionPairIter = gBuiltInFunctionMap.begin(); functionPairIter != gBuiltInFunctionMap.end(); ++functionPairIter)
			{
				const FunctionSignature *iter_signature = functionPairIter->second;
				
				if (arg1_value && (iter_signature->function_name_.compare(match_string) != 0))
					continue;
				
				p_output_stream << *iter_signature << endl;
				signature_found = true;
			}
			
			if (arg1_value && !signature_found)
				p_output_stream << "No function signature found for \"" << match_string << "\"." << endl;
			
			break;
		}
			
		case FunctionIdentifier::dateFunction:
		case FunctionIdentifier::timeFunction:
			SLIM_TERMINATION << "ERROR (ExecuteFunctionCall): function unimplemented." << endl << slim_terminate();
			break;
			
			// proxy instantiation
			
		case FunctionIdentifier::PathFunction:
			if (n_args == 1)
				result = new ScriptValue_PathProxy(arg1_value->StringAtIndex(0));
			else
				result = new ScriptValue_PathProxy();
			break;
	}
	
	// Check the return value against the signature
	signature->CheckReturn("function", result);
	
	return result;
}

ScriptValue *ExecuteMethodCall(ScriptValue_Proxy *method_object, std::string const &p_method_name, std::vector<ScriptValue*> const &p_arguments, std::ostream &p_output_stream, ScriptInterpreter &p_interpreter)
{
	ScriptValue *result = nullptr;
	
	// Get the function signature and check our arguments against it
	const FunctionSignature *method_signature = method_object->SignatureForMethod(p_method_name);
	
	method_signature->CheckArguments("method", p_arguments);
	
	// Make the method call
	result = method_object->ExecuteMethod(p_method_name, p_arguments, p_output_stream, p_interpreter);
	
	// Check the return value against the signature
	method_signature->CheckReturn("method", result);
	
	return result;
}


//
//	FunctionSignature
//
#pragma mark FunctionSignature

FunctionSignature::FunctionSignature(std::string p_function_name, FunctionIdentifier p_function_id, ScriptValueMask p_return_mask) :
function_name_(p_function_name), function_id_(p_function_id), return_mask_(p_return_mask)
{
}

FunctionSignature *FunctionSignature::AddArg(ScriptValueMask p_arg_mask)
{
	bool is_optional = !!(p_arg_mask & kScriptValueMaskOptional);
	
	if (has_optional_args_ && !is_optional)
		SLIM_TERMINATION << "ERROR (FunctionSignature::AddArg): cannot add a required argument after an optional argument has been added." << endl << slim_terminate();
	
	if (has_ellipsis_)
		SLIM_TERMINATION << "ERROR (FunctionSignature::AddArg): cannot add an argument after an ellipsis." << endl << slim_terminate();
	
	arg_masks_.push_back(p_arg_mask);
	
	if (is_optional)
		has_optional_args_ = true;
	
	return this;
}

FunctionSignature *FunctionSignature::AddEllipsis()
{
	if (has_optional_args_)
		SLIM_TERMINATION << "ERROR (FunctionSignature::AddEllipsis): cannot add an ellipsis after an optional argument has been added." << endl << slim_terminate();
	
	if (has_ellipsis_)
		SLIM_TERMINATION << "ERROR (FunctionSignature::AddEllipsis): cannot add more than one ellipsis." << endl << slim_terminate();
	
	has_ellipsis_ = true;
	
	return this;
}

FunctionSignature *FunctionSignature::AddLogical()			{ return AddArg(kScriptValueMaskLogical); }
FunctionSignature *FunctionSignature::AddInt()				{ return AddArg(kScriptValueMaskInt); }
FunctionSignature *FunctionSignature::AddFloat()			{ return AddArg(kScriptValueMaskFloat); }
FunctionSignature *FunctionSignature::AddString()			{ return AddArg(kScriptValueMaskString); }
FunctionSignature *FunctionSignature::AddProxy()			{ return AddArg(kScriptValueMaskProxy); }
FunctionSignature *FunctionSignature::AddNumeric()			{ return AddArg(kScriptValueMaskNumeric); }
FunctionSignature *FunctionSignature::AddLogicalEquiv()		{ return AddArg(kScriptValueMaskLogicalEquiv); }
FunctionSignature *FunctionSignature::AddAnyBase()			{ return AddArg(kScriptValueMaskAnyBase); }
FunctionSignature *FunctionSignature::AddAny()				{ return AddArg(kScriptValueMaskAny); }

FunctionSignature *FunctionSignature::AddLogical_O()		{ return AddArg(kScriptValueMaskLogical | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddInt_O()			{ return AddArg(kScriptValueMaskInt | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddFloat_O()			{ return AddArg(kScriptValueMaskFloat | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddString_O()			{ return AddArg(kScriptValueMaskString | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddProxy_O()			{ return AddArg(kScriptValueMaskProxy | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddNumeric_O()		{ return AddArg(kScriptValueMaskNumeric | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddLogicalEquiv_O()	{ return AddArg(kScriptValueMaskLogicalEquiv | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddAnyBase_O()		{ return AddArg(kScriptValueMaskAnyBase | kScriptValueMaskOptional); }
FunctionSignature *FunctionSignature::AddAny_O()			{ return AddArg(kScriptValueMaskAny | kScriptValueMaskOptional); }

FunctionSignature *FunctionSignature::AddLogical_S()		{ return AddArg(kScriptValueMaskLogical | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddInt_S()			{ return AddArg(kScriptValueMaskInt | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddFloat_S()			{ return AddArg(kScriptValueMaskFloat | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddString_S()			{ return AddArg(kScriptValueMaskString | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddProxy_S()			{ return AddArg(kScriptValueMaskProxy | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddNumeric_S()		{ return AddArg(kScriptValueMaskNumeric | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddLogicalEquiv_S()	{ return AddArg(kScriptValueMaskLogicalEquiv | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddAnyBase_S()		{ return AddArg(kScriptValueMaskAnyBase | kScriptValueMaskSingleton); }
FunctionSignature *FunctionSignature::AddAny_S()			{ return AddArg(kScriptValueMaskAny | kScriptValueMaskSingleton); }

FunctionSignature *FunctionSignature::AddLogical_OS()		{ return AddArg(kScriptValueMaskLogical | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddInt_OS()			{ return AddArg(kScriptValueMaskInt | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddFloat_OS()			{ return AddArg(kScriptValueMaskFloat | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddString_OS()		{ return AddArg(kScriptValueMaskString | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddProxy_OS()			{ return AddArg(kScriptValueMaskProxy | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddNumeric_OS()		{ return AddArg(kScriptValueMaskNumeric | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddLogicalEquiv_OS()	{ return AddArg(kScriptValueMaskLogicalEquiv | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddAnyBase_OS()		{ return AddArg(kScriptValueMaskAnyBase | kScriptValueMaskOptSingleton); }
FunctionSignature *FunctionSignature::AddAny_OS()			{ return AddArg(kScriptValueMaskAny | kScriptValueMaskOptSingleton); }

void FunctionSignature::CheckArguments(std::string const &p_call_type, std::vector<ScriptValue*> const &p_arguments) const
{
	// Check the number of arguments supplied
	int n_args = (int)p_arguments.size();
	
	if (!has_ellipsis_)
	{
		if (n_args > arg_masks_.size())
			SLIM_TERMINATION << "ERROR (FunctionSignature::CheckArguments): " << p_call_type << " " << function_name_ << "() requires at most " << arg_masks_.size() << " argument(s), but " << n_args << " are supplied." << endl << slim_terminate();
	}
	
	// Check the types of all arguments specified in the signature
	for (int arg_index = 0; arg_index < arg_masks_.size(); ++arg_index)
	{
		ScriptValueMask type_mask = arg_masks_[arg_index];		// the static_casts are annoying but I like scoped enums
		bool is_optional = !!(type_mask & kScriptValueMaskOptional);
		bool requires_singleton = !!(type_mask & kScriptValueMaskSingleton);
		
		type_mask &= kScriptValueMaskFlagStrip;
		
		// if no argument was passed for this slot, it needs to be an optional slot
		if (n_args <= arg_index)
		{
			if (is_optional)
				break;			// all the rest of the arguments must be optional, so we're done checking
			else
				SLIM_TERMINATION << "ERROR (FunctionSignature::CheckArguments): missing required argument for " << p_call_type << " " << function_name_ << "()." << endl << slim_terminate();
		}
		
		// an argument was passed, so check its type
		ScriptValue *argument = p_arguments[arg_index];
		ScriptValueType arg_type = argument->Type();
		
		if (type_mask != kScriptValueMaskAny)
		{
			bool type_ok = true;
			
			switch (arg_type)
			{
				case ScriptValueType::kValueNULL:		type_ok = !!(type_mask & kScriptValueMaskNULL);		break;
				case ScriptValueType::kValueLogical:	type_ok = !!(type_mask & kScriptValueMaskLogical);	break;
				case ScriptValueType::kValueString:		type_ok = !!(type_mask & kScriptValueMaskString);	break;
				case ScriptValueType::kValueInt:		type_ok = !!(type_mask & kScriptValueMaskInt);		break;
				case ScriptValueType::kValueFloat:		type_ok = !!(type_mask & kScriptValueMaskFloat);	break;
				case ScriptValueType::kValueProxy:		type_ok = !!(type_mask & kScriptValueMaskProxy);	break;
			}
			
			if (!type_ok)
				SLIM_TERMINATION << "ERROR (FunctionSignature::CheckArguments): argument " << arg_index + 1 << " cannot be type " << arg_type << " for " << p_call_type << " " << function_name_ << "()." << endl << slim_terminate();
			
			if (requires_singleton && (argument->Count() != 1))
				SLIM_TERMINATION << "ERROR (FunctionSignature::CheckArguments): argument " << arg_index + 1 << " must be a singleton (size() == 1) for " << p_call_type << " " << function_name_ << "(), but size() == " << argument->Count() << "." << endl << slim_terminate();
		}
	}
}

void FunctionSignature::CheckReturn(std::string const &p_call_type, ScriptValue *p_result) const
{
	uint32_t retmask = return_mask_;
	bool return_type_ok = true;
	
	switch (p_result->Type())
	{
		case ScriptValueType::kValueNULL:		return_type_ok = !!(retmask & kScriptValueMaskNULL);		break;
		case ScriptValueType::kValueLogical:	return_type_ok = !!(retmask & kScriptValueMaskLogical);	break;
		case ScriptValueType::kValueInt:		return_type_ok = !!(retmask & kScriptValueMaskInt);		break;
		case ScriptValueType::kValueFloat:		return_type_ok = !!(retmask & kScriptValueMaskFloat);		break;
		case ScriptValueType::kValueString:		return_type_ok = !!(retmask & kScriptValueMaskString);	break;
		case ScriptValueType::kValueProxy:		return_type_ok = !!(retmask & kScriptValueMaskProxy);		break;
	}
	
	if (!return_type_ok)
		SLIM_TERMINATION << "ERROR (FunctionSignature::CheckReturn): internal error: return value cannot be type " << p_result->Type() << " for " << p_call_type << " " << function_name_ << "()." << endl << slim_terminate();
	
	bool return_is_singleton = !!(retmask & kScriptValueMaskSingleton);
	
	if (return_is_singleton && (p_result->Count() != 1))
		SLIM_TERMINATION << "ERROR (FunctionSignature::CheckReturn): internal error: return value must be a singleton (size() == 1) for " << p_call_type << " " << function_name_ << "(), but size() == " << p_result->Count() << endl << slim_terminate();
}

std::ostream &operator<<(std::ostream &p_outstream, const FunctionSignature &p_signature)
{
	p_outstream << "- (" << StringForScriptValueMask(p_signature.return_mask_) << ")" << p_signature.function_name_ << "(";
	
	int arg_mask_count = (int)p_signature.arg_masks_.size();
	
	if (arg_mask_count == 0)
	{
		if (!p_signature.has_ellipsis_)
			p_outstream << "void";
	}
	else
	{
		for (int arg_index = 0; arg_index < arg_mask_count; ++arg_index)
		{
			ScriptValueMask type_mask = p_signature.arg_masks_[arg_index];
			
			if (arg_index > 0)
				p_outstream << ", ";
			
			p_outstream << StringForScriptValueMask(type_mask);
		}
	}
	
	if (p_signature.has_ellipsis_)
		p_outstream << ((arg_mask_count > 0) ? ", ..." : "...");
	
	p_outstream << ")";
	
	return p_outstream;
}

































































