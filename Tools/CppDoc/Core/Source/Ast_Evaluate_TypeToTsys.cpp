#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;
using namespace symbol_totsys_impl;

/***********************************************************************
TypeToTsys
  PrimitiveType				: literal
  ReferenceType				: unbounded
  ArrayType					: unbounded
  CallingConventionType		: unbounded		*
  FunctionType				: variant
  MemberType				: unbounded
  DeclType					: unbounded
  DecorateType				: unbounded
  RootType					: literal		*
  IdType					: identifier
  ChildType					: unbounded
  GenericType				: variant
***********************************************************************/

class TypeToTsysVisitor : public Object, public virtual ITypeVisitor
{
public:
	TypeTsysList&				result;
	bool						isVta = false;

	const ParsingArguments&		pa;
	TypeTsysList*				returnTypes;
	bool						memberOf = false;
	TsysCallingConvention		cc = TsysCallingConvention::None;

	TypeToTsysVisitor(const ParsingArguments& _pa, TypeTsysList& _result, TypeTsysList* _returnTypes, bool _memberOf, TsysCallingConvention _cc)
		:pa(_pa)
		, result(_result)
		, returnTypes(_returnTypes)
		, cc(_cc)
		, memberOf(_memberOf)
	{
	}

	void AddResult(ITsys* tsys)
	{
		AddTsysToResult(result, tsys);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// PrimitiveType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(PrimitiveType* self)override
	{
		AddResult(ProcessPrimitiveType(pa, self));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ReferenceType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ReferenceType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, isVta1);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
		{
			return ProcessReferenceType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ArrayType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ArrayType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, isVta1);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
		{
			return ProcessArrayType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CallingConventionType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(CallingConventionType* self)override
	{
		auto oldCc = cc;
		cc = self->callingConvention;

		self->type->Accept(this);
		cc = oldCc;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FunctionType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(FunctionType* self)override
	{
		VariadicInput<ITsys*> variadicInput(self->parameters.Count() + 1, pa);
		if (returnTypes)
		{
			variadicInput.ApplyTypes(0, *returnTypes, false);
		}
		else if (self->decoratorReturnType)
		{
			variadicInput.ApplySingle(0, self->decoratorReturnType);
		}
		else if (self->returnType)
		{
			variadicInput.ApplySingle(0, self->returnType);
		}
		else
		{
			TypeTsysList types;
			types.Add(pa.tsys->Void());
			variadicInput.ApplyTypes(0, types, false);
		}
		variadicInput.ApplyVariadicList(1, self->parameters);

		isVta = variadicInput.Expand(&self->parameters, result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessFunctionType(pa, processResult, self, cc, memberOf, args, boundedAnys);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// MemberType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(MemberType* self)override
	{
		TypeTsysList types, classTypes;
		bool typesVta = false;
		bool classTypesVta = false;
		TypeToTsysInternal(pa, self->type, types, typesVta, true, cc);
		TypeToTsysInternal(pa, self->classType, classTypes, classTypesVta);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem argType, ExprTsysItem argClass)
		{
			return ProcessMemberType(pa, self, argType, argClass);
		}, Input(types, typesVta), Input(classTypes, classTypesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DeclType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			bool typesVta = false;
			ExprToTsysInternal(pa, self->expr, types, typesVta);
			isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
			{
				return ProcessDeclType(pa, self, arg1);
			}, Input(types, typesVta));
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DecorateType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DecorateType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, isVta1);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
		{
			return ProcessDecorateType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// RootType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(RootType* self)override
	{
		throw NotConvertableException();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IdType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(IdType* self)override
	{
		CreateIdReferenceType(pa, self->resolving, false, true, result, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ChildType* self)override
	{
		if (!IsResolvedToType(self->classType))
		{
			CreateIdReferenceType(pa, self->resolving, true, false, result, isVta);
			return;
		}

		TypeTsysList classTypes;
		bool classVta = false;
		TypeToTsysInternal(pa, self->classType, classTypes, classVta);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem argClass)
		{
			ProcessChildType(pa, self, argClass, processResult);
		}, Input(classTypes, classVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// GenericType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(GenericType* self)override
	{
		vint count = self->arguments.Count() + 1;
		Array<bool> isTypes(count);
		isTypes[0] = false;

		VariadicInput<ExprTsysItem> variadicInput(count, pa);
		variadicInput.ApplySingle<Type>(0, self->type);
		variadicInput.ApplyGenericArguments(1, isTypes, self->arguments);
		isVta = variadicInput.Expand(&self->arguments, result,
			[this, self, &isTypes](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessGenericType(pa, processResult, self, isTypes, args, argSource, boundedAnys);
			});
	}
};

// Convert type AST to type system object

void TypeToTsysInternal(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, bool& isVta, bool memberOf, TsysCallingConvention cc)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, nullptr, memberOf, cc);
	t->Accept(&visitor);
	isVta = visitor.isVta;
}

void TypeToTsysInternal(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, bool& isVta, bool memberOf, TsysCallingConvention cc)
{
	TypeToTsysInternal(pa, t.Obj(), tsys, isVta, memberOf, cc);
}

void TypeToTsysNoVta(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, bool memberOf, TsysCallingConvention cc)
{
	bool isVta = false;
	TypeToTsysInternal(pa, t, tsys, isVta, memberOf, cc);
	if (isVta)
	{
		throw NotConvertableException();
	}
}

void TypeToTsysNoVta(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, bool memberOf, TsysCallingConvention cc)
{
	TypeToTsysNoVta(pa, t.Obj(), tsys, memberOf, cc);
}

void TypeToTsysAndReplaceFunctionReturnType(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& returnTypes, TypeTsysList& tsys, bool memberOf)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, &returnTypes, memberOf, TsysCallingConvention::None);
	t->Accept(&visitor);
	if (visitor.isVta)
	{
		throw NotConvertableException();
	}
}