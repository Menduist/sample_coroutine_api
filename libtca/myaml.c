#include <yaml.h>
#include "myaml.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

void dump_myaml_t(struct myaml_t *t, int level) {
	char header[30];
	int i = 0;

	if (!t) return;
	snprintf(header, 20, "%.*s", level * 2, "                      ");
	switch (t->type) {
	case MYAML_OBJECT:
		printf("\n%s=> Object\n", header);
		while (t->keys[i]) {
			printf("%s    %s: ", header, t->keys[i]);
			dump_myaml_t(t->childs[i], level + 1);
			i++;
		}
		break;
	case MYAML_ARRAY:
		printf("\n%s=> Array\n", header);
		while (t->childs[i]) {
			printf("%s %d: ", header, i);
			dump_myaml_t(t->childs[i], level + 1);
			i++;
		}
		break;
	case MYAML_SCALAR:
		printf("%s\n", t->value);
		break;
	case MYAML_NULL:
		printf("null\n");
		break;
	}
}

void dump_myaml_type(enum myaml_type t) {

}

static void dump_token(yaml_token_t *token) {
	switch (token->type) {
	case YAML_NO_TOKEN: printf("YAML_NO_TOKEN\n"); break;
	case YAML_STREAM_START_TOKEN: printf("YAML_STREAM_START_TOKEN\n"); break;
	case YAML_STREAM_END_TOKEN: printf("YAML_STREAM_END_TOKEN\n"); break;
	case YAML_VERSION_DIRECTIVE_TOKEN: printf("YAML_VERSION_DIRECTIVE_TOKEN\n"); break;
	case YAML_TAG_DIRECTIVE_TOKEN: printf("YAML_TAG_DIRECTIVE_TOKEN\n"); break;
	case YAML_DOCUMENT_START_TOKEN: printf("YAML_DOCUMENT_START_TOKEN\n"); break;
	case YAML_DOCUMENT_END_TOKEN: printf("YAML_DOCUMENT_END_TOKEN\n"); break;
	case YAML_BLOCK_SEQUENCE_START_TOKEN: printf("YAML_BLOCK_SEQUENCE_START_TOKEN\n"); break;
	case YAML_BLOCK_MAPPING_START_TOKEN: printf("YAML_BLOCK_MAPPING_START_TOKEN\n"); break;
	case YAML_BLOCK_END_TOKEN: printf("YAML_BLOCK_END_TOKEN\n"); break;
	case YAML_FLOW_SEQUENCE_START_TOKEN: printf("YAML_FLOW_SEQUENCE_START_TOKEN\n"); break;
	case YAML_FLOW_SEQUENCE_END_TOKEN: printf("YAML_FLOW_SEQUENCE_END_TOKEN\n"); break;
	case YAML_FLOW_MAPPING_START_TOKEN: printf("YAML_FLOW_MAPPING_START_TOKEN\n"); break;
	case YAML_FLOW_MAPPING_END_TOKEN: printf("YAML_FLOW_MAPPING_END_TOKEN\n"); break;
	case YAML_BLOCK_ENTRY_TOKEN: printf("YAML_BLOCK_ENTRY_TOKEN\n"); break;
	case YAML_FLOW_ENTRY_TOKEN: printf("YAML_FLOW_ENTRY_TOKEN\n"); break;
	case YAML_KEY_TOKEN: printf("YAML_KEY_TOKEN\n"); break;
	case YAML_VALUE_TOKEN: printf("YAML_VALUE_TOKEN\n"); break;
	case YAML_ALIAS_TOKEN: printf("YAML_ALIAS_TOKEN\n"); break;
	case YAML_ANCHOR_TOKEN: printf("YAML_ANCHOR_TOKEN\n"); break;
	case YAML_TAG_TOKEN: printf("YAML_TAG_TOKEN\n"); break;
	case YAML_SCALAR_TOKEN: printf("YAML_SCALAR_TOKEN %s\n", token->data.scalar.value); break;
	}
}

/*
** Parser
*/

struct parser {
	yaml_parser_t *parser;
	yaml_token_t token;
};

static struct myaml_t *myaml_parse(struct parser *parser);

static yaml_token_t *parser_next_token(struct parser *parser) {
	yaml_token_delete(&parser->token);
	yaml_parser_scan(parser->parser, &parser->token);
	//dump_token(&parser->token);
	return &parser->token;
}

