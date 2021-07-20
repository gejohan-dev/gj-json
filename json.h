#if !defined(JSON_H)
#define JSON_H

#include <gj/gj_base.h>

#if 0 //defined(GJ_DEBUG)
extern void printf(const char* s);
#define Print(Str) printf(Str)
static int depth = 0;
#define IncDepth() depth++;
#define DecDepth() depth--;
#define PadDepth() do { char __buffer[BUFFER_SIZE]; for (int i = 0; i < depth; i++) {__buffer[i] = gj_DigitToChar(i);} __buffer[depth] = '\0'; printf(__buffer); } while (0)
#else
#define Print(Str)
#define IncDepth()
#define DecDepth()
#define PadDepth()
#endif

//////////////////////////////////////////////////////////////////////
// API
//////////////////////////////////////////////////////////////////////

///////////////////////////////////
// Data
///////////////////////////////////
typedef enum JSONType
{
    JSONType_Null   = 0,
    JSONType_Object,
    JSONType_Array,
    JSONType_String,
    JSONType_Number,
    JSONType_True,
    JSONType_False
} JSONType;

typedef struct JSONObject
{
    struct JSONObject* next;
    struct JSONNode*   key;
    struct JSONNode*   value;
} JSONObject;

typedef struct JSONArray
{
    struct JSONArray* next;
    struct JSONNode*  value;
} JSONArray;

typedef struct JSONString
{
    char* string;
    int   length;
} JSONString;

typedef struct JSONNode
{
    JSONType type;
    union
    {
        JSONObject* object;
        JSONArray*  array;
        JSONString* string;
    };
} JSONNode;

typedef struct JSON
{
    MemoryArena memory_arena;
    JSONNode root;
} JSON;

///////////////////////////////////
// Methods
///////////////////////////////////
static JSON gj_init_json(void* memory, size_t memory_size);
// return (size_t)bytes read by gj_parse_json
static size_t gj_parse_json(JSON* json, void* json_data, size_t size);

//////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////
#define GJSON_OBJECT_START      '{'
#define GJSON_OBJECT_END        '}'
#define GJSON_ARRAY_START       '['
#define GJSON_ARRAY_END         ']'
#define GJSON_STRING            '"'
#define GJSON_ELEMENT_SEPARATOR ','
#define GJSON_MEMBER_COLON      ':'
#define GJSON_FRACTION          '.'
#define GJSON_SIGN_POSITIVE     '+'
#define GJSON_SIGN_NEGATIVE     '-'
#define GJSON_EXPONENT_E        'E'
#define GJSON_EXPONENT_e        'e'

#define GJSON_TRUE  "true"
#define GJSON_FALSE "false"
#define GJSON_NULL  "null"

//////////////////////////////////////////////////////////////////////
// JSONParseState/Queue
//////////////////////////////////////////////////////////////////////
typedef enum JSONStateType
{
    JSONStateType_Undefined = 0,
    JSONStateType_SkipWhitespace,
    JSONStateType_Value,
    JSONStateType_MoveCursor,
    JSONStateType_Object,
    JSONStateType_Array,
    JSONStateType_String,
    JSONStateType_Number,
    JSONStateType_True,
    JSONStateType_False,
    JSONStateType_Null
} JSONStateType;

typedef enum JSONObjectState
{
    JSONObjectState_Start       = 0,
    JSONObjectState_End         = 1,
    JSONObjectState_KeyBefore   = 2,
    JSONObjectState_Key         = 3,
    JSONObjectState_KeyAfter    = 4,
    JSONObjectState_Colon       = 5,
    JSONObjectState_ValueBefore = 6,
    JSONObjectState_Value       = 7,
    JSONObjectState_ValueAfter  = 8,
    JSONObjectState_CheckNext   = 9
} JSONObjectState;

typedef enum JSONArrayState
{
    JSONArrayState_Start       = 0,
    JSONArrayState_End         = 1,
    JSONArrayState_ValueBefore = 2,
    JSONArrayState_Value       = 3,
    JSONArrayState_ValueAfter  = 4,
    JSONArrayState_CheckNext   = 5
} JSONArrayState;

typedef enum JSONStringState
{
    JSONStringState_Start     = 0,
    JSONStringState_Char      = 1,
    JSONStringState_Backslash = 2
} JSONStringState;

