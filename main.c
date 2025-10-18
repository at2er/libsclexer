/**
 * Kick your /dev/brain and /dev/ass. :)
 * Just for debug, don't care this shits.
 */
#include "sclexer.h"
#include <stdlib.h>
#include <string.h>

enum COMMENTS {
	SINGLE_COMMENT,

	COMMENTS_COUNT
};

enum KEYWORDS {
	KW_PRINT,

	KEYWORDS_COUNT
};

enum SYMBOLS {
	SYM_PAREN_L,
	SYM_PAREN_R,
	SYM_ADD,
	SYM_ADD_ASSIGN,
	SYM_SUB,

	SYMBOLS_COUNT
};

static const char *comments[COMMENTS_COUNT] = {
	[SINGLE_COMMENT] = ";"
};

static const char *keywords[KEYWORDS_COUNT] = {
	[KW_PRINT] = "print"
};

static const char *symbols[SYMBOLS_COUNT] = {
	[SYM_PAREN_L]    = "(",
	[SYM_PAREN_R]    = ")",
	[SYM_ADD]        = "+",
	[SYM_ADD_ASSIGN] = "+=",
	[SYM_SUB]        = "-"
};

int main(int argc, char *argv[])
{
	char *src;
	const char *fpath;
	struct sclexer lexer = {0};
	if (argc < 2)
		return 1;
	fpath = argv[1];

	lexer.enable_indent = true;

	lexer.src_siz = sclexer_read_file(&src, fpath);
	lexer.src = src;

	lexer.comments = comments;
	lexer.comments_count = COMMENTS_COUNT;

	lexer.keywords = keywords;
	lexer.keywords_count = KEYWORDS_COUNT;

	lexer.symbols = symbols;
	lexer.symbols_count = SYMBOLS_COUNT;

	sclexer_init(&lexer, fpath);

	/*
	struct sclexer_tok tok = {0};
	while (sclexer_get_tok(&lexer, &tok))
		print_tok(&tok);
	*/

	struct sclexer_tok *tokens = NULL;
	size_t tokens_count = sclexer_get_tokens(&lexer, &tokens);
	for (size_t i = 0; i < tokens_count; i++)
		sclexer_print_tok(&lexer, &tokens[i]);

	return 0;
}
