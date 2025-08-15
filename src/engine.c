#include "leo/engine.h"

int leo_sum(int a, int b)
{
    return a + b;
}

LEO_API leo_EngineState * leo_NewEngineState()
{
    // TODO: implement sane defaults
    return NULL;
}

LEO_API void leo_FreeEngineState(leo_EngineState *state)
{
    // TODO: properly cleanup
    return;
}
