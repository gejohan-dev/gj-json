#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE
#include <time.h>
#include <windows.h>
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")

static HANDLE stdout_handle;
void printf(const char* s)
{
    int length = 0; const char* _s = s; while (*_s++) { length++; }
    DWORD bytes_written;
    WriteConsoleA(stdout_handle, s, length, &bytes_written, NULL);
}

#if GJ_DEBUG
// Note: error text in dw, usually not very helpful
#define Assert(Exp)                                                     \
    do {                                                                \
        if (!(Exp)) {                                                   \
            char error_message[BUFFER_SIZE];                            \
            int error_message_length = stbsp_sprintf(error_message, "Error in %s:%d\n", __FILE__ , __LINE__); \
            printf(error_message);                                      \
            LPVOID lpMsgBuf;                                            \
            LPVOID lpDisplayBuf;                                        \
            DWORD dw = GetLastError();                                  \
                                                                        \
            int format_message_length = FormatMessage(                  \
                FORMAT_MESSAGE_ALLOCATE_BUFFER |                        \
                FORMAT_MESSAGE_FROM_SYSTEM |                            \
                FORMAT_MESSAGE_IGNORE_INSERTS,                          \
                NULL,                                                   \
                dw,                                                     \
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),              \
                (LPTSTR) &lpMsgBuf,                                     \
                0, NULL );                                              \
            printf((const char*)lpMsgBuf);                              \
            __debugbreak();                                             \
        }                                                               \
    } while (0)
#else
#define Assert(Exp)
#endif

#define GJ_BASE_MEMSET
#include <gj/gj_base.h>
#if !defined(__cplusplus)
#pragma function(sqrtf)
float sqrtf(float x) { return x; }
#pragma function(tanf)
float tanf(float x) { return x; }
#endif
#include <gj/gj_math.h>

#include "json.h"
#include "json_print.h"

#include <gj/win32_platform.h>

#if defined(__cplusplus)
int main()
#else
int mainCRTStartup()
#endif
{
    int result;
    
    char* command_line = GetCommandLineA();
    // Note: Skip executable
    char buffer[BUFFER_SIZE];
    int executable_name_length = gj_parse_word(command_line, buffer, BUFFER_SIZE);
    command_line += executable_name_length;
    if (*command_line++ == ' ')
    {
        while (*command_line == ' ') { *command_line++; }
        
        int json_file_name_length = gj_parse_word(command_line, buffer, BUFFER_SIZE);
        command_line += json_file_name_length;
        // TODO: Handle
        gj_Assert(json_file_name_length < BUFFER_SIZE);
        char json_file_name[BUFFER_SIZE];
        memcpy(json_file_name, buffer, BUFFER_SIZE);
        json_file_name[json_file_name_length] = '\0';

        win32_init_platform_api();    

        PlatformFileHandle json_file_handle =
            g_platform_api.get_file_handle(json_file_name, PlatformOpenFileModeFlags_Read);

        // TODO: Might be bigger than file
        size_t json_data_buffer_size = gj_Min(1024 * 1024 * 64, json_file_handle.file_size);
        if (*command_line == ' ')
        {
            while (*command_line == ' ') { *command_line++; }
            GJParseNumber buffer_size = gj_parse_number(command_line);
            if (buffer_size.ok) json_data_buffer_size = gj_Min(buffer_size.number, json_file_handle.file_size);
        }
        
        stdout_handle = win32_get_stdout_handle();
        
        {
            stbsp_sprintf(buffer, "Reading [%s] with buffer size [%ld]...\n", json_file_name, json_data_buffer_size);
            printf(buffer);
        }

        size_t json_data_read_bytes  = 0;
        void* json_data = g_platform_api.allocate_memory(json_file_handle.file_size);
        g_platform_api.read_data_from_file_handle(json_file_handle, json_data_read_bytes, json_data_buffer_size, json_data);
    
        {
            stbsp_sprintf(buffer, "Size %lf...\n", gj_BytesToMegabytes(json_file_handle.file_size));
            printf(buffer);
        }
        
        LARGE_INTEGER start;
        QueryPerformanceCounter(&start);

        size_t working_memory_size = Gigabytes(1);
        void*  working_memory      = g_platform_api.allocate_memory(working_memory_size);
        JSON json = gj_init_json(working_memory, working_memory_size);
        while (json_data_read_bytes < json_file_handle.file_size)
        {
            if (json_data_read_bytes + json_data_buffer_size > json_file_handle.file_size)
            {
                json_data_buffer_size = json_file_handle.file_size - json_data_read_bytes;
            }
            g_platform_api.read_data_from_file_handle(json_file_handle, json_data_read_bytes, json_data_buffer_size, json_data);   
            size_t read_bytes = gj_parse_json(&json, json_data, json_data_buffer_size);
            json_data_read_bytes += read_bytes;
        }
        gj_Assert(json_data_read_bytes == json_file_handle.file_size);

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

#if GJ_DEBUG
        stbsp_sprintf(buffer, "object_total      = %lf (%lld)\n", gj_BytesToMegabytes(object_total), sizeof(JSONObject));
        printf(buffer);
        stbsp_sprintf(buffer, "array_total       = %lf (%lld)\n", gj_BytesToMegabytes(array_total), sizeof(JSONArray));
        printf(buffer);
        stbsp_sprintf(buffer, "string_total      = %lf (%lld)\n", gj_BytesToMegabytes(string_total), sizeof(JSONString));
        printf(buffer);
        stbsp_sprintf(buffer, "string_copy_total = %lf\n", gj_BytesToMegabytes(string_copy_total));
        printf(buffer);
#endif
        gjson_find(json, "login", "clue");

        g_platform_api.close_file_handle(json_file_handle);
        g_platform_api.deallocate_memory(json_data);

        {
            char read_file_message[] = "Done!\n";
            printf(read_file_message);
        }
    
        result = 0;
    }
    else
    {
        // TODO: Handle
        result = -1;
    }

    return result;
}

// CRT stuff

int _fltused = 0x9875;

#pragma function(memset)
void* memset(void* _dst, int value, size_t size)
{
    unsigned char* dst = (unsigned char*)_dst;
    for (; size > 0; size--)
    {
        *dst++ = value;
    }
    return _dst;
}

#if !defined(__cplusplus)
#pragma function(memcpy)
void *memcpy(void* _dst, void const* _src, size_t size)
{
    unsigned char* dst       = (unsigned char*)_dst;
    const unsigned char* src = (const unsigned char*)_src;
    for (; size > 0; size--)
    {
        *dst++ = *src++;
    }
    return _dst;
}

/* #pragma function(sqrt) */
#endif
