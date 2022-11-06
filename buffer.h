#ifndef _BUFFER_H
#define _BUFFER_H

#ifndef WIN32
typedef unsigned char BYTE;
typedef BYTE * PBYTE;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef PBYTE LPBYTE;
#endif
//缓冲区类 。。
typedef struct _buffer_context  //不能手动编辑里面的内容
{
	PBYTE	m_pBase;
	PBYTE	m_pPtr;
	UINT	m_nSize;
}buffer_context;

void   buffer_init(buffer_context *); //buffer 初始化
void   buffer_clean(buffer_context *);
ULONG  buffer_get_length(buffer_context *);
ULONG  buffer_write(buffer_context *,LPBYTE ,ULONG);
/*
read from buffer retuen num of bytes read
*/
ULONG  buffer_read(buffer_context *,LPBYTE,ULONG);
ULONG  buffer_get_memsize(buffer_context *);
void   buffer_free(buffer_context *);
LPBYTE buffer_getat(buffer_context *,ULONG );
void   buffer_exch(buffer_context *buff1,buffer_context *buff2);

#endif
