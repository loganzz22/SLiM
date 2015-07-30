//
//  eidos_value.h
//  Eidos
//
//  Created by Ben Haller on 4/7/15.
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

/*
 
 The class EidosValue represents any variable value in a EidosInterpreter context.  EidosValue itself is an abstract base class.
 Subclasses define types for NULL, logical, string, integer, and float types.  EidosValue types are generally vectors (although
 null is an exception); there are no scalar types in Eidos.
 
 */

#ifndef __Eidos__eidos_value__
#define __Eidos__eidos_value__

#include <iostream>
#include <vector>
#include <string>

#include "eidos_global.h"


class EidosValue;
class EidosValue_NULL;
class EidosValue_Logical;

struct EidosFunctionSignature;
class EidosInterpreter;

class EidosObjectElement;	// the value type for EidosValue_Object; defined at the bottom of this file


extern EidosValue_NULL *gStaticEidosValueNULL;
extern EidosValue_NULL *gStaticEidosValueNULLInvisible;

extern EidosValue_Logical *gStaticEidosValue_LogicalT;
extern EidosValue_Logical *gStaticEidosValue_LogicalF;


// EidosValueType is an enum of the possible types for EidosValue objects.  Note that all of these types are vectors of the stated
// type; all objects in Eidos are vectors.  The order of these types is in type-promotion order, from lowest to highest, except
// that NULL never gets promoted to any other type, and nothing ever gets promoted to object type.
enum class EidosValueType
{
	kValueNULL = 0,		// special NULL type; this cannot be mixed with other types or promoted to other types
	
	kValueLogical,		// logicals (bools)
	kValueInt,			// (64-bit) integers
	kValueFloat,		// (double-precision) floats
	kValueString,		// strings
	
	kValueObject		// a vector of EidosObjectElement objects: these represent built-in objects with member variables and methods
};

std::string StringForEidosValueType(const EidosValueType p_type);
std::ostream &operator<<(std::ostream &p_outstream, const EidosValueType p_type);

int CompareEidosValues(const EidosValue *p_value1, int p_index1, const EidosValue *p_value2, int p_index2);


// EidosValueMask is a uint32_t used as a bit mask to identify permitted types for EidosValue objects (arguments, returns)
typedef uint32_t EidosValueMask;

const EidosValueMask kValueMaskNone =			0x00000000;
const EidosValueMask kValueMaskNULL =			0x00000001;
const EidosValueMask kValueMaskLogical =			0x00000002;
const EidosValueMask kValueMaskInt =				0x00000004;
const EidosValueMask kValueMaskFloat =			0x00000008;
const EidosValueMask kValueMaskString =			0x00000010;
const EidosValueMask kValueMaskObject =			0x00000020;
	
const EidosValueMask kValueMaskOptional =		0x80000000;
const EidosValueMask kValueMaskSingleton =		0x40000000;
const EidosValueMask kValueMaskOptSingleton =	(kValueMaskOptional | kValueMaskSingleton);
const EidosValueMask kValueMaskFlagStrip =		0x3FFFFFFF;
	
const EidosValueMask kValueMaskNumeric =			(kValueMaskInt | kValueMaskFloat);										// integer or float
const EidosValueMask kValueMaskLogicalEquiv =	(kValueMaskLogical | kValueMaskInt | kValueMaskFloat);			// logical, integer, or float
const EidosValueMask kValueMaskAnyBase =			(kValueMaskNULL | kValueMaskLogicalEquiv | kValueMaskString);		// any type except object
const EidosValueMask kValueMaskAny =				(kValueMaskAnyBase | kValueMaskObject);									// any type including object

std::string StringForEidosValueMask(const EidosValueMask p_mask);
//std::ostream &operator<<(std::ostream &p_outstream, const EidosValueMask p_mask);	// can't do this since EidosValueMask is just uint32_t


//	*********************************************************************************************************
//
//	A class representing a value resulting from script evaluation.  Eidos is quite dynamically typed;
//	problems cause runtime exceptions.  EidosValue is the abstract base class for all value classes.
//

class EidosValue
{
	//	This class has its assignment operator disabled, to prevent accidental copying.
private:
	
	bool external_temporary_ = false;						// if true, the value should not be deleted, as it is owned by someone else
	bool external_permanent_ = false;						// if true, the value is owned but guaranteed long-lived; see below
	
protected:
	
