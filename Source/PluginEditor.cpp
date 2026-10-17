#include "PluginEditor.h"
#include <BinaryData.h>

#if JUCE_MAC
 #include "MacFileAccess.h"
#endif

using namespace juce;

namespace
{
    const String cmdPrefix = "http://vcam.local/cmd?";

    // Calcule la taille de fenetre pour un ratio donne (le grand cote = longSide)
    void sizeForAspect (double ratio, int longSide, int& w, int& h)
    {
        if (ratio >= 1.0)
        {
            w = longSide;
            h = roundToInt ((double) longSide / ratio);
        }
        else
        {
            h = longSide;
            w = roundToInt ((double) longSide * ratio);
        }
    }
}

//==============================================================================
PlayerOniriqueEditor::PlayerOniriqueEditor (PlayerOniriqueProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    // 1) Ecrire la page du lecteur dans un fichier temporaire.
    //    Une page en file:// peut lire des videos locales (file://) sans restriction.
    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("PlayerOnirique");
    dir.createDirectory();

    pageFile = dir.getChildFile ("player_" + Uuid().toString() + ".html");
    pageFile.replaceWithData (BinaryData::player_html, (size_t) BinaryData::player_htmlSize);

    // Polices et logo de la charte, lus par la page a cote d'elle (fonts/*.ttf, logo.png)
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const String name (BinaryData::originalFilenames[i]);
        int size = 0;
        auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        auto f = name.endsWith (".ttf") ? dir.getChildFile ("fonts").getChildFile (name) : dir.getChildFile (name);

        if (data != nullptr && ! name.endsWith (".html") && f.getSize() != (int64) size)
        {
            f.getParentDirectory().createDirectory();
            f.replaceWithData (data, (size_t) size);
        }
    }

    // 2) Creer la WebView (WebView2 sous Windows)
    using Opts = WebBrowserComponent::Options;
    Opts options;

   #if JUCE_WINDOWS
    options = options
                  .withBackend (Opts::Backend::webview2)
                  .withWinWebView2Options (Opts::WinWebView2{}
                                               .withUserDataFolder (dir.getChildFile ("WebView2Data"))
                                               .withBackgroundColour (Colour (0xff0d0809)));
   #endif

    browser = std::make_unique<PlayerBrowser> (options);
    browser->onNavigate = [this] (const String& url) { return handleNavigation (url); };
    browser->onLoaded   = [this] (const String& url)
    {
        if (url.containsIgnoreCase (pageFile.getFileName()) || url.containsIgnoreCase ("player_"))
        {
            pageReady = true;
            pushInit();
        }
    };

    addAndMakeVisible (*browser);

   #if JUCE_MAC
    if (! loadPageWithFullFileAccess (*browser, pageFile))
   #endif
        browser->goToURL (URL (pageFile).toString (true));

    // 3) Fenetre redimensionnable. Le ratio suit celui de la video (4:3, 16:9, 9:16...).
    //    Pas de poignee JUCE : c'est FL Studio qui redimensionne la fenetre.
    const auto s = proc.getSettings();

    constrainer.setSizeLimits (200, 200, 3000, 3000);
    constrainer.setFixedAspectRatio (s.aspect > 0.0 ? s.aspect : 4.0 / 3.0);
    setConstrainer (&constrainer);
    setResizable (true, false);

    int w = s.width, h = s.height;
    const double ratio = s.aspect > 0.0 ? s.aspect : 4.0 / 3.0;

    if (w < 200 || h < 200 || std::abs ((double) w / (double) h - ratio) > 0.02)
        sizeForAspect (ratio, 640, w, h);

    setSize (w, h);
    sizeRestored = true;

    proc.addChangeListener (this);
    startTimer (30);   // ~33 Hz : envoi de la position du DAW a la page (play / stop suivis sans delai visible)
}

PlayerOniriqueEditor::~PlayerOniriqueEditor()
{
    stopTimer();
    proc.removeChangeListener (this);
    browser.reset();
    pageFile.deleteFile();
}

void PlayerOniriqueEditor::resized()
{
    if (browser != nullptr)
        browser->setBounds (getLocalBounds());

    // Memorise la taille choisie (elle est rappelee a la reouverture du projet)
    if (sizeRestored)
    {
        auto s = proc.getSettings();
        s.width  = getWidth();
        s.height = getHeight();
        proc.setSettings (s);
    }
}

