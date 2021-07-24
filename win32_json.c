#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <time.h>
#include <windows.h>

#include <gj/gj_base.h>
#include <gj/gj_math.h>

#include "json.h"
#include "json_print.h"

#include <gj/win32_platform.h>

int main(int argc, char** args)
{
    int result;

    win32_init_platform_api();    
        
    // NOTE: Read JSON filename
    PlatformFileHandle json_file_handle;
    char json_file_name[] = "test\\large-file.json";
    json_file_handle = g_platform_api.get_file_handle(json_file_name, PlatformOpenFileModeFlags_Read);

    // NOTE: Read buffer size
    // TODO: Might be bigger than file
    size_t json_data_buffer_size = gj_Min(1024, json_file_handle.file_size);
        
    printf("Reading [%s] (%lf Mb) with buffer size [%ld]...\n",
           json_file_name,
           gj_BytesToMegabytes(json_file_handle.file_size),
           json_data_buffer_size);

    size_t json_data_read_bytes  = 0;
    void* json_data = g_platform_api.allocate_memory(json_data_buffer_size);
    g_platform_api.read_data_from_file_handle(json_file_handle, json_data_read_bytes, json_data_buffer_size, json_data);
            
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    size_t working_memory_size = Gigabytes(1);
    void*  working_memory      = g_platform_api.allocate_memory(working_memory_size);
    GJSON_State json = gjson_init(working_memory, working_memory_size);
    GJSON_Query query_object_key;
    query_object.type = GJSON_QueryType_ObjectKey;
    char query_key_string[] = "login";
    {
        query_object_key.string_length = 5;
        query_object_key.string        = query_key_string;
    }
    
    int hits = 0;
    while (json_data_read_bytes < json_file_handle.file_size)
    {
        if (json_data_read_bytes + json_data_buffer_size > json_file_handle.file_size)
        {
            json_data_buffer_size = json_file_handle.file_size - json_data_read_bytes;
        }
        g_platform_api.read_data_from_file_handle(json_file_handle, json_data_read_bytes, json_data_buffer_size, json_data);   
        json.data = json_data;
        json.size = json_data_buffer_size;
            
        GJSON_QueryResult result = gjson_search(&json, query_object);
        gj_Assert(result.read_bytes != 0);
        switch (result.type)
        {
            case GJSON_QueryResultType_Hit:
                hits++;
            case GJSON_QueryResultType_NeedMoreBytes:
                json_data_read_bytes += result.read_bytes;
                break;

            InvalidDefaultCase;
        }
    }
    gj_Assert(json_data_read_bytes == json_file_handle.file_size);
    printf("Hits: %d\n", hits);
    
    {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        LARGE_INTEGER perf_count_frequency; QueryPerformanceFrequency(&perf_count_frequency);
        f32 total = ((f32)(end.QuadPart - start.QuadPart) /
                     (f32)perf_count_frequency.QuadPart);
        char total_message[BUFFER_SIZE];
        stbsp_sprintf(total_message, "Total time: %lf (%f mb/s)\n", total, gj_BytesToMegabytes(json_file_handle.file_size) / total);
        printf(total_message);
    }

    g_platform_api.deallocate_memory(working_memory);
    g_platform_api.close_file_handle(json_file_handle);
    g_platform_api.deallocate_memory(json_data);

    {
        char read_file_message[] = "Done!\n";
        printf(read_file_message);
    }

    return 0;
}
