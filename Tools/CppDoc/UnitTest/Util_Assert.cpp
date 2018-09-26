#include "Util.h"

void AssertType(const WString& type, const WString& log)
{
	ParsingArguments pa;
	AssertType(type, log, pa);
}

void AssertType(const WString& type, const WString& log, ParsingArguments& pa)
{
	CppTokenReader reader(GlobalCppLexer(), type);
	auto cursor = reader.GetFirstToken();

	List<Ptr<Declarator>> declarators;
	ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
	TEST_ASSERT(!cursor);
	TEST_ASSERT(declarators.Count() == 1);
	TEST_ASSERT(!declarators[0]->name);
	TEST_ASSERT(declarators[0]->initializer == nullptr);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(declarators[0]->type, writer);
	});
	TEST_ASSERT(output == log);
}

void AssertProgram(const WString& input, const WString& log)
{
	COMPILE_PROGRAM(program, pa, input);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(program, writer);
	});

	StringReader srExpect(log);
	StringReader srActual(L"\r\n" + output);

	while (true)
	{
		TEST_ASSERT(srExpect.IsEnd() == srActual.IsEnd());
		if (srExpect.IsEnd()) break;
		
		auto expect = srExpect.ReadLine();
		auto actual = srActual.ReadLine();
		TEST_ASSERT(expect == actual);
	}
}