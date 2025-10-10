/**
 * Please Comply with open source license.
 * @license: MIT License
 * @source: https://github.com/at2er/libsclexer
 */

#ifndef __LIBSCLEXER_H
#define __LIBSCLEXER_H
#include <stdio.h>
#include <stdint.h>

#define SCLEXER_IS_STOP_CHRS(CHRS, CHR) (CHRS && strchr(CHRS, CHR))

enum SCLEXER_TOK_TYPE {
	SCLEXER_TOK_TYPE_EMPTY,
	SCLEXER_TOK_TYPE_INT,
	SCLEXER_TOK_TYPE_INT_NEG, /* negative integer */
	SCLEXER_TOK_TYPE_SPACE,
	SCLEXER_TOK_TYPE_STR,
	SCLEXER_TOK_TYPE_SYM
};

struct sclexer {
	char buf[BUFSIZ], *cur;
	FILE *fp;
	const char *fpath, *stop_chrs_in_str;
	uint64_t line, column;
};

struct sclexer_str_slice {
	char *s;
	uint64_t len;
};

/**
 * @field str:
 *   Value will drop in next line parsing!
 *   If you need use this value, please
 *   create another string to store it.
 */
union sclexer_tok_type_data {
	char c;
	int64_t sint;
	struct sclexer_str_slice str;
	uint64_t uint;
};

struct sclexer_tok {
	enum SCLEXER_TOK_TYPE type;
	union sclexer_tok_type_data type_data;
};

extern void sclexer_end(struct sclexer *lexer);

/**
 * Read a line to lexer->buf and set lexer->cur to it.
 * @return: 0 on success, or EOF.
 */
extern int sclexer_get_line(struct sclexer *lexer);

extern int sclexer_init(const char *fpath, struct sclexer *lexer);

/**
 * Move to next column.
 * Won't move to next line.
 * @return: character in next character.
 */
extern char sclexer_next_column(struct sclexer *lexer);

/**
 * Read a string slice from current file position
 * until match any character in string `s`.
 * @param s: End characters.
 */
extern int sclexer_read_to(struct sclexer_str_slice *result,
		struct sclexer *lexer,
		const char *s);

/**
 * @return: 0 on success, or 1 on error.
 */
extern int sclexer_read_tok(struct sclexer_tok *result, struct sclexer *lexer);

/**
 * Read a string slice from current file position
 * while `cond` function return true.
 * @param cond:
 *   @return:
 *     0: end
 *     1: continue
 *     2: error
 */
extern int sclexer_read_while(struct sclexer_str_slice *result,
		struct sclexer *lexer,
		int (*cond)(struct sclexer_str_slice *result,
			struct sclexer *lexer));

/**
 * @return:
 *   On success: Function result of ftell(lexer->fp).
 *   Otherwise(ERROR): -1
 */
extern long sclexer_record(struct sclexer *lexer, struct sclexer *recorder);

extern void sclexer_restore(struct sclexer *lexer,
		struct sclexer *recorder,
		long fpos);

#endif
