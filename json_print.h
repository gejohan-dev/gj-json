#if !defined(JSON_PRINT_H)
#define JSON_PRINT_H

#include "json.h"

#if 0
extern void printf(const char* s);

//////////////////////////////////////////////////////////////////////
// API
//////////////////////////////////////////////////////////////////////
void gjson_find(JSON json, const char* key, const char* value);

//////////////////////////////////////////////////////////////////////
// Internal functions
//////////////////////////////////////////////////////////////////////
void json_print_node(JSONNode* node)
{
    static int depth = 0;
    depth++;
    switch (node->type)
    {
        case JSONType_Object:
        {
            for (int i = 0; i < depth; i++) printf(" ");
            printf("==Object==\n");
            JSONObject* object = node->object;
            while (object)
            {
                for (int i = 0; i < depth; i++) printf(" ");
                printf("Key:\n");
                json_print_node(object->key);
                for (int i = 0; i < depth; i++) printf(" ");
                printf("Value:\n");
                json_print_node(object->value);
                object = object->next;
            }
        } break;
            
        case JSONType_Array:
        {
            for (int i = 0; i < depth; i++) printf(" ");
            printf("==Array==\n");
            JSONArray* array = node->array;
            while (array)
            {
                json_print_node(array->value);
                array = array->next;
            }
        } break;

        case JSONType_String:
        {
            for (int i = 0; i < depth; i++) printf(" ");
            printf(node->string->string); printf("\n");
        } break;
        case JSONType_Number: break;
        case JSONType_True:   break;
        case JSONType_False:  break;
    }
    depth--;
}

int json_find_node(JSONNode* node, const char* key, const char* value)
{
    switch (node->type)
    {
        case JSONType_Object:
        {
            JSONObject* object = node->object;
            while (object)
            {
                if (object->value->type == JSONType_String &&
                    gj_strings_equal_null_term(object->key->string->string,
                                               key) &&
                    gj_strings_equal_null_term(object->value->string->string,
                                               value))
                {
                    return gj_True;
                }
                if (json_find_node(object->value, key, value))
                {
                    json_print_node(node);
                }
                object = object->next;
            }
        } break;
            
        case JSONType_Array:
        {
            JSONArray* array = node->array;
            while (array)
            {
                if (json_find_node(array->value, key, value))
                {
                    json_print_node(node);
                }
                array = array->next;
            }
        } break;

        case JSONType_String: break;
        case JSONType_Number: break;
        case JSONType_True:   break;
        case JSONType_False:  break;
    }

    return gj_False;
}

//////////////////////////////////////////////////////////////////////
// API Implementation
//////////////////////////////////////////////////////////////////////
void gjson_find(JSON json, const char* key, const char* value)
{
    json_find_node(&json.root, key, value);
}
#endif

#endif