typedef enum JSONNumberState
{
    JSONNumberState_IntegerSign = 0,
    JSONNumberState_Integer   = 1,
    JSONNumberState_Fraction  = 2,
    JSONNumberState_FractionInteger = 3,
    JSONNumberState_Exponent,
    JSONNumberState_ExponentSign,
    JSONNumberState_ExponentInteger,
} JSONNumberState;

typedef struct JSONParseState
{
    char type;
    char state;
    JSONNode* node;
} JSONParseState;

#define JSON_PARSE_QUEUE_SIZE 100
typedef struct JSONParseQueue
{
    JSONParseState queue[JSON_PARSE_QUEUE_SIZE];
    int count;
} JSONParseQueue;

static JSONParseQueue json_parse_queue;

static void json_parse_queue_push(JSONStateType type, JSONNode* node)
{
    gj_Assert(json_parse_queue.count <= JSON_PARSE_QUEUE_SIZE);
    gj_ZeroMemory(&json_parse_queue.queue[json_parse_queue.count]);
    json_parse_queue.queue[json_parse_queue.count].type = type;
    json_parse_queue.queue[json_parse_queue.count].node = node;
    json_parse_queue.count++;
}

static void json_parse_queue_pop()
{
    gj_Assert(json_parse_queue.count > 0);
    json_parse_queue.count--;
}

static JSONParseState* json_parse_queue_current()
{
    gj_Assert(json_parse_queue.count > 0);
    return &json_parse_queue.queue[json_parse_queue.count - 1];
}

//////////////////////////////////////////////////////////////////////
// JSONParseData
//////////////////////////////////////////////////////////////////////
typedef struct JSONParseData
{
    // TODO: Unicode
    char*        data;
    size_t       size;
    unsigned int cursor;
    MemoryArena* memory_arena;
} JSONParseData;

static int gjson_parse_value      (JSONParseData* json_parse_data, JSONNode* value);
static int gjson_parse_object     (JSONParseData* json_parse_data, JSONNode* object);
static int gjson_parse_object_push(JSONParseData* json_parse_data, JSONNode* object);
static int gjson_parse_object_pop (JSONParseData* json_parse_data, JSONNode* object);
static int gjson_parse_array      (JSONParseData* json_parse_data, JSONNode* array);
static int gjson_parse_array_push (JSONParseData* json_parse_data, JSONNode* array);
static int gjson_parse_array_pop  (JSONParseData* json_parse_data, JSONNode* array);
static int gjson_parse_string     (JSONParseData* json_parse_data, JSONNode* string);
static int gjson_parse_string_push(JSONParseData* json_parse_data, JSONNode* string);
static int gjson_parse_string_pop (JSONParseData* json_parse_data, JSONNode* string);
static int gjson_parse_number     (JSONParseData* json_parse_data, JSONNode* number);
static int gjson_parse_number_push(JSONParseData* json_parse_data, JSONNode* number);
static int gjson_parse_number_pop (JSONParseData* json_parse_data, JSONNode* number);

static int gjson_parse_literal_push(JSONParseData* json_parse_data,
                                    JSONStateType type, int literal_size, JSONNode* literal);
static int gjson_parse_literal_pop(JSONParseData* json_parse_data);

static inline unsigned int gjson_get_remaining_bytes(JSONParseData* json_parse_data) { return json_parse_data->size - json_parse_data->cursor; }
static inline int gjson_out_of_bytes(JSONParseData* json_parse_data) { return json_parse_data->cursor >= json_parse_data->size; }
#define ReturnIfOutOfBytes() if (gjson_out_of_bytes(json_parse_data)) return gj_False;

static inline char gjson_feed_current_char(JSONParseData* json_parse_data)
{
    gj_Assert(json_parse_data->cursor < json_parse_data->size);
    return json_parse_data->data[json_parse_data->cursor++];
}

static inline char gjson_peek_current_char(JSONParseData* json_parse_data)
{
    gj_Assert(json_parse_data->cursor < json_parse_data->size);
    return json_parse_data->data[json_parse_data->cursor];
}

static inline char* gjson_get_current_cursor(JSONParseData* json_parse_data)
{
    /* gj_Assert(json_parse_data->cursor < json_parse_data->size); */
    return &json_parse_data->data[json_parse_data->cursor];
}


//////////////////////////////////////////////////////////////////////
// Node creation
//////////////////////////////////////////////////////////////////////
#if GJ_DEBUG
static size_t object_total      = 0;
static size_t array_total       = 0;
static size_t string_total      = 0;
static size_t string_copy_total = 0;
#endif

