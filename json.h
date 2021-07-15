#if !defined(JSON_H)
#define JSON_H

#include <gj/gj_base.h>

extern void printf(const char* s, int length);

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
    JSONArrayState_Value     = 1,
    JSONArrayState_CheckNext = 2
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


static void gjson_parse_value (JSONParseData* json_parse_data);
static void gjson_parse_object(JSONParseData* json_parse_data);
static void gjson_parse_array (JSONParseData* json_parse_data);
static void gjson_parse_string(JSONParseData* json_parse_data);
static void gjson_parse_number(JSONParseData* json_parse_data);

static inline int gjson_out_of_bytes(JSONParseData* json_parse_data) { return json_parse_data->cursor >= json_parse_data->size; }
#define ReturnIfOutOfBytes()       if (gjson_out_of_bytes(json_parse_data)) return 0;

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
    ReturnValIfOutOfBytes(0);
    
    while (gj_IsWhitespace(gjson_peek_current_char(json_parse_data)))
    {
        gjson_feed_current_char(json_parse_data);
        if (gjson_out_of_bytes(json_parse_data))
        {
            return 0;
        }
    }
    json_parse_queue_pop();
    return 1;
}

static void gjson_parse_object(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    while (1)
    {
        if (current->state == JSONObjectState_Start)
        {
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_START);
            gjson_feed_current_char(json_parse_data);

            current->state = JSONObjectState_End;

            if (!gjson_skip_whitespace(json_parse_data))
            {
                json_parse_queue_push(JSONStateType_SkipWhitespace);
                return;
            }
        }

        if (current->state == JSONObjectState_End)
        {
            if (gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END)
            {
                gjson_feed_current_char(json_parse_data);
                json_parse_queue_pop();
                return;
            }
            else
            {
                current->state = JSONObjectState_Key;
            }
        }
        
        if (current->state == JSONObjectState_Key)
        {
            ReturnIfOutOfBytes();

#define KeyPush()                                                       \
            do {                                                        \
                json_parse_queue_push(JSONStateType_SkipWhitespace);    \
                json_parse_queue_push(JSONStateType_String);            \
                json_parse_queue_push(JSONStateType_SkipWhitespace);    \
            } while (0)
            
            if (gjson_skip_whitespace(json_parse_data))
            {
                if (gjson_parse_string(json_parse_data))
                {
                    if (!gjson_skip_whitespace(json_parse_data)) KeyPush();
                }
                else KeyPush();
            }
            else KeyPush();
#undef KeyPush            
            current->state = JSONObjectState_Colon;
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
            
#define ValuePush()                                                     \
            do {                                                        \
                json_parse_queue_push(JSONStateType_SkipWhitespace);    \
                json_parse_queue_push(JSONStateType_Value);             \
                json_parse_queue_push(JSONStateType_SkipWhitespace);    \
            } while (0)
            
            if (gjson_skip_whitespace(json_parse_data))
            {
                if (gjson_parse_value(json_parse_data))
                {
                    if (!gjson_skip_whitespace(json_parse_data)) ValuePush();
                }
                else ValuePush();
            }
            else ValuePush();
            
            current->state = JSONObjectState_Colon;
        }
#undef KeyPush
            current->state = JSONObjectState_CheckNext;
        }

        if (current->state == JSONObjectState_CheckNext)
        {
            ReturnIfOutOfBytes();
        
            if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
            {
                gjson_feed_current_char(json_parse_data);
                json_parse_queue_push(JSONStateType_SkipWhitespace);
                gjson_skip_whitespace(json_parse_data);
                current->state = JSONObjectState_Key;
            }
            else
            {
                gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END);
                gjson_feed_current_char(json_parse_data);
                json_parse_queue_pop();
                return;
            }
        }
    }
}

static void gjson_parse_array(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    while (1)
    {
        if (current->state == JSONArrayState_Start)
        {
            gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_START);
            gjson_feed_current_char(json_parse_data);
            current->state = JSONArrayState_Value;
        }

        if (current->state == JSONArrayState_Value)
        {
            ReturnIfOutOfBytes();
        
            if (gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_END)
            {
                gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_END);
                gjson_feed_current_char(json_parse_data);
                json_parse_queue_pop();
                return;
            }
            else
            {
                json_parse_queue_push(JSONStateType_SkipWhitespace);
                gjson_skip_whitespace(json_parse_data);
                json_parse_queue_push(JSONStateType_Value);
                gjson_parse_value(json_parse_data);
                json_parse_queue_push(JSONStateType_SkipWhitespace);
                gjson_skip_whitespace(json_parse_data);
                current->state = JSONArrayState_CheckNext;
            }
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
                json_parse_queue_pop();
                return;
            }
        }
    }
}

