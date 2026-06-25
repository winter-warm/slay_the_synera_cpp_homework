#include "interactcomponent.h"

bool InteractComponent::tryInteract()
{
    if (!canInteract()) {
        return false;
    }
    used = true;
    return true;
}

void InteractComponent::reset()
{
    used = false;
}