static inline void* gjson_allocate(JSONParseData* json_parse_data, size_t size)
{
    // TODO: Store stuff when done with it, this solution makes it so that
    //       the file needs to be re-opened if something references something in it
#if 0
    static int id = 0;
    if (json_parse_data->memory_arena->used + size >= json_parse_data->memory_arena->size)
    {
        char buffer[BUFFER_SIZE];
        stbsp_sprintf(buffer, "gjson_tmp_%d", id++);
        PlatformFileHandle file_handle = g_platform_api.get_file_handle(buffer, PlatformOpenFileModeFlags_Write);
        g_platform_api.write_data_to_file_handle(file_handle, 0, json_parse_data->memory_arena->used, json_parse_data->memory_arena->base);
        g_platform_api.close_file_handle(file_handle);
        gj_memory_clear_arena(json_parse_data->memory_arena);
    }
#endif
    return push_size(json_parse_data->memory_arena, size);
}

static inline JSONNode* gjson_node(JSONParseData* json_parse_data)
{
    JSONNode* result = (JSONNode*)gjson_allocate(json_parse_data, sizeof(JSONNode));
    return result;
}

static inline JSONObject* gjson_node_object(JSONParseData* json_parse_data)
{
    JSONObject* result = (JSONObject*)gjson_allocate(json_parse_data, sizeof(JSONObject));
#if GJ_DEBUG
    object_total += sizeof(JSONObject);
#endif
    return result;
}

static inline JSONArray* gjson_node_array(JSONParseData* json_parse_data)
{
    JSONArray* result = (JSONArray*)gjson_allocate(json_parse_data, sizeof(JSONArray));
#if GJ_DEBUG
    array_total += sizeof(JSONArray);
#endif
    return result;
}

static inline JSONString* gjson_node_string(JSONParseData* json_parse_data)
{
    JSONString* result = (JSONString*)gjson_allocate(json_parse_data, sizeof(JSONString));
#if GJ_DEBUG
    string_total += sizeof(JSONString);
#endif
    return result;
}

static inline void gjson_node_string_copy(JSONParseData* json_parse_data, JSONNode* string_node)
{
    char* current_cursor = gjson_get_current_cursor(json_parse_data) - 1;
    string_node->string->string = (char*)gjson_allocate(json_parse_data, sizeof(char) * (string_node->string->length + 1));
    memcpy(string_node->string->string, current_cursor - string_node->string->length, string_node->string->length);
    string_node->string->string[string_node->string->length] = '\0';
#if GJ_DEBUG
    string_copy_total += string_node->string->length + 1;
#endif
}

