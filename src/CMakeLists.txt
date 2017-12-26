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
	parse/lex.c
	fileInst.c
)