	bool invisible_ = false;								// as in R; if true, the value will not normally be printed to the console
	
public:
	
	EidosValue(const EidosValue &p_original);				// can copy-construct (but see the definition; NOT default)
	EidosValue& operator=(const EidosValue&) = delete;	// no copying
	
	EidosValue(void);										// null constructor for base class
	virtual ~EidosValue(void) = 0;							// virtual destructor
	
	// basic methods
	virtual EidosValueType Type(void) const = 0;			// the type contained by the vector
	virtual int Count(void) const = 0;						// the number of values in the vector
	virtual void Print(std::ostream &p_ostream) const = 0;	// standard printing
	
	// getter only; invisible objects must be made through construction or InvisibleCopy()
	inline bool Invisible(void) const							{ return invisible_; }
	
	// Memory management flags for EidosValue objects.  This is a complex topic.  There are basically three
	// statuses that a EidosValue can have:
	//
	// Temporary: the object is referred to by a single pointer, typically, and is handed off from method to
	// method.  Whoever has the pointer owns the object and is responsible for deleting it.  Anybody who is
	// given the pointer can take ownership of the object by setting it to one of the other two statuses.  If
	// you give the pointer to a temporary object to somebody who might do that, then you need to check the
	// value of IsTemporary() before deleting your pointer to it.  This pattern of usage is common in the
	// interpreter's execution methods; values are created by executing script nodes, and are passed around
	// until they are either deleted or are taken by a symbol table.
	//
	// Externally-owned permanent: the pointer to the object is owned by a specific owner (they know who they
	// are), and others must respect that.  The object is guaranteed to be permanent and constant, however,
	// so anybody may keep the pointer and continue using it (allowing for lots of optimization).  "Permanent"
	// has a specific sense: the value must be guaranteed to live longer than the symbol table for the current
	// interpreter, so that as far as any code running in the interpreter is concerned, the value is guaranteed
	// to exist "forever".  Keeping a reference to even these objects is unsafe beyond the end of the current
	// interpreter context, though.
	//
	// Externally-owned temporary: again, the pointer is owned by a specific owner (they know who they are),
	// and others must respect that.  Here, the object is guaranteed to be constant, but only to have the same
	// lifetime as a temporary value.  So if you are given a pointer, feel free to use that pointer without
	// copying it, and to return that pointer to whoever called you, but do not keep a copy of the pointer for
	// yourself for any longer duration.  This is conceptually rather like an autoreleased pointer in Obj-C,
	// although it does not involve retain counts or an autorelease pool; at some nebulous time in the future,
	// after you yourself return, the object may go away without warning, but that is not your problem.
	//
	// The "externally-owned temporary" flag used to be called "in symbol table", because when a symbol table
	// took ownership of a value, that object then became owned (not temporary) but not permanent (and thus
	// in need of being copied if someone else also wanted a safe long-term pointer to it).  The new term is
	// a bit more clear on what the semantics really means, though, I think.
	//
	// Setting externally owned permanent is basically a promise that the value object will live longer than the
	// symbol table that it might end up in.  That is a hard guarantee to make.  Either the object has to be truly
	// permanent, or you have to be setting up the symbol table yourself so you know its lifetime.  Apart from
	// these very restricted situations, calling SetExternalPermanent() is unsafe.  Notably, setting up an
	// externally owned cached EidosValue* for a given property value is NOT SAFE unless the cache never needs
	// to be invalidated, since the user could put the cached value into the symbol table, and the symbol
	// table would assume that the value will never go away.  Instead, use the externally owned temporary flag.
	//
	// And yes, this would be much simpler if I used refcounted pointers; perhaps I will be forced to that
	// eventually, but I really don't want to pay the speed penalty unless I absolutely must.
	inline bool IsTemporary(void) const							{ return !(external_temporary_ || external_permanent_); };
	inline bool IsExternalTemporary(void) const					{ return external_temporary_; };
	inline bool IsExternalPermanent(void) const					{ return external_permanent_; };
	inline EidosValue *SetExternalTemporary()					{ external_temporary_ = true; return this; };
	inline EidosValue *SetExternalPermanent()					{ external_permanent_ = true; return this; };
	
