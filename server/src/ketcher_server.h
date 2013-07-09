#ifndef __ketcher_server_h___
#define __ketcher_server_h___

#ifndef CEXPORT
#ifdef _WIN32
#ifndef __cplusplus
#define CEXPORT __declspec(dllexport)
#else
#define CEXPORT extern "C" __declspec(dllexport)
#endif
#elif (defined __GNUC__ || defined __APPLE__)
#ifndef __cplusplus
#define CEXPORT __attribute__ ((visibility ("default")))
#else
#define CEXPORT extern "C" __attribute__ ((visibility ("default")))
#endif
#else
#ifndef __cplusplus
#define CEXPORT
#else
#define CEXPORT extern "C"
#endif
#endif
#endif

CEXPORT const char * ketcherServerRunCommand( const char *command_name, int fields_count,
                                              const char **fields, const char **values, 
                                              int *output_len,
                                              const char **content_params );

CEXPORT int ketcherServerGetCommandCount();
CEXPORT const char * ketcherServerGetCommandName( int id );

#endif // __ketcher_server_h___
