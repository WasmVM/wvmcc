add_executable(wvmcpp
	pp.c
	stack.c
	fileInst.c
	ppDirective.c
	map.c
	"list.c"
)

add_executable(wvmcc
	wvmcc.c
	fileInst.c
	parse/lex.c
	parse/util.c
	parse/externDefs.c
	parse/declarations.c
	parse/expressions.c
	parse/statements.c
	parse/structs.c
	codegen/genFuncs.c
	map.c
	"list.c"
)
target_link_libraries(wvmcc m)