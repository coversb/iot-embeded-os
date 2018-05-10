/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          ftp.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   FTP client
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-05-09     1.00                    Chen Hao
*
******************************************************************************/
#ifndef __FTP_H__
#define __FTP_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define FTP_CONNECT_OK "220"
#define FTP_USERNAME_OK "331"
#define FTP_LOGIN_OK "230"
#define FTP_REQ_DIR_OK "250"
#define FTP_TYPE_I_OK "200"
#define FTP_PASV_OK "227"
#define FTP_OPENING_FILE_OK "150"
#define FTP_GET_FILE_OK "226"
#define FTP_RSET_OK "350"
#define FTP_REST_CMD "REST 0\r\n"
#define FTP_PASV_CMD "PASV\r\n"

/******************************************************************************
* Enum
******************************************************************************/

/******************************************************************************
* Type
******************************************************************************/
typedef struct
{
    bool (*connect)(const char* server, const uint16 port, const char *user, const char *password);
    bool (*disconnect)(void);
    bool (*setPath)(const char *path);
    bool (*setMode)(void);
    uint32 (*getFileSize)(const char *fname, const uint32 timeout);
    uint32 (*getFileData)(uint8 *pdata, const uint32 maxSize);
}FTP_CLIENT;

#endif	/* __FTP_H__ */
