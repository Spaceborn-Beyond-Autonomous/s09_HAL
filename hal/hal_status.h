/*
 * ANSA HAL API v0.1
 * hal_status.h — common return/error code enum shared by every hal_*.h header.
 *
 * Rule (Stage 1 interface review checklist): this file, like every file in
 * ansa/hal/, must contain zero vendor-specific types, macros, or register
 * references. See docs/adr/ADR-001.md.
 */
#ifndef ANSA_HAL_STATUS_H
#define ANSA_HAL_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,             /* generic/unspecified failure               */
    HAL_BUSY = 2,               /* peripheral busy, retry later              */
    HAL_TIMEOUT = 3,            /* operation did not complete in time        */
    HAL_INVALID_PARAM = 4,      /* bad argument passed by caller             */
    HAL_NOT_INITIALIZED = 5,    /* hal_*_init() was not called first         */
    HAL_NOT_SUPPORTED = 6,      /* backend does not support this operation   */
    HAL_NOT_IMPLEMENTED = 7,    /* stub / null-backend placeholder response  */
    HAL_RESOURCE_UNAVAILABLE = 8 /* e.g. no free DMA channel, no free timer  */
} hal_status_t;

#ifdef __cplusplus
}
#endif

#endif /* ANSA_HAL_STATUS_H */