	// basic subscript access; abstract here since we want to force subclasses to define this
	virtual EidosValue *GetValueAtIndex(const int p_idx) const = 0;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value) = 0;
	
	// fetching individual values; these convert type if necessary, and (base class behavior) raise if impossible
	virtual bool LogicalAtIndex(int p_idx) const;
	virtual std::string StringAtIndex(int p_idx) const;
	virtual int64_t IntAtIndex(int p_idx) const;
	virtual double FloatAtIndex(int p_idx) const;
	virtual EidosObjectElement *ElementAtIndex(int p_idx) const;
	
	// methods to allow type-agnostic manipulation of EidosValues
	virtual bool IsMutable(void) const;							// returns true by default, but we have some immutable subclasses that return false
	virtual EidosValue *MutableCopy(void) const;				// just calls CopyValues() by default, but guarantees a mutable copy
	virtual EidosValue *CopyValues(void) const = 0;			// a deep copy of the receiver with external_temporary_ == invisible_ == false
	virtual EidosValue *NewMatchingType(void) const = 0;		// a new EidosValue instance of the same type as the receiver
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value) = 0;	// copy a value from another object of the same type
	virtual void Sort(bool p_ascending) = 0;
};

std::ostream &operator<<(std::ostream &p_outstream, const EidosValue &p_value);


//	*********************************************************************************************************
//
//	EidosValue_NULL and EidosValue_NULL_const represent NULL values in Eidos.  EidosValue_NULL_const
//	is used for static global EidosValue_NULL instances; we have two, one invisible, one not.
//

class EidosValue_NULL : public EidosValue
{
public:
	EidosValue_NULL(const EidosValue_NULL &p_original) = default;	// can copy-construct
	EidosValue_NULL& operator=(const EidosValue_NULL&) = delete;	// no copying
	
	EidosValue_NULL(void);
	virtual ~EidosValue_NULL(void);
	
	virtual EidosValueType Type(void) const;
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);

	virtual EidosValue *CopyValues(void) const;
	virtual EidosValue *NewMatchingType(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};

class EidosValue_NULL_const : public EidosValue_NULL
{
private:
	EidosValue_NULL_const(void) = default;
	
public:
	EidosValue_NULL_const(const EidosValue_NULL_const &p_original) = delete;	// no copy-construct
	EidosValue_NULL_const& operator=(const EidosValue_NULL_const&) = delete;	// no copying
	virtual ~EidosValue_NULL_const(void);										// destructor calls eidos_terminate()
	
	static EidosValue_NULL *Static_EidosValue_NULL(void);
	static EidosValue_NULL *Static_EidosValue_NULL_Invisible(void);
};


//	*********************************************************************************************************
//
//	EidosValue_Logical and EidosValue_Logical_const represent logical (bool) values in Eidos.
//	EidosValue_Logical_const is used for static global EidosValue_Logical instances; we have two, T and F.
//	Because there are only two values, T and F, we don't need a singleton class, we just use those globals.
//

class EidosValue_Logical : public EidosValue
{
private:
	std::vector<bool> values_;
	
public:
	EidosValue_Logical(const EidosValue_Logical &p_original) = default;		// can copy-construct
	EidosValue_Logical& operator=(const EidosValue_Logical&) = delete;	// no copying
	
	EidosValue_Logical(void);
	explicit EidosValue_Logical(std::vector<bool> &p_boolvec);
	explicit EidosValue_Logical(bool p_bool1);
	EidosValue_Logical(bool p_bool1, bool p_bool2);
	EidosValue_Logical(bool p_bool1, bool p_bool2, bool p_bool3);
	EidosValue_Logical(bool p_bool1, bool p_bool2, bool p_bool3, bool p_bool4);
	EidosValue_Logical(bool p_bool1, bool p_bool2, bool p_bool3, bool p_bool4, bool p_bool5);
	EidosValue_Logical(bool p_bool1, bool p_bool2, bool p_bool3, bool p_bool4, bool p_bool5, bool p_bool6);
	virtual ~EidosValue_Logical(void);
	
	virtual EidosValueType Type(void) const;
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	const std::vector<bool> &LogicalVector(void) const;
	virtual bool LogicalAtIndex(int p_idx) const;
	virtual std::string StringAtIndex(int p_idx) const;
	virtual int64_t IntAtIndex(int p_idx) const;
	virtual double FloatAtIndex(int p_idx) const;
	
