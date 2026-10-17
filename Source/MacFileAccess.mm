#include "MacFileAccess.h"
#import <WebKit/WebKit.h>

// JUCE ouvre une page locale en n'autorisant la lecture que dans son dossier (le dossier temporaire) :
// une video rangee ailleurs serait refusee. On charge donc la page nous-memes, avec la lecture
// autorisee sur tout le disque.
bool loadPageWithFullFileAccess (juce::Component& browser, const juce::File& page)
{
    for (auto* child : browser.getChildren())
        if (auto* holder = dynamic_cast<juce::NSViewComponent*> (child))
            if (auto* view = (NSView*) holder->getView(); [view isKindOfClass: [WKWebView class]])
            {
                [(WKWebView*) view loadFileURL: [NSURL fileURLWithPath: [NSString stringWithUTF8String: page.getFullPathName().toRawUTF8()]]
                       allowingReadAccessToURL: [NSURL fileURLWithPath: @"/"]];
                return true;
            }

    return false;
}
