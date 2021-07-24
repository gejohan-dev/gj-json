#if !defined(JSON_H)
#define JSON_H

#include <gj/gj_base.h>

//////////////////////////////////////////////////////////////////////
// API
//////////////////////////////////////////////////////////////////////

typedef struct GJSON_State
{
    void*  data;
    size_t size;
    MemoryArena memory_arena;
} GJSON_State;

typedef enum GJSON_QueryType
{
    GJSON_QueryType_ObjectKey
} GJSON_QueryType;

typedef struct GJSON_Query
{
    GJSON_QueryType type;

    union
    {
        struct
        {
            int   string_length;
            char* string;
        };
    };
} GJSON_Query;

typedef enum GJSON_QueryResultType
{
    GJSON_QueryResultType_NeedMoreBytes,
    GJSON_QueryResultType_Hit
} GJSON_QueryResultType;

typedef struct GJSON_QueryResult
{
    GJSON_QueryResultType type;
    size_t read_bytes;
} GJSON_QueryResult;

///////////////////////////////////
// Methods
///////////////////////////////////
static GJSON_State gjson_init(void* memory, size_t memory_size);
// return (size_t)bytes read by gj_parse_json
static GJSON_QueryResult gjson_search(GJSON_State* gjson, GJSON_Query query);

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
    JSONStateType_Undefined      = 0,
    JSONStateType_SkipWhitespace = 1,
    JSONStateType_Value          = 2,
    JSONStateType_MoveCursor     = 3,
    JSONStateType_Object         = 4,
    JSONStateType_Array          = 5,
    JSONStateType_String         = 6,
    JSONStateType_Number         = 7,
    JSONStateType_True           = 8,
    JSONStateType_False          = 9,
    JSONStateType_Null           = 10
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
#if GJ_DEBUG
    JSONStateType type;
#else
    char type;
#endif
    unsigned char state; // TODO: Either document or do something about largest string = 255
} JSONParseState;

#define JSON_PARSE_QUEUE_SIZE 100
typedef struct JSONParseQueue
{
    JSONParseState queue[JSON_PARSE_QUEUE_SIZE];
    int count;
} JSONParseQueue;

static JSONParseQueue json_parse_queue;

static void json_parse_queue_push(JSONStateType type)
{
    gj_Assert(json_parse_queue.count <= JSON_PARSE_QUEUE_SIZE);
    gj_ZeroMemory(&json_parse_queue.queue[json_parse_queue.count]);
    json_parse_queue.queue[json_parse_queue.count].type = type;
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
    GJSON_Query  query;
} JSONParseData;

typedef enum JSONParseResult
{
    JSONParseResult_OutOfBytes   = 0,
    JSONParseResult_QueryDone    = 1,
    JSONParseResult_QueryNotDone = 2
} JSONParseResult;

