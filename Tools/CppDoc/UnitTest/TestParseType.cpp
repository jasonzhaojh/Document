#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Primitive")
	{
	#define TEST_PRIMITIVE_TYPE(TYPE, LOG, LOGS, LOGU)\
		AssertType(L#TYPE, L#TYPE, LOG);\
		AssertType(L"signed " L#TYPE, L"signed " L#TYPE, LOGS);\
		AssertType(L"unsigned " L#TYPE, L"unsigned " L#TYPE, LOGU);\

	#define TEST_PRIMITIVE_TYPE_NOSIGN(TYPE, LOG)\
		AssertType(L#TYPE, L#TYPE, LOG);\
		AssertType(L"signed " L#TYPE, L"signed " L#TYPE);\
		AssertType(L"unsigned " L#TYPE, L"unsigned " L#TYPE);\

		TEST_PRIMITIVE_TYPE_NOSIGN	(void,			L"void"		);
		TEST_PRIMITIVE_TYPE_NOSIGN	(bool,			L"bool"		);
		TEST_PRIMITIVE_TYPE_NOSIGN	(wchar_t,		L"wchar_t"	);
		TEST_PRIMITIVE_TYPE_NOSIGN	(char16_t,		L"char16_t"	);
		TEST_PRIMITIVE_TYPE_NOSIGN	(char32_t,		L"char32_t"	);
		TEST_PRIMITIVE_TYPE_NOSIGN	(float,			L"float"	);
		TEST_PRIMITIVE_TYPE_NOSIGN	(double,		L"double"	);
		TEST_PRIMITIVE_TYPE_NOSIGN	(long double,	L"double"	);

		TEST_PRIMITIVE_TYPE			(char,			L"char",		L"__int8",				L"unsigned __int8"	);
		TEST_PRIMITIVE_TYPE			(short,			L"__int16",		L"__int16",				L"unsigned __int16"	);
		TEST_PRIMITIVE_TYPE			(int,			L"__int32",		L"__int32",				L"unsigned __int32"	);
		TEST_PRIMITIVE_TYPE			(__int8,		L"__int8",		L"__int8",				L"unsigned __int8"	);
		TEST_PRIMITIVE_TYPE			(__int16,		L"__int16",		L"__int16",				L"unsigned __int16"	);
		TEST_PRIMITIVE_TYPE			(__int32,		L"__int32",		L"__int32",				L"unsigned __int32"	);
		TEST_PRIMITIVE_TYPE			(__int64,		L"__int64",		L"__int64",				L"unsigned __int64"	);
		TEST_PRIMITIVE_TYPE			(long,			L"__int32",		L"__int32",				L"unsigned __int32"	);
		TEST_PRIMITIVE_TYPE			(long int,		L"__int32",		L"__int32",				L"unsigned __int32"	);
		TEST_PRIMITIVE_TYPE			(long long,		L"__int64",		L"__int64",				L"unsigned __int64"	);

		AssertType(L"auto",					L"auto"														);
		AssertType(L"signed",				L"signed int",							L"__int32"			);
		AssertType(L"unsigned",				L"unsigned int",						L"unsigned __int32"	);

	#undef TEST_PRIMITIVE_TYPE
	#undef TEST_PRIMITIVE_TYPE_NOSIGN
	});

	TEST_CATEGORY(L"Short")
	{
		AssertType(L"decltype(auto)",					L"decltype(auto)"													);
		AssertType(L"decltype(0)",						L"decltype(0)",							L"__int32"					);
		AssertType(L"const int",						L"int const",							L"__int32 const"			);
		AssertType(L"volatile int",						L"int volatile",						L"__int32 volatile"			);
		AssertType(L"const volatile int",				L"int const volatile",					L"__int32 const volatile"	);
	});

	TEST_CATEGORY(L"Long")
	{
		AssertType(L"int const",						L"int const",							L"__int32 const"					);
		AssertType(L"int volatile",						L"int volatile",						L"__int32 volatile"					);
		AssertType(L"int const volatile",				L"int const volatile",					L"__int32 const volatile"			);
	});

	TEST_CATEGORY(L"Short declarator")
	{
		AssertType(L"int* __ptr32",						L"int *",								L"__int32 *"		);
		AssertType(L"int* __ptr64",						L"int *",								L"__int32 *"		);
		AssertType(L"int*",								L"int *",								L"__int32 *"		);
		AssertType(L"int&",								L"int &",								L"__int32 &"		);
		AssertType(L"int&&",							L"int &&",								L"__int32 &&"		);
		AssertType(L"int& &&",							L"int & &&",							L"__int32 &"		);
		AssertType(L"int&& &",							L"int && &",							L"__int32 &"		);
	});

	TEST_CATEGORY(L"Long declarator")
	{
		AssertType(L"int[]",							L"int []",								L"__int32 []"		);
		AssertType(L"int[][]",							L"int [] []",							L"__int32 [,]"		);
		AssertType(L"int[][2]",							L"int [2] []",							L"__int32 [,]"		);
		AssertType(L"int[1][2][3]",						L"int [3] [2] [1]",						L"__int32 [,,]"		);
		AssertType(L"int([1])[2][3]",					L"int [3] [2] [1]",						L"__int32 [,,]"		);
		AssertType(L"int(*&)[][]",						L"int [] [] * &",						L"__int32 [,] * &"	);

		AssertType(L"int(int[][2])",															L"int (int [2] *)",													L"__int32 __cdecl(__int32 [] *)"		);
		AssertType(L"auto ()->int const volatile & && override noexcept throw()",				L"(auto->int const volatile & &&) () override noexcept throw()",	L"__int32 const volatile & __cdecl()"	);
		AssertType(L"auto ()const volatile & && ->int override noexcept throw()",				L"(auto->int) () const volatile & && override noexcept throw()",	L"__int32 __cdecl()"					);

		AssertType(L"int __cdecl(int)",					L"int (int) __cdecl",					L"__int32 __cdecl(__int32)"							);
		AssertType(L"int __clrcall(int)",				L"int (int) __clrcall",					L"__int32 __clrcall(__int32)"						);
		AssertType(L"int __stdcall(int)",				L"int (int) __stdcall",					L"__int32 __stdcall(__int32)"						);
		AssertType(L"int __fastcall(int)",				L"int (int) __fastcall",				L"__int32 __fastcall(__int32)"						);
		AssertType(L"int __thiscall(int)",				L"int (int) __thiscall",				L"__int32 __thiscall(__int32)"						);
		AssertType(L"int __vectorcall(int)",			L"int (int) __vectorcall",				L"__int32 __vectorcall(__int32)"					);
		AssertType(L"int(*)(int)",						L"int (int) *",							L"__int32 __cdecl(__int32) *"						);
		AssertType(L"int(__cdecl*)(int)",				L"int (int) __cdecl *",					L"__int32 __cdecl(__int32) *"						);

		AssertType(L"int(*[5])(int, int a, int b=0)",	L"int (int, a: int, b: int = 0) * [5]",	L"__int32 __cdecl(__int32, __int32, __int32) * []"	);
		AssertType(L"int(&(*[5])(void))[10]",			L"int [10] & () * [5]",					L"__int32 [] & __cdecl() * []"						);
	});

	TEST_CATEGORY(L"Complex type")
	{
		AssertType(
			L"int(__fastcall*const&((*)(int))[10])(int(&a)[], int(__stdcall*b)()noexcept, int(*c[5])(void)=0)",
			L"int (a: int * &, b: int () noexcept __stdcall *, c: int () * [5] = 0) __fastcall * const & [10] (int) *",
			L"__int32 __fastcall(__int32 * &, __int32 __stdcall() *, __int32 __cdecl() * []) * const & [] __cdecl(__int32) *"
			);
	});

	TEST_CATEGORY(L"Member type 1")
	{
		auto input = LR"(
namespace a::b
{
	struct S
	{
		enum X;
	};
}
)";
		COMPILE_PROGRAM(program, pa, input);
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"a", 0, 0, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(1, L"b", 0, 3, NamespaceDeclaration, 1, 13)
				ASSERT_SYMBOL(2, L"S", 0, 6, ClassDeclaration, 3, 8)
				ASSERT_SYMBOL(3, L"X", 0, 9, ForwardEnumDeclaration, 5, 7)
			END_ASSERT_SYMBOL;

			AssertType(pa,
				L"a::b::S::X",
				L"a :: b :: S :: X",
				L"::a::b::S::X");
			TEST_CASE_ASSERT(accessed.Count() == 4);
		}
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"a", 0, 11, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(1, L"b", 0, 14, NamespaceDeclaration, 1, 13)
			END_ASSERT_SYMBOL;

			AssertType(pa,
				L"typename ::a::b::X::Y::Z",
				L"__root :: a :: typename b :: typename X :: typename Y :: typename Z",
				L"any_t");
			TEST_CASE_ASSERT(accessed.Count() == 2);
		}
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"a", 0, 0, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(1, L"b", 0, 3, NamespaceDeclaration, 1, 13)
				ASSERT_SYMBOL(2, L"S", 0, 6, ClassDeclaration, 3, 8)
				ASSERT_SYMBOL(3, L"X", 0, 9, ForwardEnumDeclaration, 5, 7)
				ASSERT_SYMBOL(4, L"a", 0, 28, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(5, L"b", 0, 31, NamespaceDeclaration, 1, 13)
				ASSERT_SYMBOL(6, L"S", 0, 34, ClassDeclaration, 3, 8)
			END_ASSERT_SYMBOL;

			AssertType(pa,
				L"a::b::S::X(__cdecl typename a::b::S::*)()",
				L"a :: b :: S :: X () __cdecl (a :: typename b :: typename S ::) *",
				L"::a::b::S::X __cdecl() (::a::b::S ::) *");
			TEST_CASE_ASSERT(accessed.Count() == 7);
		}
	});

	TEST_CATEGORY(L"Member type 2")
	{
		auto input = LR"(
namespace a::b
{
	struct X
	{
		enum Y;
	};
}
namespace c::d
{
	struct Y
	{
		struct Z : a::b::X
		{
		};
	};
}
)";
		COMPILE_PROGRAM(program, pa, input);
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"c", 0, 0, NamespaceDeclaration, 8, 10)
				ASSERT_SYMBOL(1, L"d", 0, 3, NamespaceDeclaration, 8, 13)
				ASSERT_SYMBOL(2, L"Y", 0, 6, ClassDeclaration, 10, 8)
				ASSERT_SYMBOL(3, L"Z", 0, 9, ClassDeclaration, 12, 9)
				ASSERT_SYMBOL(4, L"Y", 0, 12, ForwardEnumDeclaration, 5, 7)
			END_ASSERT_SYMBOL;

			AssertType(pa,
				L"c::d::Y::Z::Y",
				L"c :: d :: Y :: Z :: Y",
				L"::a::b::X::Y");
			TEST_CASE_ASSERT(accessed.Count() == 5);
		}
	});
}