static yaml_token_t *parser_get_token(struct parser *parser) {
	return &parser->token;
}

static struct myaml_t *myaml_parse_scalar(struct parser *parser) {
	struct myaml_t *result = calloc(sizeof(struct myaml_t), 1);
	result->type = MYAML_SCALAR;
	result->value = strdup((const char *)parser_get_token(parser)->data.scalar.value);
	parser_next_token(parser);
	return result;
}

static struct myaml_t *myaml_parse_indentless_sequence(struct parser *parser) {
	yaml_token_t *token;
	struct myaml_t *result = calloc(sizeof(struct myaml_t), 1);
	result->type = MYAML_ARRAY;
	result->childs = calloc(sizeof(char *), 50);

	int stop = 0;
	token = parser_get_token(parser);
	while (!stop) {
		if (token->type != YAML_BLOCK_ENTRY_TOKEN) {
			return result;
		}
		token = parser_next_token(parser);
		append_to_array((void **)result->childs, myaml_parse(parser));
	}
	return result;
}

static struct myaml_t *myaml_parse_sequence(struct parser *parser) {
	yaml_token_t *token;
	struct myaml_t *result = calloc(sizeof(struct myaml_t), 1);
	result->type = MYAML_ARRAY;
	result->childs = calloc(sizeof(char *), 50);

	int stop = 0;
	token = parser_get_token(parser);
	while (!stop) {
		switch (token->type) {
		case YAML_BLOCK_END_TOKEN:
		case YAML_FLOW_SEQUENCE_END_TOKEN:
			token = parser_next_token(parser);
			stop = 1;
			break;
		case YAML_BLOCK_ENTRY_TOKEN:
		case YAML_FLOW_ENTRY_TOKEN:
		case YAML_FLOW_SEQUENCE_START_TOKEN:
			token = parser_next_token(parser);
			if (token->type == YAML_KEY_TOKEN || token->type == YAML_BLOCK_END_TOKEN) {
				append_to_array((void **)result->childs, myaml_null());
				break;
			}
			append_to_array((void **)result->childs, myaml_parse(parser));
			break;
		case YAML_NO_TOKEN:
		case YAML_BLOCK_SEQUENCE_START_TOKEN:
			token = parser_next_token(parser);
			break;
		default:
			printf("Unknown type for sequence! "); dump_token(token); token = parser_next_token(parser);
		}
	}
	return result;
}

static struct myaml_t *myaml_parse_object(struct parser *parser) {
	yaml_token_t *token;
	struct myaml_t *result = calloc(sizeof(struct myaml_t), 1);
	result->type = MYAML_OBJECT;
	result->childs = calloc(sizeof(char *), 50);
	result->keys = calloc(sizeof(char *), 50);

	int stop = 0;
	token = parser_get_token(parser);
	while (!stop) {
		switch (token->type) {
		case YAML_KEY_TOKEN:
			token = parser_next_token(parser);
			assert(token->type == YAML_SCALAR_TOKEN);
			append_to_array((void **)result->keys, strdup((const char *)token->data.scalar.value));
			token = parser_next_token(parser);
			break;
		case YAML_VALUE_TOKEN:
			token = parser_next_token(parser);
			if (token->type == YAML_KEY_TOKEN || token->type == YAML_BLOCK_END_TOKEN || token->type == YAML_FLOW_MAPPING_END_TOKEN) {
				if (token->type == YAML_BLOCK_END_TOKEN)
					stop = 1;
				append_to_array((void **)result->childs, myaml_null());
				break;
			}
			append_to_array((void **)result->childs, myaml_parse(parser));
			break;
		case YAML_BLOCK_END_TOKEN:
		case YAML_FLOW_MAPPING_END_TOKEN:
			stop = 1;
		case YAML_BLOCK_ENTRY_TOKEN:
		case YAML_FLOW_ENTRY_TOKEN:
		case YAML_BLOCK_MAPPING_START_TOKEN:
		case YAML_FLOW_MAPPING_START_TOKEN:
		case YAML_NO_TOKEN:
			token = parser_next_token(parser);
			break;
		default:
			printf("Unknown type for object! "); dump_token(token); token = parser_next_token(parser);
		}
	}
	return result;
}

