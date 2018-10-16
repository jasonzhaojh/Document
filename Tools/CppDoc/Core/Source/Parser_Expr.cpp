#include "Parser.h"
#include "Ast_Expr.h"
#include "Ast_Type.h"

/***********************************************************************
FillOperatorAndSkip
***********************************************************************/

void FillOperatorAndSkip(CppName& name, Ptr<CppTokenCursor>& cursor, vint count)
{
	auto reading = cursor->token.reading;
	vint length = 0;

	name.type = CppNameType::Normal;
	name.tokenCount = count;
	for (vint i = 0; i < count; i++)
	{
		name.nameTokens[i] = cursor->token;
		length += cursor->token.length;
		SkipToken(cursor);
	}

	name.name = WString(reading, length);
}

/***********************************************************************
ParseIdExpr
***********************************************************************/

Ptr<IdExpr> ParseIdExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	CppName cppName;
	if (ParseCppName(cppName, cursor))
	{
		auto rsr = ResolveSymbol(pa, cppName, SearchPolicy::SymbolAccessableInScope);
		if (!rsr.types)
		{
			auto type = MakePtr<IdExpr>();
			type->name = cppName;
			type->resolving = rsr.values;
			if (pa.recorder)
			{
				pa.recorder->Index(type->name, type->resolving);
			}
			return type;
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
TryParseChildExpr
***********************************************************************/

Ptr<ChildExpr> TryParseChildExpr(const ParsingArguments& pa, Ptr<Type> classType, Ptr<CppTokenCursor>& cursor)
{
	CppName cppName;
	if (ParseCppName(cppName, cursor))
	{
		auto rsr = ResolveChildSymbol(pa, classType, cppName);
		if (!rsr.types)
		{
			auto type = MakePtr<ChildExpr>();
			type->classType = classType;
			type->name = cppName;
			type->resolving = rsr.values;
			if (pa.recorder && type->resolving)
			{
				pa.recorder->Index(type->name, type->resolving);
			}
			return type;
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
	return nullptr;
}

/***********************************************************************
ParsePrimitiveExpr
***********************************************************************/

Ptr<Expr> ParsePrimitiveExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (cursor)
	{
		switch ((CppTokens)cursor->token.token)
		{
		case CppTokens::EXPR_TRUE:
		case CppTokens::EXPR_FALSE:
		case CppTokens::INT:
		case CppTokens::HEX:
		case CppTokens::BIN:
		case CppTokens::FLOAT:
		case CppTokens::CHAR:
			{
				auto literal = MakePtr<LiteralExpr>();
				literal->tokens.Add(cursor->token);
				SkipToken(cursor);
				return literal;
			}
		case CppTokens::STRING:
			{
				auto literal = MakePtr<LiteralExpr>();
				while (cursor && (CppTokens)cursor->token.token == CppTokens::STRING)
				{
					literal->tokens.Add(cursor->token);
					SkipToken(cursor);
				}
				return literal;
			}
		case CppTokens::EXPR_THIS:
			{
				SkipToken(cursor);
				return MakePtr<ThisExpr>();
			}
		case CppTokens::EXPR_NULLPTR:
			{
				SkipToken(cursor);
				return MakePtr<NullptrExpr>();
			}
		case CppTokens::LPARENTHESIS:
			{
				SkipToken(cursor);
				auto expr = MakePtr<ParenthesisExpr>();
				expr->expr = ParseExpr(pa, true, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::EXPR_DYNAMIC_CAST:
		case CppTokens::EXPR_STATIC_CAST:
		case CppTokens::EXPR_CONST_CAST:
		case CppTokens::EXPR_REINTERPRET_CAST:
		case CppTokens::EXPR_SAFE_CAST:
			{
				auto expr = MakePtr<CastExpr>();
				expr->castType = CppCastType::SafeCast;
				switch ((CppTokens)cursor->token.token)
				{
				case CppTokens::EXPR_DYNAMIC_CAST:		expr->castType = CppCastType::DynamicCast;		 break;
				case CppTokens::EXPR_STATIC_CAST:		expr->castType = CppCastType::StaticCast;		 break;
				case CppTokens::EXPR_CONST_CAST:		expr->castType = CppCastType::ConstCast;		 break;
				case CppTokens::EXPR_REINTERPRET_CAST:	expr->castType = CppCastType::ReinterpretCast;	 break;
				}
				SkipToken(cursor);

				RequireToken(cursor, CppTokens::LT);
				expr->type = ParseType(pa, cursor);
				RequireToken(cursor, CppTokens::GT);
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				expr->expr = ParseExpr(pa, true, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::EXPR_TYPEID:
			{
				SkipToken(cursor);
				auto expr = MakePtr<TypeidExpr>();
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				{
					auto oldCursor = cursor;
					try
					{
						expr->expr = ParseExpr(pa, true, cursor);
						goto SUCCESS_EXPR;
					}
					catch (const StopParsingException&)
					{
						cursor = oldCursor;
					}
				}
				expr->type = ParseType(pa, cursor);
			SUCCESS_EXPR:
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
			break;
		}

		{
			auto oldCursor = cursor;
			try
			{
				auto type = ParseLongType(pa, cursor);

				if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
				{
					if (auto expr = TryParseChildExpr(pa, type, cursor))
					{
						return expr;
					}
				}

				if (TestToken(cursor, CppTokens::LPARENTHESIS))
				{
					auto expr = MakePtr<FuncAccessExpr>();
					expr->type = type;
					if (!TestToken(cursor, CppTokens::RPARENTHESIS))
					{
						while (true)
						{
							expr->arguments.Add(ParseExpr(pa, false, cursor));
							if (TestToken(cursor, CppTokens::RPARENTHESIS))
							{
								break;
							}
							else
							{
								RequireToken(cursor, CppTokens::COMMA);
							}
						}
					}
					return expr;
				}

				throw StopParsingException(cursor);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
				goto GIVE_UP_CHILD_SYMBOL;
			}
		}
	GIVE_UP_CHILD_SYMBOL:

		if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
		{
			if (auto expr = TryParseChildExpr(pa, MakePtr<RootType>(), cursor))
			{
				return expr;
			}
		}
		else
		{
			if (auto expr = ParseIdExpr(pa, cursor))
			{
				return expr;
			}
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
ParsePostfixUnaryExpr
***********************************************************************/

Ptr<Expr> ParsePostfixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParsePrimitiveExpr(pa, cursor);
	while (true)
	{
		if (!TestToken(cursor, CppTokens::DOT, CppTokens::MUL, false) && TestToken(cursor, CppTokens::DOT))
		{
			auto newExpr = MakePtr<FieldAccessExpr>();
			newExpr->type = CppFieldAccessType::Dot;
			newExpr->expr = expr;
			if (!ParseCppName(newExpr->name, cursor, false) && !ParseCppName(newExpr->name, cursor, true))
			{
				throw StopParsingException(cursor);
			}
			expr = newExpr;
		}
		else if (!TestToken(cursor, CppTokens::SUB, CppTokens::GT, CppTokens::MUL, false) && TestToken(cursor, CppTokens::SUB, CppTokens::GT))
		{
			auto newExpr = MakePtr<FieldAccessExpr>();
			newExpr->type = CppFieldAccessType::Arrow;
			newExpr->expr = expr;
			if (!ParseCppName(newExpr->name, cursor, false) && !ParseCppName(newExpr->name, cursor, true))
			{
				throw StopParsingException(cursor);
			}
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::LBRACKET))
		{
			auto newExpr = MakePtr<ArrayAccessExpr>();
			newExpr->expr = expr;
			newExpr->index = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::RBRACKET);
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::LPARENTHESIS))
		{
			auto newExpr = MakePtr<FuncAccessExpr>();
			newExpr->expr = expr;
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				while (true)
				{
					newExpr->arguments.Add(ParseExpr(pa, false, cursor));
					if (TestToken(cursor, CppTokens::RPARENTHESIS))
					{
						break;
					}
					else
					{
						RequireToken(cursor, CppTokens::COMMA);
					}
				}
			}
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::ADD, CppTokens::ADD, false) || TestToken(cursor, CppTokens::SUB, CppTokens::SUB, false))
		{
			auto newExpr = MakePtr<PostfixUnaryExpr>();
			FillOperatorAndSkip(newExpr->opName, cursor, 2);
			newExpr->operand = expr;
			expr = newExpr;
		}
		else
		{
			break;
		}
	}
	return expr;
}

/***********************************************************************
ParsePrefixUnaryExpr
***********************************************************************/

Ptr<Expr> ParsePrefixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::EXPR_SIZEOF))
	{
		auto newExpr = MakePtr<SizeofExpr>();
		auto oldCursor = cursor;
		try
		{
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			newExpr->type = ParseType(pa, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			return newExpr;
		}
		catch (const StopParsingException&)
		{
			cursor = oldCursor;
		}
		newExpr->expr = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else if (TestToken(cursor, CppTokens::ADD, CppTokens::ADD, false) || TestToken(cursor, CppTokens::SUB, CppTokens::SUB, false))
	{
		auto newExpr = MakePtr<PrefixUnaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 2);
		newExpr->operand = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else if (
		TestToken(cursor, CppTokens::REVERT, false) ||
		TestToken(cursor, CppTokens::NOT, false) ||
		TestToken(cursor, CppTokens::ADD, false) ||
		TestToken(cursor, CppTokens::SUB, false) ||
		TestToken(cursor, CppTokens::AND, false) ||
		TestToken(cursor, CppTokens::MUL, false))
	{
		auto newExpr = MakePtr<PrefixUnaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 1);
		newExpr->operand = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else if (TestToken(cursor, CppTokens::NEW))
	{
		auto newExpr = MakePtr<NewExpr>();
		if (TestToken(cursor, CppTokens::LPARENTHESIS))
		{
			while (true)
			{
				newExpr->placementArguments.Add(ParseExpr(pa, false, cursor));
				if (TestToken(cursor, CppTokens::RPARENTHESIS))
				{
					break;
				}
				else
				{
					RequireToken(cursor, CppTokens::COMMA);
				}
			}
		}

		newExpr->type = ParseLongType(pa, cursor);
		if (TestToken(cursor, CppTokens::LPARENTHESIS))
		{
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				while (true)
				{
					newExpr->arguments.Add(ParseExpr(pa, false, cursor));
					if (TestToken(cursor, CppTokens::RPARENTHESIS))
					{
						break;
					}
					else if (!TestToken(cursor, CppTokens::COMMA))
					{
						throw StopParsingException(cursor);
					}
				}
			}
		}
		return newExpr;
	}
	else if (TestToken(cursor, CppTokens::DELETE))
	{
		auto newExpr = MakePtr<DeleteExpr>();
		if ((newExpr->arrayDelete = TestToken(cursor, CppTokens::LBRACKET)))
		{
			RequireToken(cursor, CppTokens::RBRACKET);
		}
		newExpr->expr = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else
	{
		Ptr<Type> type;
		auto oldCursor = cursor;
		try
		{
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			type = ParseType(pa, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
		}
		catch (const StopParsingException&)
		{
			cursor = oldCursor;
		}

		if (type)
		{
			auto newExpr = MakePtr<CastExpr>();
			newExpr->castType = CppCastType::CCast;
			newExpr->type = type;
			newExpr->expr = ParsePrefixUnaryExpr(pa, cursor);
			return newExpr;
		}
		else
		{
			return ParsePostfixUnaryExpr(pa, cursor);
		}
	}
}

/***********************************************************************
ParseBinaryExpr
***********************************************************************/

Ptr<Expr> ParseBinaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	return ParsePrefixUnaryExpr(pa, cursor);
}

/***********************************************************************
ParseIfExpr
***********************************************************************/

Ptr<Expr> ParseIfExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	return ParseBinaryExpr(pa, cursor);
}

/***********************************************************************
ParseAssignExpr
***********************************************************************/

Ptr<Expr> ParseAssignExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	return ParseIfExpr(pa, cursor);
}

/***********************************************************************
ParseThrowExpr
***********************************************************************/

Ptr<Expr> ParseThrowExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::THROW))
	{
		auto newExpr = MakePtr<ThrowExpr>();
		if (!TestToken(cursor, CppTokens::SEMICOLON, false))
		{
			newExpr->expr = ParseAssignExpr(pa, cursor);
		}
		return newExpr;
	}
	else
	{
		return ParseAssignExpr(pa, cursor);
	}
}

/***********************************************************************
ParseExpr
***********************************************************************/

Ptr<Expr> ParseExpr(const ParsingArguments& pa, bool allowComma, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParseThrowExpr(pa, cursor);
	while (allowComma)
	{
		if (TestToken(cursor, CppTokens::COMMA, false))
		{
			auto newExpr = MakePtr<BinaryExpr>();
			FillOperatorAndSkip(newExpr->opName, cursor, 1);
			newExpr->left = expr;
			newExpr->right = ParseThrowExpr(pa, cursor);
			expr = newExpr;
		}
		else
		{
			break;
		}
	}
	return expr;
}