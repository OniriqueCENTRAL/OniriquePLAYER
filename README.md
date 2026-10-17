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

- Le plugin envoie la position du DAW à la page en permanence (toutes les 30 ms en lecture, toutes les 500 ms à
  l'arrêt) ; c'est la page qui décide de la suivre selon SYNC. Les commandes de la page (SYNC, son, ouvrir, format)
  passent par le canal natif de JUCE.
- Pause ou stop du DAW : l'image s'arrête exactement à sa position (moins de 15 ms d'écart mesuré).
- Petit décalage pendant la lecture, ou à la reprise : la vidéo accélère ou ralentit le temps de le rattraper, sans
  saut d'image.
- Curseur déplacé, ou écart trop grand pour la vitesse : un seul saut, qui vise la position que le DAW aura atteinte à
  la fin du saut (durée mesurée sur la vidéo). Jamais deux sauts d'affilée : des sauts enchaînés figent l'image sur
  les vidéos lourdes (téléphone, internet).
- Mesures dans le moteur de la fenêtre, avec 8 pauses et reprises rapprochées sur une vidéo d'internet de 29 s :
  toutes les reprises se recalent (1 à 2 jamais recalées avant), écart en lecture de 59 à 62 ms au pire, 32 à 34 ms
  en moyenne (avant : 77 à 165 ms au pire).
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
