#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

/** Mac : charge la page du lecteur en autorisant la lecture des videos partout sur le disque.
    Renvoie false si la vue web n'a pas ete trouvee (l'appelant charge alors la page normalement). */
bool loadPageWithFullFileAccess (juce::Component& browser, const juce::File& page);
