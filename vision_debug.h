/* 
 * File:   vision_debug.h
 * Author: akrapivniy
 *
 * Created on 22 мая 2015 г., 15:03
 */

#ifndef VISION_DEBUG_H
#define	VISION_DEBUG_H

#if 1

#define MODULE_NAME "vision"

#define MSG(fmt,args...)	do {printf (MODULE_NAME":"fmt"\n", ##args); fflush(stdout);} while (0)
#define ERROR(fmt,args...)	do {printf (MODULE_NAME" error:%s:"fmt"\n",__func__, ##args); fflush(stdout);} while (0)
#define DEBUG(fmt,args...)	do {printf (MODULE_NAME":%s:%d:"fmt"\n",__func__, __LINE__, ##args); fflush(stdout);} while (0)
#define INITPOINT               int debug_point=0;
#define POINT(name)             do {printf ("[%s:%d]",name,debug_point++);fflush(stdout);} while (0)
#define DUMP_VAL(name)          DEBUG (#name"[%08x] = 0x%08x",&name,name)
#define DUMP_RES32(name)        DEBUG (#name" = 0x%08x",name)
#define DUMP_RES8(name)         DEBUG (#name" = 0x%02x",(uint32_t)name)

#define debug_info		MSG
#define debug_init		DEBUG
#define debug_process		DEBUG
#define debug_interrupt		DEBUG
#define fatal_error		ERROR
#define bad_error		ERROR

#else 
#define debug_info(fmt,args...)
#define debug_init(fmt,args...)
#define debug_process(fmt,args...)
#define debug_interrupt(fmt,args...)
#define fatal_error(fmt,args...)
#define bad_error(fmt,args...)
#endif


#endif	/* VISION_DEBUG_H */