//////////////////////////////////////////////////////////////////////
// Parsing
//////////////////////////////////////////////////////////////////////
#define PushParse(Name, _JSONStateType)                                 \
    static inline int Name##_push(JSONParseData* json_parse_data, JSONNode* json) \
    {                                                                   \
        IncDepth();                                                     \
        PadDepth();                                                     \
        Print(#Name "_push\n");                                         \
        json_parse_queue_push(_JSONStateType, json);                    \
        if (!Name(json_parse_data, json))                               \
        {                                                               \
            return gj_False;                                            \
        }                                                               \
        PadDepth();                                                     \
        DecDepth();                                                     \
        Print(#Name "_pop\n");                                          \
        json_parse_queue_pop();                                         \
        return gj_True;                                                 \
    }

#define PopParse(Name)                                                  \
    static inline int Name##_pop(JSONParseData* json_parse_data, JSONNode* json) \
    {                                                                   \
        if (Name(json_parse_data, json))                                \
        {                                                               \
            PadDepth();                                                 \
            DecDepth();                                                 \
            Print(#Name "_pop\n");                                      \
            json_parse_queue_pop();                                     \
            return gj_True;                                             \
        }                                                               \
        return gj_False;                                                \
    }

// TODO: Unecessary JSON* argument
static inline int gjson_skip_whitespace(JSONParseData* json_parse_data, JSONNode* _ignored)
{
    ReturnIfOutOfBytes();
    while (gj_IsWhitespace(gjson_peek_current_char(json_parse_data)))
    {
        gjson_feed_current_char(json_parse_data);
        ReturnIfOutOfBytes();
    }
    return gj_True;
}
PushParse(gjson_skip_whitespace, JSONStateType_SkipWhitespace)
PopParse(gjson_skip_whitespace)

static int gjson_parse_object(JSONParseData* json_parse_data, JSONNode* object)
{
    JSONParseState* current = json_parse_queue_current();
    while (gj_True)
    {
        if (current->state == JSONObjectState_Start)
        {
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_START);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONObjectState_End;
            gjson_skip_whitespace_push(json_parse_data, NULL);

            object->type = JSONType_Object;
        }

        if (current->state == JSONObjectState_End)
        {
            ReturnIfOutOfBytes();
            if (gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END)
            {
                gjson_feed_current_char(json_parse_data);
                return gj_True;
            }
            else
            {
                current->state = JSONObjectState_KeyBefore;
            }
        }
        
        if (current->state == JSONObjectState_KeyBefore)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_Key;
            gjson_skip_whitespace_push(json_parse_data, NULL);
        }

        if (current->state == JSONObjectState_Key)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_KeyAfter;

            JSONNode* string = gjson_node(json_parse_data);
            gjson_parse_string_push(json_parse_data, string);

            JSONObject* object_value;
            if (!object->object)
            {
                object->object = gjson_node_object(json_parse_data);
                object_value = object->object;
            }
            else
            {
                object_value = gjson_node_object(json_parse_data);
                object_value->next = object->object;
                object->object = object_value;
            }
            object_value->key = string;
        }

        if (current-> state == JSONObjectState_KeyAfter)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_Colon;
            gjson_skip_whitespace_push(json_parse_data, NULL);
        }
        
        if (current->state == JSONObjectState_Colon)
        {
            ReturnIfOutOfBytes();
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_MEMBER_COLON);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONObjectState_ValueBefore;
        }

        if (current->state == JSONObjectState_ValueBefore)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_Value;
            gjson_skip_whitespace_push(json_parse_data, NULL);
        }

        if (current->state == JSONObjectState_Value)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_ValueAfter;

            JSONNode* value = gjson_node(json_parse_data);
            gjson_parse_value(json_parse_data, value);

            object->object->value = value;
        }

        if (current->state == JSONObjectState_ValueAfter)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_CheckNext;
            gjson_skip_whitespace_push(json_parse_data, NULL);
        }
        
        if (current->state == JSONObjectState_CheckNext)
        {
            ReturnIfOutOfBytes();
        
            if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
            {
                gjson_feed_current_char(json_parse_data);
                current->state = JSONObjectState_KeyBefore;
                if (!gjson_skip_whitespace_push(json_parse_data, NULL))
                {
                    return gj_False;
                }
            }
            else
            {
                gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END);
                gjson_feed_current_char(json_parse_data);
                return gj_True;
            }
        }
    }
}
PushParse(gjson_parse_object, JSONStateType_Object)
PopParse(gjson_parse_object)

static int gjson_parse_array(JSONParseData* json_parse_data, JSONNode* array)
{
    JSONParseState* current = json_parse_queue_current();
    while (gj_True)
    {
        if (current->state == JSONArrayState_Start)
        {
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_START);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONArrayState_End;

            array->type = JSONType_Array;
        }

        if (current->state == JSONArrayState_End)
        {
            ReturnIfOutOfBytes();
            
            if (gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_END)
            {
                gjson_feed_current_char(json_parse_data);
                return gj_True;
            }
            else
            {
                current->state = JSONArrayState_ValueBefore;
            }
        }

        if (current->state == JSONArrayState_ValueBefore)
        {
            ReturnIfOutOfBytes();
            current->state = JSONArrayState_Value;
            gjson_skip_whitespace_push(json_parse_data, NULL);
        }

        if (current->state == JSONArrayState_Value)
        {
            ReturnIfOutOfBytes();
            current->state = JSONArrayState_ValueAfter;

            JSONNode* value = gjson_node(json_parse_data);
            gjson_parse_value(json_parse_data, value);

            JSONArray* array_value;
            if (!array->array)
            {
                array->array = gjson_node_array(json_parse_data);
                array_value = array->array;
            }
            else
            {
                array_value = gjson_node_array(json_parse_data);
                array_value->next = array->array;
                array->array = array_value;
            }
            array_value->value = value;
        }

        if (current->state == JSONArrayState_ValueAfter)
        {
            ReturnIfOutOfBytes();
            current->state = JSONArrayState_CheckNext;
            gjson_skip_whitespace_push(json_parse_data, NULL);
        }
        
        if (current->state == JSONArrayState_CheckNext)
        {
            ReturnIfOutOfBytes();
            if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
            {
                gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR);
                gjson_feed_current_char(json_parse_data);
                current->state = JSONArrayState_ValueBefore;
            }
            else
            {
                gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_END);
                gjson_feed_current_char(json_parse_data);
                return gj_True;
            }
        }
    }
    return gj_False;
}
PushParse(gjson_parse_array, JSONStateType_Array)
PopParse(gjson_parse_array)

