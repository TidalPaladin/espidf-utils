#include "Helpers/helpers.h"

uint32_t ulMinStackSizeOffset(const uint32_t ulOffsetWords)
{
    return configMINIMAL_STACK_SIZE + ulOffsetWords;
}