//==============================================================================
void PlayerOniriqueEditor::applyAspect (double ratio)
{
    if (ratio < 0.2 || ratio > 5.0)
        return;

    constrainer.setFixedAspectRatio (ratio);

    auto s = proc.getSettings();
    s.aspect = ratio;
    proc.setSettings (s);

    // Deja au bon format (ex. reouverture d'un projet) : on garde la taille de l'utilisateur
    if (getHeight() > 0 && std::abs ((double) getWidth() / (double) getHeight() - ratio) < 0.02)
        return;

    int w = 0, h = 0;
    sizeForAspect (ratio, jlimit (480, 1400, jmax (getWidth(), getHeight())), w, h);
    setSize (w, h);
}

//==============================================================================
void PlayerOniriqueEditor::runJS (const String& script)
{
    if (browser != nullptr && pageReady)
        browser->evaluateJavascript (script);
}

void PlayerOniriqueEditor::pushInit()
{
    if (! pageReady)
        return;

    const auto s = proc.getSettings();

    String src;
    if (s.path.isNotEmpty() && File (s.path).existsAsFile())
        src = URL (File (s.path)).toString (true);

    auto* o = new DynamicObject();
    o->setProperty ("sync",  s.sync);
    o->setProperty ("muted", s.muted);
    o->setProperty ("src",   src);

    runJS ("vcamInit(" + JSON::toString (var (o), true) + ");");
}

void PlayerOniriqueEditor::changeListenerCallback (ChangeBroadcaster*)
{
    pushInit();   // l'etat du projet vient d'etre recharge
}

//==============================================================================
//  Envoi de la position de lecture de FL Studio vers la page
//==============================================================================
void PlayerOniriqueEditor::timerCallback()
{
    if (! pageReady || ! proc.getSettings().sync)
        return;

    // Si FL n'appelle plus le plugin (arret / smart disable), on considere que ca ne joue pas
    const bool fresh   = (Time::getMillisecondCounter() - proc.lastProcessTick.load()) < 300;
    const bool playing = fresh && proc.hostPlaying.load();

    runJS ("vcamHost(" + String (playing ? "true" : "false") + ","
           + String (proc.hostTime.load(), 3) + ");");
}

//==============================================================================
//  Commandes venant de la page
//==============================================================================
bool PlayerOniriqueEditor::handleNavigation (const String& url)
{
    SafePointer<PlayerOniriqueEditor> safe (this);

    // Commande de l'interface (ouvrir, sync, son, format de la video)
    if (url.startsWithIgnoreCase (cmdPrefix))
    {
        const auto query = url.substring (cmdPrefix.length());
        MessageManager::callAsync ([safe, query]
        {
            if (safe != nullptr) safe->handleCommand (query);
        });
        return false;   // on annule la navigation
    }

    // Fichier .mp4 / .mov depose sur la fenetre (si FL laisse passer le glisser-deposer)
    if (url.startsWithIgnoreCase ("file:"))
    {
        const auto f = URL (url).getLocalFile();

        if (f != pageFile && f.hasFileExtension ("mp4;mov"))
        {
            MessageManager::callAsync ([safe, f]
            {
                if (safe != nullptr) safe->setVideoFile (f);
            });
            return false;
        }
    }

    return true;
}

void PlayerOniriqueEditor::handleCommand (const String& query)
{
    String name, value;

    for (auto& part : StringArray::fromTokens (query, "&", ""))
    {
        if (part.startsWith ("name="))
            name = part.fromFirstOccurrenceOf ("=", false, false);
        else if (part.startsWith ("v="))
            value = part.fromFirstOccurrenceOf ("=", false, false);
    }

    if (name == "open")
    {
        chooseVideo();
        return;
    }

    if (name == "aspect")
    {
        applyAspect (value.getDoubleValue());
        return;
    }

    auto s = proc.getSettings();

    if      (name == "sync") s.sync  = (value == "1");
    else if (name == "mute") s.muted = (value == "1");
    else return;

    proc.setSettings (s);
}

void PlayerOniriqueEditor::chooseVideo()
{
    auto start = File (proc.getSettings().path).getParentDirectory();
    if (! start.isDirectory())
        start = File::getSpecialLocation (File::userMoviesDirectory);

    chooser = std::make_unique<FileChooser> ("Choisir une video (mp4 / mov)", start, "*.mp4;*.mov");

    SafePointer<PlayerOniriqueEditor> safe (this);
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [safe] (const FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;

                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  safe->setVideoFile (f);
                          });
}

void PlayerOniriqueEditor::setVideoFile (const File& f)
{
    auto s = proc.getSettings();
    s.path = f.getFullPathName();
    proc.setSettings (s);
    pushInit();
}