static int gjson_parse_string(JSONParseData* json_parse_data, JSONNode* string)
{
    JSONParseState* current = json_parse_queue_current();
    
    if (current->state == JSONStringState_Start)
    {
        gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_STRING);
        gjson_feed_current_char(json_parse_data);
        current->state = JSONStringState_Char;

        string->type   = JSONType_String;
        string->string = gjson_node_string(json_parse_data);
    }

    while (gj_True)
    {
        ReturnIfOutOfBytes();
        
        if (current->state == JSONStringState_Char)
        {        
            while (gj_True)
            {
                ReturnIfOutOfBytes();
                
                char current_char = gjson_feed_current_char(json_parse_data);
                if (current_char == GJSON_STRING)
                {
                    gjson_node_string_copy(json_parse_data, string);
                    return gj_True;
                }
                else if (current_char == '\\')
                {
                    current->state = JSONStringState_Backslash;
                    break;
                }

                string->string->length++;
            }
        }

        if (current->state == JSONStringState_Backslash)
        {
            ReturnIfOutOfBytes();
            
            char current_char = gjson_feed_current_char(json_parse_data);
            current->state = JSONStringState_Char;

            string->string->length++;
        }
    }
}
PushParse(gjson_parse_string, JSONStateType_String)
PopParse(gjson_parse_string)

static int gjson_parse_number(JSONParseData* json_parse_data, JSONNode* number)
{
    JSONParseState* current = json_parse_queue_current();

    number->type = JSONType_Number;
    
    if (current->state == JSONNumberState_IntegerSign)
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_SIGN_NEGATIVE)
        {
            // TODO
            gjson_feed_current_char(json_parse_data);
        }
        current->state = JSONNumberState_Integer;
    }

    if (current->state == JSONNumberState_Integer)
    {
        while (gj_IsDigit(gjson_peek_current_char(json_parse_data)))
        {
            // TODO
            gjson_feed_current_char(json_parse_data);
            ReturnIfOutOfBytes();
        }
        current->state = JSONNumberState_Fraction;
    }

    if (current->state == JSONNumberState_Fraction)
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_FRACTION)
        {
            // TODO
            gjson_feed_current_char(json_parse_data);
            current->state = JSONNumberState_FractionInteger;
        }
        else
        {
            current->state = JSONNumberState_Exponent;
        }
    }
    
    if (current->state == JSONNumberState_FractionInteger)
    {
        while (gj_IsDigit(gjson_peek_current_char(json_parse_data)))
        {
            // TODO
            gjson_feed_current_char(json_parse_data);
            ReturnIfOutOfBytes();
        }
        current->state = JSONNumberState_Exponent;
    }

    if (current->state == JSONNumberState_Exponent)
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_EXPONENT_e ||
            gjson_peek_current_char(json_parse_data) == GJSON_EXPONENT_E)
        {
            gj_Assert(gj_False);
            current->state = JSONNumberState_ExponentSign;
        }
        else
        {
            return gj_True;
        }
    }
    
    if (current->state == JSONNumberState_ExponentSign)
    {
        if (gjson_feed_current_char(json_parse_data) == GJSON_SIGN_NEGATIVE)
        {
            // TODO
        }
        current->state = JSONNumberState_ExponentInteger;
    }
    
    if (current->state == JSONNumberState_ExponentInteger)
    {
        // TODO
        gj_Assert(gj_False);
    }

    return gj_True;
}
PushParse(gjson_parse_number, JSONStateType_Number)
PopParse(gjson_parse_number)

