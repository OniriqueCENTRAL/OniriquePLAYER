# PLAYER ONIRIQUE

Plugin gratuit (VST3 Windows et Mac, Audio Unit sur Mac) : une fenêtre qui lit une vidéo `.mp4` / `.mov`, calée sur
la lecture du DAW. Charte ONIRIQUE (nuit / sang / braise, Unbounded, Grenze Gotisch, JetBrains Mono, sigil).

- La fenêtre prend le format de la vidéo (4:3, 16:9, 9:16, carré...) et garde ce format quand on la redimensionne.
- Les contrôles n'apparaissent qu'au survol de la souris.
- L'audio du DAW traverse le plugin sans aucune modification.
- La vidéo choisie, SYNC, le son et la taille de la fenêtre sont gardés dans le projet.

## SYNC

Allumé (par défaut) : la vidéo suit le DAW, play, stop et curseur. Éteint : lecteur libre (lecture / pause, barre de
position, boucle).

- Petit décalage pendant la lecture : la vidéo accélère ou ralentit de quelques pour cent le temps de le rattraper,
  sans saut d'image.
- Curseur déplacé, ou décalage de plus d'une demi-seconde : saut direct. Le plugin mesure le temps que prend un saut
  dans la vidéo et vise la position que le DAW aura atteinte à la fin du saut.
- Mesuré dans le moteur de la fenêtre (même scénario : lecture, saut pendant la lecture, stop, curseur déplacé, relance),
  sur trois vidéos : écart moyen image / DAW de 23 à 36 ms, contre 52 à 101 ms avant (l'ancienne version ne corrigeait
  rien en dessous de 350 ms).
- FL Studio : mettre le plugin sur le Master. Sur une piste muette, « Smart disable » arrête le plugin et la vidéo
  se met en pause : le désactiver pour ce plugin si besoin.

## Formats

Décodage par le moteur web du système (WebView2 sous Windows, WebKit sur Mac). H.264 + AAC (mp4 / mov) marche partout ;
HEVC / ProRes dépendent des codecs de la machine.

## Compiler (Windows)

Une fois : le SDK WebView2 (PowerShell)

    Register-PackageSource -provider NuGet -name nugetRepository -location https://www.nuget.org/api/v2
    Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.1901.177 -Source nugetRepository

Puis :

    cmake -B build -G "Visual Studio 17 2022" -A x64 "-DFETCHCONTENT_SOURCE_DIR_JUCE=C:/Users/mathe/Downloads/ONIRIQUE PLUGINS/_juce/8.0.6"
    cmake --build build --config Release

Résultat : `build\PlayerOnirique_artefacts\Release\VST3\PLAYER ONIRIQUE.vst3`.

Mac : compilé par GitHub Actions à chaque envoi sur `main` (`.github/workflows/build.yml`), qui publie ensuite la
version Windows + Mac dans ONIRIQUE CENTRAL (identifiant `player-onirique`).

## Vérifier dans un hôte

`tools/host_check.cpp` charge le VST3 comme un DAW (stop, lecture, saut pendant la lecture, stop) et enregistre la
fenêtre en PNG, avec la position du DAW dans le titre :

    cmake --build build --config Release --target PlayerOniriqueHostCheck
    build\PlayerOniriqueHostCheck_artefacts\Release\PlayerOniriqueHostCheck.exe "build\PlayerOnirique_artefacts\Release\VST3\PLAYER ONIRIQUE.vst3" video.mp4 captures

Seules les images prises à l'arrêt sont fiables : Windows ne recopie pas l'image d'une vidéo décodée par la carte
graphique pendant la lecture.