static void gjson_parse_string(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    if (current->state == JSONStringState_Start)
    {
        gj_Assert(gjson_peek_current_char(json_parse_data) == GJSON_STRING);
        gjson_feed_current_char(json_parse_data);
        current->state = JSONStringState_Char;
    }

    while (1)
    {
        /* ReturnIfOutOfBytes(); */
        if (gjson_out_of_bytes(json_parse_data))
        {
            return;
        }
        
        if (current->state == JSONStringState_Char)
        {        
            while (1)
            {
                /* ReturnIfOutOfBytes(); */
                if (gjson_out_of_bytes(json_parse_data))
                {
                    return;
                }
                
                char current_char = gjson_feed_current_char(json_parse_data);
                if (current_char == GJSON_STRING)
                {
                    json_parse_queue_pop();
                    return;
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

static void gjson_parse_number(JSONParseData* json_parse_data)
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
            gj_Assert(0);
            current->state = JSONNumberState_ExponentSign;
        }
        else
        {
            json_parse_queue_pop();
            return;
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
        gj_Assert(0);
    }    
}

static void gjson_parse_value(JSONParseData* json_parse_data)
{
    ReturnIfOutOfBytes();
    json_parse_queue_pop();
    
    char current_char = gjson_peek_current_char(json_parse_data);
    switch (current_char)
    {
        case GJSON_OBJECT_START:
        {
            json_parse_queue_push(JSONStateType_Object);
            gjson_parse_object(json_parse_data);
        } break;
        
        case GJSON_ARRAY_START:
        {
            json_parse_queue_push(JSONStateType_Array);
            gjson_parse_array(json_parse_data);
        } break;
        
        case GJSON_STRING:
        {
            json_parse_queue_push(JSONStateType_String);
            gjson_parse_string(json_parse_data);
        } break;
        
        default:
        {
            if (current_char == GJSON_SIGN_NEGATIVE || gj_IsDigit(current_char))
            {
                json_parse_queue_push(JSONStateType_Number);
                gjson_parse_number(json_parse_data);
            }
            else
            {
                gj_Assert(0);
#if 0
                char* current_cursor = gjson_get_current_cursor(json_parse_data);
                if (gj_strings_equal(current_cursor, GJSON_TRUE, gj_string_length(GJSON_TRUE)))
                {
                    // TODO
                    json_parse_queue_push(JSONStateType_MoveCursor);
                }
                else if (gj_strings_equal(current_cursor, GJSON_FALSE, gj_string_length(GJSON_FALSE)))
                {
                    // TODO
                    json_parse_queue_push(JSONStateType_MoveCursor);
                }
                else if (gj_strings_equal(current_cursor, GJSON_NULL, gj_string_length(GJSON_NULL)))
                {
                    // TODO
                    json_parse_queue_push(JSONStateType_MoveCursor);
                }
#endif
            }
        } break;
    }
}

//////////////////////////////////////////////////////////////////////
// API Implementation
//////////////////////////////////////////////////////////////////////
static size_t gj_parse_json(JSON* json, void* json_data, size_t size)
{
    size_t result = 0;

    JSONParseData json_parse_data;
    json_parse_data.data   = (char*)json_data;
    json_parse_data.size   = size;
    json_parse_data.cursor = 0;

    if (json_parse_queue.count == 0)
    {
        json_parse_queue_push(JSONStateType_SkipWhitespace);
        json_parse_queue_push(JSONStateType_Value);
        json_parse_queue_push(JSONStateType_SkipWhitespace);
    }

    while (!gjson_out_of_bytes(&json_parse_data) && json_parse_queue.count > 0)
    {
        JSONParseState* current = json_parse_queue_current();
        switch (current->type)
        {
            case JSONStateType_SkipWhitespace: gjson_skip_whitespace(&json_parse_data); break;
            case JSONStateType_Value:          gjson_parse_value(&json_parse_data);     break;
            case JSONStateType_Object:         gjson_parse_object(&json_parse_data);    break;
            case JSONStateType_Array:          gjson_parse_array(&json_parse_data);     break;
            case JSONStateType_String:         gjson_parse_string(&json_parse_data);    break;
            case JSONStateType_Number:         gjson_parse_number(&json_parse_data);    break;
                
            default: gj_Assert(0); break;
        }
    }

    result = json_parse_data.cursor;
    return result;
}

#endif
