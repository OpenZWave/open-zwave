/* This file is includes the pre-processed ValueIDIndexesDefines.h to avoid problems
 * with MSVC not supporting enough arguments with Macro's.
 * If you are adding a ValueID, you should add its index ENUM to ValuIDIndexDefines.def and the run
 * 'make updateIndexDefines' to regenerate the the ValueIDIndexDefines.h file
 */

#include <cstring>
#include "Utils.h"
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <stdexcept>


#ifndef _CommandClassCommands_H
#define _CommandClassCommands_H

namespace OpenZWave
{
#ifdef _MSC_VER
#define strncpy(x, y, z) strncpy_s(x, sizeof(x), y, sizeof(x)-1)
#endif
/* GCC Bitches bout the strncpy in _names function, but its safe as we do null terminate it, GCC just doesn't see it */
#ifndef _MSC_VER
#pragma GCC diagnostic push
#if defined(__GNUC__) && (__GNUC__ >= 8)
#       pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
#endif
#include "CommandClassCommands_Defines.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
}

#endif