	virtual void PushLogical(bool p_logical);
	virtual void SetLogicalAtIndex(const int p_idx, bool p_logical);
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	
	virtual EidosValue *CopyValues(void) const;
	virtual EidosValue *NewMatchingType(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};

class EidosValue_Logical_const : public EidosValue_Logical
{
private:
	EidosValue_Logical_const(void) = default;
	
public:
	EidosValue_Logical_const(const EidosValue_Logical_const &p_original) = delete;	// no copy-construct
	EidosValue_Logical_const& operator=(const EidosValue_Logical_const&) = delete;	// no copying
	explicit EidosValue_Logical_const(bool p_bool1);
	virtual ~EidosValue_Logical_const(void);											// destructor calls eidos_terminate()
	
	static EidosValue_Logical *Static_EidosValue_Logical_T(void);
	static EidosValue_Logical *Static_EidosValue_Logical_F(void);
	
	virtual bool IsMutable(void) const;
	virtual EidosValue *MutableCopy(void) const;
	
	// prohibited actions
	virtual void PushLogical(bool p_logical);
	virtual void SetLogicalAtIndex(const int p_idx, bool p_logical);
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};


//	*********************************************************************************************************
//
//	EidosValue_String represents string values in Eidos.  At the present time it doesn't seem worth
//	having a singleton class, since working with strings is not that common in Eidos and it unlikely
//	to happen in hotspot areas like callbacks.
//

class EidosValue_String : public EidosValue
{
private:
	std::vector<std::string> values_;
	
public:
	EidosValue_String(const EidosValue_String &p_original) = default;	// can copy-construct
	EidosValue_String& operator=(const EidosValue_String&) = delete;	// no copying
	
	EidosValue_String(void);
	explicit EidosValue_String(std::vector<std::string> &p_stringvec);
	explicit EidosValue_String(const std::string &p_string1);
	EidosValue_String(const std::string &p_string1, const std::string &p_string2);
	EidosValue_String(const std::string &p_string1, const std::string &p_string2, const std::string &p_string3);
	EidosValue_String(const std::string &p_string1, const std::string &p_string2, const std::string &p_string3, const std::string &p_string4);
	EidosValue_String(const std::string &p_string1, const std::string &p_string2, const std::string &p_string3, const std::string &p_string4, const std::string &p_string5);
	EidosValue_String(const std::string &p_string1, const std::string &p_string2, const std::string &p_string3, const std::string &p_string4, const std::string &p_string5, const std::string &p_string6);
	virtual ~EidosValue_String(void);
	
	virtual EidosValueType Type(void) const;
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	const std::vector<std::string> &StringVector(void) const;
	virtual bool LogicalAtIndex(int p_idx) const;
	virtual std::string StringAtIndex(int p_idx) const;
	virtual int64_t IntAtIndex(int p_idx) const;
	virtual double FloatAtIndex(int p_idx) const;
	
	void PushString(const std::string &p_string);
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	
	virtual EidosValue *CopyValues(void) const;
	virtual EidosValue *NewMatchingType(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};


//	*********************************************************************************************************
//
//	EidosValue_Int represents integer (C++ int64_t) values in Eidos.  The subclass
//	EidosValue_Int_vector is the standard instance class, used to hold vectors of floats.
//	EidosValue_Int_singleton_const is used for speed, to represent single constant values.
//

class EidosValue_Int : public EidosValue
{
protected:
	EidosValue_Int(const EidosValue_Int &p_original) = default;		// can copy-construct
	EidosValue_Int(void) = default;									// default constructor
	
public:
	EidosValue_Int& operator=(const EidosValue_Int&) = delete;		// no copying
	virtual ~EidosValue_Int(void);
	
	virtual EidosValueType Type(void) const;
	virtual int Count(void) const = 0;
	virtual void Print(std::ostream &p_ostream) const = 0;
	
	virtual bool LogicalAtIndex(int p_idx) const = 0;
	virtual std::string StringAtIndex(int p_idx) const = 0;
	virtual int64_t IntAtIndex(int p_idx) const = 0;
	virtual double FloatAtIndex(int p_idx) const = 0;
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const = 0;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value) = 0;
	