static JSONParseResult gjson_parse_value      (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_object     (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_object_push(JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_object_pop (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_array      (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_array_push (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_array_pop  (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_string     (JSONParseData* json_parse_data, const char* match, int match_length);
static JSONParseResult gjson_parse_string_push(JSONParseData* json_parse_data, const char* match, int match_length);
static JSONParseResult gjson_parse_string_pop (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_number     (JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_number_push(JSONParseData* json_parse_data);
static JSONParseResult gjson_parse_number_pop (JSONParseData* json_parse_data);

static JSONParseResult gjson_parse_literal_push(JSONParseData* json_parse_data, JSONStateType type, JSONParseResult literal_size);
static JSONParseResult gjson_parse_literal_pop(JSONParseData* json_parse_data);

static inline unsigned int gjson_get_remaining_bytes(JSONParseData* json_parse_data) { return json_parse_data->size - json_parse_data->cursor; }
static inline int gjson_out_of_bytes(JSONParseData* json_parse_data) { return json_parse_data->cursor >= json_parse_data->size; }
#define ReturnIfOutOfBytes() if (gjson_out_of_bytes(json_parse_data)) return JSONParseResult_OutOfBytes;

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
    gj_Assert(json_parse_data->cursor < json_parse_data->size);
    return &json_parse_data->data[json_parse_data->cursor];
}

static inline void gjson_increment_cursor(JSONParseData* json_parse_data, int increment)
{
    gj_Assert(json_parse_data->cursor + 1 < json_parse_data->size);
    json_parse_data->cursor += increment;
}

//////////////////////////////////////////////////////////////////////
// Parsing
//////////////////////////////////////////////////////////////////////
typedef struct GJSON_QueryPotentialMatch
{
    char x;
} GJSON_QueryPotentialMatch;

#define PushParse(Name, _JSONStateType)                                 \
    static inline JSONParseResult Name##_push(JSONParseData* json_parse_data)       \
    {                                                                   \
        json_parse_queue_push(_JSONStateType);                          \
        JSONParseResult result = Name(json_parse_data);                 \
        if (result == JSONParseResult_QueryNotDone)                     \
        {                                                               \
            json_parse_queue_pop();                                     \
        }                                                               \
        return result;                                                  \
    }

#define PopParse(Name)                                                  \
    static inline JSONParseResult Name##_pop(JSONParseData* json_parse_data)        \
    {                                                                   \
        JSONParseResult result = Name(json_parse_data);                 \
        if (result == JSONParseResult_QueryNotDone)                     \
        {                                                               \
            json_parse_queue_pop();                                     \
        }                                                               \
        return result;                                                  \
    }

// TODO: Unecessary JSON* argument
static inline JSONParseResult gjson_skip_whitespace(JSONParseData* json_parse_data)
{
    ReturnIfOutOfBytes();
    while (gj_IsWhitespace(gjson_peek_current_char(json_parse_data)))
    {
        gjson_feed_current_char(json_parse_data);
        ReturnIfOutOfBytes();
    }
    return JSONParseResult_QueryNotDone;
}
PushParse(gjson_skip_whitespace, JSONStateType_SkipWhitespace)
PopParse(gjson_skip_whitespace)

static JSONParseResult gjson_parse_object(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    while (gj_True)
    {
        if (current->state == JSONObjectState_Start)
        {
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_START);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONObjectState_End;
            gjson_skip_whitespace_push(json_parse_data);
            
        }

        if (current->state == JSONObjectState_End)
        {
            ReturnIfOutOfBytes();
            if (gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END)
            {
                gjson_feed_current_char(json_parse_data);
                return JSONParseResult_QueryNotDone;
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
            gjson_skip_whitespace_push(json_parse_data);
        }

        if (current->state == JSONObjectState_Key)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_KeyAfter;
            int   match_length = 0;
            char* match        = NULL;
            if (json_parse_data->query.type == GJSON_QueryType_ObjectKey)
            {
                match_length = json_parse_data->query.string_length;
                match        = json_parse_data->query.string;
            }
            JSONParseResult result = gjson_parse_string_push(json_parse_data, match, match_length);
            if (result == JSONParseResult_QueryDone) return result;
        }

        if (current-> state == JSONObjectState_KeyAfter)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_Colon;
            gjson_skip_whitespace_push(json_parse_data);
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
            gjson_skip_whitespace_push(json_parse_data);
        }

        if (current->state == JSONObjectState_Value)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_ValueAfter;
            JSONParseResult result = gjson_parse_value(json_parse_data);
            if (result == JSONParseResult_QueryDone) return result;
        }

        if (current->state == JSONObjectState_ValueAfter)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_CheckNext;
            gjson_skip_whitespace_push(json_parse_data);
        }
        
        if (current->state == JSONObjectState_CheckNext)
        {
            ReturnIfOutOfBytes();
        
            if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
            {
                gjson_feed_current_char(json_parse_data);
                current->state = JSONObjectState_KeyBefore;
            }
            else
            {
                gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END);
                gjson_feed_current_char(json_parse_data);
                return JSONParseResult_QueryNotDone;
            }
        }
    }
}
PushParse(gjson_parse_object, JSONStateType_Object)
PopParse(gjson_parse_object)

static JSONParseResult gjson_parse_array(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    while (gj_True)
    {
        if (current->state == JSONArrayState_Start)
        {
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_START);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONArrayState_End;
        }

        if (current->state == JSONArrayState_End)
        {
            ReturnIfOutOfBytes();
            
            if (gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_END)
            {
                gjson_feed_current_char(json_parse_data);
                return JSONParseResult_QueryNotDone;
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
            gjson_skip_whitespace_push(json_parse_data);
        }

        if (current->state == JSONArrayState_Value)
        {
            ReturnIfOutOfBytes();
            current->state = JSONArrayState_ValueAfter;
            JSONParseResult result = gjson_parse_value(json_parse_data);
            if (result == JSONParseResult_QueryDone) return result;
        }

        if (current->state == JSONArrayState_ValueAfter)
        {
            ReturnIfOutOfBytes();
            current->state = JSONArrayState_CheckNext;
            gjson_skip_whitespace_push(json_parse_data);
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
                return JSONParseResult_QueryNotDone;
            }
        }
    }
    InvalidCodePath;
    return JSONParseResult_QueryNotDone;
}
PushParse(gjson_parse_array, JSONStateType_Array)
PopParse(gjson_parse_array)

static JSONParseResult gjson_parse_string(JSONParseData* json_parse_data, const char* match, int match_length)
{
    JSONParseState* current = json_parse_queue_current();
    
    if (current->state == JSONStringState_Start)
    {
        gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_STRING);
        gjson_feed_current_char(json_parse_data);

        unsigned int remaining_bytes = gjson_get_remaining_bytes(json_parse_data);
        char* current_cursor = gjson_get_current_cursor(json_parse_data);
        if (remaining_bytes > match_length &&
            memcmp(match, current_cursor, match_length) == 0)
        {
            json_parse_queue_pop();
            return JSONParseResult_QueryDone;
        }
        else if (memcmp(match, current_cursor, remaining_bytes) == 0)
        {
            /* current->state        = remaining_bytes; */
        }
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
                    if (current->string_match && (current->string_cursor == json_parse_data->query.string_length))
                    {
                        json_parse_queue_pop();
                        return JSONParseResult_QueryDone;
                    }
                    else return JSONParseResult_QueryNotDone;
                }
                else if (current_char == '\\')
                {
                    current->state = JSONStringState_Backslash;
                    break;
                }

                if (current->string_match && json_parse_data->query.type == GJSON_QueryType_String)
                {
                    if (json_parse_data->query.string_length < current->string_cursor ||
                        json_parse_data->query.string[current->string_cursor] != current_char)
                    {
                        current->string_match = gj_False;
                    }
                }
                else current->string_match = gj_False;
                current->string_cursor++;
            }
        }

        if (current->state == JSONStringState_Backslash)
        {
            ReturnIfOutOfBytes();
            
            char current_char = gjson_feed_current_char(json_parse_data);
            current->state = JSONStringState_Char;

            current->string_cursor++;
        }
    }
}
PushParse(gjson_parse_string, JSONStateType_String)
PopParse(gjson_parse_string)

