#include "sclexer.h"
#include <stdlib.h>
#include <string.h>

struct lisp_val {
	union lisp_val_data {
		char *str;
		int64_t i;
		uint64_t u;
	} data;
	enum LISP_VAL_TYPE {
		LISP_VAL_TYPE_INT,
		LISP_VAL_TYPE_STR,
		LISP_VAL_TYPE_UINT
	} type;
};

enum PARSE_ARG_RESULT {
	PARSE_ARG_RESULT_CONTINUE,
	PARSE_ARG_RESULT_CONTINUE_WITH_ARG,
	PARSE_ARG_RESULT_END,
	PARSE_ARG_RESULT_FAULT
};

static int cmd_print(int argc, struct lisp_val argv[]);
static int parse_sym(struct sclexer_tok *tok, struct sclexer *lexer);
static int parse_sexpr(struct sclexer *lexer);
static int parse_sexpr_arg(struct lisp_val *res, struct sclexer *lexer);
static int parse_sexpr_arg_str(struct lisp_val *res, struct sclexer *lexer);
static int process(struct sclexer *lexer);

int cmd_print(int argc, struct lisp_val argv[])
{
	for (int i = 0; i < argc; i++)
		puts(argv[i].data.str);
	return 0;
}

int parse_sym(struct sclexer_tok *tok, struct sclexer *lexer)
{
	if (tok->kind_data.c == ';') {
		return 0;
	} else if (tok->kind_data.c == '(') {
		return parse_sexpr(lexer);
	}
	return 1;
}

int parse_sexpr(struct sclexer *lexer)
{
	char *cmd = NULL;
	struct sclexer_tok first;
	int ret = 0, argc = 0;
	struct lisp_val argv[8];
	if (sclexer_read_tok(&first, lexer))
		return 1;
	if (first.kind == SCLEXER_TOK_KIND_STR) {
		cmd = calloc(first.kind_data.str.len + 1, sizeof(char));
		strncpy(cmd, first.kind_data.str.s, first.kind_data.str.len);
	}
	while ((ret = parse_sexpr_arg(&argv[argc], lexer))
			!= PARSE_ARG_RESULT_END) {
		if (ret == PARSE_ARG_RESULT_FAULT)
			return 1;
		if (ret == PARSE_ARG_RESULT_CONTINUE_WITH_ARG)
			argc++;
	}
	switch (first.kind) {
	case SCLEXER_TOK_KIND_STR:
		if (strcmp("print", cmd) == 0)
			return cmd_print(argc, argv);
		break;
	case SCLEXER_TOK_KIND_SYM:
		switch (first.kind_data.c) {
		case '+':
			ret = argv[0].data.i;
			for (int i = 1; i < argc; i++)
				ret += argv[i].data.i;
			printf("(apply +): %d\n", ret);
			break;
		case '-':
			ret = argv[0].data.i;
			for (int i = 1; i < argc; i++)
				ret -= argv[i].data.i;
			printf("(apply -): %d\n", ret);
			break;
		}
		break;
	default: return 1; break;
	}
	return 0;
}

int parse_sexpr_arg(struct lisp_val *res, struct sclexer *lexer)
{
	struct sclexer_tok tok;
	if (sclexer_read_tok(&tok, lexer))
		return 1;
	switch (tok.kind) {
	case SCLEXER_TOK_KIND_EMPTY:
		if (sclexer_get_line(lexer))
			return PARSE_ARG_RESULT_FAULT;
	case SCLEXER_TOK_KIND_SPACE:
		return PARSE_ARG_RESULT_CONTINUE;
		break;
	case SCLEXER_TOK_KIND_INT:
		res->data.u = tok.kind_data.uint;
		res->type = LISP_VAL_TYPE_UINT;
		break;
	case SCLEXER_TOK_KIND_INT_NEG:
		res->data.u = tok.kind_data.sint;
		res->type = LISP_VAL_TYPE_INT;
		break;
	case SCLEXER_TOK_KIND_SYM:
		if (tok.kind_data.c == ')')
			return PARSE_ARG_RESULT_END;
		if (tok.kind_data.c == '"')
			return parse_sexpr_arg_str(res, lexer);
		break;
	default: return PARSE_ARG_RESULT_FAULT; break;
	}
	return PARSE_ARG_RESULT_CONTINUE_WITH_ARG;
}

int parse_sexpr_arg_str(struct lisp_val *res, struct sclexer *lexer)
{
	struct sclexer_str_slice slice;
	if (sclexer_read_to(&slice, lexer, "\""))
		return PARSE_ARG_RESULT_FAULT;
	res->type = LISP_VAL_TYPE_STR;
	res->data.str = calloc(slice.len, sizeof(char));
	strncpy(res->data.str, slice.s, slice.len - 1);
	return PARSE_ARG_RESULT_CONTINUE_WITH_ARG;
}

int process(struct sclexer *lexer)
{
	struct sclexer_tok tok;
	if (sclexer_read_tok(&tok, lexer))
		return 1;
	switch (tok.kind) {
	case SCLEXER_TOK_KIND_EMPTY:
	case SCLEXER_TOK_KIND_SPACE:
		break;
	case SCLEXER_TOK_KIND_SYM:
		return parse_sym(&tok, lexer);
	default: return 1; break;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	struct sclexer lexer;
	int ret = 0;
	if (argc < 2)
		return 1;
	if (sclexer_init(argv[1], &lexer))
		return 1;

	while (!sclexer_get_line(&lexer)) {
		if ((ret = process(&lexer)) > 0)
			return 1;
		if (ret == -1)
			break;
	}

	sclexer_end(&lexer);
	return 0;
}