	virtual EidosValue *CopyValues(void) const = 0;
	virtual EidosValue *NewMatchingType(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value) = 0;
	virtual void Sort(bool p_ascending) = 0;
};

class EidosValue_Int_vector : public EidosValue_Int
{
private:
	std::vector<int64_t> values_;
	
public:
	EidosValue_Int_vector(const EidosValue_Int_vector &p_original) = default;		// can copy-construct
	EidosValue_Int_vector& operator=(const EidosValue_Int_vector&) = delete;	// no copying
	
	EidosValue_Int_vector(void);
	explicit EidosValue_Int_vector(std::vector<int> &p_intvec);
	explicit EidosValue_Int_vector(std::vector<int64_t> &p_intvec);
	//explicit EidosValue_Int_vector(int64_t p_int1);		// disabled to encourage use of EidosValue_Int_singleton_const for this case
	EidosValue_Int_vector(int64_t p_int1, int64_t p_int2);
	EidosValue_Int_vector(int64_t p_int1, int64_t p_int2, int64_t p_int3);
	EidosValue_Int_vector(int64_t p_int1, int64_t p_int2, int64_t p_int3, int64_t p_int4);
	EidosValue_Int_vector(int64_t p_int1, int64_t p_int2, int64_t p_int3, int64_t p_int4, int64_t p_int5);
	EidosValue_Int_vector(int64_t p_int1, int64_t p_int2, int64_t p_int3, int64_t p_int4, int64_t p_int5, int64_t p_int6);
	virtual ~EidosValue_Int_vector(void);
	
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	const std::vector<int64_t> &IntVector(void) const;
	virtual bool LogicalAtIndex(int p_idx) const;
	virtual std::string StringAtIndex(int p_idx) const;
	virtual int64_t IntAtIndex(int p_idx) const;
	virtual double FloatAtIndex(int p_idx) const;
	
	void PushInt(int64_t p_int);
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	
	virtual EidosValue *CopyValues(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};

class EidosValue_Int_singleton_const : public EidosValue_Int
{
private:
	int64_t value_;
	
public:
	EidosValue_Int_singleton_const(const EidosValue_Int_singleton_const &p_original) = delete;	// no copy-construct
	EidosValue_Int_singleton_const& operator=(const EidosValue_Int_singleton_const&) = delete;	// no copying
	EidosValue_Int_singleton_const(void) = delete;
	explicit EidosValue_Int_singleton_const(int64_t p_int1);
	virtual ~EidosValue_Int_singleton_const(void);
	
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	virtual bool LogicalAtIndex(int p_idx) const;
	virtual std::string StringAtIndex(int p_idx) const;
	virtual int64_t IntAtIndex(int p_idx) const;
	virtual double FloatAtIndex(int p_idx) const;
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual EidosValue *CopyValues(void) const;
	
	virtual bool IsMutable(void) const;
	virtual EidosValue *MutableCopy(void) const;
	
	// prohibited actions
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};


//	*********************************************************************************************************
//
//	EidosValue_Float represents floating-point (C++ double) values in Eidos.  The subclass
//	EidosValue_Float_vector is the standard instance class, used to hold vectors of floats.
//	EidosValue_Float_singleton_const is used for speed, to represent single constant values.
//

class EidosValue_Float : public EidosValue
{
protected:
	EidosValue_Float(const EidosValue_Float &p_original) = default;		// can copy-construct
	EidosValue_Float(void) = default;										// default constructor
	
public:
	EidosValue_Float& operator=(const EidosValue_Float&) = delete;		// no copying
	virtual ~EidosValue_Float(void);
	
	virtual EidosValueType Type(void) const;
	virtual int Count(void) const = 0;
	virtual void Print(std::ostream &p_ostream) const = 0;
	
	virtual bool LogicalAtIndex(int p_idx) const = 0;
	virtual std::string StringAtIndex(int p_idx) const = 0;
	virtual int64_t IntAtIndex(int p_idx) const = 0;
	virtual double FloatAtIndex(int p_idx) const = 0;
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const = 0;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value) = 0;
	
