#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE
#include <time.h>
#include <windows.h>
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")

static HANDLE stdout_handle;
void printf(const char* s, const int length)
{
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
            printf(error_message, error_message_length);                \
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
            printf(lpMsgBuf, format_message_length);                    \
            __debugbreak();                                             \
        }                                                               \
    } while (0)
#else
#define Assert(Exp)
#endif

#define GJ_BASE_MEMSET
#include <gj/gj_base.h>

#include "json.h"

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

        size_t json_data_buffer_size = 1024 * 1024 * 64;
        if (*command_line == ' ')
        {
            while (*command_line == ' ') { *command_line++; }
            GJParseNumber buffer_size = gj_parse_number(command_line);
            if (buffer_size.ok) json_data_buffer_size = buffer_size.number;
        }
        
        win32_init_platform_api();    

        stdout_handle = win32_get_stdout_handle();
        
        {
            int read_file_message_length = stbsp_sprintf(buffer, "Reading [%s] with buffer size [%ld]...\n", json_file_name, json_data_buffer_size);
            printf(buffer, read_file_message_length);
        }

        PlatformFileHandle json_file_handle =
            g_platform_api.get_file_handle(json_file_name, PlatformOpenFileModeFlags_Read);
        size_t json_data_read_bytes  = 0;
        void* json_data = g_platform_api.allocate_memory(json_file_handle.file_size);
        g_platform_api.read_data_from_file_handle(json_file_handle, json_data_read_bytes, json_data_buffer_size, json_data);
    
        {
            int read_file_message_length = stbsp_sprintf(buffer, "Size %lld...\n", json_file_handle.file_size / (1024*1024));
            printf(buffer, read_file_message_length);
        }
        
        LARGE_INTEGER start;
        QueryPerformanceCounter(&start);
    
        JSON json;
        gj_ZeroMemory(&json);
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
            int total_message_length = stbsp_sprintf(total_message, "Total time: %f (%f mb/s)\n", total, (json_file_handle.file_size / (1024*1024)) / total);
            printf(total_message, total_message_length);
        }
        
        g_platform_api.close_file_handle(json_file_handle);
        g_platform_api.deallocate_memory(json_data);

        {
            char read_file_message[] = "Done!\n";
            printf(read_file_message, 6);
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
#endif
