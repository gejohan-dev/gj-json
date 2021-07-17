#if !defined(JSON_H)
#define JSON_H

#include <gj/gj_base.h>

#if GJ_DEBUG
extern void printf(const char* s, int length);
#define Print(Str) printf(Str, gj_ArrayCount(Str))
static int depth = 0;
#define PadDepth() do { char __buffer[BUFFER_SIZE]; for (int i = 0; i < depth; i++) {__buffer[i] = gj_DigitToChar(i);} __buffer[depth] = '\0'; printf(__buffer, depth); } while (0)
#endif

//////////////////////////////////////////////////////////////////////
// API
//////////////////////////////////////////////////////////////////////

///////////////////////////////////
// Data
///////////////////////////////////
typedef enum JSON_Type
{
    JSON_Type_Null   = 0,
    JSON_Type_Object,
    JSON_Type_Array,
    JSON_Type_String,
    JSON_Type_Number,
    JSON_Type_True,
    JSON_Type_False
} JSON_Type;

typedef struct JSON
{
    JSON_Type type;
} JSON;

///////////////////////////////////
// Methods
///////////////////////////////////
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
    JSONStateType_Null = 0,
    JSONStateType_SkipWhitespace,
    JSONStateType_Value,
    JSONStateType_MoveCursor,
    JSONStateType_Object,
    JSONStateType_Array,
    JSONStateType_String,
    JSONStateType_Number
} JSONStateType;

typedef enum JSONObjectState
{
    JSONObjectState_Start     = 0,
    JSONObjectState_End       = 1,
    JSONObjectState_Key       = 2,
    JSONObjectState_Colon     = 3,
    JSONObjectState_Value     = 4,
    JSONObjectState_CheckNext = 5
} JSONObjectState;

typedef enum JSONArrayState
{
    JSONArrayState_Start     = 0,
    JSONArrayState_End       = 1,
    JSONArrayState_Value     = 2,
    JSONArrayState_CheckNext = 3
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
    JSONObjectState state;
#else
    char type;
    char state;
#endif
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
    json_parse_queue.queue[json_parse_queue.count].type  = type;
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
} JSONParseData;


static int gjson_parse_value (JSONParseData* json_parse_data);
static int gjson_parse_object(JSONParseData* json_parse_data);
static int gjson_parse_array (JSONParseData* json_parse_data);
static int gjson_parse_string(JSONParseData* json_parse_data);
static int gjson_parse_number(JSONParseData* json_parse_data);

static inline int gjson_out_of_bytes(JSONParseData* json_parse_data) { return json_parse_data->cursor >= json_parse_data->size; }
#define ReturnIfOutOfBytes() if (gjson_out_of_bytes(json_parse_data)) return gj_False;