	virtual EidosValue *CopyValues(void) const = 0;
	virtual EidosValue *NewMatchingType(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value) = 0;
	virtual void Sort(bool p_ascending) = 0;
};

class EidosValue_Float_vector : public EidosValue_Float
{
private:
	std::vector<double> values_;
	
public:
	EidosValue_Float_vector(const EidosValue_Float_vector &p_original) = default;		// can copy-construct
	EidosValue_Float_vector& operator=(const EidosValue_Float_vector&) = delete;	// no copying
	
	EidosValue_Float_vector(void);
	explicit EidosValue_Float_vector(std::vector<double> &p_doublevec);
	EidosValue_Float_vector(double *p_doublebuf, int p_buffer_length);
	//explicit EidosValue_Float_vector(double p_float1);		// disabled to encourage use of EidosValue_Float_singleton_const for this case
	EidosValue_Float_vector(double p_float1, double p_float2);
	EidosValue_Float_vector(double p_float1, double p_float2, double p_float3);
	EidosValue_Float_vector(double p_float1, double p_float2, double p_float3, double p_float4);
	EidosValue_Float_vector(double p_float1, double p_float2, double p_float3, double p_float4, double p_float5);
	EidosValue_Float_vector(double p_float1, double p_float2, double p_float3, double p_float4, double p_float5, double p_float6);
	virtual ~EidosValue_Float_vector(void);
	
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	const std::vector<double> &FloatVector(void) const;
	virtual bool LogicalAtIndex(int p_idx) const;
	virtual std::string StringAtIndex(int p_idx) const;
	virtual int64_t IntAtIndex(int p_idx) const;
	virtual double FloatAtIndex(int p_idx) const;
	
	void PushFloat(double p_float);
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	
	virtual EidosValue *CopyValues(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};

class EidosValue_Float_singleton_const : public EidosValue_Float
{
private:
	double value_;
	
public:
	EidosValue_Float_singleton_const(const EidosValue_Float_singleton_const &p_original) = delete;	// no copy-construct
	EidosValue_Float_singleton_const& operator=(const EidosValue_Float_singleton_const&) = delete;	// no copying
	EidosValue_Float_singleton_const(void) = delete;
	explicit EidosValue_Float_singleton_const(double p_float1);
	virtual ~EidosValue_Float_singleton_const(void);
	
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	virtual bool LogicalAtIndex(int p_idx) const;
	virtual std::string StringAtIndex(int p_idx) const;
	virtual int64_t IntAtIndex(int p_idx) const;
	virtual double FloatAtIndex(int p_idx) const;
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual EidosValue *CopyValues(void) const;
	
	virtual bool IsMutable(void) const;
	virtual EidosValue *MutableCopy(void) const;
	
	// prohibited actions
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	virtual void Sort(bool p_ascending);
};


//	*********************************************************************************************************
//
//	EidosValue_Object represents objects in Eidos: entities that have properties and can respond to
//	methods.  The subclass EidosValue_Object_vector is the standard instance class, used to hold vectors
//	of floats.  EidosValue_Object_singleton_const is used for speed, to represent single constant values.
//

class EidosValue_Object : public EidosValue
{
protected:
	EidosValue_Object(const EidosValue_Object &p_original) = default;				// can copy-construct
	EidosValue_Object(void) = default;												// default constructor
	
public:
	EidosValue_Object& operator=(const EidosValue_Object&) = delete;				// no copying
	virtual ~EidosValue_Object(void);
	
	virtual EidosValueType Type(void) const;
	virtual const std::string *ElementType(void) const = 0;
	virtual int Count(void) const = 0;
	virtual void Print(std::ostream &p_ostream) const = 0;
	
	virtual EidosObjectElement *ElementAtIndex(int p_idx) const = 0;
	virtual EidosValue *GetValueAtIndex(const int p_idx) const = 0;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value) = 0;
	
	virtual EidosValue *CopyValues(void) const = 0;
	virtual EidosValue *NewMatchingType(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value) = 0;
	virtual void Sort(bool p_ascending);
	
	// Member and method support; defined only on EidosValue_Object, not EidosValue.  The methods that a
	// EidosValue_Object instance defines depend upon the type of the EidosObjectElement objects it contains.
	virtual std::vector<std::string> ReadOnlyMembersOfElements(void) const = 0;
	virtual std::vector<std::string> ReadWriteMembersOfElements(void) const = 0;
	virtual EidosValue *GetValueForMemberOfElements(EidosGlobalStringID p_member_id) const = 0;
	virtual EidosValue *GetRepresentativeValueOrNullForMemberOfElements(EidosGlobalStringID p_member_id) const = 0;			// used by code completion
	virtual void SetValueForMemberOfElements(EidosGlobalStringID p_member_id, EidosValue *p_value) = 0;
	