static struct myaml_t *myaml_parse(struct parser *parser) {
	yaml_token_t *token = parser_get_token(parser);
	switch (parser_get_token(parser)->type) {
	case YAML_BLOCK_MAPPING_START_TOKEN:
	case YAML_FLOW_MAPPING_START_TOKEN:
		return myaml_parse_object(parser);
	case YAML_BLOCK_SEQUENCE_START_TOKEN:
	case YAML_FLOW_SEQUENCE_START_TOKEN:
		return myaml_parse_sequence(parser);
	case YAML_BLOCK_ENTRY_TOKEN: //indentless sequence
		return myaml_parse_indentless_sequence(parser);
	case YAML_SCALAR_TOKEN: return myaml_parse_scalar(parser);
	case YAML_STREAM_START_TOKEN:
	case YAML_DOCUMENT_START_TOKEN:
	case YAML_NO_TOKEN:
		parser_next_token(parser);
		return myaml_parse(parser);
	case YAML_STREAM_END_TOKEN:
		return 0;
	default:
		printf("Unknown for type parse ! "); dump_token(token);
	}
	return 0;
}

struct myaml_t *myaml_loadf(FILE *conf) {
	yaml_parser_t parser;
	struct parser mparse;

	if (!yaml_parser_initialize(&parser)) {
		printf("Failed to init yaml parser!\n");
	}
	if (!conf) {
		printf("Failed to open conf file !\n");
	}
	yaml_parser_set_input_file(&parser, conf);

	mparse.parser = &parser;
	yaml_parser_scan(&parser, &mparse.token);
	struct myaml_t *result = myaml_parse(&mparse);
	yaml_parser_delete(&parser);
	return result;
}

struct myaml_t *myaml_loads(char *str) {
	yaml_parser_t parser;
	struct parser mparse;

	if (!yaml_parser_initialize(&parser)) {
		printf("Failed to init yaml parser!\n");
	}
	yaml_parser_set_input_string(&parser, str, strlen(str));

	mparse.parser = &parser;
	yaml_parser_scan(&parser, &mparse.token);
	struct myaml_t *result = myaml_parse(&mparse);
	yaml_parser_delete(&parser);
	return result;
}
/*
** User fonctions
*/
int myaml_array_size(struct myaml_t *obj) {
	if (obj->type != MYAML_ARRAY) return -1;
	return array_len((void **)obj->childs);
}

struct myaml_t *myaml_array_get(struct myaml_t *obj, int index) {
	assert(index < myaml_array_size(obj));
	return obj->childs[index];
}

struct myaml_t *myaml_null(void) {
	static struct myaml_t *result = 0;

	if (!result) {
		result = calloc(sizeof(struct myaml_t), 1);
		result->type = MYAML_NULL;
	}
	return result;
}

struct myaml_t *myaml_object_get(struct myaml_t *obj, char *key) {
	if (obj->type != MYAML_OBJECT) return 0;
	int i = 0;
	while (obj->keys[i]) {
		if (strcmp(obj->keys[i], key) == 0) return obj->childs[i];
		i++;
	}
	return 0;
}

char *myaml_scalar_value(struct myaml_t *obj) {
	if (obj->type != MYAML_SCALAR) return 0;
	return obj->value;
}

void myaml_free(struct myaml_t *f) {
	int i = 0;
	switch (f->type) {
	case MYAML_OBJECT:
		while (f->keys[i]) {
			free(f->keys[i]);
			myaml_free(f->childs[i]);
			i++;
		}
		free(f->keys);
		free(f->childs);
		break;
	case MYAML_ARRAY:
		while (f->childs[i]) {
			myaml_free(f->childs[i]);
			i++;
		}
		free(f->childs);
		break;
	case MYAML_SCALAR:
		free(f->value);
		break;
	case MYAML_NULL:
		return;
	}
	free(f);
}

/*
** Pack / Unpack
*/
struct unpacker {
	const char *fmt;
	char error[255];
	va_list *ap;
};

static int unpack(struct myaml_t *f, struct unpacker *unpacker);

static char next_token(struct unpacker *unpacker) {
	//printf("next_token old: %c", unpacker->fmt[0]);
	do {
		unpacker->fmt = unpacker->fmt + 1;
	} while (strchr("{}[]asi?r", unpacker->fmt[0]) == 0 && unpacker->fmt[0]);
	//printf(" new: %c\n", unpacker->fmt[0]);
	return unpacker->fmt[0];
}

static char get_token(struct unpacker *unpacker) {
	return unpacker->fmt[0];
}

