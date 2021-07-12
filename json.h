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
    JSONObjectState_Key       = 1,
    JSONObjectState_Colon     = 2,
    JSONObjectState_Value     = 3,
    JSONObjectState_CheckNext = 4
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

typedef struct JSONParseState
{
    JSONStateType type;
    int n;
} JSONParseState;

#define JSON_PARSE_QUEUE_SIZE 100
typedef struct JSONParseQueue
{
    JSONParseState queue[JSON_PARSE_QUEUE_SIZE];
    int count;
} JSONParseQueue;

static JSONParseQueue json_parse_queue;

static void json_parse_queue_push(JSONStateType type, int n)
{
    Assert(json_parse_queue.count <= JSON_PARSE_QUEUE_SIZE);
    gj_ZeroMemory(&json_parse_queue.queue[json_parse_queue.count]);
    json_parse_queue.queue[json_parse_queue.count].type = type;
    json_parse_queue.queue[json_parse_queue.count].n    = n;
    json_parse_queue.count++;
}

static void json_parse_queue_pop()
{
    Assert(json_parse_queue.count > 0);
    json_parse_queue.count--;
}

static JSONParseState* json_parse_queue_current()
{
    Assert(json_parse_queue.count > 0);
    return &json_parse_queue.queue[json_parse_queue.count - 1];
}

//////////////////////////////////////////////////////////////////////
// JSONParseData
//////////////////////////////////////////////////////////////////////
typedef struct JSONParseData
{
    // TODO: Unicode
    char*  data;
    size_t size;
    size_t data_cursor;
    
    JSON json;
} JSONParseData;

static void gjson_parse_value (JSONParseData* json_parse_data);
static void gjson_parse_object(JSONParseData* json_parse_data);
static void gjson_parse_array (JSONParseData* json_parse_data);
static void gjson_parse_string(JSONParseData* json_parse_data);
static void gjson_parse_number(JSONParseData* json_parse_data);

static inline int gjson_out_of_bytes(JSONParseData* json_parse_data) { return json_parse_data->data_cursor >= json_parse_data->size; }

static inline char gjson_feed_current_char(JSONParseData* json_parse_data)
{
    Assert(json_parse_data->data_cursor < json_parse_data->size);
    return json_parse_data->data[json_parse_data->data_cursor++];
}

static inline char gjson_peek_current_char(JSONParseData* json_parse_data)
{
    Assert(json_parse_data->data_cursor < json_parse_data->size);
    return json_parse_data->data[json_parse_data->data_cursor];
}

static inline char* gjson_get_current_cursor(JSONParseData* json_parse_data)
{
    Assert(json_parse_data->data_cursor < json_parse_data->size);
    return &json_parse_data->data[json_parse_data->data_cursor];
}

static inline void gjson_move_cursor(JSONParseData* json_parse_data, size_t n)
{
    json_parse_queue_pop();
    if (json_parse_data->size - json_parse_data->data_cursor < n)
    {
        json_parse_data->data_cursor = json_parse_data->size;
        json_parse_queue_push(JSONStateType_MoveCursor,
                              n - (json_parse_data->size - json_parse_data->data_cursor));
    }
    else
    {
        json_parse_data->data_cursor += n;
    }
}

static inline void gjson_skip_whitespace(JSONParseData* json_parse_data)
{
    if (gj_IsWhitespace(gjson_peek_current_char(json_parse_data)))
    {
        json_parse_queue_push(JSONStateType_MoveCursor, 1);
    }
    else
    {
        json_parse_queue_pop();
    }
}

static void gjson_parse_object(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    if (current->n == JSONObjectState_Start)
    {
        Assert(gjson_feed_current_char(json_parse_data) == GJSON_OBJECT_START);
        current->n = JSONObjectState_Key;
    }
    else if (current->n == JSONObjectState_Key)
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_OBJECT_END)
        {
            gjson_move_cursor(json_parse_data, 1);
            json_parse_queue_pop();
            return;
        }

        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
        json_parse_queue_push(JSONStateType_String,         0);
        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
        current->n = JSONObjectState_Colon;
    }
    else if (current->n == JSONObjectState_Colon)
    {
        Assert(gjson_feed_current_char(json_parse_data) == GJSON_MEMBER_COLON);
        current->n = JSONObjectState_Value;
    }
    else if (current->n == JSONObjectState_Value)
    {
        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
        json_parse_queue_push(JSONStateType_Value,          0);
        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
        current->n = JSONObjectState_CheckNext;
    }
    else if (current->n == JSONObjectState_CheckNext)
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
        {
            Assert(gjson_feed_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR);
            json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
            current->n = JSONObjectState_Key;
        }
        else
        {
            Assert(gjson_feed_current_char(json_parse_data) == GJSON_OBJECT_END);
            json_parse_queue_pop();
        }
    }
}

static void gjson_parse_array(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    if (current->n == JSONArrayState_Start)
    {
        Assert(gjson_feed_current_char(json_parse_data) == GJSON_ARRAY_START);
        current->n = JSONArrayState_Value;
    }
    else if (current->n == JSONArrayState_Value)
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_ARRAY_END)
        {
            Assert(gjson_feed_current_char(json_parse_data) == GJSON_ARRAY_END);
            json_parse_queue_pop();
            return;
        }

        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
        json_parse_queue_push(JSONStateType_Value,          0);
        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
        current->n = JSONArrayState_CheckNext;
    }
    else if (current->n == JSONArrayState_CheckNext)
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR)
        {
            Assert(gjson_feed_current_char(json_parse_data) == GJSON_ELEMENT_SEPARATOR);
            current->n = JSONArrayState_Value;
        }
        else
        {
            Assert(gjson_feed_current_char(json_parse_data) == GJSON_ARRAY_END);
            json_parse_queue_pop();
        }
    }
}