	virtual std::vector<std::string> MethodsOfElements(void) const = 0;
	virtual const EidosFunctionSignature *SignatureForMethodOfElements(EidosGlobalStringID p_method_id) const = 0;
	virtual EidosValue *ExecuteClassMethodOfElements(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter) = 0;
	virtual EidosValue *ExecuteInstanceMethodOfElements(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter) = 0;
};

class EidosValue_Object_vector : public EidosValue_Object
{
private:
	std::vector<EidosObjectElement *> values_;		// these use a retain/release system of ownership; see below
	
public:
	EidosValue_Object_vector(const EidosValue_Object_vector &p_original);				// can copy-construct, but it is not the default constructor since we need to deep copy
	EidosValue_Object_vector& operator=(const EidosValue_Object_vector&) = delete;		// no copying
	
	EidosValue_Object_vector(void);
	explicit EidosValue_Object_vector(std::vector<EidosObjectElement *> &p_elementvec);
	//explicit EidosValue_Object_vector(EidosObjectElement *p_element1);		// disabled to encourage use of EidosValue_Object_singleton_const for this case
	virtual ~EidosValue_Object_vector(void);
	
	virtual const std::string *ElementType(void) const;
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	virtual EidosObjectElement *ElementAtIndex(int p_idx) const;
	void PushElement(EidosObjectElement *p_element);
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	
	virtual EidosValue *CopyValues(void) const;
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	void SortBy(const std::string &p_property, bool p_ascending);
	
	// Member and method support; defined only on EidosValue_Object, not EidosValue.  The methods that a
	// EidosValue_Object instance defines depend upon the type of the EidosObjectElement objects it contains.
	virtual std::vector<std::string> ReadOnlyMembersOfElements(void) const;
	virtual std::vector<std::string> ReadWriteMembersOfElements(void) const;
	virtual EidosValue *GetValueForMemberOfElements(EidosGlobalStringID p_member_id) const;
	virtual EidosValue *GetRepresentativeValueOrNullForMemberOfElements(EidosGlobalStringID p_member_id) const;			// used by code completion
	virtual void SetValueForMemberOfElements(EidosGlobalStringID p_member_id, EidosValue *p_value);
	
	virtual std::vector<std::string> MethodsOfElements(void) const;
	virtual const EidosFunctionSignature *SignatureForMethodOfElements(EidosGlobalStringID p_method_id) const;
	virtual EidosValue *ExecuteClassMethodOfElements(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter);
	virtual EidosValue *ExecuteInstanceMethodOfElements(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter);
};

class EidosValue_Object_singleton_const : public EidosValue_Object
{
private:
	EidosObjectElement *value_;		// these use a retain/release system of ownership; see below
	
public:
	EidosValue_Object_singleton_const(const EidosValue_Object_singleton_const &p_original) = delete;		// no copy-construct
	EidosValue_Object_singleton_const& operator=(const EidosValue_Object_singleton_const&) = delete;		// no copying
	EidosValue_Object_singleton_const(void) = delete;
	explicit EidosValue_Object_singleton_const(EidosObjectElement *p_element1);
	virtual ~EidosValue_Object_singleton_const(void);
	
	virtual const std::string *ElementType(void) const;
	virtual int Count(void) const;
	virtual void Print(std::ostream &p_ostream) const;
	
	virtual EidosObjectElement *ElementAtIndex(int p_idx) const;
	
	virtual EidosValue *GetValueAtIndex(const int p_idx) const;
	virtual EidosValue *CopyValues(void) const;
	
	virtual bool IsMutable(void) const;
	virtual EidosValue *MutableCopy(void) const;
	
	// prohibited actions
	virtual void SetValueAtIndex(const int p_idx, EidosValue *p_value);
	virtual void PushValueFromIndexOfEidosValue(int p_idx, const EidosValue *p_source_script_value);
	