static int unpack_array(struct myaml_t *f, struct unpacker *unpacker) {
	assert(!f || f->type == MYAML_ARRAY);
	int stop = 0;
	next_token(unpacker);
	int i = 0;
	while (get_token(unpacker) != ']') {
		struct myaml_t *child = f ? myaml_array_get(f, i++) : 0;
		if (!child) {
			sprintf(unpacker->error, "No child %d!", i);
			return 1;
		}
		if (unpack(child, unpacker)) return 1;
	}
	next_token(unpacker);
	return 0;
}

static int unpack_object(struct myaml_t *f, struct unpacker *unpacker) {
	assert(!f || f->type == MYAML_OBJECT);
	int stop = 0;
	next_token(unpacker);
	while (get_token(unpacker) != '}') {
		if (get_token(unpacker) != 's') {
			sprintf(unpacker->error, "ERROR! Excepted 's', got '%c'", get_token(unpacker));
			return 1;
		}
		next_token(unpacker);
		int required = 1;
		if (get_token(unpacker) == '?') {
			required = 0;
			next_token(unpacker);
		}
		char *key = va_arg(*unpacker->ap, char *);
		struct myaml_t *child = f ? myaml_object_get(f, key) : 0;
		if (!child && required) {
			sprintf(unpacker->error, "No child %s!", key);
			return 1;
		}
		if (unpack(child, unpacker)) return 1;
	}
	next_token(unpacker);
	return 0;
}

static int unpack_autoarray(struct myaml_t *s, struct unpacker *unpacker) {
	assert(get_token(unpacker) == 'a');
	next_token(unpacker);
	if (!s) { //Empty array I guess
		next_token(unpacker);
		return 0;
	}
	char ***starget;
	int array_size;
	int i;
	switch (get_token(unpacker)) {
		case 's':
			starget = va_arg(*unpacker->ap, char ***);
			array_size = myaml_array_size(s);
			*starget = calloc(sizeof(char *), array_size + 1);
			i = 0;
			while (i < array_size) {
				(*starget)[i] = strdup(s->childs[i]->value);
				i++;
			}
			break;
		default:
			sprintf(unpacker->error, "Error, unsupported type for auto array '%c'", get_token(unpacker));
			return 1;
	}
	next_token(unpacker);
	return 0;
}


static int unpack_raw(struct myaml_t *s, struct unpacker *unpacker) {
	assert(get_token(unpacker) == 'r');
	struct myaml_t **target = va_arg(*unpacker->ap, struct myaml_t **);
	*target = s;
	next_token(unpacker);
	return 0;
}

static int unpack_scalar(struct myaml_t *s, struct unpacker *unpacker) {
	assert(!s || s->type == MYAML_SCALAR);
	if (s) {
		char **starget;
		int *itarget;
		switch (get_token(unpacker)) {
			case 's':
				starget = va_arg(*unpacker->ap, char **);
				*starget = strdup(s->value);
				break;
			case 'i':
				itarget = va_arg(*unpacker->ap, int *);
				*itarget = atoi(s->value);
				break;
			default:
				sprintf(unpacker->error, "Error, unknown scalar '%c'", get_token(unpacker));
				return 1;
		}
	}
	else va_arg(*unpacker->ap, void *);
	next_token(unpacker);
	return 0;
}

static int unpack(struct myaml_t *f, struct unpacker *unpacker) {
	switch (get_token(unpacker)) {
		case '{':
			return unpack_object(f, unpacker);
		case '[':
			return unpack_array(f, unpacker);
		case 's':
		case 'i':
			return unpack_scalar(f, unpacker);
		case 'r':
			return unpack_raw(f, unpacker);
		case 'a':
			return unpack_autoarray(f, unpacker);
		default:
			sprintf(unpacker->error, "Error, unknown type in unpack '%c'", get_token(unpacker));
			return 1;
	}
}

int myaml_unpack(struct myaml_t *f, const char *fmt, ...) {
	struct unpacker unpacker;
	va_list ap_copy;
	memset(&unpacker, 0, sizeof(struct unpacker));
	va_start(ap_copy, fmt);
	unpacker.fmt = fmt;
	unpacker.ap = &ap_copy;
	int result = unpack(f, &unpacker);
	va_end(ap_copy);
	if (strlen(unpacker.error) > 0) {
		printf("Unpacking failed! %s\n", unpacker.error);
	}
	return result;
}
