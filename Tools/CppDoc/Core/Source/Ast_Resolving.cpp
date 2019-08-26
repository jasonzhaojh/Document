#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	Add: Add something to ExprTsysList
	***********************************************************************/

	bool AddInternal(ExprTsysList& list, const ExprTsysItem& item)
	{
		if (list.Contains(item)) return false;
		list.Add(item);
		return true;
	}

	void AddInternal(ExprTsysList& list, ExprTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddInternal(list, items[i]);
		}
	}

	bool AddVar(ExprTsysList& list, const ExprTsysItem& item)
	{
		return AddInternal(list, { item.symbol,ExprTsysType::LValue,item.tsys });
	}

	bool AddNonVar(ExprTsysList& list, const ExprTsysItem& item)
	{
		return AddInternal(list, { item.symbol,ExprTsysType::PRValue,item.tsys });
	}

	void AddNonVar(ExprTsysList& list, ExprTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddNonVar(list, items[i]);
		}
	}

	bool AddTemp(ExprTsysList& list, ITsys* tsys)
	{
		if (tsys->GetType() == TsysType::RRef)
		{
			return AddInternal(list, { nullptr,ExprTsysType::XValue,tsys });
		}
		else if (tsys->GetType() == TsysType::LRef)
		{
			return AddInternal(list, { nullptr,ExprTsysType::LValue,tsys });
		}
		else
		{
			return AddInternal(list, { nullptr,ExprTsysType::PRValue,tsys });
		}
	}

	void AddTemp(ExprTsysList& list, TypeTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddTemp(list, items[i]);
		}
	}

	/***********************************************************************
	CreateUniversalInitializerType: Create init types from element types
	***********************************************************************/

	void CreateUniversalInitializerType(const ParsingArguments& pa, Array<ExprTsysList>& argTypesList, Array<vint>& indices, vint index, ExprTsysList& result)
	{
		if (index == argTypesList.Count())
		{
			Array<ExprTsysItem> params(index);
			for (vint i = 0; i < index; i++)
			{
				params[i] = argTypesList[i][indices[i]];
			}
			AddInternal(result, { nullptr,ExprTsysType::PRValue,pa.tsys->InitOf(params) });
		}
		else
		{
			for (vint i = 0; i < argTypesList[index].Count(); i++)
			{
				indices[index] = i;
				CreateUniversalInitializerType(pa, argTypesList, indices, index + 1, result);
			}
		}
	}

	void CreateUniversalInitializerType(const ParsingArguments& pa, Array<ExprTsysList>& argTypesList, ExprTsysList& result)
	{
		Array<vint> indices(argTypesList.Count());
		CreateUniversalInitializerType(pa, argTypesList, indices, 0, result);
	}

	/***********************************************************************
	CalculateValueFieldType: Given thisItem, fill the field type fo ExprTsysList
		Value: t.f
		Ptr: t->f
	***********************************************************************/

	void CalculateValueFieldType(const ExprTsysItem* thisItem, Symbol* symbol, ITsys* fieldType, bool forFieldDeref, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		thisItem->tsys->GetEntity(cv, refType);

		auto type = fieldType->CVOf(cv);
		auto lrefType = forFieldDeref ? type->LRefOf() : type;

		if (refType == TsysRefType::LRef)
		{
			AddInternal(result, { symbol,ExprTsysType::LValue,lrefType });
		}
		else if (refType == TsysRefType::RRef)
		{
			if (thisItem->type == ExprTsysType::LValue)
			{
				AddInternal(result, { symbol,ExprTsysType::LValue,lrefType });
			}
			else
			{
				AddInternal(result, { symbol,ExprTsysType::XValue,type->RRefOf() });
			}
		}
		else if (forFieldDeref)
		{
			AddInternal(result, { symbol,ExprTsysType::LValue,lrefType });
		}
		else
		{
			AddInternal(result, { symbol,thisItem->type,type });
		}
	}

	/***********************************************************************
	EvaluateVarSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	TypeTsysList& EvaluateVarSymbol(const ParsingArguments& pa, ForwardVariableDeclaration* varDecl)
	{
		auto symbol = varDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		ev.Allocate();

		auto newPa = pa.WithContext(symbol->GetParentScope());

		if (varDecl->needResolveTypeFromInitializer)
		{
			if (auto rootVarDecl = dynamic_cast<VariableDeclaration*>(varDecl))
			{
				if (!rootVarDecl->initializer)
				{
					throw NotResolvableException();
				}

				ExprTsysList types;
				ExprToTsysNoVta(newPa, rootVarDecl->initializer->arguments[0].item, types);

				for (vint k = 0; k < types.Count(); k++)
				{
					auto type = ResolvePendingType(pa, rootVarDecl->type, types[k]);
					if (!ev.Get().Contains(type))
					{
						ev.Get().Add(type);
					}
				}
			}
			else
			{
				throw NotResolvableException();
			}
		}
		else
		{
			TypeToTsysNoVta(newPa, varDecl->type, ev.Get(), nullptr);
		}
		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return ev.Get();
	}

	/***********************************************************************
	EvaluateFuncSymbol: Evaluate the declared type for a function
	***********************************************************************/

	bool IsMemberFunction(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		ITsys* classScope = nullptr;
		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				classScope = pa.tsys->DeclOf(parent);
			}
		}

		bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol);
		return classScope && !isStaticSymbol;
	}

	void FinishEvaluatingSymbol(const ParsingArguments& pa, FunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		auto newPa = pa.WithContext(symbol->GetParentScope());
		TypeTsysList returnTypes;
		CopyFrom(returnTypes, ev.Get());
		ev.Get().Clear();

		TypeTsysList processedReturnTypes;
		{
			auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>();
			auto pendingType = funcType->decoratorReturnType ? funcType->decoratorReturnType : funcType->returnType;
			for (vint i = 0; i < returnTypes.Count(); i++)
			{
				auto tsys = ResolvePendingType(newPa, pendingType, { nullptr,ExprTsysType::PRValue,returnTypes[i] });
				if (!processedReturnTypes.Contains(tsys))
				{
					processedReturnTypes.Add(tsys);
				}
			}
		}
		TypeToTsysAndReplaceFunctionReturnType(newPa, funcDecl->type, processedReturnTypes, ev.Get(), nullptr, IsMemberFunction(pa, funcDecl));
		ev.progress = symbol_component::EvaluationProgress::Evaluated;
	}

	TypeTsysList& EvaluateFuncSymbol(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		
		if (funcDecl->needResolveTypeFromStatement)
		{
			if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
			{
				EnsureFunctionBodyParsed(rootFuncDecl);
				auto funcPa = pa.WithContext(symbol);
				EvaluateStat(funcPa, rootFuncDecl->statement);
				if (ev.Count() == 0)
				{
					throw NotResolvableException();
				}
			}
			else
			{
				throw NotResolvableException();
			}
		}
		else
		{
			auto newPa = pa.WithContext(symbol->GetParentScope());
			ev.Allocate();
			TypeToTsysNoVta(newPa, funcDecl->type, ev.Get(), nullptr, IsMemberFunction(pa, funcDecl));
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return ev.Get();
	}

	/***********************************************************************
	EvaluateClassSymbol: Evaluate base types for a class
	***********************************************************************/

	symbol_component::Evaluation& EvaluateClassSymbol(const ParsingArguments& pa, ClassDeclaration* classDecl)
	{
		auto symbol = classDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return ev;
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		ev.Allocate(classDecl->baseTypes.Count());

		{
			auto newPa = pa.WithContext(symbol);
			for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
			{
				TypeToTsysNoVta(newPa, classDecl->baseTypes[i].f1, ev.Get(i), nullptr);
			}
		}
		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return ev;
	}

	/***********************************************************************
	EvaluateTypeAliasSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	TypeTsysList& EvaluateTypeAliasSymbol(const ParsingArguments& pa, TypeAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			switch (ev.progress)
			{
			case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
			case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
			}
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
		}

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = esContext ? esContext->evaluatedTypes : symbol->GetEvaluationForUpdating_NFb().Get();

		TypeTsysList types;
		TypeToTsysNoVta(newPa, usingDecl->type, types, (esContext ? &esContext->gaContext : nullptr));

		if (usingDecl->templateSpec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(pa, usingDecl->templateSpec, params, genericFunction);

			for (vint i = 0; i < types.Count(); i++)
			{
				evaluatedTypes.Add(types[i]->GenericFunctionOf(params, genericFunction));
			}
		}
		else
		{
			CopyFrom(evaluatedTypes, types);
		}

		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return evaluatedTypes;
	}

	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	TypeTsysList& EvaluateValueAliasSymbol(const ParsingArguments& pa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			switch (ev.progress)
			{
			case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
			case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
			}
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
		}

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = esContext ? esContext->evaluatedTypes : symbol->GetEvaluationForUpdating_NFb().Get();

		TypeTsysList types;
		if (usingDecl->needResolveTypeFromInitializer)
		{
			ExprTsysList tsys;
			ExprToTsysNoVta(newPa, usingDecl->expr, tsys, (esContext ? &esContext->gaContext : nullptr));
			for (vint i = 0; i < tsys.Count(); i++)
			{
				auto entityType = tsys[i].tsys;
				if (entityType->GetType() == TsysType::Zero)
				{
					entityType = pa.tsys->Int();
				}
				if (!types.Contains(entityType))
				{
					types.Add(entityType);
				}
			}
		}
		else
		{
			TypeToTsysNoVta(newPa, usingDecl->type, types, (esContext ? &esContext->gaContext : nullptr));
		}

		if (usingDecl->templateSpec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(pa, usingDecl->templateSpec, params, genericFunction);

			for (vint i = 0; i < types.Count(); i++)
			{
				evaluatedTypes.Add(types[i]->GenericFunctionOf(params, genericFunction));
			}
		}
		else
		{
			CopyFrom(evaluatedTypes, types);
		}

		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return evaluatedTypes;
	}

	/***********************************************************************
	EvaluateGenericArgumentSymbol: Evaluate the declared value for a generic argument
	***********************************************************************/

	ITsys* EvaluateGenericArgumentSymbol(Symbol* symbol)
	{
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::GenericTypeArgument:
		case symbol_component::SymbolKind::GenericValueArgument:
			return symbol->GetEvaluationForUpdating_NFb().Get()[0];
		default:
			throw NotResolvableException();
		}
	}

	/***********************************************************************
	VisitSymbol: Fill a symbol to ExprTsysList
		thisItem: When afterScope==false
			it represents typeof(x) in x.name or typeof(&x) in x->name
			it could be null if it is initiated by IdExpr
	***********************************************************************/

	void VisitSymbolInternal(const ParsingArguments& pa, GenericArgContext* gaContext, const ExprTsysItem* thisItem, Symbol* symbol, bool afterScope, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		ITsys* classScope = nullptr;
		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				classScope = pa.tsys->DeclOf(parent);
			}
		}

		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Variable:
			{
				auto varDecl = symbol->GetAnyForwardDecl<ForwardVariableDeclaration>();
				auto& evTypes = EvaluateVarSymbol(pa, varDecl.Obj());
				bool isStaticSymbol = IsStaticSymbol<ForwardVariableDeclaration>(symbol);

				for (vint k = 0; k < evTypes.Count(); k++)
				{
					auto tsys = evTypes[k];

					if (isStaticSymbol)
					{
						AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
					}
					else if (afterScope)
					{
						if (classScope)
						{
							AddInternal(result, { symbol,ExprTsysType::PRValue,tsys->MemberOf(classScope) });
						}
						else
						{
							AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
						}
					}
					else
					{
						if (thisItem)
						{
							CalculateValueFieldType(thisItem, symbol, tsys, false, result);
						}
						else
						{
							AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
						}
					}
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::FunctionSymbol:
			{
				auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				auto& evTypes = EvaluateFuncSymbol(pa, funcDecl.Obj());
				bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol);
				bool isMember = classScope && !isStaticSymbol;

				for (vint k = 0; k < evTypes.Count(); k++)
				{
					auto tsys = evTypes[k];

					if (isMember && afterScope)
					{
						tsys = tsys->MemberOf(classScope)->PtrOf();
					}
					else
					{
						tsys = tsys->PtrOf();
					}

					AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::EnumItem:
			{
				auto tsys = pa.tsys->DeclOf(symbol->GetParentScope());
				AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::ValueAlias:
			{
				auto usingDecl = symbol->GetImplDecl_NFb<ValueAliasDeclaration>();
				auto& evTypes = EvaluateValueAliasSymbol(pa, usingDecl.Obj());
				AddTemp(result, evTypes);
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::GenericValueArgument:
			{
				if (symbol->ellipsis)
				{
					if (!allowVariadic)
					{
						throw NotConvertableException();
					}
					hasVariadic = true;

					if (gaContext)
					{
						auto argumentKey = pa.tsys->DeclOf(symbol);
						vint index = gaContext->arguments.Keys().IndexOf(argumentKey);
						if (index != -1)
						{
							auto& replacedTypes = gaContext->arguments.GetByIndex(index);
							for (vint i = 0; i < replacedTypes.Count(); i++)
							{
								auto replacedType = replacedTypes[i];
								if (!replacedType)
								{
									throw NotConvertableException();
								}
								switch (replacedType->GetType())
								{
								case TsysType::Init:
									{
										Array<ExprTsysList> initArgs(replacedType->GetParamCount());
										for (vint j = 0; j < initArgs.Count(); j++)
										{
											AddTemp(initArgs[j], EvaluateGenericArgumentSymbol(symbol));
										}
										CreateUniversalInitializerType(pa, initArgs, result);
									}
									break;
								case TsysType::Any:
									AddTemp(result, pa.tsys->Any());
									break;
								default:
									throw NotConvertableException();
								}
							}
							return;
						}
					}

					AddTemp(result, pa.tsys->Any());
				}
				else
				{
					AddTemp(result, EvaluateGenericArgumentSymbol(symbol));
					hasNonVariadic = true;
				}
			}
			return;
		}
		throw IllegalExprException();
	}

	void VisitSymbol(const ParsingArguments& pa, Symbol* symbol, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, nullptr, nullptr, symbol, false, result, false, hasVariadic, hasNonVariadic);
	}

	void VisitSymbolForScope(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, nullptr, thisItem, symbol, true, result, false, hasVariadic, hasNonVariadic);
	}

	void VisitSymbolForField(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, nullptr, thisItem, symbol, false, result, false, hasVariadic, hasNonVariadic);
	}

	/***********************************************************************
	FindMembersByName: Fill all members of a name to ExprTsysList
	***********************************************************************/

	Ptr<Resolving> FindMembersByName(const ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);
		if (entity->GetType() == TsysType::Decl)
		{
			auto symbol = entity->GetDecl();
			auto fieldPa = pa.WithContext(symbol);
			auto rar = ResolveSymbol(fieldPa, name, SearchPolicy::ChildSymbol);
			if (totalRar) totalRar->Merge(rar);
			return rar.values;
		}
		return nullptr;
	}

	/***********************************************************************
	VisitResolvedMember: Fill all resolved member symbol to ExprTsysList
	***********************************************************************/

	void VisitResolvedMemberInternal(const ParsingArguments& pa, GenericArgContext* gaContext, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		ExprTsysList varTypes, funcTypes;
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto targetTypeList = &result;
			auto symbol = resolving->resolvedSymbols[i];
			
			if (auto parent = symbol->GetParentScope())
			{
				switch (parent->kind)
				{
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
					switch (symbol->kind)
					{
					case symbol_component::SymbolKind::Variable:
						if (!IsStaticSymbol<ForwardVariableDeclaration>(symbol))
						{
							targetTypeList = &varTypes;
						}
						break;
					case symbol_component::SymbolKind::FunctionBodySymbol:
						if (!IsStaticSymbol<ForwardFunctionDeclaration>(symbol))
						{
							targetTypeList = &funcTypes;
						}
						break;
					}
					break;
				}
			}

			VisitSymbolInternal(pa, gaContext, (targetTypeList == &result ? nullptr : thisItem), resolving->resolvedSymbols[i], false, *targetTypeList, allowVariadic, hasVariadic, hasNonVariadic);
		}

		if (thisItem)
		{
			AddInternal(result, varTypes);

			TsysCV thisCv;
			TsysRefType thisRef;
			thisItem->tsys->GetEntity(thisCv, thisRef);
			FilterFieldsAndBestQualifiedFunctions(thisCv, thisRef, funcTypes);
			AddInternal(result, funcTypes);
		}
		else
		{
			AddInternal(result, varTypes);
			AddInternal(result, funcTypes);
		}
	}

	void VisitResolvedMember(const ParsingArguments& pa, GenericArgContext* gaContext, Ptr<Resolving> resolving, ExprTsysList& result, bool& hasVariadic, bool& hasNonVariadic)
	{
		VisitResolvedMemberInternal(pa, gaContext, nullptr, resolving, result, true, hasVariadic, hasNonVariadic);
	}

	void VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitResolvedMemberInternal(pa, nullptr, thisItem, resolving, result, false, hasVariadic, hasNonVariadic);
	}

	/***********************************************************************
	VisitFunctors: Find qualified functors (including functions and operator())
	***********************************************************************/

	void VisitFunctors(const ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		parentItem.tsys->GetEntity(cv, refType);

		CppName opName;
		opName.name = name;
		if (auto resolving = FindMembersByName(pa, opName, nullptr, parentItem))
		{
			for (vint j = 0; j < resolving->resolvedSymbols.Count(); j++)
			{
				auto symbol = resolving->resolvedSymbols[j];
				VisitSymbolForField(pa, &parentItem, symbol, result);
			}
			FindQualifiedFunctors(pa, cv, refType, result, false);
		}
	}

	/***********************************************************************
	Promote: Integer promition
	***********************************************************************/

	void Promote(TsysPrimitive& primitive)
	{
		switch (primitive.type)
		{
		case TsysPrimitiveType::Bool: primitive.type = TsysPrimitiveType::SInt; break;
		case TsysPrimitiveType::SChar: primitive.type = TsysPrimitiveType::SInt; break;
		case TsysPrimitiveType::UChar: if (primitive.bytes < TsysBytes::_4) primitive.type = TsysPrimitiveType::UInt; break;
		case TsysPrimitiveType::UWChar: primitive.type = TsysPrimitiveType::UInt; break;
		}

		switch (primitive.type)
		{
		case TsysPrimitiveType::SInt:
			if (primitive.bytes < TsysBytes::_4)
			{
				primitive.bytes = TsysBytes::_4;
			}
			break;
		case TsysPrimitiveType::UInt:
			if (primitive.bytes < TsysBytes::_4)
			{
				primitive.type = TsysPrimitiveType::SInt;
				primitive.bytes = TsysBytes::_4;
			}
			break;
		}
	}

	/***********************************************************************
	ArithmeticConversion: Calculate the result type from two given primitive type
	***********************************************************************/

	bool FullyContain(TsysPrimitive large, TsysPrimitive small)
	{
		if (large.type == TsysPrimitiveType::Float && small.type != TsysPrimitiveType::Float)
		{
			return true;
		}

		if (large.type != TsysPrimitiveType::Float && small.type == TsysPrimitiveType::Float)
		{
			return false;
		}

		if (large.bytes <= small.bytes)
		{
			return false;
		}

		if (large.type == TsysPrimitiveType::Float && small.type == TsysPrimitiveType::Float)
		{
			return true;
		}

		bool sl = large.type == TsysPrimitiveType::SInt || large.type == TsysPrimitiveType::SChar;
		bool ss = small.type == TsysPrimitiveType::SInt || small.type == TsysPrimitiveType::SChar;

		return sl == ss || sl;
	}

	TsysPrimitive ArithmeticConversion(TsysPrimitive leftP, TsysPrimitive rightP)
	{
		if (FullyContain(leftP, rightP))
		{
			Promote(leftP);
			return leftP;
		}

		if (FullyContain(rightP, leftP))
		{
			Promote(rightP);
			return rightP;
		}

		Promote(leftP);
		Promote(rightP);

		TsysPrimitive primitive;
		primitive.bytes = leftP.bytes > rightP.bytes ? leftP.bytes : rightP.bytes;

		if (leftP.type == TsysPrimitiveType::Float || rightP.type == TsysPrimitiveType::Float)
		{
			primitive.type = TsysPrimitiveType::Float;
		}
		else if (leftP.bytes > rightP.bytes)
		{
			primitive.type = leftP.type;
		}
		else if (leftP.bytes < rightP.bytes)
		{
			primitive.type = rightP.type;
		}
		else
		{
			bool sl = leftP.type == TsysPrimitiveType::SInt || leftP.type == TsysPrimitiveType::SChar;
			bool sr = rightP.type == TsysPrimitiveType::SInt || rightP.type == TsysPrimitiveType::SChar;
			if (sl && !sr)
			{
				primitive.type = rightP.type;
			}
			else if (!sl && sr)
			{
				primitive.type = leftP.type;
			}
			else
			{
				primitive.type = leftP.type;
			}
		}

		return primitive;
	}
}