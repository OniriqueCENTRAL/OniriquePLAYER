// Outil de développement (pas dans le produit, Windows) : charge le VST3 comme un DAW, lui donne une vidéo et une
// position de lecture simulée (stop, lecture, saut du curseur pendant la lecture, stop), ouvre sa fenêtre et la
// photographie à l'écran aux moments clés. Le titre de la fenêtre affiche la position du « DAW » : avec une vidéo qui
// montre son propre chronomètre, les deux nombres doivent coïncider sur chaque image.
//   host_check <chemin du .vst3> <vidéo .mp4> <dossier des images>
#include <juce_audio_processors/juce_audio_processors.h>
#include <windows.h>
#include <cstdio>

struct Step { double at; bool playing; double pos; };   // à partir de « at » s : lecture ? position de départ
static const Step steps[] = { { 0, false, 5 }, { 3, true, 5 }, { 11, true, 25 }, { 16, false, 30 } };
static const double shots[] = { 2.5, 6, 9, 10.5, 12.5, 15, 17.5 };

struct SimHead : juce::AudioPlayHead
{
    double start = juce::Time::getMillisecondCounterHiRes() / 1000.0;

    void now (bool& playing, double& t) const
    {
        const double el = juce::Time::getMillisecondCounterHiRes() / 1000.0 - start;
        Step s = steps[0];
        for (auto& st : steps) if (el >= st.at) s = st;
        playing = s.playing;
        t = s.playing ? s.pos + (el - s.at) : s.pos;
    }

    juce::Optional<PositionInfo> getPosition() const override
    {
        bool playing; double t;
        now (playing, t);
        PositionInfo p;
        p.setIsPlaying (playing);
        p.setTimeInSeconds (t);
        p.setTimeInSamples ((juce::int64) (t * 48000.0));
        p.setBpm (120.0);
        return p;
    }
};

static void grab (HWND w, const juce::File& out)
{
    RECT r; GetWindowRect (w, &r);
    const int W = r.right - r.left, H = r.bottom - r.top;
    HDC screen = GetDC (nullptr), mem = CreateCompatibleDC (screen);
    HBITMAP bmp = CreateCompatibleBitmap (screen, W, H);
    SelectObject (mem, bmp);
    PrintWindow (w, mem, 2 /* PW_RENDERFULLCONTENT : contenu WebView2 compris, même sous une autre fenêtre */);
    std::vector<juce::uint32> px ((size_t) (W * H));
    BITMAPINFO bi {}; bi.bmiHeader = { sizeof (BITMAPINFOHEADER), W, -H, 1, 32, BI_RGB };   // hauteur < 0 : de haut en bas
    GetDIBits (mem, bmp, 0, (UINT) H, px.data(), &bi, DIB_RGB_COLORS);
    DeleteObject (bmp); DeleteDC (mem); ReleaseDC (nullptr, screen);
    juce::Image img (juce::Image::RGB, W, H, false);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) img.setPixelAt (x, y, juce::Colour (0xff000000 | px[(size_t) (y * W + x)]));
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

int main (int argc, char** argv)
{
    if (argc < 4) return 2;
    juce::ScopedJuceInitialiser_GUI init;
    juce::VST3PluginFormat vst3;
    juce::OwnedArray<juce::PluginDescription> found;
    vst3.findAllTypesForFile (found, argv[1]);
    if (found.isEmpty()) { std::puts ("plugin introuvable"); return 1; }
    juce::String err;
    auto plugin = vst3.createInstanceFromDescription (*found[0], 48000.0, 512, err);
    if (plugin == nullptr) { std::printf ("chargement : %s\n", err.toRawUTF8()); return 1; }

    juce::XmlElement xml ("PlayerOniriqueState");
    xml.setAttribute ("path", juce::File (juce::String::fromUTF8 (argv[2])).getFullPathName());
    xml.setAttribute ("sync", true);
    xml.setAttribute ("muted", true);
    // Format carré exprès : la fenêtre ne reprend celui de la vidéo que si la page a pu le dire au plugin
    // (vérifie le canal page -> plugin). Images attendues au format de la vidéo.
    xml.setAttribute ("aspect", 1.0);
    xml.setAttribute ("width", 480);
    xml.setAttribute ("height", 480);
    // Comme un DAW : l'état du plugin, en base64, dans l'enveloppe VST3 de l'hôte JUCE
    juce::MemoryBlock inner, state;
    juce::AudioProcessor::copyXmlToBinary (xml, inner);
    juce::XmlElement wrapper ("VST3PluginState");
    wrapper.createNewChildElement ("IComponent")->addTextElement (inner.toBase64Encoding());
    juce::AudioProcessor::copyXmlToBinary (wrapper, state);
    plugin->setStateInformation (state.getData(), (int) state.getSize());

    SimHead head;
    plugin->setPlayHead (&head);
    plugin->setPlayConfigDetails (2, 2, 48000.0, 512);
    plugin->prepareToPlay (48000.0, 512);

    std::atomic<bool> run { true };
    std::thread daw ([&]
    {
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        double next = juce::Time::getMillisecondCounterHiRes();
        while (run)
        {
            buf.clear();
            plugin->processBlock (buf, midi);
            next += 512.0 / 48.0;
            while (juce::Time::getMillisecondCounterHiRes() < next) std::this_thread::sleep_for (std::chrono::microseconds (500));
        }
    });

    juce::DocumentWindow win ("host", juce::Colours::black, 0, true);
    win.setUsingNativeTitleBar (true);
    win.setContentNonOwned (plugin->createEditorIfNeeded(), true);
    win.setTopLeftPosition (80, 80);
    win.setAlwaysOnTop (true);
    win.setVisible (true);

    head.start = juce::Time::getMillisecondCounterHiRes() / 1000.0 + 2.0;   // 2 s pour charger la page et la vidéo
    juce::MessageManager::getInstance()->runDispatchLoopUntil (2000);

    const juce::File dir (juce::String::fromUTF8 (argv[3]));
    dir.createDirectory();
    for (double shot : shots)
    {
        while (juce::Time::getMillisecondCounterHiRes() / 1000.0 - head.start < shot)
        {
            bool playing; double t;
            head.now (playing, t);
            win.setName ((playing ? "DAW PLAY " : "DAW STOP ") + juce::String (t, 2));
            juce::MessageManager::getInstance()->runDispatchLoopUntil (15);
        }
        bool playing; double t;
        head.now (playing, t);
        win.setName ((playing ? "DAW PLAY " : "DAW STOP ") + juce::String (t, 2));
        juce::MessageManager::getInstance()->runDispatchLoopUntil (1);
        grab ((HWND) win.getWindowHandle(), dir.getChildFile (juce::String (shot, 1) + "s.png"));
        std::printf ("%.1f s : DAW %s %.2f\n", shot, playing ? "PLAY" : "STOP", t);
    }

    run = false; daw.join();
    win.clearContentComponent();
    delete plugin->getActiveEditor();
    plugin->releaseResources();
    return 0;
}