static JSONParseResult gjson_parse_number(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();

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
            return JSONParseResult_QueryNotDone;
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

    return JSONParseResult_QueryNotDone;
}
PushParse(gjson_parse_number, JSONStateType_Number)
PopParse(gjson_parse_number)

static JSONParseResult gjson_parse_literal_push(JSONParseData* json_parse_data, JSONStateType type, int literal_size)
{
    json_parse_queue_push(type);
    unsigned int remaining_bytes = gjson_get_remaining_bytes(json_parse_data);
    if (remaining_bytes > literal_size)
    {
        json_parse_data->cursor += literal_size;
        json_parse_queue_pop();
        return JSONParseResult_QueryNotDone;
    }
    else
    {
        JSONParseState* parse_state = json_parse_queue_current();
        parse_state->state = literal_size - remaining_bytes;
        json_parse_data->cursor += remaining_bytes;
        return JSONParseResult_OutOfBytes;
    }    
}

static JSONParseResult gjson_parse_literal_pop(JSONParseData* json_parse_data)
{
    JSONParseState* parse_state = json_parse_queue_current();    
    unsigned int remaining_bytes = gjson_get_remaining_bytes(json_parse_data);
    if (remaining_bytes > parse_state->state)
    {
        json_parse_data->cursor += parse_state->state;
        json_parse_queue_pop();
        return JSONParseResult_QueryNotDone;
    }
    else
    {
        parse_state->state = parse_state->state - remaining_bytes;
        json_parse_data->cursor += remaining_bytes;
        return JSONParseResult_OutOfBytes;
    }
}

