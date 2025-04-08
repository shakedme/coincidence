//
// Created by Shaked Melman on 31/03/2025.
//

#ifndef COINCIDENCE_UTIL_H
#define COINCIDENCE_UTIL_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace Util {
    template<class ComponentClass>
    static ComponentClass *getChildComponentOfClass(juce::Component *parent) {
        for (int i = 0; i < parent->getNumChildComponents(); ++i) {
            auto *childComp = parent->getChildComponent(i);

            if (auto c = dynamic_cast<ComponentClass *> (childComp))
                return c;

            if (auto c = getChildComponentOfClass<ComponentClass>(childComp))
                return c;
        }

        return nullptr;
    }
}


#endif //COINCIDENCE_UTIL_H