	// Member and method support; defined only on EidosValue_Object, not EidosValue.  The methods that a
	// EidosValue_Object instance defines depend upon the type of the EidosObjectElement objects it contains.
	virtual std::vector<std::string> ReadOnlyMembersOfElements(void) const;
	virtual std::vector<std::string> ReadWriteMembersOfElements(void) const;
	virtual EidosValue *GetValueForMemberOfElements(EidosGlobalStringID p_member_id) const;
	virtual EidosValue *GetRepresentativeValueOrNullForMemberOfElements(EidosGlobalStringID p_member_id) const;			// used by code completion
	virtual void SetValueForMemberOfElements(EidosGlobalStringID p_member_id, EidosValue *p_value);
	
	virtual std::vector<std::string> MethodsOfElements(void) const;
	virtual const EidosFunctionSignature *SignatureForMethodOfElements(EidosGlobalStringID p_method_id) const;
	virtual EidosValue *ExecuteClassMethodOfElements(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter);
	virtual EidosValue *ExecuteInstanceMethodOfElements(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter);
};


//	*********************************************************************************************************
//
// This is the value type of which EidosValue_Object is a vector, just as double is the value type of which
// EidosValue_Float is a vector.  EidosValue_Object is just a bag; this class is the abstract base class of
// the things that can be contained in that bag.  This class defines the methods that can be used by an
// instance of EidosValue_Object; EidosValue_Object just forwards such things on to this class.
// EidosObjectElement obeys sharing semantics; many EidosValue_Objects can refer to the same element, and
// EidosObjectElement never copies itself.  To manage its lifetime, refcounting is used.  Externally-owned
// objects (from the Context) do not use this refcount, since their lifetime is defined externally, but
// internally-owned objects (such as Path) use the refcount and delete themselves when they are done.
class EidosObjectElement
{
public:
	EidosObjectElement(const EidosObjectElement &p_original) = delete;		// can copy-construct
	EidosObjectElement& operator=(const EidosObjectElement&) = delete;		// no copying
	
	EidosObjectElement(void);
	virtual ~EidosObjectElement(void);
	
	virtual const std::string *ElementType(void) const = 0;
	virtual void Print(std::ostream &p_ostream) const;		// standard printing; prints ElementType()
	
	// refcounting; virtual no-ops here (for externally-owned objects), implemented in EidosObjectElementInternal
	virtual EidosObjectElement *Retain(void);
	virtual EidosObjectElement *Release(void);
	
	virtual std::vector<std::string> ReadOnlyMembers(void) const;
	virtual std::vector<std::string> ReadWriteMembers(void) const;
	virtual bool MemberIsReadOnly(EidosGlobalStringID p_member_id) const;
	virtual EidosValue *GetValueForMember(EidosGlobalStringID p_member_id);
	virtual void SetValueForMember(EidosGlobalStringID p_member_id, EidosValue *p_value);
	
	virtual std::vector<std::string> Methods(void) const;
	virtual const EidosFunctionSignature *SignatureForMethod(EidosGlobalStringID p_method_id) const;
	virtual EidosValue *ExecuteMethod(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter);
	
	// Utility methods for printing errors, checking types, etc.; the goal is to make subclasses as trim as possible
	void TypeCheckValue(const std::string &p_method_name, EidosGlobalStringID p_member_id, EidosValue *p_value, EidosValueMask p_type_mask);
	void RangeCheckValue(const std::string &p_method_name, EidosGlobalStringID p_member_id, bool p_in_range);
};

std::ostream &operator<<(std::ostream &p_outstream, const EidosObjectElement &p_element);


// A base class for EidosObjectElement subclasses that are internal to Eidos.  See comment above.
class EidosObjectElementInternal : public EidosObjectElement
{
private:
	uint32_t refcount_ = 1;				// start life with a refcount of 1; the allocator does not need to call Retain()
	
public:
	EidosObjectElementInternal(const EidosObjectElementInternal &p_original) = delete;		// can copy-construct
	EidosObjectElementInternal& operator=(const EidosObjectElementInternal&) = delete;		// no copying
	
	EidosObjectElementInternal(void);
	virtual ~EidosObjectElementInternal(void);
	
	virtual EidosObjectElement *Retain(void);
	virtual EidosObjectElement *Release(void);
};


#endif /* defined(__Eidos__eidos_value__) */



































































