#ifndef COMMON_TWENET_DEFS_H_
#define COMMON_TWENET_DEFS_H_

// Select Modules (define befor include "ToCoNet.h")
#ifdef MWLIB_MULTINONE
#define ToCoNet_USE_MOD_TXRXQUEUE_MULTINONE
#else
#define ToCoNet_USE_MOD_TXRXQUEUE_BIG
#endif
#define ToCoNet_USE_MOD_CHANNEL_MGR

#define ToCoNet_USE_MOD_CHANNEL_MGR

// includes
#include "ToCoNet.h"
#include "ToCoNet_event.h"
#include "ToCoNet_mod_prototype.h"

#endif /* COMMON_TWENET_DEFS_H_ */