static void gjson_parse_string(JSONParseData* json_parse_data)
{
    JSONParseState* current = json_parse_queue_current();
    if (current->n == JSONStringState_Start)
    {
        Assert(gjson_feed_current_char(json_parse_data) == GJSON_STRING);
        current->n = JSONStringState_Char;
    }
    else if (current->n == JSONStringState_Char)
    {
        char current_char = gjson_feed_current_char(json_parse_data);
        if (current_char == GJSON_STRING)
        {
            json_parse_queue_pop();
            return;
        }
        else if (current_char == '\\')
        {
            current->n = JSONStringState_Backslash;
        }
    }
    else if (current->n == JSONStringState_Backslash)
    {
        char current_char = gjson_feed_current_char(json_parse_data);
        current->n = JSONStringState_Char;
        /* if (current_char == '\\' || current_char == '"') */
        /* { */
        /* current->n = JSONStringState_Char; */
        /* } */
        /* else InvalidCodePath; */
    }
}

static void gjson_parse_number(JSONParseData* json_parse_data)
{
    // Note: Implicit integer
    {
        if (gjson_peek_current_char(json_parse_data) == GJSON_SIGN_NEGATIVE)
        {
            // TODO
            gjson_move_cursor(json_parse_data, 1);
        }
        int integer_length;
        int integer = gj_parse_number(gjson_get_current_cursor(json_parse_data), &integer_length);
        gjson_move_cursor(json_parse_data, integer_length);
    }
    
    // Note: Implicit fraction
    if (gjson_peek_current_char(json_parse_data) == GJSON_FRACTION)
    {
        gjson_move_cursor(json_parse_data, 1);
        int integer_length;
        int integer = gj_parse_number(gjson_get_current_cursor(json_parse_data), &integer_length);
        gjson_move_cursor(json_parse_data, integer_length);
    }

    // Note: Implicit exponent
    if (gjson_peek_current_char(json_parse_data) == GJSON_EXPONENT_E ||
        gjson_peek_current_char(json_parse_data) == GJSON_EXPONENT_e)
    {
        gjson_move_cursor(json_parse_data, 1);

        // Note: Implicit sign
        if (gjson_peek_current_char(json_parse_data) == GJSON_SIGN_POSITIVE ||
            gjson_peek_current_char(json_parse_data) == GJSON_SIGN_NEGATIVE)
        {
            // TODO
            gjson_move_cursor(json_parse_data, 1);            
        }

        // Note: Implicit digits
        int integer_length;
        int integer = gj_parse_number(gjson_get_current_cursor(json_parse_data), &integer_length);
        gjson_move_cursor(json_parse_data, integer_length);
    }    
}

static void gjson_parse_value(JSONParseData* json_parse_data)
{
    json_parse_queue_pop();
    char current_char = gjson_peek_current_char(json_parse_data);
    switch (current_char)
    {
        case GJSON_OBJECT_START: json_parse_queue_push(JSONStateType_Object, 0); break;
        case GJSON_ARRAY_START:  json_parse_queue_push(JSONStateType_Array,  0); break;
        case GJSON_STRING:       json_parse_queue_push(JSONStateType_String, 0); break;
        default:
        {
            if (current_char == GJSON_SIGN_NEGATIVE || gj_IsDigit(current_char))
            {
                json_parse_queue_push(JSONStateType_Number, 0);
            }
            else
            {
                char* current_cursor = gjson_get_current_cursor(json_parse_data);
                if (gj_strings_equal(current_cursor, GJSON_TRUE, gj_string_length(GJSON_TRUE)))
                {
                    // TODO
                    json_parse_queue_push(JSONStateType_MoveCursor, 4);
                }
                else if (gj_strings_equal(current_cursor, GJSON_FALSE, gj_string_length(GJSON_FALSE)))
                {
                    // TODO
                    json_parse_queue_push(JSONStateType_MoveCursor, 5);
                }
                else if (gj_strings_equal(current_cursor, GJSON_NULL, gj_string_length(GJSON_NULL)))
                {
                    // TODO
                    json_parse_queue_push(JSONStateType_MoveCursor, 4);
                }
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
    json_parse_data.data        = (char*)json_data;
    json_parse_data.size        = size;
    json_parse_data.data_cursor = 0;

    if (json_parse_queue.count == 0)
    {
        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
        json_parse_queue_push(JSONStateType_Value, 0);
        json_parse_queue_push(JSONStateType_SkipWhitespace, 0);
    }

    while (!gjson_out_of_bytes(&json_parse_data))
    {
        JSONParseState* current = json_parse_queue_current();
        switch (current->type)
        {
            case JSONStateType_SkipWhitespace: gjson_skip_whitespace(&json_parse_data);       break;
            case JSONStateType_Value:          gjson_parse_value(&json_parse_data);           break;
            case JSONStateType_MoveCursor:     gjson_move_cursor(&json_parse_data, current->n); break;
            case JSONStateType_Object:         gjson_parse_object(&json_parse_data);          break;
            case JSONStateType_Array:          gjson_parse_array(&json_parse_data);           break;
            case JSONStateType_String:         gjson_parse_string(&json_parse_data);          break;
            case JSONStateType_Number:         gjson_parse_number(&json_parse_data);          break;
                
            default: Assert(0); break;
        }
    }
    result = json_parse_data.data_cursor;
    return result;
}

#endif