static int gjson_parse_literal_push(JSONParseData* json_parse_data,
                                    JSONStateType type, int literal_size, JSONNode* literal)
{
    json_parse_queue_push(type, literal);
    unsigned int remaining_bytes = gjson_get_remaining_bytes(json_parse_data);
    if (remaining_bytes > literal_size)
    {
        json_parse_data->cursor += literal_size;
        json_parse_queue_pop();
        return gj_True;
    }
    else
    {
        JSONParseState* parse_state = json_parse_queue_current();
        parse_state->state = literal_size - remaining_bytes;
        json_parse_data->cursor += remaining_bytes;
        return gj_False;
    }    
}

static int gjson_parse_literal_pop(JSONParseData* json_parse_data)
{
    JSONParseState* parse_state = json_parse_queue_current();    
    unsigned int remaining_bytes = gjson_get_remaining_bytes(json_parse_data);
    if (remaining_bytes > parse_state->state)
    {
        json_parse_data->cursor += parse_state->state;
        json_parse_queue_pop();
        return gj_True;
    }
    else
    {
        parse_state->state = parse_state->state - remaining_bytes;
        json_parse_data->cursor += remaining_bytes;
        return gj_False;
    }
}

static int gjson_parse_value(JSONParseData* json_parse_data, JSONNode* value)
{
    ReturnIfOutOfBytes();
    
    char current_char = gjson_peek_current_char(json_parse_data);
    switch (current_char)
    {
        case GJSON_OBJECT_START: return gjson_parse_object_push(json_parse_data, value);
        case GJSON_ARRAY_START:  return gjson_parse_array_push(json_parse_data, value);
        case GJSON_STRING:       return gjson_parse_string_push(json_parse_data, value);
        
        default:
        {
            if (current_char == GJSON_SIGN_NEGATIVE || gj_IsDigit(current_char))
            {
                return gjson_parse_number_push(json_parse_data, value);
            }
            else
            {
                if (current_char == 't')
                {
                    value->type = JSONType_True;
                    gjson_parse_literal_push(json_parse_data, JSONStateType_True, 4, value);
                }
                else if (current_char == 'f')
                {
                    value->type = JSONType_False;
                    gjson_parse_literal_push(json_parse_data, JSONStateType_False, 5, value);
                }
                else if (current_char == 'n')
                {
                    value->type = JSONType_Null;
                    gjson_parse_literal_push(json_parse_data, JSONStateType_Null, 4, value);
                }
                else InvalidCodePath;
            }
        } break;
    }

    return gj_True;
}

//////////////////////////////////////////////////////////////////////
// API Implementation
//////////////////////////////////////////////////////////////////////
static JSON gj_init_json(void* memory, size_t memory_size)
{
    JSON result;
    gj_ZeroMemory(&result);
    initialize_arena(&result.memory_arena, memory_size, (u8*)memory);
    return result;
}

static size_t gj_parse_json(JSON* json, void* json_data, size_t size)
{
    Print("gj_parse_json\n");
    
    size_t result = 0;

    JSONParseData json_parse_data;
    json_parse_data.data         = (char*)json_data;
    json_parse_data.size         = size;
    json_parse_data.cursor       = 0;
    json_parse_data.memory_arena = &json->memory_arena;

    if (json_parse_queue.count == 0)
    {
        gjson_skip_whitespace_push(&json_parse_data, NULL);
        gjson_parse_value(&json_parse_data, &json->root);
        gjson_skip_whitespace_push(&json_parse_data, NULL);
    }

    while (!gjson_out_of_bytes(&json_parse_data) && json_parse_queue.count > 0)
    {
        JSONParseState* current = json_parse_queue_current();
        switch (current->type)
        {
            case JSONStateType_SkipWhitespace: gjson_skip_whitespace_pop(&json_parse_data, NULL); break;
                
            case JSONStateType_Value:          gjson_parse_value(&json_parse_data,      current->node); break;
            case JSONStateType_Object:         gjson_parse_object_pop(&json_parse_data, current->node); break;
            case JSONStateType_Array:          gjson_parse_array_pop(&json_parse_data,  current->node); break;
            case JSONStateType_String:         gjson_parse_string_pop(&json_parse_data, current->node); break;
            case JSONStateType_Number:         gjson_parse_number_pop(&json_parse_data, current->node); break;

            case JSONStateType_True:
            case JSONStateType_False:
            case JSONStateType_Null:
            {
                gjson_parse_literal_pop(&json_parse_data);
            } break;
            
            default: gj_Assert(gj_False); break;
        }
    }

    result = json_parse_data.cursor;
    return result;
}

#endif