static JSONParseResult gjson_parse_value(JSONParseData* json_parse_data)
{
    ReturnIfOutOfBytes();
    
    char current_char = gjson_peek_current_char(json_parse_data);
    switch (current_char)
    {
        case GJSON_OBJECT_START: return gjson_parse_object_push(json_parse_data);
        case GJSON_ARRAY_START:  return gjson_parse_array_push(json_parse_data);
        case GJSON_STRING:       return gjson_parse_string_push(json_parse_data);
        
        default:
        {
            if (current_char == GJSON_SIGN_NEGATIVE || gj_IsDigit(current_char))
            {
                return gjson_parse_number_push(json_parse_data);
            }
            else
            {
                if (current_char == 't')
                {
                    return gjson_parse_literal_push(json_parse_data, JSONStateType_True, 4);
                }
                else if (current_char == 'f')
                {
                    return gjson_parse_literal_push(json_parse_data, JSONStateType_False, 5);
                }
                else if (current_char == 'n')
                {
                    return gjson_parse_literal_push(json_parse_data, JSONStateType_Null, 4);
                }
                else InvalidCodePath;
            }
        } break;
    }

    InvalidCodePath;
    
    return gj_True;
}

//////////////////////////////////////////////////////////////////////
// API Implementation
//////////////////////////////////////////////////////////////////////
static GJSON_State gjson_init(void* memory, size_t memory_size)
{
    GJSON_State result;
    gj_ZeroMemory(&result);
    initialize_arena(&result.memory_arena, memory_size, (u8*)memory);
    return result;
}

static GJSON_QueryResult gjson_search(GJSON_State* gjson, GJSON_Query query)
{
    GJSON_QueryResult result;
    gj_ZeroMemory(&result);

    JSONParseData json_parse_data;
    json_parse_data.data         = (char*)gjson->data;
    json_parse_data.size         = gjson->size;
    json_parse_data.cursor       = 0;
    json_parse_data.memory_arena = &gjson->memory_arena;
    json_parse_data.query        = query;

#define CheckReturn(Exp)                                                \
    do {                                                                \
        switch (Exp)                                                    \
        {                                                               \
            case JSONParseResult_OutOfBytes:                            \
                result.type = GJSON_QueryResultType_NeedMoreBytes;      \
                result.read_bytes = json_parse_data.cursor;             \
                return result;                                          \
            case JSONParseResult_QueryDone:                             \
                result.type = GJSON_QueryResultType_Hit;                \
                result.read_bytes = json_parse_data.cursor;             \
                return result;                                          \
            case JSONParseResult_QueryNotDone: break;                   \
                                                                        \
            InvalidDefaultCase;                                         \
        }                                                               \
    } while(gj_False)
    
    if (json_parse_queue.count == 0)
    {
        CheckReturn(gjson_skip_whitespace_push(&json_parse_data));
        CheckReturn(gjson_parse_value(&json_parse_data));
        CheckReturn(gjson_skip_whitespace_push(&json_parse_data));
    }

    while (!gjson_out_of_bytes(&json_parse_data) && json_parse_queue.count > 0)
    {
        JSONParseState* current = json_parse_queue_current();
        switch (current->type)
        {
            case JSONStateType_SkipWhitespace: CheckReturn(gjson_skip_whitespace_pop(&json_parse_data)); break;
                
            case JSONStateType_Value:          CheckReturn(gjson_parse_value(&json_parse_data));      break;
            case JSONStateType_Object:         CheckReturn(gjson_parse_object_pop(&json_parse_data)); break;
            case JSONStateType_Array:          CheckReturn(gjson_parse_array_pop(&json_parse_data));  break;
            case JSONStateType_String:         CheckReturn(gjson_parse_string_pop(&json_parse_data)); break;
            case JSONStateType_Number:         CheckReturn(gjson_parse_number_pop(&json_parse_data)); break;

            case JSONStateType_True:
            case JSONStateType_False:
            case JSONStateType_Null:
            {
                CheckReturn(gjson_parse_literal_pop(&json_parse_data));
            } break;
            
            default: gj_Assert(gj_False); break;
        }
    }

    if (gjson_out_of_bytes(&json_parse_data))
    {
        result.type = GJSON_QueryResultType_NeedMoreBytes;
        result.read_bytes = json_parse_data.cursor;
    }
    else InvalidCodePath;
    
    return result;
}

#endif
