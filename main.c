#include "sclexer.h"
#include <stdlib.h>
#include <string.h>

//#define ENABLE_MORE_LOC_MSG
#ifdef ENABLE_MORE_LOC_MSG
#define TOK_LOC_FMT "\x1b[31mf:%s,l:%lu,c:%lu\x1b[0m"
#define TOK_LOC_UNWRAP(TOK) (TOK)->loc.fpath, (TOK)->loc.line, (TOK)->loc.column
#else
#define TOK_LOC_FMT "\x1b[31ml:%lu,c:%lu\x1b[0m"
#define TOK_LOC_UNWRAP(TOK) (TOK)->loc.line, (TOK)->loc.column
#endif

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

static void print_tok(struct sclexer_tok *tok);

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

void print_tok(struct sclexer_tok *tok)
{
	switch (tok->kind) {
	case SCLEXER_IDENT:
		printf("<%s: len=%lu, '%.*s'> "TOK_LOC_FMT"\n",
				sclexer_kind_names(tok->kind),
				tok->src.len,
				(int)tok->src.len,
				tok->src.begin,
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_INT:
		printf("<%s: %ld> "TOK_LOC_FMT"\n",
				sclexer_kind_names(tok->kind),
				tok->data.sint,
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_INT_NEG:
		printf("<%s: %lu> "TOK_LOC_FMT"\n",
				sclexer_kind_names(tok->kind),
				tok->data.uint,
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_KEYWORD:
		printf("<%s: '%s'> "TOK_LOC_FMT"\n",
				sclexer_kind_names(tok->kind),
				keywords[tok->data.keyword],
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_STRING:
		printf("<%s: len=%lu: '%.*s'> "TOK_LOC_FMT"\n",
				sclexer_kind_names(tok->kind),
				tok->data.str.len,
				(int)tok->data.str.len,
				tok->data.str.begin,
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_SYMBOL:
		printf("<%s: %ld: '%s'> "TOK_LOC_FMT"\n",
				sclexer_kind_names(tok->kind),
				tok->data.symbol,
				symbols[tok->data.symbol],
				TOK_LOC_UNWRAP(tok));
		break;
	default:
		printf("<%s> "TOK_LOC_FMT"\n",
				sclexer_kind_names(tok->kind),
				TOK_LOC_UNWRAP(tok));
		break;
	}
}

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
	for (size_t i = 0; i < tokens_count; i++) {
		print_tok(&tokens[i]);
	}

	return 0;
}
