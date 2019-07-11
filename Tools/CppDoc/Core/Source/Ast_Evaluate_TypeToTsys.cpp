#include "Ast_Expr.h"
#include "Ast_Resolving_ExpandPotentialVta.h"

using namespace symbol_totsys_impl;

/***********************************************************************
TypeToTsys
  PrimitiveType				: literal		*
  ReferenceType				: unbounded		*
  ArrayType					: unbounded		*
  CallingConventionType		: unbounded
  FunctionType				: variant
  MemberType				: unbounded		*
  DeclType					: unbounded
  DecorateType				: unbounded		*
  RootType					: literal
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
	GenericArgContext*			gaContext = nullptr;
	bool						memberOf = false;
	TsysCallingConvention		cc = TsysCallingConvention::None;

	TypeToTsysVisitor(const ParsingArguments& _pa, TypeTsysList& _result, TypeTsysList* _returnTypes, GenericArgContext* _gaContext, bool _memberOf, TsysCallingConvention _cc)
		:pa(_pa)
		, result(_result)
		, returnTypes(_returnTypes)
		, gaContext(_gaContext)
		, cc(_cc)
		, memberOf(_memberOf)
	{
	}

	static void AddResult(TypeTsysList& result, ITsys* tsys)
	{
		if (!result.Contains(tsys))
		{
			result.Add(tsys);
		}
	}

	void AddResult(ITsys* tsys)
	{
		AddResult(result, tsys);
	}

	template<typename T>
	static void CheckVta(VariadicList<T>& arguments, Array<TypeTsysList>& tsyses, Array<bool>& isVtas, vint offset, vint count, bool& hasBoundedVta, bool& hasUnboundedVta, vint& unboundedVtaCount)
	{
		for (vint i = offset; i < count; i++)
		{
			if (isVtas[i])
			{
				if (arguments[i - offset].isVariadic)
				{
					hasBoundedVta = true;
				}
				else
				{
					hasUnboundedVta = true;
				}
			}
		}

		if (hasBoundedVta && hasUnboundedVta)
		{
			throw NotConvertableException();
		}

		if (hasUnboundedVta)
		{
			for (vint i = 0; i < count; i++)
			{
				if (isVtas[i])
				{
					for (vint j = 0; j < tsyses[i].Count(); j++)
					{
						auto tsys = tsyses[i][j];
						if (tsys->GetType() == TsysType::Init)
						{
							vint currentVtaCount = tsyses[i][j]->GetParamCount();
							if (unboundedVtaCount == -1)
							{
								unboundedVtaCount = currentVtaCount;
							}
							else if (unboundedVtaCount != currentVtaCount)
							{
								throw NotConvertableException();
							}
						}
					}
				}
			}
		}
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
		TypeToTsysInternal(pa, self->type, items1, gaContext, isVta1);
		isVta = ExpandPotentialVta(pa, result, [=](ExprTsysItem arg1)
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
		TypeToTsysInternal(pa, self->type, items1, gaContext, isVta1);
		isVta = ExpandPotentialVta(pa, result, [=](ExprTsysItem arg1)
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

	static ITsys* CreateUnboundedFunctionType(Array<TypeTsysList>& tsyses, Array<bool>& isVtas, Array<vint>& tsysIndex, vint count, vint unboundedVtaIndex, const TsysFunc& func)
	{
		Array<ITsys*> params(count - 1);
		for (vint i = 0; i < count - 1; i++)
		{
			if (isVtas[i + 1])
			{
				params[i] = tsyses[i + 1][tsysIndex[i + 1]]->GetParam(unboundedVtaIndex);
			}
			else
			{
				params[i] = tsyses[i + 1][tsysIndex[i + 1]];
			}
		}

		if (isVtas[0])
		{
			return tsyses[0][tsysIndex[0]]->GetParam(unboundedVtaIndex)->FunctionOf(params, func);
		}
		else
		{
			return tsyses[0][tsysIndex[0]]->FunctionOf(params, func);
		}
	}

	void CreateFunctionType(Array<TypeTsysList>& tsyses, Array<bool>& isVtas, bool isBoundedVta, Array<vint>& tsysIndex, vint level, vint count, vint unboundedVtaCount, const TsysFunc& func)
	{
		if (level == count)
		{
			if (isBoundedVta)
			{
				vint paramCount = 0;
				for (vint i = 1; i < count; i++)
				{
					if (isVtas[i])
					{
						// every vtaCount should equal, which is ensured in Visit(FunctionType*)
						paramCount += tsyses[i][tsysIndex[i]]->GetParamCount();
					}
					else
					{
						paramCount++;
					}
				}

				Array<ITsys*> params(paramCount);
				vint currentParam = 0;
				for (vint i = 1; i < count; i++)
				{
					if (isVtas[i])
					{
						auto tsysVta = tsyses[i][tsysIndex[i]];
						vint paramVtaCount = tsysVta->GetParamCount();
						for (vint j = 0; j < paramVtaCount; j++)
						{
							params[currentParam++] = tsysVta->GetParam(j);
						}
					}
					else
					{
						params[currentParam++] = tsyses[i][tsysIndex[i]];
					}
				}
				AddResult(tsyses[0][tsysIndex[0]]->FunctionOf(params, func));
			}
			else
			{
				if (unboundedVtaCount == -1)
				{
					AddResult(CreateUnboundedFunctionType(tsyses, isVtas, tsysIndex, count, -1, func));
				}
				else
				{
					Array<ExprTsysItem> params(unboundedVtaCount);
					for (vint v = 0; v < unboundedVtaCount; v++)
					{
						params[v] = { nullptr,ExprTsysType::PRValue,CreateUnboundedFunctionType(tsyses, isVtas, tsysIndex, count, v, func) };
					}
					AddResult(pa.tsys->InitOf(params));
				}
			}
		}
		else
		{
			vint levelCount = tsyses[level].Count();
			for (vint i = 0; i < levelCount; i++)
			{
				tsysIndex[level] = i;
				CreateFunctionType(tsyses, isVtas, isBoundedVta, tsysIndex, level + 1, count, unboundedVtaCount, func);
			}
		}
	}

	void Visit(FunctionType* self)override
	{
		vint count = self->parameters.Count() + 1;
		Array<TypeTsysList> tsyses(count);
		Array<bool> isVtas(count);
		isVtas[0] = false;

		if (returnTypes)
		{
			CopyFrom(tsyses[0], *returnTypes);
		}
		else if (self->decoratorReturnType)
		{
			TypeToTsysInternal(pa, self->decoratorReturnType, tsyses[0], gaContext, isVtas[0]);
		}
		else if (self->returnType)
		{
			TypeToTsysInternal(pa, self->returnType, tsyses[0], gaContext, isVtas[0]);
		}
		else
		{
			tsyses[0].Add(pa.tsys->Void());
		}

		for (vint i = 1; i < count; i++)
		{
			TypeToTsysInternal(pa, self->parameters[i - 1].item->type, tsyses[i], gaContext, isVtas[i]);
		}

		bool hasBoundedVta = false;
		bool hasUnboundedVta = isVtas[0];
		vint unboundedVtaCount = -1;
		CheckVta(self->parameters, tsyses, isVtas, 1, count, hasBoundedVta, hasUnboundedVta, unboundedVtaCount);
		isVta = hasUnboundedVta;

		for (vint i = 0; i < count; i++)
		{
			if (isVtas[i])
			{
				for (vint j = 0; j < tsyses[i].Count(); j++)
				{
					if (tsyses[i][j]->GetType() != TsysType::Init)
					{
						AddResult(pa.tsys->Any());
						return;
					}
				}
			}
		}

		{
			Array<vint> tsysIndex(count);
			memset(&tsysIndex[0], 0, sizeof(vint) * count);

			TsysFunc func(cc, self->ellipsis);
			if (func.callingConvention == TsysCallingConvention::None)
			{
				func.callingConvention =
					memberOf && !func.ellipsis
					? TsysCallingConvention::ThisCall
					: TsysCallingConvention::CDecl
					;
			}
			CreateFunctionType(tsyses, isVtas, hasBoundedVta, tsysIndex, 0, count, unboundedVtaCount, func);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// MemberType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(MemberType* self)override
	{
		auto oldMemberOf = memberOf;
		memberOf = true;

		TypeTsysList types, classTypes;
		bool typesVta = false;
		bool classTypesVta = false;
		TypeToTsysInternal(pa, self->type, types, gaContext, typesVta, memberOf, cc);
		TypeToTsysInternal(pa, self->classType, classTypes, gaContext, classTypesVta);
		isVta = typesVta || classTypesVta;

		for (vint i = 0; i < types.Count(); i++)
		{
			for (vint j = 0; j < classTypes.Count(); j++)
			{
				auto type = types[i];
				auto classType = classTypes[j];
				if (typesVta && type->GetType() == TsysType::Init)
				{
					if (classTypesVta)
					{
						if (classType->GetType() == TsysType::Init)
						{
							if (type->GetParamCount() != classType->GetParamCount())
							{
								throw NotConvertableException();
							}

							Array<ExprTsysItem> params(type->GetParamCount());
							for (vint k = 0; k < params.Count(); k++)
							{
								params[k] = GetExprTsysItem(ProcessMemberType(
									pa,
									self,
									{ type->GetInit().headers[k],type->GetParam(k) },
									{ classType->GetInit().headers[k],classType->GetParam(k) }
								));
							}
							AddResult(pa.tsys->InitOf(params));
						}
						else
						{
							AddResult(pa.tsys->Any());
						}
					}
					else
					{
						Array<ExprTsysItem> params(type->GetParamCount());
						for (vint k = 0; k < params.Count(); k++)
						{
							params[k] = GetExprTsysItem(ProcessMemberType(
								pa,
								self,
								{ type->GetInit().headers[k],type->GetParam(k) },
								GetExprTsysItem(classType)
							));
						}
						AddResult(pa.tsys->InitOf(params));
					}
				}
				else
				{
					if (classTypesVta)
					{
						if (classType->GetType() == TsysType::Init)
						{
							Array<ExprTsysItem> params(classType->GetParamCount());
							for (vint k = 0; k < params.Count(); k++)
							{
								params[k] = GetExprTsysItem(ProcessMemberType(
									pa,
									self,
									GetExprTsysItem(type),
									{ classType->GetInit().headers[k],classType->GetParam(k) }
								));
							}
							AddResult(pa.tsys->InitOf(params));
						}
						else
						{
							AddResult(pa.tsys->Any());
						}
					}
					else
					{
						AddResult(ProcessMemberType(
							pa,
							self,
							GetExprTsysItem(type),
							GetExprTsysItem(classType)
						));
					}
				}
			}
		}

		memberOf = oldMemberOf;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DeclType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
			for (vint i = 0; i < types.Count(); i++)
			{
				auto exprTsys = types[i].tsys;
				if (exprTsys->GetType() == TsysType::Zero)
				{
					exprTsys = pa.tsys->Int();
				}
				AddResult(exprTsys);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DecorateType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DecorateType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, gaContext, isVta1);
		isVta = ExpandPotentialVta(pa, result, [=](ExprTsysItem arg1)
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

	static void CreateIdReferenceType(const ParsingArguments& pa, GenericArgContext* gaContext, Ptr<Resolving> resolving, bool allowAny, bool allowVariadic, TypeTsysList& result, bool& isVta)
	{
		if (!resolving)
		{
			if (allowAny)
			{
				AddResult(result, pa.tsys->Any());
				return;
			}
			else
			{
				throw NotConvertableException();
			}
		}
		else if (resolving->resolvedSymbols.Count() == 0)
		{
			throw NotConvertableException();
		}

		bool hasVariadic = false;
		bool hasNonVariadic = false;
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto symbol = resolving->resolvedSymbols[i];
			switch (symbol->kind)
			{
			case symbol_component::SymbolKind::Enum:
			case symbol_component::SymbolKind::Class:
			case symbol_component::SymbolKind::Struct:
			case symbol_component::SymbolKind::Union:
				AddResult(result, pa.tsys->DeclOf(symbol));
				hasNonVariadic = true;
				continue;
			case symbol_component::SymbolKind::TypeAlias:
				{
					auto usingDecl = symbol->definition.Cast<TypeAliasDeclaration>();
					symbol_type_resolving::EvaluateSymbol(pa, usingDecl.Obj());
					auto& types = symbol->evaluation.Get();
					for (vint j = 0; j < types.Count(); j++)
					{
						AddResult(result, types[j]);
					}
					hasNonVariadic = true;
				}
				continue;
			case symbol_component::SymbolKind::GenericTypeArgument:
				{
					auto& types = symbol->evaluation.Get();
					for (vint j = 0; j < types.Count(); j++)
					{
						if (gaContext)
						{
							auto type = types[j];
							vint index = gaContext->arguments.Keys().IndexOf(type);
							if (index != -1)
							{
								auto& replacedTypes = gaContext->arguments.GetByIndex(index);
								for (vint k = 0; k < replacedTypes.Count(); k++)
								{
									if (symbol->ellipsis)
									{
										if (!allowVariadic)
										{
											throw NotConvertableException();
										}
										auto replacedType = replacedTypes[k];
										if (replacedType->GetType() == TsysType::Any || replacedType->GetType() == TsysType::Init)
										{
											AddResult(result, replacedTypes[k]);
											hasVariadic = true;
										}
										else
										{
											throw NotConvertableException();
										}
									}
									else
									{
										AddResult(result, replacedTypes[k]);
										hasNonVariadic = true;
									}
								}
								continue;
							}
						}

						if (symbol->ellipsis)
						{
							if (!allowVariadic)
							{
								throw NotConvertableException();
							}
							AddResult(result, pa.tsys->Any());
							hasVariadic = true;
						}
						else
						{
							AddResult(result, types[j]);
							hasNonVariadic = true;
						}
					}
				}
				continue;
			}
			throw NotConvertableException();
		}

		if (hasVariadic && hasNonVariadic)
		{
			throw NotConvertableException();
		}
		isVta = hasVariadic;
	}

	void Visit(IdType* self)override
	{
		CreateIdReferenceType(pa, gaContext, self->resolving, false, true, result, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildType
	//////////////////////////////////////////////////////////////////////////////////////

	void ResolveChildTypeWithGenericArguments(ChildType* self, ITsys* type, Ptr<Resolving>& resolving)
	{
		if (type->GetType() == TsysType::Decl)
		{
			auto newPa = pa.WithContext(type->GetDecl());
			auto rsr = ResolveSymbol(newPa, self->name, SearchPolicy::ChildSymbol);
			if (rsr.types)
			{
				if (!resolving)
				{
					resolving = rsr.types;
				}
				else
				{
					for (vint i = 0; i < rsr.types->resolvedSymbols.Count(); i++)
					{
						if (!resolving->resolvedSymbols.Contains(rsr.types->resolvedSymbols[i]))
						{
							resolving->resolvedSymbols.Add(rsr.types->resolvedSymbols[i]);
						}
					}
				}
			}
		}
	}

	Ptr<Resolving> ResolveChildTypeWithGenericArguments(ChildType* self, TypeTsysList& types)
	{
		Ptr<Resolving> resolving;

		for (vint i = 0; i < types.Count(); i++)
		{
			ResolveChildTypeWithGenericArguments(self, types[i], resolving);
		}

		return resolving;
	}

	void Visit(ChildType* self)override
	{
		TypeTsysList types;
		bool parentIsVta = false;
		{
			bool allNamespaces = true;
			if (auto resolvableType = self->classType.Cast<ResolvableType>())
			{
				if (auto resolving = resolvableType->resolving)
				{
					for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
					{
						if (resolving->resolvedSymbols[i]->kind != symbol_component::SymbolKind::Namespace)
						{
							allNamespaces = false;
							break;
						}
					}
				}
			}

			if (!allNamespaces)
			{
				TypeToTsysInternal(pa, self->classType, types, gaContext, parentIsVta);
			}
		}

		if (parentIsVta)
		{
			for (vint i = 0; i < types.Count(); i++)
			{
				auto parentType = types[i];
				if (parentType->GetType() == TsysType::Any)
				{
					AddResult(pa.tsys->Any());
				}
				else if (parentType->GetType() == TsysType::Init)
				{
					Array<ExprTsysList> argTypesList(parentType->GetParamCount());
					for (vint j = 0; j < parentType->GetParamCount(); j++)
					{
						TypeTsysList childTypes;
						Ptr<Resolving> resolving;
						ResolveChildTypeWithGenericArguments(self, parentType->GetParam(j), resolving);
						CreateIdReferenceType(pa, gaContext, resolving, true, false, childTypes, isVta);

						symbol_type_resolving::AddTemp(argTypesList[j], childTypes);
					}

					ExprTsysList initTypes;
					symbol_type_resolving::CreateUniversalInitializerType(pa, argTypesList, initTypes);
					for (vint j = 0; j < initTypes.Count(); j++)
					{
						AddResult(initTypes[j].tsys);
					}
				}
				else
				{
					throw NotConvertableException();
				}
			}
			isVta = true;
		}
		else
		{
			if (gaContext && !self->resolving)
			{
				if (auto resolving = ResolveChildTypeWithGenericArguments(self, types))
				{
					CreateIdReferenceType(pa, gaContext, resolving, true, false, result, isVta);
				}
			}
			else
			{
				CreateIdReferenceType(pa, gaContext, self->resolving, true, false, result, isVta);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// GenericType
	//////////////////////////////////////////////////////////////////////////////////////

	void CreateGenericType(ITsys* genericFunction, Array<TypeTsysList>& argumentTypes, Array<bool>& isTypes, Array<bool>& isVtas, vint count, TypeTsysList& genericTypes)
	{
		if (genericFunction->GetType() == TsysType::GenericFunction)
		{
			auto declSymbol = genericFunction->GetGenericFunction().declSymbol;
			if (!declSymbol)
			{
				throw NotConvertableException();
			}

			symbol_type_resolving::EvaluateSymbolContext esContext;
			if (!symbol_type_resolving::ResolveGenericParameters(pa, genericFunction, argumentTypes, isTypes, &esContext.gaContext))
			{
				throw NotConvertableException();
			}

			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::GenericTypeArgument:
				genericFunction->GetElement()->ReplaceGenericArgs(esContext.gaContext, esContext.evaluatedTypes);
				break;
			case symbol_component::SymbolKind::TypeAlias:
				{
					auto decl = declSymbol->definition.Cast<TypeAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					symbol_type_resolving::EvaluateSymbol(pa, decl.Obj(), &esContext);
				}
				break;
			default:
				throw NotConvertableException();
			}

			for (vint j = 0; j < esContext.evaluatedTypes.Count(); j++)
			{
				AddResult(genericTypes, esContext.evaluatedTypes[j]);
			}
		}
		else if (genericFunction->GetType() == TsysType::Any)
		{
			AddResult(genericTypes, pa.tsys->Any());
		}
		else
		{
			throw NotConvertableException();
		}
	}

	void Visit(GenericType* self)override
	{
		TypeTsysList genericTypes;
		vint count = self->arguments.Count();
		Array<TypeTsysList> argumentTypes(count);
		Array<bool> isTypes(count);
		Array<bool> isVtas(count);

		TypeToTsysNoVta(pa, self->type, genericTypes, gaContext);
		symbol_type_resolving::ResolveGenericArguments(pa, self->arguments, argumentTypes, isTypes, isVtas, gaContext);

		bool hasBoundedVta = false;
		bool hasUnboundedVta = false;
		vint unboundedVtaCount = -1;
		CheckVta(self->arguments, argumentTypes, isVtas, 0, count, hasBoundedVta, hasUnboundedVta, unboundedVtaCount);
		isVta = hasUnboundedVta;

		// TODO: Implement variadic template argument passing
		if (hasBoundedVta)
		{
			throw NotConvertableException();
		}

		for (vint i = 0; i < count; i++)
		{
			if (isVtas[i])
			{
				for (vint j = 0; j < argumentTypes[i].Count(); j++)
				{
					if (argumentTypes[i][j]->GetType() != TsysType::Init)
					{
						// TODO: Solve this in ResolveGenericParameters for unknown-amount variant template argument passing
						AddResult(pa.tsys->Any());
						return;
					}
				}
			}
		}

		if (hasUnboundedVta)
		{
			Array<TypeTsysList> unboundedVtaTypes(count);
			for (vint i = 0; i < count; i++)
			{
				if (isVtas[i])
				{
					CopyFrom(unboundedVtaTypes[i], argumentTypes[i]);
				}
			}

			Array<ExprTsysList> unboundedGenericTypes(unboundedVtaCount);
			for (vint i = 0; i < unboundedVtaCount; i++)
			{
				TypeTsysList unboundedGenericTypesItem;
				for (vint j = 0; j < count; j++)
				{
					if (isVtas[j])
					{
						auto& fromTypes = unboundedVtaTypes[j];
						auto& toTypes = argumentTypes[j];
						for (vint k = 0; k < fromTypes.Count(); k++)
						{
							toTypes[k] = fromTypes[k]->GetParam(i);
						}
					}
				}

				for (vint j = 0; j < genericTypes.Count(); j++)
				{
					auto genericFunction = genericTypes[j];
					CreateGenericType(genericFunction, argumentTypes, isTypes, isVtas, count, unboundedGenericTypesItem);
				}

				for (vint j = 0; j < unboundedGenericTypesItem.Count(); j++)
				{
					unboundedGenericTypes[i].Add({ nullptr,ExprTsysType::PRValue,unboundedGenericTypesItem[j] });
				}
			}

			ExprTsysList initTypes;
			symbol_type_resolving::CreateUniversalInitializerType(pa, unboundedGenericTypes, initTypes);

			for (vint i = 0; i < initTypes.Count(); i++)
			{
				AddResult(initTypes[i].tsys);
			}
		}
		else
		{
			for (vint i = 0; i < genericTypes.Count(); i++)
			{
				auto genericFunction = genericTypes[i];
				CreateGenericType(genericFunction, argumentTypes, isTypes, isVtas, count, result);
			}
		}
	}
};

// Convert type AST to type system object

void TypeToTsysInternal(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, GenericArgContext* gaContext, bool& isVta, bool memberOf, TsysCallingConvention cc)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, nullptr, gaContext, memberOf, cc);
	t->Accept(&visitor);
	isVta = visitor.isVta;
}
void TypeToTsysInternal(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, GenericArgContext* gaContext, bool& isVta, bool memberOf, TsysCallingConvention cc)
{
	TypeToTsysInternal(pa, t.Obj(), tsys, gaContext, isVta, memberOf, cc);
}

void TypeToTsysNoVta(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf, TsysCallingConvention cc)
{
	bool isVta = false;
	TypeToTsysInternal(pa, t, tsys, gaContext, isVta, memberOf, cc);
	if (isVta)
	{
		throw NotConvertableException();
	}
}

void TypeToTsysNoVta(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf, TsysCallingConvention cc)
{
	TypeToTsysNoVta(pa, t.Obj(), tsys, gaContext, memberOf, cc);
}

void TypeToTsysAndReplaceFunctionReturnType(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& returnTypes, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, &returnTypes, gaContext, memberOf, TsysCallingConvention::None);
	t->Accept(&visitor);
	if (visitor.isVta)
	{
		throw NotConvertableException();
	}
}