#define PushParse(Name, _JSONStateType)                                 \
    static inline int Name##_push(JSONParseData* json_parse_data)       \
    {                                                                   \
        depth++;                                                        \
        PadDepth();                                                     \
        Print(#Name "_push\n");                                         \
        json_parse_queue_push(_JSONStateType);                          \
        if (!Name(json_parse_data))                                     \
        {                                                               \
            return gj_False;                                            \
        }                                                               \
        PadDepth();                                                     \
        depth--;                                                        \
        Print(#Name "_pop\n");                                          \
        json_parse_queue_pop();                                         \
        return gj_True;                                                 \
    }

#define PopParse(Name)                                                  \
    static inline int Name##_pop(JSONParseData* json_parse_data)        \
    {                                                                   \
        if (Name(json_parse_data))                                      \
        {                                                               \
            PadDepth();                                                 \
            depth--;                                                    \
            Print(#Name "_pop\n");                                      \
            json_parse_queue_pop();                                     \
            return gj_True;                                             \
        }                                                               \
        return gj_False;                                                \
    }

#if 0
#define ParseOrPushKey()                                        \
    if (gjson_skip_whitespace_push(json_parse_data))            \
    {                                                           \
        if (gjson_parse_string_push(json_parse_data))           \
        {                                                       \
            if (!gjson_skip_whitespace_push(json_parse_data))   \
            {                                                   \
                return gj_False;                                \
            }                                                   \
        }                                                       \
        else                                                    \
        {                                                       \
            return gj_False;                                    \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        return gj_False;                                        \
    }


#define ParseOrPushValue()                                      \
    if (gjson_skip_whitespace_push(json_parse_data))            \
    {                                                           \
        if (gjson_parse_string_push(json_parse_data))           \
        {                                                       \
            if (!gjson_skip_whitespace_push(json_parse_data))   \
            {                                                   \
                return gj_False;                                \
            }                                                   \
        }                                                       \
        else                                                    \
        {                                                       \
            return gj_False;                                    \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        return gj_False;                                        \
    }
#endif

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

static inline int gjson_skip_whitespace(JSONParseData* json_parse_data)
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

static int gjson_parse_object(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    while (gj_True)
    {
        if (current->state == JSONObjectState_Start)
        {
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_START);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONObjectState_End;
            if (!gjson_skip_whitespace_push(json_parse_data))
            {
                return gj_False;
            }
        }

        if (current->state == JSONObjectState_End)
        {
            ReturnIfOutOfBytes();
            
            if (gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END)
            {
                gjson_feed_current_char(json_parse_data);
                json_parse_queue_pop();
                return gj_True;
            }
            else
            {
                current->state = JSONObjectState_Key;
            }
        }
        
        if (current->state == JSONObjectState_Key)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_Colon;
            int result = gjson_skip_whitespace_push(json_parse_data);
            result &= gjson_parse_string_push(json_parse_data);
            result &= gjson_skip_whitespace_push(json_parse_data);
            if (!result) return gj_False;
        }
        
        if (current->state == JSONObjectState_Colon)
        {
            ReturnIfOutOfBytes();
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_MEMBER_COLON);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONObjectState_Value;
        }

        if (current->state == JSONObjectState_Value)
        {
            ReturnIfOutOfBytes();
            current->state = JSONObjectState_CheckNext;
            int result = gjson_skip_whitespace_push(json_parse_data);
            result &= gjson_parse_value(json_parse_data);
            result &= gjson_skip_whitespace_push(json_parse_data);
            if (!result) return gj_False;
        }

        if (current->state == JSONObjectState_CheckNext)
        {
            ReturnIfOutOfBytes();
        
            if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
            {
                gjson_feed_current_char(json_parse_data);
                current->state = JSONObjectState_Key;
                if (!gjson_skip_whitespace_push(json_parse_data))
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

static int gjson_parse_array(JSONParseData* json_parse_data)
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
                return gj_True;
            }
            else
            {
                current->state = JSONArrayState_Value;
            }
        }

        if (current->state == JSONArrayState_Value)
        {
            ReturnIfOutOfBytes();
            current->state = JSONArrayState_CheckNext;
            int result = gjson_skip_whitespace_push(json_parse_data);
            result &= gjson_parse_value(json_parse_data);
            result &= gjson_skip_whitespace_push(json_parse_data);
            if (!result) return gj_False;
        }

        if (current->state == JSONArrayState_CheckNext)
        {
            ReturnIfOutOfBytes();
            if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
            {
                gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR);
                gjson_feed_current_char(json_parse_data);
                current->state = JSONArrayState_Value;
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

static int gjson_parse_string(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    
    if (current->state == JSONStringState_Start)
    {
        gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_STRING);
        gjson_feed_current_char(json_parse_data);
        current->state = JSONStringState_Char;
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
                    return gj_True;
                }
                else if (current_char == '\\')
                {
                    current->state = JSONStringState_Backslash;
                    break;
                }
        
            }
        }

        if (current->state == JSONStringState_Backslash)
        {
            ReturnIfOutOfBytes();
            
            char current_char = gjson_feed_current_char(json_parse_data);
            current->state = JSONStringState_Char;
        }
    }
}
PushParse(gjson_parse_string, JSONStateType_String)
PopParse(gjson_parse_string)

static int gjson_parse_number(JSONParseData* json_parse_data)
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

static int gjson_parse_value(JSONParseData* json_parse_data)
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
                char* current_cursor = gjson_get_current_cursor(json_parse_data);
                if (gj_strings_equal(current_cursor, GJSON_TRUE, gj_string_length(GJSON_TRUE)))
                {
                    // TODO
                    gj_Assert(json_parse_data->cursor + 4 < json_parse_data->size);
                    json_parse_data->cursor += 4;
                }
                else if (gj_strings_equal(current_cursor, GJSON_FALSE, gj_string_length(GJSON_FALSE)))
                {
                    // TODO
                    gj_Assert(json_parse_data->cursor + 5 < json_parse_data->size);
                    json_parse_data->cursor += 5;
                }
                else if (gj_strings_equal(current_cursor, GJSON_NULL, gj_string_length(GJSON_NULL)))
                {
                    // TODO
                    gj_Assert(json_parse_data->cursor + 4 < json_parse_data->size);
                    json_parse_data->cursor += 4;
                }
            }
        } break;
    }

    return gj_True;
}
/* PushParse(gjson_parse_value, JSONStateType_Value) */
/* PopParse(gjson_parse_value) */

//////////////////////////////////////////////////////////////////////
// API Implementation
//////////////////////////////////////////////////////////////////////
static size_t gj_parse_json(JSON* json, void* json_data, size_t size)
{
    Print("gj_parse_json\n");
    
    size_t result = 0;

    JSONParseData json_parse_data;
    json_parse_data.data   = (char*)json_data;
    json_parse_data.size   = size;
    json_parse_data.cursor = 0;

    if (json_parse_queue.count == 0)
    {
        gjson_skip_whitespace_push(&json_parse_data);
        gjson_parse_value(&json_parse_data);
        gjson_skip_whitespace_push(&json_parse_data);
    }

    while (!gjson_out_of_bytes(&json_parse_data) && json_parse_queue.count > 0)
    {
        JSONParseState* current = json_parse_queue_current();
        switch (current->type)
        {
            case JSONStateType_SkipWhitespace: gjson_skip_whitespace_pop(&json_parse_data); break;
            case JSONStateType_Value:          gjson_parse_value(&json_parse_data);         break;
            case JSONStateType_Object:         gjson_parse_object_pop(&json_parse_data);    break;
            case JSONStateType_Array:          gjson_parse_array_pop(&json_parse_data);     break;
            case JSONStateType_String:         gjson_parse_string_pop(&json_parse_data);    break;
            case JSONStateType_Number:         gjson_parse_number_pop(&json_parse_data);    break;
                
            default: gj_Assert(gj_False); break;
        }
    }

    result = json_parse_data.cursor;
    return result;
}

#endif
