# Draft

- The tool consumes a single preprocessed C++ source file, the following syntax is collected for this purpose.
- The syntax has less restrictions than the real C++ language specification, because the tool assumes the input is correct.

## Refactoring Work Items

- [ ] Add exception messages so that the final application won't crash leaving nothing.
- [ ] `CreateIdReferenceExpr`: Remove dependency to `Ptr<Resolving>`
- [ ] `FindMembersByName`: Remove dependency to `ResolveSymbolResult`

## TODO

- Built-in types and functions can be found [here](https://clang.llvm.org/docs/LanguageExtensions.html)
  - `__is_*` and `__has_*` return null, arguments could be expressions or types, create a new Expr subclass.
    - This should be handled since arguments are not expressions
  - When more built-in things are found, list below:
    - Types
      - `__make_integer_seq<A, B, Size>` -> `A<B>`
      - `__underlying_type` -> `int` // should be enum's integral type
    - Functions (by default: ADL searching found nothing and evaluate to no type)
      - `__builtin_addressof(EXPR)` -> `&EXPR` without calling overloaded operators
      - `__builtin_huge_valf(...)` -> float
      - `__builtin_nanf(...)` -> float
      - `__builtin_nansf(...)` -> float

- [ ] More `template` on functions. [post](https://en.cppreference.com/w/cpp/language/function_template)
  - [ ] `SearchForFunctionWithSameSignature` returns all compatible `FunctionSymbol`s
  - [ ] Generate `ClassMemberCache` for generated special members
  - [ ] Call a function with some or all template arguments unspecified.
    - [ ] Only the last function parameter could be variadic, when there is no ellipsis (`ParseSingleDeclarator_Function` function performs a test).
    - [ ] Refactor `ResolveGenericParameters` so that it could also resolve function parameters with variadic or ellipsis.
      - [x] New parameter `allowPartialApply`, will throw when it is false and template arguments are not enough.
      - [ ] `ResolveGenericParameters` expand provided values or types to argument list eagerly.
        - If there are multiple values/types arguments declarated together, all arguments are still offered to the first variadic argument, offered arguments are not grouped by values/types.
          - For example, `template<typename..., typename...>`'s first argument accepts all provided arguments, the second of which deducted from parameters.
          - For example, `template<int..., typename...>` with `<1, int>` is invalid, because `int` is not a `int` constant expression.
          - `ResolveGenericParameters` only read informations about parameters to be filled to the first variadic argument (if `allowPartialApply` is false and this is not the argument, it becomes an error)
      - [ ] When the first function vta is {}, it could be treated as "to be inferred", if function arguments suggest so
      - [ ] When a function is not applied with arguments, it is treated as {}. In this case, type arguments should all be provided.
        - Which means there could be at most 1 vta, otherwise type arguments could not be all provided whatever you do.
      - [ ] When it is used to resolve function parameters or class partial specializations, only the last argument could be variadic or ellipsis(function only).
        - Function has no partial specialization.
    - [ ] Refactor `ResolvePendingType` to call `ResolveTemplateArgument`, which resolve all specified template argument according to value argument.
      - Resolve multiple arguments at the same time.
    - [ ] Allow multiple template arguments to be variadic as long as they are type inferenced (aka not assigned to directly) at call sites.
    - [ ] Test resolving complex argument types containing template arguments.
    - [ ] Overload functions with some or all template arguments unspecified.
  - [ ] `...` arguments
    - [ ] Refactor and rename `ResolveGenericParameters` to also handle function arguments.
  - [ ] Full Specialization
    - [ ] Connect functions with forward declarations.
      - [ ] When there are constant arguments, the shape of the expression should match, considering `NameExpr` and `ChildExpr` identical.
    - [ ] When overloading, specialized functions are not considered. When a template function wins, then choose among the primiary and its specializations.
- [ ] Next Demo! (UnitTest_Cases::STL)
  - [ ] Check carefully around all links.
    - [ ] Extract `<div>` token rendering functions.
    - [ ] Template arguments are not located in HTML.
    - [ ] `expr->Accept` should have 3 links.
  - [ ] Show progress while parsing cases.
- [ ] More `template` on value aliases (partial specialization)
- [ ] More `template` on classes (partial specialization).
  - [ ] Test scenario: first see the forward declaration of a generic class and evaluate its type, and then see the implementation and evaluate its type again.
  - [ ] Connect methods with forward declarations inside multiple levels of template classes.
    - [ ] Parse `template<typename T> template<typename U> template<typename V> void A<T*>::B<const U&>::F(){}`
      - matches `template<typename X>class A<X*> { template<typename Y>class B<const Y&> { void template<typename Z>F(); }; };`
    - [ ] When there are constant arguments, the shape of the expression should match, considering `NameExpr` and `ChildExpr` identical.
  - [ ] With a `DeclInstant` is created, best possible choices are returned at the same time.
- [ ] SFINAE on choosing among generic functions or classes.
- [ ] `std::initialization_list`.
- [ ] Lambda expressions.
- [ ] `decltype(EXPR)::ChildType`
- [ ] `::new`
- [ ] `::delete`
- [ ] Pass variadic template argument to placement new expression.
- [ ] `GenerateMembers` on `DeclInstance`.

## Finisher Work Items

- [ ] Parse `UnitTest_Cases`
- [ ] Update CodePack.exe to produce `#include` only header and cpp files, so that the compiler can index preprocessed files with `#line` directly, without having to parse CodePack.exe produced comments.
- [ ] Produce `Preprocessed.txt` from `#include` only files, not from compacted files.
- [ ] Generic HTML index. (multiple pages)
- [ ] Attach document content to declarations.
- [ ] Enable markdown inside xml elements in comment.
- [ ] Resolve symbols in markdown in comment.
- [ ] Enable `<example>` with predefined project types, extract all example, compile and index them.

## Lexical Conventions

Consumable UTF-32 code points:

- Punctuators: `_ { } [ ] # ( ) < > % : ; . ? * + - / ^ & | ~ ! = , \ " '`
- Legal characters in identifiers: `00A8, 00AA, 00AD, 00AF, 00B2-00B5, 00B7-00BA, 00BC-00BE, 00C0-00D6, 00D8-00F6, 00F8-00FF, 0100-02FF, 0370-167F, 1681-180D, 180F-1DBF, 1E00-1FFF, 200B-200D, 202A-202E, 203F-2040, 2054, 2060-206F, 2070-20CF, 2100-218F, 2460-24FF, 2776-2793, 2C00-2DFF, 2E80-2FFF, 3004-3007, 3021-302F, 3031-303F, 3040-D7FF, F900-FD3D, FD40-FDCF, FDF0-FE1F, FE30-FE44, FE47-FFFD, 10000-1FFFD, 20000-2FFFD, 30000-3FFFD, 40000-4FFFD, 50000-5FFFD, 60000-6FFFD, 70000-7FFFD, 80000-8FFFD, 90000-9FFFD, A0000-AFFFD, B0000-BFFFD, C0000-CFFFD, D0000-DFFFD, E0000-EFFFD`
- Legal characters in identifiers except the first: `0300-036F, 1DC0-1DFF, 20D0-20FF, FE20-FE2F`
- Keywords: [Keywords](https://docs.microsoft.com/en-us/cpp/cpp/keywords-cpp?view=vs-2017)
- Integers: `(\d+('?\d+)*|0x[0-9a-fA-F]+)[uU]?[lL]?[lL]?`
- Floating: `(\d+\.|\.\d+|\d+.\d+)([eE][+-]?\d+)?[fFlL]?`
- Binaries: `0[bB]\d+`
- Strings: `(u|U|L|u8)?"([^\"]|\\.)*"`
- Raw Strings: `(u|U|L|u8)?R"<name>([anything that is not ')<name>"']*)<name>"`
- Characters: `(u|U|L|u8)?'([^\']|\\.)'`
- User-Defined Literals: Integer, character, float-point, boolean and string literals followed by an identifier without any delimiter.
- Comment:
  - `//[^/\r\n]?[^\r\n]*\r?\n`
  - `/\*([^*]|\*+[^*/])*\*/`
  - `///[^\r\n]*\r?\n`: It can be put before any declaration for documentation

## EBNF for this Document

- `[ X ]`: optional
- `{ X }`: repeating 0+ times
  - `{ X }+`: `X {X}`
  - `{ X Y ... }`: `[X {Y X}]`
  - `{ X Y ... }+`: `X {Y X}`
- `( X )`: priority
- `X | Y`: alternation

## Supported C++ Syntax

- Type parser function: parse a type, with zero, optional or more declarator.
  - zero declarator: still need a declarator, but it cannot have an identity name. e.g. type in generic type
  - optional declarator: the declarator has optional identity name. e.g. function parameter
  - else: each declarator should have an identity name. e.g. variable declaration
- Declarator parser function: parse a type, with (maybe) optional identity name.

### SPECIFIERS

Specifiers can be put before any declaration, it will be ignored by the tool

- attributes: e.g. `[[noreturn]]`
- `__declspec( ... )`

### QUALIFIERS

- {`constexpr` | `const` | `volatile` | `&` | `&&`}+

### CALL

- `__cdecl`
- `__clrcall`
- `__stdcall`
- `__fastcall`
- `__thiscall`
- `__vectorcall`

### EXCEPTION-SPEC

- `noexcept`
- `throw` `(` {TYPE `,` ...} `)`

### INITIALIZER

- `=` EXPR
- `{` {EXPR `,` ...} `}`
- `(` {EXPR `,` ...} `)`
  - When this initializer is ambiguous a function declaration, the initializer wins.

### FUNCTION-TAIL

- `(` {TYPE-OPTIONAL [INITIALIZER] `,` ...} `)` {QUALIFIERS | EXCEPTION-SPEC | `->` TYPE | `override`)}

### DECLARATOR

- [x] `operator` OPERATOR
- [ ] IDENTIFIER [SPECIALIZATION-SPEC]: Only allowed for functions
- [x] SPECIFIERS DECLARATOR
- [x] CALL DECLARATOR
- [x] `alignas` `(` EXPR `)` DECLARATOR
- [x] TYPE `::` DECLARATOR
- [x] `(` DECLARATOR `)`
- [x] (`*` [`__ptr32` | `__ptr64`] | `&` | `&&`) DECLARATOR
- [x] (`constexpr` | `const` | `volatile`) DECLARATOR
- [x] DECLARATOR `[` [EXPR] `]`
- [x] DECLARATOR FUNCTION-TAIL

### TEMPLATE-SPEC

- `template` `<` {TEMPLATE-SPEC-ITEM `,` ...} `>`
- **TEMPLATE-SPEC-ITEM**:
  - TYPE-OPTIONAL-INITIALIZER
  - (`template`|`class`) [`...`] [IDENTIFIER] [`=` TYPE]
  - TEMPLATE-SPEC `class` [IDENTIFIER] [`=` TYPE]

### SPECIALIZATION-SPEC

- `<` {TYPE | EXPR} `>`

### FUNCTION

- [TEMPLATE-SPEC] {`static` | `virtual` | `explicit` | `inline` | `__forceinline`} TYPE-SINGLE (`;` | STAT) [ = (`0` | `default` | `delete`)
  - TEMPLATE-SPEC
  - SPECIALIZATION-SPEC
- [TEMPLATE-SPEC] {`static` | `virtual` | `explicit` | `inline` | `__forceinline`} `operator` TYPE-ZERO (`;` | STAT) [ = (`0` | `default` | `delete`)
  - TEMPLATE-SPEC

### OBJECT

- [TEMPLATE-SPEC] (`class` | `struct`) [[SPECIFIERS] IDENTIFIER [SPECIALIZATION-SPEC]] [`abstract`] [`:` {TYPE `,` ...}+] [`{` {DECL} `}`
  - TEMPLATE-SPEC: Not allowed when the class is defined after `typedef`, or is anonymous followed by variable declaration.
  - SPECIALIZATION-SPEC
- `enum` [`class` | `struct`] [[SPECIFIERS]IDENTIFIER] [`:` TYPE] [`{` {IDENTIFIER [`=` EXPR] `,` ...} [`,`] `}`
- [TEMPLATE-SPEC] `union` [[SPECIFIERS]IDENTIFIER [SPECIALIZATION-SPEC]] [`{` {DECL} `}`
  - TEMPLATE-SPEC: Not allowed when the class is defined after `typedef`, or is anonymous followed by variable declaration.
  - SPECIALIZATION-SPEC

### DECL (Declaration)

- **Friend**: `friend` DECL `;`
- **Extern**" `extern` [STRING] (DECL `;` | `{` {DECLARATION ...} `}`)
- **Type definition**: (CLASS_STRUCT | ENUM | UNION) {DECLARATOR [INITIALIZER] `,` ...}+ `;`
- **Type alias**:
  - `typedef` (CLASS_STRUCT | ENUM | UNION) {DECLARATOR `,` ...}+ `;` (**no template**)
  - `typedef` TYPE-MULTIPLE-INITIALIZER `;` (**no template**)
    - TEMPLATE-SPEC and SPECIALIZATION-SPEC are disallowed here
- **Type definition**: [TEMPLATE-SPEC] `using` IDENTIFIER `=` TYPE `;` (**no specialization**)
  - TEMPLATE-SPEC
- **Import**: `using` { [`typename`] [TYPE `::` IDENTIFIER] `,` ...} `;`
- **Variable**: {`register` | `static` | `thread_local` | `mutable`} TYPE-MULTIPLE-INITIALIZER `;`
  - TEMPLATE-SPEC: for value aliases
- **Namespace** `namespace` {IDENTIFIER `::` ...}+ `{` {DECLARATION} `}`
- **Ctor, Dtor**: [`~`] IDENTIFIER ({TYPE [DECLARATOR] [INITIALIZER] `,` ...}) [EXCEPTION-SPEC] STAT
- FUNCTION

## TYPE (Type)

- `auto`
- `decltype` `(` (EXPR) `)`
- (`constexpr` | `const` | `volatile`) TYPE
- TYPE (`constexpr` | `const` | `volatile`)
- `void` | `bool`
- `char` | `wchar_t` | `char16_t` | `char32_t`
- [`signed` | `unsigned`] (`__int8` | `__int16` | `__int32` | `__int64` | `__m64` | `__m128` | `__m128d` | `__m128i`)
- [TYPE `::` [`typename`]] IDENTIFIER
- TYPE `<` {(TYPE | EXPR) `,` ...}+ `>`
- TYPE `...`

## STAT (Statement)

- IDENTIFIER `:` STAT
- `default` `:` STAT
- `case` EXPR `:` STAT
- `;`
- `{` {STAT ...} `}`
- {EXPR `,` ...}+ `;`
- DECL
- `break` `;`
- `continue` `;`
- `while` `(` EXPR `)` STAT
- `do` STAT `while` `(` EXPR `)` `;`
- `for` ([TYPE {[DECLARATOR] [INITIALIZER] `,` ...}] `;` [EXPR] `;` [EXPR]) STAT
- `for` (TYPE-SINGLE-DECLARATOR `:` EXPR) STAT
- `if` [`constexpr`] `(`[STAT `;`] [TYPE IDENTIFIER `=`] EXPR `)` STAT [`else` STAT]
- `switch` `(` {STAT} EXPR `)` `{` STAT `}`
- `try` STAT `catch` `(` TYPE-OPTIONAL-DECLARATOR `)` STAT
- `return` EXPR `;`
- `goto` IDENTIFIER `;`
- `__try` STAT `__except` `(` EXPR `)` STAT
- `__try` STAT `__finally` STAT
- `__leave` `;`
- (`__if_exists` | `__if_not_exists`) `(` EXPR `)` STAT

## EXPR (Expression)

- LITERAL
- `{` ... `}`
- `this`
- `nullptr`
- IDENTIFIER
- `::` IDENTIFIER
- TYPE `::` IDENTIFIER
- `(` EXPR-WITH-COMMA `)`
- OPERATOR EXPR
  - predefined operators
- EXPR OPERATOR
  - EXPR `(` {EXPR `,` ...} `)`
  - EXPR `[` EXPR `]`
  - EXPR `<` {TYPE `,` ...} `>`
  - predefined operators
- EXPR OPERATOR EXPR
  - EXPR (`.` | `->`) IDENTIFIER
  - EXPR (`.*` | `->*`) EXPR
  - predefined operators
- EXPR `?` EXPR `:` EXPR
- (`dynamic_cast` | `static_cast` | `const_cast` | `reinterpret_cast` | `safe_cast`) `<` TYPE `>` `(` EXPR `)`
- `typeid` `(` (TYPE | EXPR) `)`
- `sizeof` [`...`] EXPR
  - `...`
- `sizeof` [`...`] `(` TYPE `)`
  - `...`
- `(` TYPE `)` EXPR
- `new` [`(` {EXPR `,` ...}+ `)`] TYPE [`(` {EXPR `,` ... } `)` | [`{` {EXPR `,` ... } `}`]]
- `delete` [`[` `]`] EXPR
- `throw` EXPR
- EXPR `...`
- `::new`
- `::delete`
- `[` {`&` | `=` | [IDENTIFIER `=`] EXPR | } `]` FUNCTION-TAIL STAT

### Operators

[Built-in Operators, Precedence and Associativity](https://docs.microsoft.com/en-us/cpp/cpp/cpp-built-in-operators-precedence-and-associativity?view=vs-2017)

### Precedence Groups

1. : primitive
2. : `::`
3. : `.` `->` `[]` `()` `x++` `x--`
4. (<-): `sizeof` `new` `delete` `++x` `--x` `~` `!` `-x` `+x` `&x` `*x` `(T)E`
5. : `.*` `->*`
6. : `*` `/` `%`
7. : `+` `-`
8. : `<<` `>>`
9. : `<` `>` `<=` `>=`
10. : `==` `!=`
11. : `&`
12. : `^`
13. : `|`
14. : `&&`
15. : `||`
16. (<-): `a?b:c`
17. (<-): `=` `*=` `/=` `%=` `+=` `-=` `<<=` `>>=` `&=` `|=` `^=`
18. (<-): `throw`
19. : `,`
