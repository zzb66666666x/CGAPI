#ifndef _PARSE_H
#define _PARSE_H

#ifdef __cplusplus
extern "C"{
#endif

// for user
int parse_string(const char* str, char** output_buffer, int* buf_size);
int parse_file(const char* filename, char** output_buffer, int* buf_size);

// for parser
#define FAILURE -1
#define SUCCESS 0

#define LAYOUT_UNDEF	-1
#define NORMAL_VAR		0
#define INPUT_VAR		1
#define OUTPUT_VAR		2
#define UNIFORM_VAR		3
#define TYPE_VOID		0
#define TYPE_VEC2		1
#define TYPE_VEC3		2
#define TYPE_VEC4		3
#define TYPE_MAT2		4
#define TYPE_MAT3		5
#define TYPE_MAT4		6
#define TYPE_FLOAT		7
#define TYPE_INT		8
#define TYPE_DOUBLE		9
#define TYPE_BOOL		10

typedef struct{
	char* data;
	int size;
	int usage;
}buffer_t;

extern int init_buffer(buffer_t* buf, int target_size);
extern void reset_buffer(buffer_t * buf);
extern void free_buffer(buffer_t * buf);
extern int register_code(buffer_t * buf, const char* code);
extern void free_lexer_buffer();

// insert io function called input_port/output_port code buffer (as string)
extern void generate_data_path(buffer_t* buf);

// global variables
extern buffer_t parser_out;

#ifdef __cplusplus
}
#endif

#endif