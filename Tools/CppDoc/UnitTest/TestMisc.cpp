#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Hidden types")
	{
		auto input = LR"(
	struct A;
	struct A* A();

	enum B;
	enum B* (*B)();
	)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"A()",				L"A()",									L"::A * $PR"	);
		AssertExpr(pa, L"struct A()",		L"enum_class_struct_union A()",			L"::A $PR"		);
		AssertExpr(pa, L"B()",				L"B()",									L"::B * $PR"	);
		AssertExpr(pa, L"enum B()",			L"enum_class_struct_union B()",			L"::B $PR"		);
	});
}