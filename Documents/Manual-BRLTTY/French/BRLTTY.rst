==============================
Manuel de référence de BRLTTY
==============================

Accès à l'écran d'une console pour les personnes non-voyantes utilisant des afficheurs braille

:Authors: Nikhil Nair `<nn201@cus.cam.ac.uk> <mailto:nn201@cus.cam.ac.uk>`__; Nicolas Pitre `<nico@fluxnic.net> <mailto:nico@fluxnic.net>`__; Stéphane Doyon `<s.doyon@videotron.ca> <mailto:s.doyon@videotron.ca>`__; Dave Mielke `<dave@mielke.cc> <mailto:dave@mielke.cc>`__
:Traduction française: Jean-Philippe Mengual `<texou@accelibreinfo.eu> <mailto:texou@accelibreinfo.eu>`__ pour `Traduc.org <http://www.traduc.org/>`__ (édition originale) ; révision 2026 par Nicolas Pitre `<nico@fluxnic.net> <mailto:nico@fluxnic.net>`__, avec assistance par IA (Claude, Anthropic)
:Date: Version 6.9.1, avril 2026

Copyright (c) 1995-2026 par Les Développeurs de BRLTTY. BRLTTY est un logiciel libre, et n'est accompagné d'ABSOLUMENT AUCUNE GARANTIE. Il est placé sous les termes de la version 2.1 ou ultérieure de la **GNU Lesser General Public License** publiée par **The Free Software Foundation**.


Introduction
============

BRLTTY donne à un utilisateur brailliste l'accès aux consoles texte sous
Linux et systèmes apparentés. Il s'exécute en arrière-plan (démon),
pilote un afficheur braille rafraîchissable et y reproduit ce qui
apparaîtrait normalement à l'écran, afin que l'utilisateur puisse le
lire. Il peut être démarré très tôt dans la séquence d'amorçage, ce qui
maintient le système utilisable en mode mono-utilisateur et lors d'une
restauration au démarrage, aussi bien que pour le travail courant au
quotidien.

BRLTTY reproduit une portion rectangulaire de l'écran — appelée *la
fenêtre* dans ce manuel — sous forme de texte braille sur l'afficheur.
Les touches de l'afficheur déplacent la fenêtre sur l'écran, activent
ou désactivent des options d'affichage et déclenchent les diverses
commandes de BRLTTY.

Parmi les fonctionnalités phares figurent la poursuite et le
rapatriement du curseur, le braille abrégé (l'anglais et le français
sont fournis d'origine ; de nombreuses autres langues sont couvertes
par des tables additionnelles), le gel de l'écran pour une relecture
posée, la revue des attributs, le copier-coller, des styles de curseur
et de clignotement configurables, un menu interactif des préférences,
un mode apprentissage en ligne pour découvrir les commandes, une
sortie vocale optionnelle et une API programmable pour les
applications clientes. Voir :ref:`Premiers pas <getting-started>` pour
l'installation et les premiers pas, et :ref:`Obtenir de l'aide
<getting-help>` pour la liste de diffusion et le suivi des problèmes.

Linux est la plateforme principale. D'autres plateformes sont prises
en charge à divers degrés — voir ``Documents/README.Android``,
``Documents/README.Windows`` et les README spécifiques à chaque
plateforme dans ``Documents/`` pour plus de détails.

.. _getting-started:

Premiers pas
============

Installation
------------

La plupart des distributions Linux fournissent BRLTTY sous forme de
paquet. Installez-le comme à votre habitude — par exemple ::

  apt install brltty                # Debian, Ubuntu, dérivés
  dnf install brltty                # Fedora, RHEL, dérivés
  zypper install brltty             # openSUSE
  pacman -S brltty                  # Arch

Si vous préférez compiler depuis les sources, clonez le dépôt ou
décompressez l'archive, lancez ``./configure`` (``./configure --help``
liste les options de compilation), puis ``make`` et ``make install``.
La hiérarchie des fichiers installés est décrite par le script shell
``brltty-config`` (voir :ref:`L'utilitaire brltty-config
<utility-brltty-config>`).

Connectez votre afficheur braille avant de démarrer BRLTTY pour la
première fois. Les afficheurs sur port série doivent être branchés et
sous tension d'abord ; les afficheurs USB et Bluetooth peuvent être
branchés à chaud dès lors que l'intégration Udev est en place (voir
"Démarrage au boot" plus bas).

Démarrage manuel
----------------

Une fois installé, lancez BRLTTY en tant que root ::

  brltty

Il lit sa configuration dans un fichier
(voir :ref:`Le fichier de configuration <configure>`)
qui fixe les valeurs par défaut comme le pilote braille, le
périphérique auquel l'afficheur est connecté, et la table de texte. La
plupart de ces valeurs peuvent être remplacées en ligne de commande
(voir :ref:`Options en ligne de commande <options>`).

Quelques options reviennent souvent juste après l'installation :

* :ref:`-h <options-help>` liste toutes les options.
* :ref:`-V <options-version>` affiche la version de BRLTTY, la version
  de son API, et les pilotes intégrés.
* :ref:`-v <options-verify>` affiche les valeurs des options telles que
  BRLTTY les utiliserait après fusion du fichier de configuration, de
  l'environnement et de la ligne de commande — pratique pour vérifier
  un réglage avant de s'y engager.
* :ref:`-n <options>` garde BRLTTY au premier plan (pas de passage en
  démon) et se combine bien avec :ref:`-e <options>` (journal sur la
  sortie d'erreur standard) et :ref:`-l debug <options>` pour
  diagnostiquer les problèmes de démarrage.

Ce que vous devriez voir
------------------------

Une bannière de démarrage donnant le nom du programme et son numéro de
version apparaît brièvement sur l'afficheur
(voir l'option en ligne de commande :ref:`-M <options-message-timeout>`
pour le délai). L'afficheur montre ensuite une petite zone
rectangulaire de l'écran contenant le curseur ; par défaut, le curseur
lui-même est représenté par les points 7 et 8 superposés sur le
caractère où il se trouve.

Au fil de la frappe, l'afficheur suit le curseur — c'est la
**poursuite du curseur**. Tout ce qui change à l'écran est répercuté
sur l'afficheur en temps réel.

La portion d'écran que l'afficheur montre à un instant donné est
appelée la **fenêtre** braille. La fenêtre peut être déplacée
indépendamment du curseur lorsque vous voulez lire ailleurs sur
l'écran (un long message d'erreur, la ligne au-dessus de l'invite, une
autre partie d'une page de manuel) — c'est ce que font la plupart des
commandes de BRLTTY.

Trouver votre chemin
--------------------

Deux commandes méritent d'être mémorisées avant toutes les autres :

* :ref:`HELP <command-HELP>` — affiche une aide sur une ligne pour la
  prochaine touche que vous appuierez, puis revient au fonctionnement
  normal.
* :ref:`LEARN <command-LEARN>` — annonce le nom de chaque commande au
  fur et à mesure que vous appuyez sur sa touche ; pratique pour
  explorer les attributions de touches qui ne vous sont pas familières.
  Appuyez sur Entrée au clavier pour quitter le mode apprentissage
  (voir :ref:`Mode apprentissage des commandes <learn>` pour les
  détails).

Les deux sont des bascules : invoquez-les à nouveau pour revenir. Le
chapitre :ref:`Commandes <commands>` regroupe toutes les commandes par
finalité ; ``Documents/README.CommandReference`` est la référence
alphabétique.

Démarrage au boot
-----------------

Vous voudrez très probablement que BRLTTY soit démarré automatiquement
à l'amorçage, de sorte que votre afficheur soit déjà actif lorsque
l'invite de connexion apparaît. Sur les distributions Linux modernes,
BRLTTY est livré sous forme de service systemd empaqueté — installez
le paquet ``brltty`` de votre distribution et activez l'unité. Voir
``Documents/README.Systemd`` pour les noms des unités, la différence
entre l'instance par défaut et les instances par périphérique, et la
façon dont la prise en charge du branchement à chaud Udev est câblée
pour qu'un afficheur USB démarre BRLTTY automatiquement à la
connexion.

.. _utilities:

Utilitaires
===========

Quelques petites commandes accompagnent ``brltty`` lui-même pour
un usage en script ou en ligne de commande. Chacune accepte
``--help`` qui affiche la liste complète des options.


.. _utility-brltty-config:

brltty-config
-------------

Script shell qui expose à d'autres scripts l'agencement des
répertoires choisi à la compilation de BRLTTY. À sourcer (avec la
commande ``.``) plutôt qu'à exécuter, depuis un shell compatible
Bourne::

  . brltty-config

Le script définit des variables d'environnement reflétant la
disposition d'exécution figée à la compilation : ``BRLTTY_VERSION``,
``BRLTTY_EXECUTE_ROOT``, ``BRLTTY_PROGRAM_DIRECTORY``,
``BRLTTY_LIBRARY_DIRECTORY``, ``BRLTTY_WRITABLE_DIRECTORY``,
``BRLTTY_DATA_DIRECTORY``, ``BRLTTY_MANPAGE_DIRECTORY``,
``BRLAPI_VERSION``, et d'autres encore. Lancez-le une fois et
inspectez l'environnement pour la liste complète.


.. _utility-brltty-tmux:

brltty-tmux
-----------

Dans une session ``tmux``, votre shell, votre éditeur et les autres
applications écrivent dans un écran virtuel que ``tmux`` maintient
en espace utilisateur — leur texte n'est donc pas directement
visible par le pilote d'écran console habituel de BRLTTY.
``brltty-tmux`` fait le pont.

Lancez-le depuis n'importe quelle fenêtre de la session tmux que
vous voulez suivre::

  brltty-tmux

Il démarre une *seconde* instance de BRLTTY dont le pilote d'écran
est le pilote Tmux (``tx``) et le pilote braille est BrlAPI
(``ba``). Cette seconde instance lit l'état de l'écran de la
session tmux via tmux lui-même et transmet la sortie braille via
BrlAPI à l'instance principale de BRLTTY, celle qui pilote
réellement l'afficheur. L'instance principale continue son travail
pour la console système ; pendant que vous lisez à l'intérieur de
tmux, elle se contente de relayer.

Le basculement entre consoles ne fonctionne correctement que si
``brltty-tmux`` s'exécute avec les privilèges du super-utilisateur,
puisque déterminer la console virtuelle au premier plan exige ces
privilèges. Passez ``-s`` pour qu'il s'élève automatiquement via
``sudo``, ou ``-c`` pour rester sous l'utilisateur courant (un
avertissement signalera alors que le basculement de console peut
se comporter incorrectement). Les arguments situés après ``--``
sont transmis tels quels au ``brltty`` interne — pratique pour,
par exemple, surcharger l'adresse de l'afficheur braille::

  brltty-tmux -s -- -d bg:address=01:23:45:67:89:AB

Pour que le pont fonctionne, BrlAPI doit être activé dans
l'instance principale de BRLTTY.


.. _utility-brltty-clip:

brltty-clip
-----------

Lit ou écrit le presse-papier de BRLTTY depuis la ligne de
commande, via BrlAPI. Pratique pour injecter le presse-papier
système (ou tout autre texte) dans celui de BRLTTY en vue d'un
collage braille par la commande
:ref:`PASTE <command-PASTE>`, ou pour récupérer un extrait que
:ref:`COPY_SMART_NEW <command-COPY_SMART_NEW>` ou
:ref:`CLIP_NEW <command-CLIP_NEW>` ont déposé et le passer à un
autre outil.

Formes courantes::

  echo 'texte à envoyer' | brltty-clip          # définit le presse-papier
  brltty-clip -s 'texte à envoyer'              # définit le presse-papier
  brltty-clip -g                                # affiche le presse-papier
  brltty-clip -g -r                             # affiche, sans saut de ligne final

Les options ``-g`` (``--get-content``) et ``-s``
(``--set-content``) sont mutuellement exclusives avec des
arguments positionnels de fichiers. Sans option et avec un ou
plusieurs noms de fichiers, le contenu de ces fichiers (ou de
l'entrée standard si l'argument vaut ``-``) est concaténé puis
stocké comme nouveau contenu du presse-papier.

.. _using:

Utilisation de BRLTTY
=====================

Ce chapitre décrit le fonctionnement des commandes, de la configuration
et des options de BRLTTY une fois le programme démarré. Si vous n'en
êtes pas encore là, le chapitre :ref:`Premiers pas <getting-started>`
explique comment l'amener jusque là.

Les paramètres les plus souvent ajustés en ligne de commande sont le
pilote braille
(:ref:`-b <options-braille-driver>` /
:ref:`braille-driver <configure-braille-driver>`),
le périphérique auquel l'afficheur est connecté
(:ref:`-d <options-braille-device>` /
:ref:`braille-device <configure-braille-device>`),
et les éventuels paramètres propres au pilote
(:ref:`-B <options-braille-parameters>` /
:ref:`braille-parameters <configure-braille-parameters>`).
Les autres paramètres ont des valeurs par défaut raisonnables.

Quand BRLTTY tourne, la portion de l'écran montrée à un instant donné
sur l'afficheur s'appelle la **fenêtre**. Tout ce que vous tapez ou que
l'écran redessine s'y reflète en temps réel, et la fenêtre suit le
curseur (poursuite du curseur). Pour lire ailleurs sur l'écran — un
long message d'erreur au-dessus de l'invite, la ligne que vous venez
de quitter dans un éditeur, le bas d'une page de manuel — vous
déplacez la fenêtre indépendamment du curseur ; c'est ce que font la
plupart des commandes de BRLTTY.


.. _commands:

Commandes
---------

Les commandes de BRLTTY sont désignées par leur nom dans tout ce
manuel. La touche (ou combinaison de touches) qui déclenche chacune
dépend de votre afficheur braille — chaque pilote livre des
attributions par défaut, modifiables via les tables de touches (voir
le chapitre :ref:`Tables <tables>`).

Les deux commandes à mémoriser en premier sont
:ref:`HELP <command-HELP>`, qui affiche les attributions de touches
de votre pilote, et :ref:`LEARN <command-LEARN>`, qui vous laisse
appuyer sur les touches pour découvrir leur effet. Les deux sont à
bascule : invoquez-les à nouveau pour revenir au fonctionnement
normal.

Ce chapitre regroupe les commandes les plus courantes par usage. Pour
la liste alphabétique complète — y compris la syntaxe des modificateurs
des commandes ancrées sur un caractère — voir
``Documents/README.CommandReference``.


.. _vertical-motion:

Déplacement vertical
~~~~~~~~~~~~~~~~~~~~

.. _command-LNUP-LNDN:

LNUP/LNDN
  Monte/descend d'une ligne. Lorsque le saut des lignes identiques
  (:ref:`SKPIDLNS <command-SKPIDLNS>`) est actif, se comporte comme
  :ref:`PRDIFLN/NXDIFLN <command-PRDIFLN-NXDIFLN>`.

.. _command-WINUP-WINDN:

WINUP/WINDN
  Monte/descend d'une fenêtre (cinq lignes si la fenêtre n'en fait
  qu'une).

.. _command-PRDIFLN-NXDIFLN:

PRDIFLN/NXDIFLN
  Monte/descend à la ligne la plus proche dont le contenu diffère.

.. _command-ATTRUP-ATTRDN:

ATTRUP/ATTRDN
  Monte/descend à la ligne la plus proche dont la mise en relief des
  caractères diffère.

.. _command-TOP-BOT:

TOP/BOT
  Première/dernière ligne.

.. _command-TOP_LEFT-BOT_LEFT:

TOP_LEFT/BOT_LEFT
  Coin supérieur gauche / coin inférieur gauche.

.. _command-PRPGRPH-NXPGRPH:

PRPGRPH/NXPGRPH
  Paragraphe précédent/suivant (la première ligne non vide après une
  ligne vide).

.. _command-PRPROMPT-NXPROMPT:

PRPROMPT/NXPROMPT
  Invite de commande précédente/suivante.

.. _command-PRSEARCH-NXSEARCH:

PRSEARCH/NXSEARCH
  Recherche en arrière/en avant le contenu du presse-papier
  (voir :ref:`Copier-coller <cut>`). Boucle au bord de l'écran ;
  insensible à la casse.

Les commandes :ref:`PRINDENT/NXINDENT <command-PRINDENT-NXINDENT>` et
:ref:`PRDIFCHAR/NXDIFCHAR <command-PRDIFCHAR-NXDIFCHAR>` se déplacent
elles aussi verticalement, mais prennent un numéro de colonne
(typiquement une touche de routage) indiquant la colonne à examiner.


.. _horizontal-motion:

Déplacement horizontal
~~~~~~~~~~~~~~~~~~~~~~

.. _command-CHRLT-CHRRT:

CHRLT/CHRRT
  Un caractère à gauche/droite.

.. _command-HWINLT-HWINRT:

HWINLT/HWINRT
  Une demi-fenêtre à gauche/droite.

.. _command-FWINLT-FWINRT:

FWINLT/FWINRT
  Une fenêtre à gauche/droite. Boucle au bord de l'écran et peut
  sauter les fenêtres vides
  (voir :ref:`SKPBLNKWINS <command-SKPBLNKWINS>`).

.. _command-FWINLTSKIP-FWINRTSKIP:

FWINLTSKIP/FWINRTSKIP
  Va à la fenêtre non vide la plus proche, à gauche/droite.

.. _command-LNBEG-LNEND:

LNBEG/LNEND
  Début/fin de la ligne.

La commande :ref:`SETLEFT <command-SETLEFT>`, à touche de routage,
permet aussi un déplacement horizontal vers une colonne précise.


.. _implicit-motion:

Déplacement implicite
~~~~~~~~~~~~~~~~~~~~~

.. _command-HOME:

HOME
  Va à l'emplacement du curseur.

.. _command-BACK:

BACK
  Revient à la position où la dernière commande de déplacement avait
  placé la fenêtre braille. Pratique après qu'une poursuite du curseur
  (ou un autre déplacement implicite) vous a éloigné de votre lecture.

.. _command-RETURN:

RETURN
  - Si le dernier déplacement de la fenêtre braille était automatique
    (par exemple une poursuite du curseur), se comporte comme
    :ref:`BACK <command-BACK>`.
  - Sinon, si le curseur se trouve hors de la fenêtre braille,
    se comporte comme :ref:`HOME <command-HOME>`.

La commande :ref:`GOTOMARK <command-GOTOMARK>`, à touche de routage,
saute à une position préalablement mémorisée par ``SETMARK``.


.. _feature-activation:

Activation de fonctionnalités
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Chaque commande de cette section a trois formes — **activée**,
**désactivée** et **bascule** — affectées à des touches (ou
combinaisons) distinctes par votre table de touches. Sauf indication
contraire, chaque fonctionnalité est initialement désactivée et
s'applique globalement ; sa valeur initiale peut aussi être réglée
dans le :ref:`menu des préférences <preferences-menu>`.

.. _command-FREEZE:

FREEZE
  Gèle l'image de l'écran, ce qui permet de lire à votre rythme même
  si l'application continue d'écrire.

.. _command-DISPMD:

DISPMD
  Affiche la mise en relief des caractères (attributs — couleur,
  vidéo inversée, clignotement) au lieu des caractères eux-mêmes.
  Utile pour repérer un élément mis en valeur. Par terminal virtuel.

.. _command-SIXDOTS:

SIXDOTS
  Affiche les caractères en braille à six points plutôt qu'à huit ;
  les points 7 et 8 restent disponibles pour la représentation du
  curseur et le soulignement des attributs. Si une table de braille
  abrégé est en vigueur (voir l'option
  :ref:`-c <options-contraction-table>`), elle est utilisée.
  Modifiable aussi via la préférence
  :ref:`Apparence du texte <preference-text-style>`.

.. _command-SLIDEWIN:

SLIDEWIN
  Lorsque la poursuite du curseur est active, fait glisser la fenêtre
  horizontalement pour garder le curseur près du centre, plutôt que
  de sauter par pas d'une fenêtre. Modifiable aussi via la préférence
  :ref:`Faire défiler la fenêtre <preference-sliding-window>`.

.. _command-SKPIDLNS:

SKPIDLNS
  Saute les lignes dont le contenu correspond à la ligne courante.
  Affecte :ref:`LNUP/LNDN <command-LNUP-LNDN>` ainsi que le
  comportement de bouclage de
  :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` et de
  :ref:`FWINLTSKIP/FWINRTSKIP <command-FWINLTSKIP-FWINRTSKIP>`.
  Modifiable aussi via la préférence
  :ref:`Sauter les lignes identiques <preference-skip-identical-lines>`.

.. _command-SKPBLNKWINS:

SKPBLNKWINS
  Saute les fenêtres vides lors de la lecture. Affecte
  :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>`.

.. _command-CSRVIS:

CSRVIS
  Montre le curseur en superposant un motif de points sur son
  caractère. Initialement **activée**. Modifiable aussi via la
  préférence :ref:`Afficher le curseur <preference-show-cursor>`.

.. _command-CSRHIDE:

CSRHIDE
  Cache le curseur (a priorité sur :ref:`CSRVIS <command-CSRVIS>` pour
  le terminal virtuel courant) afin de pouvoir lire le caractère
  sous-jacent.

.. _command-CSRTRK:

CSRTRK
  Poursuite du curseur : déplace automatiquement la fenêtre braille
  quand le curseur en sort. Initialement **activée**, par terminal
  virtuel. Si le curseur saute à un moment inopportun, utilisez
  :ref:`BACK <command-BACK>` pour revenir.

.. _command-CSRSIZE:

CSRSIZE
  Représente le curseur par un bloc plein (les huit points) plutôt
  que par un soulignement (points 7 et 8). Modifiable aussi via la
  préférence :ref:`Apparence du curseur <preference-cursor-style>`.

.. _command-CSRBLINK:

CSRBLINK
  Fait clignoter la représentation du curseur. Modifiable aussi via
  la préférence
  :ref:`Curseur clignotant <preference-blinking-cursor>`.

.. _command-ATTRVIS:

ATTRVIS
  Souligne les caractères mis en relief par des combinaisons des
  points 7 et 8 :

  pas de soulignement
    Blanc sur noir (normal), gris sur noir, blanc sur bleu, noir sur
    cyan.

  points 7 et 8
    Noir sur blanc (vidéo inversée).

  point 8
    Tout le reste.

  Modifiable aussi via la préférence
  :ref:`Afficher les attributs <preference-show-attributes>`.

.. _command-ATTRBLINK:

ATTRBLINK
  Fait clignoter le soulignement des attributs. Initialement
  **activée**. Modifiable aussi via la préférence
  :ref:`Attributs clignotants <preference-blinking-attributes>`.

.. _command-CAPBLINK:

CAPBLINK
  Fait clignoter les lettres majuscules. Modifiable aussi via la
  préférence
  :ref:`Majuscules clignotantes <preference-blinking-capitals>`.

.. _command-TUNES:

TUNES
  Joue une courte mélodie (voir :ref:`Mélodies d'alerte <tunes>`) lors
  des événements significatifs. Initialement **activée**.

.. _command-AUTOREPEAT:

AUTOREPEAT
  Répète automatiquement une commande à intervalle régulier tant que
  sa touche reste enfoncée. Seuls certains pilotes le prennent en
  charge ; beaucoup d'afficheurs ne signalent pas le relâchement
  comme un événement distinct. Initialement **activée**.

.. _command-AUTOSPEAK:

AUTOSPEAK
  Énonce automatiquement la nouvelle ligne lors d'un déplacement
  vertical, les caractères au fur et à mesure qu'ils sont tapés ou
  effacés, et le caractère vers lequel se déplace le curseur.

.. _command-ASPK_EMP_LINE:

ASPK_EMP_LINE
  Bascule l'annonce des lignes vides par AUTOSPEAK (au lieu de les
  passer silencieusement).


.. _mode-selection:

Sélection du mode
~~~~~~~~~~~~~~~~~

.. _command-HELP:

HELP
  Bascule sur la page d'aide du pilote braille — votre référence des
  touches. Utilisez les commandes habituelles de déplacement
  :ref:`vertical <vertical-motion>` et
  :ref:`horizontal <horizontal-motion>` pour la lire, et invoquez
  ``HELP`` à nouveau pour quitter.

.. _command-INFO:

INFO
  Bascule sur l'affichage d'état
  (voir :ref:`L'affichage d'état <status>`) : positions du curseur et
  de la fenêtre, états des fonctionnalités. Invoquez à nouveau pour
  quitter.

.. _command-LEARN:

LEARN
  Bascule en mode apprentissage des commandes
  (voir :ref:`Mode apprentissage des commandes <learn>`). Appuyez sur
  une touche pour découvrir la commande qu'elle envoie. Invoquez
  ``LEARN`` à nouveau pour quitter. Indisponible si BRLTTY a été
  compilé avec ``--disable-learn-mode``.


.. _preference-maintenance:

Gestion des préférences
~~~~~~~~~~~~~~~~~~~~~~~

.. _command-PREFMENU:

PREFMENU
  Bascule sur le menu des préférences
  (voir :ref:`Le menu des préférences <preferences-menu>`). Invoquez
  à nouveau pour quitter.

.. _command-PREFSAVE:

PREFSAVE
  Sauvegarde les préférences courantes
  (voir :ref:`Préférences <preferences>`).

.. _command-PREFLOAD:

PREFLOAD
  Restaure les préférences sauvegardées le plus récemment.


.. _menu-navigation:

Navigation dans le menu
~~~~~~~~~~~~~~~~~~~~~~~

.. _command-MENU_FIRST_ITEM-MENU_LAST_ITEM:

MENU_FIRST_ITEM/MENU_LAST_ITEM
  Premier/dernier élément du menu.

.. _command-MENU_PREV_ITEM-MENU_NEXT_ITEM:

MENU_PREV_ITEM/MENU_NEXT_ITEM
  Élément précédent/suivant.

.. _command-MENU_PREV_SETTING-MENU_NEXT_SETTING:

MENU_PREV_SETTING/MENU_NEXT_SETTING
  Décrémente/incrémente le paramètre de l'élément courant.


.. _speech-controls:

Contrôles de la synthèse vocale
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _command-SAY_LINE:

SAY_LINE
  Énonce la ligne courante. La préférence
  :ref:`Mode dire la ligne <preference-sayline-mode>` détermine si
  l'énoncé en cours est interrompu d'abord.

.. _command-SAY_ABOVE:

SAY_ABOVE
  Énonce du haut de l'écran jusqu'à la ligne courante.

.. _command-SAY_BELOW:

SAY_BELOW
  Énonce de la ligne courante jusqu'au bas de l'écran.

.. _command-MUTE:

MUTE
  Coupe immédiatement la synthèse vocale.

.. _command-SPKHOME:

SPKHOME
  Va à l'emplacement du curseur de lecture vocale.

.. _command-SAY_SLOWER-SAY_FASTER:

SAY_SLOWER/SAY_FASTER
  Diminue/augmente le débit de la synthèse vocale
  (voir :ref:`Débit de la synthèse <preference-speech-rate>`).
  Dépend du pilote.

.. _command-SAY_SOFTER-SAY_LOUDER:

SAY_SOFTER/SAY_LOUDER
  Diminue/augmente le volume de la synthèse vocale
  (voir :ref:`Volume de la synthèse <preference-speech-volume>`).
  Dépend du pilote.

.. _command-SPK_PUNCT_LEVEL:

SPK_PUNCT_LEVEL
  Fait défiler le niveau de ponctuation — *aucun*, *partiel*,
  *majoritaire*, *intégral* — qui contrôle combien de signes de
  ponctuation sont énoncés
  (voir :ref:`Niveau de ponctuation <preference-punctuation-level>`).


.. _speech-navigation:

Navigation par la synthèse vocale
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Ces commandes déplacent le curseur de lecture vocale et énoncent
l'unité sur laquelle il se pose.

SPEAK_CURR_CHAR / SPEAK_PREV_CHAR / SPEAK_NEXT_CHAR
  Énonce le caractère courant, précédent ou suivant.

SPEAK_FRST_CHAR / SPEAK_LAST_CHAR
  Énonce le premier ou le dernier caractère de la ligne courante.

SPEAK_CURR_WORD / SPEAK_PREV_WORD / SPEAK_NEXT_WORD
  Énonce le mot courant, précédent ou suivant.

SPEAK_CURR_LINE / SPEAK_PREV_LINE / SPEAK_NEXT_LINE
  Énonce la ligne courante, précédente ou suivante.

SPEAK_FRST_LINE / SPEAK_LAST_LINE
  Énonce la première ou la dernière ligne de l'écran.


Basculement entre terminaux virtuels
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _command-SWITCHVT_PREV-SWITCHVT_NEXT:

SWITCHVT_PREV/SWITCHVT_NEXT
  Bascule sur le terminal virtuel précédent/suivant.

La commande :ref:`SWITCHVT <command-SWITCHVT>`, à touche de routage,
saute directement à un terminal numéroté.


Autres commandes
~~~~~~~~~~~~~~~~

.. _command-CSRJMP_VERT:

CSRJMP_VERT
  Rapatrie le curseur sur la ligne du haut de la fenêtre braille
  (voir :ref:`Rapatriement du curseur <routing>`) en simulant les
  flèches haut/bas. Plus sûr que la variante à touche de routage en
  ce qu'elle ne simule pas les flèches horizontales.

.. _command-PASTE:

PASTE
  Insère le contenu courant du presse-papier à l'emplacement du
  curseur (voir :ref:`Copier-coller <cut>`).

.. _command-CLIP_SHOW:

CLIP_SHOW
  Affiche le contenu courant du presse-papier sous forme de message.
  Affectée par défaut à un appui long sur ``CLIP_SAVE``.

.. _command-CLIP_CLEAR:

CLIP_CLEAR
  Vide le presse-papier. Affectée par défaut à un appui long sur
  ``CLIP_RESTORE``.

.. _command-RESTARTBRL:

RESTARTBRL
  Arrête puis relance le pilote de l'afficheur braille.

.. _command-RESTARTSPEECH:

RESTARTSPEECH
  Arrête puis relance le pilote de la synthèse vocale.


.. _commands-characters:

Commandes à caractère
~~~~~~~~~~~~~~~~~~~~~

Ces commandes prennent un numéro de colonne, fourni typiquement par
l'appui sur l'une des touches de routage de votre afficheur.

.. _command-ROUTE:

ROUTE
  Rapatrie le curseur sur le caractère désigné par la touche de
  routage (voir :ref:`Rapatriement du curseur <routing>`), en
  simulant les flèches du clavier.

.. _command-CLIP_NEW:

CLIP_NEW
  Ancre le début d'un bloc de copie sur la touche de routage, en
  remplaçant le presse-papier (voir :ref:`Copier-coller <cut>`).

.. _command-CLIP_ADD:

CLIP_ADD
  Comme :ref:`CLIP_NEW <command-CLIP_NEW>`, mais conserve le contenu
  du presse-papier.

.. _command-COPY_RECT:

COPY_RECT
  Ancre la fin du bloc de copie sur la touche de routage et ajoute
  la région rectangulaire au presse-papier.

.. _command-COPY_LINE:

COPY_LINE
  Comme :ref:`COPY_RECT <command-COPY_RECT>`, mais pour une région
  linéaire (qui suit le retour à la ligne).

.. _command-COPYCHARS:

COPYCHARS
  Copie dans le presse-papier le bloc de caractères ancré par les
  deux touches de routage.

.. _command-APNDCHARS:

APNDCHARS
  Comme :ref:`COPYCHARS <command-COPYCHARS>`, mais en ajout.

.. _command-COPY_SMART_NEW:

COPY_SMART_NEW
  Détecte une URL, une adresse e-mail ou un nom d'hôte à la colonne
  de la touche de routage et le copie dans le presse-papier, en
  remplaçant le contenu précédent. Évite de pointer manuellement les
  deux extrémités d'une longue sélection.

.. _command-COPY_SMART_ADD:

COPY_SMART_ADD
  Comme :ref:`COPY_SMART_NEW <command-COPY_SMART_NEW>`, mais en
  ajout.

.. _command-PRINDENT-NXINDENT:

PRINDENT/NXINDENT
  Monte/descend à la ligne la plus proche dont l'indentation est au
  plus celle de la colonne de la touche de routage.

.. _command-DESCCHAR:

DESCCHAR
  Affiche brièvement une description du caractère désigné par la
  touche de routage — valeur décimale et hexadécimale, couleurs de
  premier plan et de fond, attributs (``bright``, ``blink``).
  Exemple ::

    char 65 (0x41): white on black bright blink

.. _command-SETLEFT:

SETLEFT
  Repositionne la fenêtre braille de façon que son bord gauche soit
  à la colonne de la touche de routage. Sur les afficheurs équipés
  de touches de routage, cela rend largement superflu le déplacement
  pas à pas via :ref:`CHRLT/CHRRT <command-CHRLT-CHRRT>` et
  :ref:`HWINLT/HWINRT <command-HWINLT-HWINRT>`.

.. _command-PRDIFCHAR-NXDIFCHAR:

PRDIFCHAR/NXDIFCHAR
  Monte/descend à la ligne la plus proche dont le caractère à la
  colonne de la touche de routage diffère.


.. _commands-base:

Commandes de base
~~~~~~~~~~~~~~~~~

.. _command-SWITCHVT:

SWITCHVT
  Bascule sur le terminal virtuel dont le numéro (à partir de 1)
  correspond à la touche de routage. Voir aussi
  :ref:`SWITCHVT_PREV/SWITCHVT_NEXT <command-SWITCHVT_PREV-SWITCHVT_NEXT>`.

.. _command-SETMARK:

SETMARK
  Marque la position courante de la fenêtre braille dans un registre
  associé à la touche de routage. Par terminal virtuel.

.. _command-GOTOMARK:

GOTOMARK
  Déplace la fenêtre braille à la position préalablement
  :ref:`marquée <command-SETMARK>` avec la même touche de routage.
  Par terminal virtuel.

.. _configure:

Fichier de configuration
------------------------

Un fichier de configuration vous permet de fixer des valeurs par défaut
pour ne pas avoir à passer les mêmes options à chaque invocation.
BRLTTY lit ``/etc/brltty.conf`` par défaut, ou bien le fichier désigné
par l'option ``-f`` ; ce fichier est facultatif, et BRLTTY fonctionne
très bien sans, pourvu que les réglages nécessaires soient fournis
autrement.

La référence faisant autorité pour le contenu de ce fichier est
``Documents/brltty.conf`` dans l'arbre des sources (également installé
en tant que ``/etc/brltty.conf`` par la plupart des distributions).
Chaque directive y est documentée par un bloc de commentaires
explicatif, avec des exemples — c'est le fichier que la plupart des
utilisateurs éditeront, et lire ses commentaires reste le moyen le
plus rapide d'apprendre le format.

Chaque directive consiste en un mot-clé suivi d'un opérande sur une
seule ligne. Les lignes vides sont ignorées, ainsi que tout ce qui
suit un ``#`` jusqu'à la fin de la ligne. Les mots-clés ne sont pas
sensibles à la casse. Les directives qui reviennent le plus souvent
dans les configurations utilisateur :

.. _configure-braille-driver:

``braille-driver`` *pilote*\ ``,``...|\ ``auto``
  Le pilote d'afficheur braille (voir :ref:`Spécification de pilote
  <operand-driver>`). ``auto`` déclenche l'autodétection, ce qui est
  aussi le comportement par défaut.
  Contournable avec :ref:`-b <options-braille-driver>`.

.. _configure-braille-device:

``braille-device`` *périphérique*\ ``,``...
  Où l'afficheur est connecté (voir :ref:`Spécification du périphérique
  braille <operand-braille-device>`).
  Contournable avec :ref:`-d <options-braille-device>`.

.. _configure-braille-parameters:

``braille-parameters`` [*pilote*\ ``:``]*nom*\ ``=``\ *valeur*\ ``,``...
  Paramètres propres au pilote. Un nom peut être qualifié par un code
  de pilote (voir :ref:`Codes d'identification des pilotes <drivers>`)
  pour qu'un réglage donné ne s'applique qu'à ce pilote ; sinon il
  s'applique à tous. Les paramètres disponibles sont listés dans le
  README de chaque pilote. Contournable avec
  :ref:`-B <options-braille-parameters>`.

.. _configure-text-table:

``text-table`` *fichier*\ \|\ ``auto``
  La :ref:`table de texte <table-text>` (correspondance
  caractère-vers-braille). ``auto`` (le défaut) en sélectionne une
  d'après la locale.
  Contournable avec :ref:`-t <options-text-table>`.

.. _configure-attributes-table:

``attributes-table`` *fichier*
  La :ref:`table d'attributs <table-attributes>` utilisée pour afficher
  l'information d'attributs d'écran.
  Contournable avec :ref:`-a <options-attributes-table>`.

.. _configure-contraction-table:

``contraction-table`` *fichier*
  La :ref:`table de braille abrégé <table-contraction>` utilisée
  lorsque le braille abrégé à 6 points est actif (voir la commande
  :ref:`SIXDOTS <command-SIXDOTS>` et la préférence :ref:`Apparence du
  texte <preference-text-style>`).
  Contournable avec :ref:`-c <options-contraction-table>`.

.. _configure-keyboard-table:

``keyboard-table`` *fichier*\ \|\ ``auto``
  La :ref:`table de touches <table-key>` qui associe les touches du
  clavier à des commandes BRLTTY. Par défaut, aucune n'est utilisée.
  Contournable avec :ref:`-k <options-keyboard-table>`.

.. _configure-keyboard-properties:

``keyboard-properties`` *nom*\ ``=``\ *valeur*\ ``,``...
  Quels claviers BRLTTY doit surveiller (voir :ref:`Propriétés du
  clavier <keyboard-properties>`). Par défaut, tous le sont.
  Contournable avec :ref:`-K <options-keyboard-properties>`.

Pour les directives non couvertes ici — synthèse vocale, écran, MIDI,
PCM, BrlAPI, ``preferences-file``, ``release-device``, et quelques
autres — voir les commentaires de ``Documents/brltty.conf``.

.. _options:

Options en ligne de commande
----------------------------

Les options ci-dessous sont celles que les nouveaux utilisateurs
emploient le plus souvent. Pour la liste complète — y compris les
options propres à une plateforme, avancées ou rarement utilisées —
exécutez ``brltty --help`` ou consultez la page de manuel
``brltty(1)``.

Afficheur, pilote et périphérique :

.. _options-braille-driver:

``-b``\ *pilote*\ ``,``...|\ ``auto`` ``--braille-driver=``\ *pilote*\ ``,``...
  Pilote de l'afficheur braille (voir :ref:`Spécification de pilote
  <operand-driver>`). ``auto`` déclenche la détection automatique.

.. _options-braille-device:

``-d``\ *périphérique*\ ``,``... ``--braille-device=``\ *périphérique*\ ``,``...
  Où l'afficheur est connecté.

.. _options-braille-parameters:

``-B``\ [*pilote*\ ``:``]*nom*\ ``=``\ *valeur*\ ``,``... ``--braille-parameters=``\ ...
  Paramètres propres à un pilote ; voir le README de chaque pilote.

``-s``\ *pilote*\ ``,``...|\ ``auto`` ``--speech-driver=``\ *pilote*\ ``,``...
  Pilote de synthèse vocale, ou ``auto``.

``-x``\ *pilote* ``--screen-driver=``\ *pilote*
  Pilote d'écran. Par défaut : détecté automatiquement.

Tables :

.. _options-text-table:

``-t``\ *fichier* ``--text-table=``\ *fichier*
  :ref:`Table de texte <table-text>`. Par défaut : selon la locale.

.. _options-attributes-table:

``-a``\ *fichier* ``--attributes-table=``\ *fichier*
  :ref:`Table d'attributs <table-attributes>`.

.. _options-contraction-table:

``-c``\ *fichier* ``--contraction-table=``\ *fichier*
  :ref:`Table de braille abrégé <table-contraction>` pour le mode
  six points.

.. _options-keyboard-table:

``-k``\ *fichier* ``--keyboard-table=``\ *fichier*
  :ref:`Table de touches <table-key>` associant les touches du
  clavier à des commandes.

.. _options-keyboard-properties:

``-K``\ *nom*\ ``=``\ *valeur*\ ``,``... ``--keyboard-properties=``\ ...
  Restreint les claviers que BRLTTY surveille (voir :ref:`Propriétés
  du clavier <keyboard-properties>`).

Fichiers et contrôle d'exécution :

``-f``\ *fichier* ``--configuration-file=``\ *fichier*
  Utilise un :ref:`fichier de configuration <configure>` autre que
  celui par défaut.

``-n`` ``--no-daemon``
  Reste au premier plan (ne se détache pas).

``-N`` ``--no-api``
  Désactive le serveur BrlAPI.

.. _options-message-timeout:

``-M``\ *csec* ``--message-timeout=``\ *csec*
  Durée pendant laquelle BRLTTY affiche ses propres messages sur
  l'afficheur, en centièmes de seconde. Par défaut : ``400``
  (4 secondes).

Journalisation et diagnostic :

``-l``\ *niveau* ``--log-level=``\ *niveau*
  Seuil de gravité des messages de journal. ``niveau`` peut être un
  nombre de 0 à 7 (0 = urgence, 7 = débogage) ou le nom (qui peut
  être abrégé). Par défaut : ``information``.

``-q`` ``--quiet``
  Abaisse le niveau de journal par défaut. Avec :ref:`-v
  <options-verify>` ou :ref:`-V <options-version>` il devient
  ``notice`` ; sinon ``warning``.

``-e`` ``--standard-error``
  Envoie les messages de diagnostic sur la sortie d'erreur standard
  plutôt que dans ``syslog``.

.. _options-help:

``-h`` ``--help``
  Affiche le résumé des options et quitte.

.. _options-version:

``-V`` ``--version``
  Affiche les informations de version de BRLTTY, de son API et des
  pilotes liés au binaire, puis quitte. Avec ``-q``, seules les
  versions sont affichées.

.. _options-verify:

``-v`` ``--verify``
  Affiche la valeur résolue de chaque option (après combinaison des
  valeurs par défaut, du fichier de configuration, de la ligne de
  commande et — si :ref:`-E <options-environment-variables>` est
  actif — des variables d'environnement), puis quitte. Lorsque
  plusieurs pilotes ou périphériques sont indiqués, effectue la
  détection automatique dans le cadre de la vérification.

.. _options-environment-variables:

``-E`` ``--environment-variables``
  Pour toute option non spécifiée, prend la valeur par défaut dans
  une variable d'environnement ``BRLTTY_*``. Le nom de la variable
  associée à une option est son nom long, en majuscules, avec les
  tirets remplacés par des soulignés et le préfixe ``BRLTTY_``
  (par exemple, ``--braille-device`` est lu depuis
  ``BRLTTY_BRAILLE_DEVICE``). Utile pour passer des paramètres via
  les paramètres d'amorçage du noyau Linux.

Description des fonctionnalités
===============================


.. _routing:

Rapatriement du curseur
-----------------------

Lorsque vous déplacez la fenêtre braille à l'écran pour examiner un
texte, par exemple dans un éditeur, vous avez souvent besoin d'amener
le curseur sur un caractère précis à l'intérieur de la fenêtre. La
tâche est en pratique délicate, pour plusieurs raisons. Vous ne savez
pas forcément où se trouve le curseur, et vous risquez de perdre votre
position en le cherchant. Le curseur peut aussi se déplacer de façon
imprévisible quand on appuie sur les flèches (certains éditeurs, par
exemple, n'autorisent pas le curseur à dépasser la fin de la ligne
courante). Le rapatriement du curseur fait justement cela : il sait où
se trouve le curseur, simule les appuis sur les flèches que vous
auriez à effectuer manuellement et suit le curseur pendant qu'il se
déplace.

Certains afficheurs braille disposent d'un bouton, dit *touche de
routage* (souvent *routine-curseur* dans la documentation des
fabricants), au-dessus de chaque cellule. Ces touches utilisent la
commande :ref:`ROUTE <command-ROUTE>` pour amener le curseur
directement à l'emplacement voulu.

Le rapatriement du curseur est très pratique et efficace, mais, à
strictement parler, il n'est pas totalement fiable. Sa mise en œuvre
actuelle suppose des séquences d'échappement de touches du curseur
VT100. Certaines applications réagissent en outre de façon non
standard à l'appui sur une touche du curseur. Un problème mineur
rencontré dans certains éditeurs (comme ``vi``), déjà mentionné plus
haut, est qu'ils introduisent des déplacements horizontaux
imprévisibles lorsqu'on demande un déplacement vertical, parce qu'ils
n'autorisent pas le curseur à dépasser la fin d'une ligne. Un
problème majeur rencontré dans certains navigateurs web (comme
``lynx``) est que les flèches haut et bas servent à passer d'un lien
à l'autre (ce qui peut sauter des lignes ou déplacer le curseur
horizontalement, mais déplace rarement le curseur d'une seule ligne
dans la direction voulue), et que les flèches gauche et droite
servent à sélectionner les liens (ce qui n'a strictement rien à voir
avec un déplacement de curseur, et change même complètement le
contenu de l'écran).

Le rapatriement du curseur peut mal se comporter sur un système très
chargé, et fonctionne assurément mal sur un système distant accédé
via une liaison lente, à cause de toutes les vérifications nécessaires
en cours de route pour gérer les déplacements imprévisibles du
curseur et donner à toute erreur une chance d'être annulée. Aussi
malin que tente d'être BRLTTY, il doit pour l'essentiel attendre de
voir ce qui se passe après chaque appui simulé sur une flèche.

Une fois la demande émise, BRLTTY tente d'amener le curseur à la
position voulue jusqu'à ce que l'un des événements suivants survienne :
un délai expire avant l'arrivée à destination, le curseur semble
partir dans la mauvaise direction, ou vous changez de terminal
virtuel. Le déplacement vertical est tenté en premier pour amener le
curseur à la bonne ligne ; s'il réussit, le déplacement horizontal
est tenté ensuite pour atteindre la bonne colonne. Si une nouvelle
demande survient pendant qu'une autre est en cours, la première est
abandonnée et la seconde démarre.

Une commande de rapatriement plus sûre mais moins puissante,
:ref:`CSRJMP_VERT <command-CSRJMP_VERT>`, n'utilise que le déplacement
vertical pour amener le curseur n'importe où sur la première ligne de
la fenêtre braille. Elle est particulièrement utile avec des
applications (comme ``lynx``) où le déplacement horizontal du curseur
ne doit jamais être tenté.


.. _cut:

Copier-coller
-------------

BRLTTY dispose de son propre presse-papier pour récupérer du texte à
l'écran et le réinjecter à la position du curseur — pratique pour les
noms de fichiers longs, les lignes de commande, les adresses
électroniques, les URL et tout ce qu'il est fastidieux de retaper. Le
flux de base se déroule en trois étapes :

#. **Marquer le début de la zone.** Déplacez la fenêtre braille pour
   que le premier caractère soit visible, puis appelez
   :ref:`CLIP_NEW <command-CLIP_NEW>` (démarrer un nouveau
   presse-papier) ou :ref:`CLIP_ADD <command-CLIP_ADD>` (ajouter au
   presse-papier existant) et appuyez sur la touche de routage
   correspondant à ce caractère.

#. **Marquer la fin de la zone.** Déplacez-vous pour que le dernier
   caractère soit visible, puis appelez
   :ref:`COPY_LINE <command-COPY_LINE>` (mode linéaire, le texte
   suit le fil de l'écran) ou :ref:`COPY_RECT <command-COPY_RECT>`
   (rectangulaire) et appuyez sur la touche de routage
   correspondant à ce caractère. Les espaces de fin sont supprimés sur chaque ligne ;
   les caractères de contrôle sont remplacés par des espaces.

#. **Coller.** Placez le curseur à l'endroit voulu et appelez
   :ref:`PASTE <command-PASTE>`. Le presse-papier n'est pas
   consommé : vous pouvez recoller le même contenu autant de fois
   que nécessaire.

Pour les URL, adresses électroniques et noms d'hôtes spécifiquement,
les commandes :ref:`COPY_SMART_NEW <command-COPY_SMART_NEW>` et
:ref:`COPY_SMART_ADD <command-COPY_SMART_ADD>` évitent la danse à
deux marquages : appelez la commande (touches associées + touche
de routage) sur n'importe quel caractère de l'URL ou de
l'adresse, et le détecteur de motifs de BRLTTY récupère l'ensemble.

Deux commandes utilitaires aident à gérer le presse-papier
lui-même : :ref:`CLIP_SHOW <command-CLIP_SHOW>` affiche le contenu
courant sur la ligne braille, et
:ref:`CLIP_CLEAR <command-CLIP_CLEAR>` le vide. Le presse-papier sert
aussi de tampon de chaîne de recherche pour les commandes
:ref:`PRSEARCH/NXSEARCH <command-PRSEARCH-NXSEARCH>`.

Pour transférer du texte dans ou hors du presse-papier depuis un
shell, utilisez la commande hôte
:ref:`brltty-clip <utility-brltty-clip>` — elle dialogue avec le
BRLTTY en cours d'exécution via BrlAPI et lit ou écrit le même
presse-papier que celui manipulé par les commandes clavier.


.. _gpm:

Prise en charge du pointeur (souris) via GPM
--------------------------------------------

Si BRLTTY est compilé avec l'option ``--enable-gpm`` sur un système
où l'application ``gpm`` est installée, il interagit avec le
pointeur (souris).

Déplacer le pointeur entraîne la fenêtre braille (voir la préférence
:ref:`Window Follows Pointer <preference-window-follows-pointer>`).
Chaque fois que le pointeur sort du bord de la fenêtre braille,
celle-ci est entraînée avec lui (un caractère à la fois). Cela donne
au brailliste un autre moyen, en deux dimensions, d'inspecter le
contenu de l'écran ou de déplacer rapidement la fenêtre braille à un
endroit voulu. Cela permet aussi à un observateur voyant de déplacer
facilement la fenêtre braille sur quelque chose qu'il aimerait faire
lire au brailliste.

``gpm`` utilise la vidéo inverse pour montrer où se trouve le
pointeur. Le soulignement des caractères en surbrillance (voir la
commande :ref:`ATTRVIS <command-ATTRVIS>`) devrait donc être activé
lorsque le brailliste souhaite utiliser le pointeur.

Cette fonctionnalité donne aussi au brailliste accès au copier-coller
de ``gpm``. Reportez-vous à la documentation de ``gpm`` pour les
détails ; voici quelques notes sur son fonctionnement.

- Copier le caractère courant dans le tampon de copie : un simple
  clic sur le bouton gauche.
- Copier le mot courant (délimité par des espaces) : double-clic sur
  le bouton gauche.
- Copier la ligne courante : triple-clic sur le bouton gauche.
- Copier une zone linéaire :

  #. Placez le pointeur sur le premier caractère de la zone.
  #. Pressez (et maintenez) le bouton gauche.
  #. Déplacez le pointeur jusqu'au dernier caractère de la zone
     (tous les caractères sélectionnés s'affichent en surbrillance).
  #. Relâchez le bouton gauche.

- Coller (insérer) le contenu courant du tampon de copie : clic sur
  le bouton du milieu d'une souris à trois boutons, ou clic sur le
  bouton droit d'une souris à deux boutons.
- Ajouter au tampon de copie : bouton droit d'une souris à trois
  boutons.


.. _tunes:

Mélodies d'alerte
-----------------

BRLTTY signale les événements significatifs en jouant de courtes
mélodies prédéfinies. Cette fonctionnalité s'active et se désactive
soit par la commande :ref:`TUNES <command-TUNES>`, soit par la
préférence :ref:`Alert Tunes <tunes>`. Les mélodies sont jouées par
défaut sur le bipeur interne, mais d'autres sorties sont disponibles
via la préférence :ref:`Tune Device <preference-tune-device>`.

Chaque événement significatif est associé, par ordre de priorité
décroissant, à un ou plusieurs des éléments suivants :

une mélodie
  Si une mélodie est associée à l'événement, si la préférence
  :ref:`Alert Tunes <tunes>` (voir aussi la commande
  :ref:`TUNES <command-TUNES>`) est active, et si le périphérique de
  son sélectionné (voir la préférence
  :ref:`Tune Device <preference-tune-device>`) peut être ouvert, la
  mélodie est jouée.

un motif de points
  Si un motif de points est associé à l'événement, et si la
  préférence :ref:`Alert Dots <preference-alert-dots>` est active,
  le motif est brièvement affiché sur chaque cellule braille.
  Certains afficheurs ne réagissent pas assez vite pour que ce
  mécanisme fonctionne efficacement.

un message
  Si un message est associé à l'événement, et si la préférence
  :ref:`Alert Messages <preference-alert-messages>` est active, il
  est affiché pendant quelques secondes (voir l'option en ligne de
  commande :ref:`-M <options-message-timeout>`).


Ces événements comprennent :

- Le démarrage ou l'arrêt du pilote d'afficheur braille.
- L'achèvement d'une commande longue.
- L'impossibilité d'exécuter une commande.
- La pose d'un repère.
- La pose du début ou de la fin d'une zone à copier.
- L'activation ou la désactivation d'une fonctionnalité.
- L'activation ou la désactivation de la poursuite du curseur.
- Le gel ou le dégel de l'image de l'écran.
- Le retour à la ligne de la fenêtre braille, vers le début de la
  ligne suivante ou vers la fin de la ligne précédente.
- Le saut de lignes identiques.
- L'impossibilité d'effectuer un déplacement demandé.
- Le démarrage, l'achèvement ou l'échec du rapatriement du curseur.


.. _preferences:

Réglages des préférences
------------------------

Au démarrage, BRLTTY charge un fichier qui contient vos réglages de
préférences. Ce fichier n'a pas besoin d'exister ; il est créé la
première fois que les réglages sont sauvegardés via la commande
:ref:`PREFSAVE <command-PREFSAVE>`. Les réglages sauvegardés le plus
récemment peuvent être rechargés à tout moment par la commande
:ref:`PREFLOAD <command-PREFLOAD>`.

Le fichier s'appelle ``/etc/brltty-``\ *pilote*\ ``.prefs``, où
*pilote* est le :ref:`code d'identification <drivers>` à deux lettres
du pilote.


.. _preferences-menu:

Le menu des préférences
~~~~~~~~~~~~~~~~~~~~~~~

Les préférences sont sauvegardées sous forme de données binaires :
elles ne sont donc pas modifiables à la main — BRLTTY fournit un menu
intégré à la place. Le menu s'active par la commande
:ref:`PREFMENU <command-PREFMENU>` ; appelez-la de nouveau pour
appliquer les nouveaux réglages, sortir du menu et reprendre le
fonctionnement normal. ``PREFLOAD`` annule toutes les modifications
faites depuis l'entrée dans le menu. Voir
:ref:`Menu Navigation Commands <menu-navigation>` pour l'ensemble des
touches qui sélectionnent les éléments et ajustent les réglages ; les
touches de routage permettent aussi de choisir directement un
réglage.

Le menu est auto-documenté et fait foi pour la liste des préférences
disponibles — les éléments non listés ci-dessous se comportent comme
leur nom dans le menu le suggère. Le reste de cette section couvre
les préférences que le manuel cite explicitement, les nouveautés
ajoutées depuis BRLTTY 6.5, et quelques réglages que les nouveaux
utilisateurs ajustent typiquement très tôt.


Préférences sélectionnées
~~~~~~~~~~~~~~~~~~~~~~~~~

.. _preference-text-style:

Apparence du texte
  Afficher le contenu de l'écran avec les huit points (``8-dot``) ou
  seulement les points 1 à 6 (``6-dot``). En mode 6 points, si une
  table de braille abrégé est sélectionnée, le braille abrégé est
  affiché. Modifiable aussi via la commande
  :ref:`SIXDOTS <command-SIXDOTS>`.

.. _preference-skip-identical-lines:

Passer les lignes identiques
  Lors d'un déplacement d'une ligne vers le haut ou vers le bas,
  passer les lignes dont le contenu est identique à la ligne
  courante. Modifiable aussi via la commande
  :ref:`SKPIDLNS <command-SKPIDLNS>`.

.. _preference-sliding-window:

Fenêtre glissante
  Quand la poursuite du curseur ferait sortir le curseur de la
  fenêtre braille, faire glisser la fenêtre pour que le curseur reste
  près du centre, plutôt que de sauter par largeurs de fenêtre
  entières. Modifiable aussi via la commande
  :ref:`SLIDEWIN <command-SLIDEWIN>`.

.. _preference-show-cursor:

Afficher le curseur
  Afficher ou non le curseur de l'écran sur l'afficheur braille.
  Modifiable aussi via la commande :ref:`CSRVIS <command-CSRVIS>`.

.. _preference-cursor-style:

Apparence du curseur
  Représenter le curseur par les huit points (un bloc plein) ou
  seulement les points 7 et 8 (un soulignement). Modifiable aussi
  via la commande :ref:`CSRSIZE <command-CSRSIZE>`.

.. _preference-blinking-cursor:

Clignotement du curseur
  Rendre le curseur alternativement visible et invisible à un rythme
  fixe. Modifiable aussi via la commande
  :ref:`CSRBLINK <command-CSRBLINK>`.

.. _preference-show-attributes:

Afficher les attributs
  Souligner les caractères en surbrillance à l'écran (gras, vidéo
  inverse, etc.). Modifiable aussi via la commande
  :ref:`ATTRVIS <command-ATTRVIS>`.

.. _preference-blinking-attributes:

Clignotement des attributs
  Faire clignoter le soulignement des attributs. Modifiable aussi via
  la commande :ref:`ATTRBLINK <command-ATTRBLINK>`.

.. _preference-blinking-capitals:

Clignotement des majuscules
  Faire clignoter les lettres majuscules pour qu'elles ressortent.
  Modifiable aussi via la commande
  :ref:`CAPBLINK <command-CAPBLINK>`.

Répétition automatique
  Tant que la combinaison de touches d'une commande répétable reste
  enfoncée, répéter la commande à intervalle régulier après un délai
  initial. La fiabilité dépend de ce que le pilote d'afficheur
  braille remonte ou non les événements de relâchement de touche.
  Modifiable aussi via la commande
  :ref:`AUTOREPEAT <command-AUTOREPEAT>`.

.. _preference-window-follows-pointer:

La fenêtre suit le pointeur
  Lorsque le pointeur de la souris se déplace, entraîner la fenêtre
  braille avec lui. Disponible uniquement si la prise en charge de
  GPM a été compilée.

.. _preference-tune-device:

Périphérique pour les sons
  Périphérique audio sur lequel jouer les mélodies d'alerte : bipeur
  interne, PCM (interface audio numérique de la carte son), MIDI ou
  synthèse FM. Des sous-réglages de volume distincts sont disponibles
  pour PCM, MIDI et FM. La valeur par défaut est ``Beeper`` lorsqu'il
  est pris en charge, sinon ``PCM``.

.. _preference-alert-dots:

Points d'avertissement
  Lorsqu'un événement significatif a un motif de points associé
  (voir :ref:`Alert Tunes <tunes>`), afficher brièvement le motif
  sur chaque cellule braille. Un réglage distinct *Alert Dots
  Duration* (ajouté après BRLTTY 6.5 ; valeur par défaut : 0,4 s)
  contrôle la durée d'affichage du motif — utile pour les afficheurs
  dont les actuateurs réagissent lentement. Supprimé si une mélodie
  se déclenche pour le même événement.

.. _preference-alert-messages:

Messages d'avertissement
  Lorsqu'un événement significatif a un message associé (voir
  :ref:`Alert Tunes <tunes>`), l'afficher sur l'afficheur braille
  pendant quelques secondes (voir l'option en ligne de commande
  :ref:`-M <options-message-timeout>`). Supprimé si une mélodie ou
  des points d'avertissement se déclenchent pour le même événement.

.. _preference-sayline-mode:

Mode dire-la-ligne
  À l'exécution de la commande :ref:`SAY_LINE <command-SAY_LINE>`,
  soit abandonner la synthèse en cours (``Immediate``, valeur par
  défaut), soit mettre la nouvelle ligne en file derrière (``Enqueue``).

Lecture automatique
  Quand cette préférence est activée, énoncer automatiquement la
  nouvelle ligne lors d'un déplacement vertical de la fenêtre
  braille, les caractères saisis ou supprimés, et le caractère sur
  lequel le curseur se déplace. Modifiable aussi via la commande
  :ref:`AUTOSPEAK <command-AUTOSPEAK>`.

.. _preference-speech-rate:

Vitesse de la synthèse
  Régler le débit de la synthèse vocale (``0`` le plus lent,
  ``20`` le plus rapide). Dépendant du pilote ; modifiable aussi via
  les commandes
  :ref:`SAY_SLOWER/SAY_FASTER <command-SAY_SLOWER-SAY_FASTER>`.

.. _preference-speech-volume:

Volume de la synthèse
  Régler le volume de la synthèse vocale (``0`` le plus faible,
  ``20`` le plus fort). Dépendant du pilote ; modifiable aussi via
  les commandes
  :ref:`SAY_SOFTER/SAY_LOUDER <command-SAY_SOFTER-SAY_LOUDER>`.

.. _preference-punctuation-level:

Niveau de ponctuation
  Quantité de ponctuation que la synthèse vocale énonce à voix
  haute. Les niveaux disponibles sont ``None``, ``Some``, ``Most`` et
  ``All`` — le niveau ``Most`` (ajouté après BRLTTY 6.5) se situe
  entre ``Some`` et ``All`` pour les utilisateurs qui veulent
  l'essentiel de la ponctuation mais trouvent ``All`` trop bavard.
  Dépendant du pilote.

Énoncer les lignes vides
  Lorsque réglé sur ``Yes`` (valeur par défaut depuis son ajout après
  BRLTTY 6.5), annoncer quand le curseur arrive sur une ligne vide
  pour que le saut de ligne soit audible. Réglez sur ``No`` pour
  passer les lignes vides en silence.

.. _preference-text-table:

Table de texte
  Sélectionner la table de texte à l'exécution. Voir
  :ref:`Text Tables <table-text>` et l'option en ligne de commande
  :ref:`-t <options-text-table>`. Cette préférence n'est pas
  sauvegardée.

.. _preference-attributes-table:

Table d'attributs
  Sélectionner la table d'attributs à l'exécution. Voir
  :ref:`Attributes Tables <table-attributes>` et l'option en ligne
  de commande :ref:`-a <options-attributes-table>`. Cette préférence
  n'est pas sauvegardée.

Table de braille abrégé
  Sélectionner la table de braille abrégé à l'exécution. Voir
  :ref:`Contraction Tables <table-contraction>` et l'option en ligne
  de commande :ref:`-c <options-contraction-table>`. Cette préférence
  n'est pas sauvegardée.

.. _preference-keyboard-table:

Table de touches du clavier
  Sélectionner la table de touches à l'exécution. Voir
  :ref:`Key Tables <table-key>` et l'option en ligne de commande
  :ref:`-k <options-keyboard-table>`. Cette préférence n'est pas
  sauvegardée.


.. _status:

L'affichage d'état
------------------

L'affichage d'état est un résumé de l'état courant de BRLTTY qui
tient entièrement dans la fenêtre braille. Certains afficheurs
braille disposent d'un jeu de cellules d'état qui affichent en
permanence une partie de ces informations (voir la documentation du
pilote de votre afficheur). Les données présentées ne sont pas
statiques et peuvent changer à tout moment, en réaction aux mises à
jour de l'écran ou aux commandes de BRLTTY.

Utilisez la commande :ref:`INFO <command-INFO>` pour passer à
l'affichage d'état, et de nouveau pour revenir à l'écran. La
disposition des informations dépend de la taille de la fenêtre
braille.


Afficheurs de 21 cellules ou plus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Des abréviations courtes ont été employées, bien qu'elles soient un
peu cryptiques, afin de respecter la disposition exacte en colonnes.

.. parsed-literal::

  *wx*:*wy* *cx*:*cy* *vt* *tcmfdu*

*wx*\ ``:``\ *wy*
  La colonne et la ligne (à partir de 1) sur l'écran qui
  correspondent au coin supérieur gauche de la fenêtre braille.

*cx*\ ``:``\ *cy*
  La colonne et la ligne (à partir de 1) sur l'écran qui
  correspondent à la position du curseur.

*vt*
  Le numéro (à partir de 1) du terminal virtuel courant.

*t*
  L'état de la fonction de poursuite du curseur (voir la commande
  :ref:`CSRTRK <command-CSRTRK>`).

  vide
    Poursuite du curseur inactive.

  ``t``
    Poursuite du curseur active.

*c*
  L'état des fonctions de visibilité du curseur (voir les commandes
  :ref:`CSRVIS <command-CSRVIS>` et
  :ref:`CSRBLINK <command-CSRBLINK>`).

  vide
    Le curseur n'est pas visible et ne clignotera pas quand il le
    deviendra.

  ``b``
    Le curseur n'est pas visible, et clignotera quand il le
    deviendra.

  ``v``
    Le curseur est visible et ne clignote pas.

  ``B``
    Le curseur est visible et clignote.

*m*
  Le mode d'affichage courant (voir la commande
  :ref:`DISPMD <command-DISPMD>`).

  ``t``
    Le contenu de l'écran (texte) est affiché.

  ``a``
    La surbrillance de l'écran (les attributs) est affichée.

*f*
  L'état de la fonction de gel d'écran (voir la commande
  :ref:`FREEZE <command-FREEZE>`).

  vide
    L'écran n'est pas gelé.

  ``f``
    L'écran est gelé.

*d*
  Le nombre de points braille utilisés pour afficher chaque
  caractère (voir la commande :ref:`SIXDOTS <command-SIXDOTS>`).

  ``8``
    Les huit points sont utilisés.

  ``6``
    Seuls les points 1 à 6 sont utilisés.

*u*
  L'état des fonctions d'affichage des majuscules (voir la commande
  :ref:`CAPBLINK <command-CAPBLINK>`).

  vide
    Les majuscules ne clignotent pas.

  ``B``
    Les majuscules clignotent.


Afficheurs de 20 cellules ou moins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Des abréviations courtes ont été employées, bien qu'elles soient un
peu cryptiques, afin de respecter la disposition exacte en colonnes.

.. parsed-literal::

  *xx**yy**s* *vt* *tcmfdu*

*xx*
  Les colonnes (à partir de 1) sur l'écran qui correspondent à la
  position du curseur (dans la moitié supérieure des cellules) et au
  coin supérieur gauche de la fenêtre braille (dans la moitié
  inférieure des cellules).

*yy*
  Les lignes (à partir de 1) sur l'écran qui correspondent à la
  position du curseur (dans la moitié supérieure des cellules) et au
  coin supérieur gauche de la fenêtre braille (dans la moitié
  inférieure des cellules).

*s*
  Les réglages de certaines fonctions de BRLTTY. Une fonction est
  active si le point qui lui correspond est levé.

  Point 1
    Image d'écran gelée (voir la commande
    :ref:`FREEZE <command-FREEZE>`).

  Point 2
    Affichage des attributs (voir la commande
    :ref:`DISPMD <command-DISPMD>`).

  Point 3
    Mélodies d'alerte (voir la commande
    :ref:`TUNES <command-TUNES>`).

  Point 4
    Curseur visible (voir la commande
    :ref:`CSRVIS <command-CSRVIS>`).

  Point 5
    Curseur en bloc (voir la commande
    :ref:`CSRSIZE <command-CSRSIZE>`).

  Point 6
    Curseur clignotant (voir la commande
    :ref:`CSRBLINK <command-CSRBLINK>`).

  Point 7
    Poursuite du curseur (voir la commande
    :ref:`CSRTRK <command-CSRTRK>`).

  Point 8
    Fenêtre glissante (voir la commande
    :ref:`SLIDEWIN <command-SLIDEWIN>`).

*vt*
  Le numéro (à partir de 1) du terminal virtuel courant.

*t*
  L'état de la fonction de poursuite du curseur (voir la commande
  :ref:`CSRTRK <command-CSRTRK>`).

  vide
    Poursuite du curseur inactive.

  ``t``
    Poursuite du curseur active.

*c*
  L'état des fonctions de visibilité du curseur (voir les commandes
  :ref:`CSRVIS <command-CSRVIS>` et
  :ref:`CSRBLINK <command-CSRBLINK>`).

  vide
    Le curseur n'est pas visible et ne clignotera pas quand il le
    deviendra.

  ``b``
    Le curseur n'est pas visible, et clignotera quand il le
    deviendra.

  ``v``
    Le curseur est visible et ne clignote pas.

  ``B``
    Le curseur est visible et clignote.

*m*
  Le mode d'affichage courant (voir la commande
  :ref:`DISPMD <command-DISPMD>`).

  ``t``
    Le contenu de l'écran (texte) est affiché.

  ``a``
    La surbrillance de l'écran (les attributs) est affichée.

*f*
  L'état de la fonction de gel d'écran (voir la commande
  :ref:`FREEZE <command-FREEZE>`).

  vide
    L'écran n'est pas gelé.

  ``f``
    L'écran est gelé.

*d*
  Le nombre de points braille utilisés pour afficher chaque
  caractère (voir la commande :ref:`SIXDOTS <command-SIXDOTS>`).

  ``8``
    Les huit points sont utilisés.

  ``6``
    Seuls les points 1 à 6 sont utilisés.

*u*
  L'état des fonctions d'affichage des majuscules (voir la commande
  :ref:`CAPBLINK <command-CAPBLINK>`).

  vide
    Les majuscules ne clignotent pas.

  ``B``
    Les majuscules clignotent.


.. _learn:

Mode apprentissage des commandes
--------------------------------

Le mode apprentissage des commandes est une façon interactive
d'apprendre ce que font les touches de l'afficheur braille. On y
accède soit par la commande :ref:`LEARN <command-LEARN>`, soit via
l'utilitaire ``brltest``. Cette fonctionnalité n'est pas disponible
si l'option de compilation --disable-learn-mode a été utilisée.

À l'entrée dans ce mode, le message ``command learn mode`` est écrit
sur l'afficheur braille. Ensuite, à chaque appui sur une touche (ou
combinaison de touches) de l'afficheur, un court message décrivant la
fonction BRLTTY associée est écrit. Le mode se quitte immédiatement
si la touche (ou combinaison) de la commande
:ref:`LEARN <command-LEARN>` est pressée. Il se termine
automatiquement, et le message ``done`` est écrit, si dix secondes
s'écoulent sans qu'aucune touche de l'afficheur ne soit pressée.
Notez que certains afficheurs ne signalent pas l'événement au pilote,
ou que certains pilotes ne le signalent pas à BRLTTY, avant que
toutes les touches ne soient relâchées.

Si un message est plus long que la largeur de l'afficheur braille,
il est découpé en segments. Chaque segment, sauf le dernier, mesure
un caractère de moins que la largeur de l'afficheur, le caractère le
plus à droite étant un signe moins. Chaque segment reste à l'écran
soit pendant quelques secondes (voir l'option en ligne de commande
:ref:`-M <options-message-timeout>`), soit jusqu'à ce qu'une touche
de l'afficheur soit pressée.

.. _tables:

Tables
======

Le comportement de BRLTTY se règle au moyen de quatre familles de
tables, toutes au format texte simple. Elles partagent la même
syntaxe de base — une directive par ligne, encodage ``UTF-8``, lignes
vides et lignes commençant par ``#`` ignorées — et peuvent être
réparties dans des sous-tables (``*.tti``, ``*.ati``, ``*.cti``,
``*.kti``) tirées au moyen d'une directive ``include``. Ce chapitre
explique à quoi sert chaque famille et comment en sélectionner une à
l'exécution. Pour la syntaxe exacte des directives — la référence
nécessaire pour écrire ou modifier une table — reportez-vous au
fichier README spécifique à chaque famille, indiqué dans la section
correspondante.


.. _table-text:

Tables de texte
---------------

Les fichiers nommés ``*.ttb`` sont des tables de texte. Elles
indiquent à BRLTTY comment traduire chaque caractère affiché à
l'écran en sa configuration braille informatique correspondante à
huit points. La correspondance entre caractères et points varie d'une
langue à l'autre (et même entre conventions au sein d'une même
langue) ; BRLTTY fournit donc une table de texte par locale.

La table de texte est sélectionnée automatiquement au démarrage en
fonction de votre locale, avec repli sur la table
:ref:`North American Braille Computer Code <nabcc>` (NABCC) à défaut
de meilleure correspondance. Pour en choisir une explicitement,
utilisez l'option en ligne de commande
:ref:`-t <options-text-table>`, la directive
:ref:`text-table <configure-text-table>` du fichier de configuration,
ou la préférence Text Table.

Les tables de texte fournies sont les suivantes :

.. csv-table::
   :header-rows: 1
   :file: ../../text-table.csv

Voir ``Documents/README.TextTables`` pour le format de fichier des
tables de texte et la référence des directives.


.. _table-attributes:

Tables d'attributs
------------------

Les fichiers nommés ``*.atb`` sont des tables d'attributs. Au lieu
d'afficher le texte présent à l'écran, elles permettent d'en afficher
les attributs *visuels* — couleur de premier plan et d'arrière-plan,
intensité, clignotement — sous forme de configurations de points
braille. Le mode attributs s'active et se désactive avec la commande
:ref:`DISPMD <command-DISPMD>` ; une table d'attributs détermine la
façon dont les huit bits d'attribut ``VGA`` se traduisent en huit
points d'une cellule braille.

Les tables d'attributs fournies avec BRLTTY sont les suivantes :

.. csv-table::
   :header-rows: 1
   :file: ../../attributes-table.csv

Pour en choisir une, utilisez l'option en ligne de commande
:ref:`-a <options-attributes-table>`, la directive
:ref:`attributes-table <configure-attributes-table>` du fichier de
configuration, ou la préférence Attributes Table.

Voir ``Documents/README.AttributesTables`` pour le format de fichier
des tables d'attributs et la référence des directives.


.. _table-contraction:

Tables de braille abrégé
------------------------

Les fichiers nommés ``*.ctb`` sont des tables de braille abrégé. Là
où une table de texte associe un caractère à une cellule braille, une
table de braille abrégé encode les conventions d'abréviation propres
au braille littéraire : séquences de lettres courantes, mots entiers
et motifs de ponctuation se traduisent chacun par des séquences de
cellules plus courtes. C'est ce qui permet de tenir bien plus de
texte sur un afficheur de petite taille. Cette abréviation porte
historiquement le nom de braille « grade 2 » (par opposition au
braille « grade 1 », non abrégé) ; les règles diffèrent selon la
langue, si bien que les tables de braille abrégé, comme les tables de
texte, sont fournies à raison d'une par locale.

Le braille abrégé est affiché lorsque les deux conditions suivantes
sont réunies :

- une table de braille abrégé a été sélectionnée — voir l'option en
  ligne de commande :ref:`-c <options-contraction-table>` et la
  directive
  :ref:`contraction-table <configure-contraction-table>` du fichier
  de configuration, et
- le mode braille à six points est actif — basculez-le avec la
  commande :ref:`SIXDOTS <command-SIXDOTS>`, ou définissez l'état
  initial via la préférence
  :ref:`Text Style <preference-text-style>`.

La prise en charge du braille abrégé n'est pas compilée si l'option
de compilation ``--disable-contracted-braille`` a été utilisée.

Les tables de braille abrégé fournies sont les suivantes :

.. csv-table::
   :header-rows: 1
   :file: ../../contraction-table.csv

Voir ``Documents/README.ContractionTables`` pour le format de fichier
des tables de braille abrégé, la référence des opcodes et la
mécanique des classes de caractères.


.. _table-key:

Tables de touches
-----------------

Les fichiers nommés ``*.ktb`` sont des tables de touches. Elles
associent les touches physiques — sur un afficheur braille, sur un
clavier standard, ou sur les deux — à des commandes BRLTTY. La
plupart des utilisateurs n'ont jamais à se préoccuper des tables de
touches : chaque pilote d'afficheur braille est livré avec des
attributions par défaut sensées pour chaque modèle pris en charge,
si bien que le branchement d'un afficheur fonctionne « du premier
coup ». Une table de touches devient intéressante si vous souhaitez
réattribuer des touches, adapter un afficheur récent à un pilote plus
ancien, ou utiliser votre clavier d'ordinateur en saisie braille.

Deux conventions de nommage coexistent :

- **Les tables de touches d'afficheur braille** ont des noms de la
  forme ``brl-``\ *xx*\ ``-``\ *modèle*\ ``.ktb``, où *xx* est le
  :ref:`code d'identification de pilote <drivers>` à deux lettres et
  *modèle* identifie l'afficheur.

- **Les tables de clavier** ont des noms de la forme
  ``kbd-``\ *type*\ ``.ktb`` et pilotent le clavier d'ordinateur
  ordinaire lorsque BRLTTY le surveille.

Les tables de clavier fournies avec BRLTTY sont les suivantes :

.. csv-table::
   :header-rows: 1
   :file: ../../keyboard-table.csv

Pour spécifier une table de clavier, utilisez l'option en ligne de
commande :ref:`-k <options-keyboard-table>`, la directive
:ref:`keyboard-table <configure-keyboard-table>` du fichier de
configuration, ou la préférence
:ref:`Keyboard Table <preference-keyboard-table>`.

Voir ``Documents/README.KeyTables`` pour le format de fichier des
tables de touches et la référence complète des directives (``bind``,
``context``, ``hotkey``, ``map``, ``hide``, ``superimpose``,
``assign``, ``ifkey``, ``ignore``, ``include``, ``note``, ``title``).


.. _keyboard-properties:

Propriétés du clavier
~~~~~~~~~~~~~~~~~~~~~

Par défaut, BRLTTY surveille tous les claviers qu'il détecte. Pour
restreindre cette liste — utile lorsque plusieurs claviers sont
connectés et qu'un seul doit servir à la saisie braille —
spécifiez une ou plusieurs des propriétés suivantes via l'option en
ligne de commande :ref:`-K <options-keyboard-properties>` ou la
directive
:ref:`keyboard-properties <configure-keyboard-properties>` du
fichier de configuration :

type
  Le type de bus : l'un de ``any``, ``ps2``, ``usb``,
  ``bluetooth``.

vendor
  L'identifiant USB du fabricant, un entier non signé sur 16 bits.

product
  L'identifiant USB du produit, un entier non signé sur 16 bits.

Les identifiants du fabricant et du produit peuvent être donnés en
décimal (sans préfixe), octal (préfixé par ``0``) ou hexadécimal
(préfixé par ``0x``). La valeur ``0`` signifie « correspond à toute
valeur », équivalent à ne pas spécifier la propriété du tout.

.. _getting-help:

Obtenir de l'aide
=================


Liste de diffusion
------------------

Le moyen le plus rapide d'obtenir une réponse à une question est la
liste de diffusion du projet. Envoyez votre message à
`<BRLTTY@brltty.app> <mailto:BRLTTY@brltty.app>`_ ; si vous n'êtes
pas abonné, votre message sera brièvement retenu pour approbation par
le modérateur. Pour vous abonner, parcourir les archives ou modifier
vos paramètres, rendez-vous sur
`http://brltty.app/mailman/listinfo/brltty
<http://brltty.app/mailman/listinfo/brltty>`_.


Rapports de bogues et demandes de fonctionnalités
-------------------------------------------------

Le suivi des problèmes se trouve sur GitHub :
`https://github.com/brltty/brltty/issues
<https://github.com/brltty/brltty/issues>`_. Un rapport de bogue utile
contient :

* La version de BRLTTY (``brltty -V``).
* La marque et le modèle de l'afficheur braille.
* Le système d'exploitation hôte et sa version.
* Le fichier de configuration utilisé, s'il diffère de celui par défaut.
* Les traces pertinentes. Relancer BRLTTY avec ``-n -e -l debug`` le
  maintient au premier plan et émet des diagnostics détaillés sur la
  sortie d'erreur — collez la portion qui entoure l'incident.


Lecture de la documentation des sources
---------------------------------------

Pour les questions d'implémentation ou le comportement spécifique d'un
pilote, l'arborescence des sources contient une famille de README
thématiques dans ``Documents/`` : ``Bluetooth``, ``Devices``,
``Customize``, ``Profiles``, ``Polling``, ``Systemd``, ``X11``,
``CommandReference``, ``TextTables``, ``AttributesTables``,
``ContractionTables``, ``KeyTables``, ``BrailleDots``, et d'autres.
``Documents/brltty.conf`` est le modèle de configuration largement
commenté — la référence la plus rapide pour la syntaxe de toute
directive. Le manuel de BrlAPI couvre séparément l'interface de
programmation.

Afficheurs braille pris en charge
=================================

BRLTTY prend en charge les afficheurs braille suivants :

.. csv-table::
   :header-rows: 1
   :file: ../../braille-driver.csv

.. _synthesizers:

Synthétiseurs vocaux pris en charge
===================================

BRLTTY prend en charge les synthétiseurs vocaux suivants :

.. csv-table::
   :header-rows: 1
   :file: ../../speech-driver.csv

.. _drivers:

Codes d'identification des pilotes
==================================


.. csv-table::
   :header-rows: 1
   :file: ../../driver-code.csv

Pilotes d'écran pris en charge
==============================

BRLTTY prend en charge les pilotes d'écran suivants :

.. csv-table::
   :header-rows: 1
   :file: ../../screen-driver.csv

Le pilote natif de votre système d'exploitation (``lx`` sous Linux,
``hd`` sous Hurd, ``wn`` sous Windows, ``an`` sous Android) est
sélectionné automatiquement. Le pilote ``sc`` donne accès aux
sessions ``screen`` et sert de solution de repli par défaut sur les
systèmes dépourvus de pilote natif — ``screen`` doit être patché
(voir le sous-répertoire ``Patches``) et en cours d'exécution. Le
pilote ``tx``, utilisé par
:ref:`brltty-tmux <utility-brltty-tmux>`, surveille une session tmux.

Syntaxe des opérandes
=====================


.. _operand-driver:

Spécification de pilote
-----------------------

Un pilote d'afficheur braille ou de synthèse vocale se spécifie par
son :ref:`code d'identification <drivers>` à deux lettres.

Vous pouvez fournir une liste de pilotes séparés par des virgules.
La détection automatique essaie alors chaque pilote listé tour à tour.
Quelques essais peuvent être nécessaires pour trouver l'ordre le plus
fiable, certains pilotes se prêtant mieux que d'autres à
l'autodétection.

Si vous indiquez le seul mot ``auto``, la détection automatique
n'utilise que les pilotes connus comme fiables à cet usage.


.. _operand-braille-device:

Spécification du périphérique braille
-------------------------------------

La forme générale d'une spécification de périphérique braille (voir
l'option de ligne de commande :ref:`-d <options-braille-device>`
et la directive :ref:`braille-device <configure-braille-device>` du
fichier de configuration) est ``qualificateur:``\ *donnée*. Pour
compatibilité avec les versions antérieures, ``serial:`` est supposé
si le qualificateur est omis.

Les types de périphérique pris en charge sont :

Bluetooth
  Pour un périphérique Bluetooth, spécifiez ``bluetooth:``\ *adresse*.
  L'adresse se compose de six nombres hexadécimaux à deux chiffres
  séparés par des deux-points, par exemple ``01:23:45:67:89:AB``.

Série
  Pour un périphérique série, spécifiez ``serial:``\ */chemin/vers/périphérique*.
  Le qualificateur ``serial:`` est facultatif (pour compatibilité).
  Un chemin relatif est ancré sur ``/dev`` (l'emplacement habituel
  des périphériques sur un système de type Unix). Les spécifications
  suivantes désignent toutes le premier port série sur Linux :

  - ``serial:/dev/ttyS0``
  - ``serial:ttyS0``
  - ``/dev/ttyS0``
  - ``ttyS0``

USB
  Pour un périphérique USB, spécifiez ``usb:``. BRLTTY cherche alors
  le premier périphérique USB correspondant au pilote d'afficheur
  braille utilisé. Si cela ne suffit pas, par exemple si plusieurs
  afficheurs braille USB partagent le même pilote, vous pouvez
  affiner la spécification en y ajoutant le numéro de série de
  l'afficheur, par exemple ``usb:12345``. N.B. : l'identification par
  numéro de série ne fonctionne pas pour tous les modèles, certains
  fabricants ne renseignant pas le descripteur USB de numéro de
  série, ou le renseignant sans valeur unique.


Vous pouvez aussi fournir une liste de périphériques braille séparés
par des virgules. La détection automatique essaie alors chaque
périphérique listé tour à tour. C'est particulièrement utile pour un
afficheur braille doté de plusieurs interfaces, par exemple à la fois
un port série et un port USB. Dans ce cas, il vaut généralement mieux
lister le port USB en premier, par exemple ``usb:,serial:/dev/ttyS0``,
le premier se détectant en général plus fiablement que le second.


.. _operand-pcm-device:

Spécification du périphérique PCM
---------------------------------

Dans la plupart des cas, le périphérique PCM est le chemin complet
vers un périphérique système approprié. Exceptions :

ALSA
  Le nom du périphérique physique ou logique, suivi le cas échéant
  de ses arguments, soit *nom*\ [``:``\ *argument*\ ``,``\ ...].


Le périphérique PCM par défaut est :

.. list-table::
   :header-rows: 1

   * - Plateforme
     - Périphérique

   * - FreeBSD
     - /dev/dsp

   * - Linux/ALSA
     - hw:0,0

   * - Linux/OSS
     - /dev/dsp

   * - NetBSD
     - /dev/audio

   * - OpenBSD
     - /dev/audio

   * - Qnx
     - le périphérique de sortie PCM préféré

   * - Solaris
     - /dev/audio


.. _operand-midi-device:

Spécification du périphérique MIDI
----------------------------------

Dans la plupart des cas, le périphérique MIDI est le chemin complet
vers un périphérique système approprié. Exceptions :

ALSA
  Le client et le port séparés par un deux-points, soit
  *client*\ ``:``\ *port*. Chacun se spécifie soit par un nombre,
  soit par une sous-chaîne sensible à la casse de son nom.


Le périphérique MIDI par défaut est :

.. list-table::
   :header-rows: 1

   * - Plateforme
     - Périphérique

   * - Linux/ALSA
     - le premier port de sortie MIDI disponible

   * - Linux/OSS
     - /dev/sequencer

.. _dots:

Numérotation standard des points braille
========================================

Une cellule braille standard contient six points répartis en trois lignes
et deux colonnes :

- Point 1 : en haut à gauche
- Point 2 : au milieu à gauche
- Point 3 : en bas à gauche
- Point 4 : en haut à droite
- Point 5 : au milieu à droite
- Point 6 : en bas à droite

Le braille informatique ajoute une quatrième ligne en bas :

- Point 7 : en dessous à gauche
- Point 8 : en dessous à droite

Voir ``Documents/README.BrailleDots`` pour un schéma et davantage d'explications.

.. _nabcc:

Tables braille de référence
===========================

Cette section regroupe deux tables de référence : le NABCC, table de
texte par défaut de BRLTTY, et la *Table française unifiée*, codage
historique conservé pour les utilisateurs francophones.


North American Braille Computer Code (NABCC)
--------------------------------------------

La table de texte par défaut de BRLTTY met en œuvre le *North American
Braille Computer Code* (NABCC), un codage à huit points qui étend le
braille ASCII à six points pour couvrir l'ensemble du jeu Latin-1. La
correspondance caractère/points faisant foi se trouve dans le fichier
``Tables/Text/en-nabcc.tti`` à la racine de l'arborescence des sources
de BRLTTY.


Table française unifiée
-----------------------


::

     Num Hex  Points     Description

   +-------------------------------------------------------------------------+
   |  0  00   [ 32145 8] code nul                                            |
   |  1  01   [73214 68] début de lecture                                    |
   |  2  02   [  21 5 8] début de texte                                      |
   |  3  03   [ 3214 68] fin de texte                                        |
   |  4  04   [7  145 8] fin de transmission                                 |
   |  5  05   [   1 5 8] ENQUIRY                                             |
   |  6  06   [7 214  8] Connaissance                                        |
   |  7  07   [  2145 8] son (bell)                                          |
   |  8  08   [7 21 5 8] effacement                                          |
   |  9  09   [7 214 68] tabulation                                          |
   |  10 0A   [  2 4568] LINE FEED (LF)                                      |
   |  11 0B   [ 3 1   8] tabulation                                          |
   |  12 0C   [7321   8] pied FORM FEED (FF)                                 |
   |  13 0D   [73 14  8] retour chariot                                      |
   |  14 0E   [ 3 145 8] SHIFT OUT                                           |
   |  15 0F   [ 32   68] SHIFT IN                                            |
   |  16 10   [73214  8] échapper lien noir                                  |
   |  17 11   [732145 8] contrôle de périphérique 1                          |
   |  18 12   [7321 5 8] contrôle de périphérique 2                          |
   |  19 13   [732 4  8] contrôle de périphérique 3                          |
   |  20 14   [732 45 8] contrôle de périphérique 4                          |
   |  21 15   [73 1 6 8] Connaissance négative                               |
   |  22 16   [7321  68] [SYNCHRONOUS IDLE]                                  |
   |  23 17   [7 2 4568] fin du bloc de transmission                         |
   |  24 18   [73 14 68] annule                                              |                                             |
   |  25 19   [ 321  68] fin du milieu                                       |
   |  26 1A   [7  1 568] remplacer                                           |
   |  27 1B   [7 21 568] échappement                                         |
   |  28 1C   [7  14 68] séparateur 4                                        |
   |  29 1D   [7 21  68] séparateur 3                                        |
   |  30 1E   [732  5  ] séparateur 2                                        |
   |  31 1F   [732   6 ] flèche droite                                       |
   |  32 20   [        ] espace                                              |
   |  33 21   [ 32  5  ] point d'exclamation                                 |
   |  34 22   [ 32  56 ] guillemet                                           |
   |  35 23   [ 3  4568] dièse                                               |
   |  36 24   [73   5  ] dollar                                              |
   |  37 25   [ 3  4 68] pour cent                                           |
   |  38 26   [ 3214568] e commercial                                        |
   |  39 27   [ 3      ] apostrophe                                          |
   |  40 28   [ 32   6 ] parenthèse ouvrant                                  |
   |  41 29   [ 3   56 ] parenthèse fermant                                  |
   |  42 2A   [ 3   5  ] astérisque (étoile)                                 |
   |  43 2B   [732  5 8] plus                                                |
   |  44 2C   [  2     ] virgule                                             |                                       |
   |  45 2D   [ 3    6 ] tiret                                               |
   |  46 2E   [  2  56 ] point                                               |
   |  47 2F   [ 3  4   ] barre oblique                                       |
   |  48 30   [ 3  456 ] zéro                                                |
   |  49 31   [   1  6 ] un                                                  |
   |  50 32   [  21  6 ] deux                                                |
   |  51 33   [   14 6 ] trois                                               |
   |  52 34   [   1456 ] quatre                                              |
   |  53 35   [   1 56 ] cinq                                                |
   |  54 36   [  214 6 ] six                                                 |
   |  55 37   [  21456 ] sept                                                |
   |  56 38   [  21 56 ] huit                                                |
   |  57 39   [  2 4 6 ] neuf                                                |
   |  58 3A   [  2  5  ] deux-points                                         |
   |  59 3B   [ 32     ] inférieur                                           |
   |  60 3C   [ 32    8] inférieur                                           |
   |  61 3D   [732  568] égal                                                |
   |  62 3E   [7    56 ] supérieur                                           |
   |  63 3F   [  2   6 ] point d'interrogation                               |
   |  64 40   [ 3  45  ] arrobas                                             |
   |  65 41   [7  1    ] a majuscule                                         |
   |  66 42   [7 21    ] b majuscule                                         |
   |  67 43   [7  14   ] c majuscule                                         |
   |  68 44   [7  145  ] d majuscule                                         |
   |  69 45   [7  1 5  ] e majuscule                                         |
   |  70 46   [7 214   ] f majuscule                                         |
   |  71 47   [7 2145  ] g majuscule                                         |
   |  72 48   [7 21 5  ] h majuscule                                         |
   |  73 49   [7 2 4   ] i majuscule                                         |
   |  74 4A   [7 2 45  ] j majuscule                                         |
   |  75 4B   [73 1    ] k majuscule                                         |
   |  76 4C   [7321    ] l majuscule                                         |
   |  77 4D   [73 14   ] m majuscule                                         |
   |  78 4E   [73 145  ] n majuscule                                         |
   |  79 4F   [73 1 5  ] o majuscule                                         |
   |  80 50   [73214   ] p majuscule                                         |
   |  81 51   [732145  ] q majuscule                                         |
   |  82 52   [7321 5  ] r majuscule                                         |
   |  83 53   [732 4   ] s majuscule                                         |
   |  84 54   [732 45  ] t majuscule                                         |
   |  85 55   [73 1  6 ] u majuscule                                         |
   |  86 56   [7321  6 ] v majuscule                                         |
   |  87 57   [7 2 456 ] w majuscule                                         |
   |  88 58   [73 14 6 ] x majuscule                                         |
   |  89 59   [73 1456 ] y majuscule                                         |
   |  90 5A   [73 1 56 ] z majuscule                                         |
   |  91 5B   [732   68] crochet ouvrant                                     |
   |  92 5C   [ 3  4  8] barre oblique inversée                              |
   |  93 5D   [73   568] crochet fermant                                     |
   |  94 5E   [    4   ] accent circonflexe                                  |
   |  95 5F   [7    5 8] souligné                                            |
   |  96 60   [      6 ] accent grave                                        |
   |  97 61   [   1    ] a minuscule                                         |
   |  98 62   [  21    ] b minuscule                                         |
   |  99 63   [   14   ] c minuscule                                         |
   | 100 64   [   145  ] d minuscule                                         |
   | 101 65   [   1 5  ] e minuscule                                         |
   | 102 66   [  214   ] f minuscule                                         |
   | 103 67   [  2145  ] g minuscule                                         |
   | 104 68   [  21 5  ] h minuscule                                         |
   | 105 69   [  2 4   ] i minuscule                                         |
   | 106 6A   [  2 45  ] j minuscule                                         |
   | 107 6B   [ 3 1    ] k minuscule                                         |
   | 108 6C   [ 321    ] l minuscule                                         |
   | 109 6D   [ 3 14   ] m minuscule                                         |
   | 110 6E   [ 3 145  ] n minuscule                                         |
   | 111 6F   [ 3 1 5  ] o minuscule                                         |
   | 112 70   [ 3214   ] p minuscule                                         |
   | 113 71   [ 32145  ] q minuscule                                         |
   | 114 72   [ 321 5  ] r minuscule                                         |
   | 115 73   [ 32 4   ] s minuscule                                         |
   | 116 74   [ 32 45  ] t minuscule                                         |
   | 117 75   [ 3 1  6 ] u minuscule                                         |
   | 118 76   [ 321  6 ] v minuscule                                         |
   | 119 77   [  2 456 ] w minuscule                                         |
   | 120 78   [ 3 14 6 ] x minuscule                                         |
   | 121 79   [ 3 1456 ] y minuscule                                         |
   | 122 7A   [ 3 1 56 ] z minuscule                                         |
   | 123 7B   [732    8] ouvre accolade                                      |
   | 124 7C   [    4568] barre verticale                                     |
   | 125 7D   [7    568] ferme accolade                                      |
   | 126 7E   [ 3     8] tilde minuscule                                     |
   | 127 7F   [ 321   8] effacement                                          |
   | 128 20AC [7  1 5 8] euro                                                |
   | 129 201A [7     6 ] guillemet bas simple                                |
   | 130 0192 [  214  8] florin                                              |
   | 131 201E [     56 ] guillemet bas double                                |
   | 132 2026 [ 3    68] points de suspension                                |
   | 133 2020 [ 3   568] obel                                                |
   | 134 2021 [73   56 ] double obel                                         |
   | 135 2C6  [    4  8] circonflexe minuscule                               |
   | 136 2030 [73  4 68] pour mille                                          |
   | 137 0160 [732 4 68] s carron majuscule                                  |
   | 138 2039 [7    5  ] guillemet fermant                                   |
   | 139 0152 [7 2 4 6 ] e dans o majuscule                                  |
   | 140 017D [73 1 568] z carron majuscule                                  |
   | 141 2018 [      68] guillemet simple ouvrant                            |
   | 142 2019 [73      ] guillemet simple fermant                            |
   | 143 201C [73     8] guillemet double ouvrant                            |
   | 144 201D [7     68] guillemett double fermant                           |
   | 145 2022 [7 2145 8] puce                                                |
   | 146 2013 [7   4  8] tiret cadre atteint                                 |
   | 147 2014 [7   45 8] tiret EM                                            |
   | 148 02DC [7   4 6 ] tilde minuscule                                     |
   | 149 2122 [ 32 45 8] marque déposée                                      |
   | 150 0161 [ 32 4 68] s avec carron minuscule                             |
   | 151 203A [     5 8] guillemet ouvrant                                   |
   | 152 0153 [  2 4 68] e dans o minuscule                                  |
   | 153 017E [73 1 568] z carron minuscule                                  |
   | 154 0178 [7  14568] y tréma majuscule                                   |
   | 160 A0   [7       ] espace insécable (forcé)                            |
   | 161 A1   [ 32  5 8] point d'exclamation inversé                         |
   | 162 A2   [7  14  8] cent                                                |
   | 163 A3   [732     ] livre                                               |
   | 164 A4   [    45  ] devise                                              |
   | 165 A5   [7 2  568] yen                                                 |
   | 166 A6   [    45 8] trait coupé vertical                                |
   | 167 A7   [ 3214  8] paragraphe                                          |
   | 168 A8   [    4 6 ] tréma                                               |
   | 169 A9   [   14  8] copyright                                           |
   | 170 AA   [7  1  68] ordinal féminin                                     |
   | 171 AB   [ 32  568] guillemet français ouvrant                          |
   | 172 AC   [7 2  56 ] négation logique                                    |
   | 173 AD   [7      8] trait d'union conditionne                           |
   | 174 AE   [ 321 5 8] marque déposée                                      |
   | 175 AF   [ 3 14  8] macron                                              |
   | 176 B0   [7 2   6 ] degré                                               |
   | 177 B1   [73    68] plus ou moins                                       |
   | 178 B2   [7   45  ] carré (puissance 2)                                 |
   | 179 B3   [7   456 ] cube (puissance 3)                                  |
   | 180 B4   [     5  ] accent aigu                                         |
   | 181 B5   [7 2  5  ] micro                                               |
   | 182 B6   [7   4568] pied de mouche                                      |
   | 183 B7   [       8] point médiant                                       |
   | 184 B8   [    456 ] cédille                                             |
   | 185 B9   [7   4   ] exposant                                            |
   | 186 BA   [7 2   68] ordinal masculin                                    |
   | 187 BB   [732  56 ] guillemetts français fermant                        |
   | 188 BC   [ 3 1  68] un quart                                            |
   | 189 BD   [    4 68] un demi                                             |
   | 190 BE   [ 3 14 68] trois quart                                         |
   | 191 BF   [  2   68] point d'interrogation inversé                       |
   | 192 C0   [7321 56 ] a grave majuscule                                   |
   | 193 C1   [7321 568] a aigu majuscule                                    |
   | 194 C2   [7  1  6 ] a circonflexe majuscule                             |
   | 195 C3   [7  1   8] a tilde majuscule                                   |
   | 196 C4   [73  456 ] a tréma majuscule                                   |
   | 197 C5   [7 2     ] a avec rond au-dessus majuscule                     |
   | 198 C6   [73  45  ] a e majuscule                                       |
   | 199 C7   [73214 6 ] c cédille majuscule                                 |
   | 200 C8   [732 4 6 ] e grave majuscule                                   |
   | 201 C9   [7321456 ] e aigu majuscule                                    |
   | 202 CA   [7 21  6 ] e circonflexe majuscule                             |
   | 203 CB   [7 214 6 ] e tréma majuscule                                   |
   | 204 CC   [7 2 4  8] i grave majuscule                                   |
   | 205 CD   [73  4   ] i aigu majuscule                                    |
   | 206 CE   [7  14 6 ] i circonflexe majuscule                             |
   | 207 CF   [7 21456 ] i tréma majuscule                                   |
   | 208 D0   [7 21   8] Eth majuscule                                       |
   | 209 D1   [73 145 8] n tilde majuscule                                   |
   | 210 D2   [73 1 5 8] o grave majuscule                                   |
   | 211 D3   [73  4 6 ] o aigu majuscule                                    |
   | 212 D4   [73 1456 ] o circonflexe majuscule                             |
   | 213 D5   [73 1   8] o tilde majuscule                                   |
   | 214 D6   [7 2 4 68] o tréma majuscule                                   |
   | 215 D7   [73   5 8] multipilé par                                       |
   | 216 D8   [73  4568] o barré majuscule                                   |
   | 217 D9   [732 456 ] u grave majuscule                                   |
   | 218 DA   [732 4568] u aigu majuscule                                    |
   | 219 DB   [7  1 56 ] u circonflexe majuscule                             |
   | 220 DC   [7 21 56 ] u tréma majuscule                                   |
   | 221 DD   [73 14568] y aigu majuscule                                    |
   | 222 DE   [  2 45 8] Thorn majuscule                                     |
   | 223 DF   [ 32 4  8] szeth                                               |
   | 224 E0   [ 321 56 ] a grave minuscule                                   |
   | 225 E1   [ 321 568] a aigu minuscule                                    |
   | 226 E2   [   1  68] a circonflexe minuscule                             |
   | 227 E3   [   1   8] a qilde minuscule                                   |
   | 228 E4   [73  45 8] a tréma minuscule                                   |
   | 229 E5   [  2    8] a avec rond en tête minuscule                       |
   | 230 E6   [ 3  45 8] a e minuscule                                       |
   | 231 E7   [ 3214 6 ] c cédille minuscule                                 |
   | 232 E8   [ 32 4 6 ] e grave minuscule                                   |
   | 233 E9   [ 321456 ] e aigu minuscule                                    |
   | 234 EA   [  21  68] e circonflexe minuscule                             |
   | 235 EB   [  214 68] e tréma minuscule                                   |
   | 236 EC   [  2 4  8] i grave minuscule                                   |
   | 237 ED   [73  4  8] i aigu minuscule                                    |
   | 238 EE   [   14 68] i circonflexe minuscule                             |
   | 239 EF   [  214568] i tréma minuscule                                   |
   | 240 F0   [  21   8] Eth minuscule                                       |
   | 241 F1   [7 214568] n tilde minuscule                                   |
   | 242 F2   [ 3 1 5 8] o grave minuscule                                   |
   | 243 F3   [ 3  4 6 ] o aigu minuscule                                    |
   | 244 F4   [   14568] o circonflexe minuscule                             |
   | 245 F5   [7   4 68] o tilde minuscule                                   |
   | 246 F6   [ 3   5 8] o tréma minuscule                                   |
   | 247 F7   [7 2  5 8] divisé par                                          |
   | 248 F8   [     568] o barré minuscule                                   |
   | 249 F9   [ 32 456 ] u grave minuscule                                   |
   | 250 FA   [ 32 4568] u aigu minuscule                                    |
   | 251 FB   [   1 568] u circonflexe minuscule                             |
   | 252 FC   [  21 568] u tréma minuscule                                   |
   | 253 FD   [ 3 14568] y aigu minuscule                                    |
   | 254 FE   [  2 45 8] thorn minuscule                                     |
   | 255 FF   [  2  568] y tréma minuscule                                   |
   +-------------------------------------------------------------------------+

.. _midi:

Table des instruments MIDI
==========================

Les mélodies d'alerte jouées via le synthétiseur MIDI d'une carte son
utilisent la numérotation standard des instruments General MIDI : le
programme numéro 0 correspond à ``Acoustic Grand Piano`` et le
programme numéro 127 à ``Gunshot``. Voir la spécification *General
MIDI Level 1* pour la liste complète.

Formalités
==========


Licence
-------

BRLTTY est un logiciel libre, distribué sous les termes de la
`GNU Lesser General Public License
<http://www.gnu.org/licenses/licenses.html#LGPL>`_, version 2.1 ou
ultérieure. Il est fourni **sans aucune garantie** — pas même la
garantie implicite de qualité marchande ou d'adéquation à un usage
particulier. Le texte intégral de la licence se trouve dans le
fichier ``LICENSE-LGPL`` à la racine de l'arborescence des sources.


Auteurs
-------

BRLTTY a vu le jour au début des années 1990 grâce au travail de
Nikhil Nair, Nicolas Pitre et Stéphane Doyon. Dave Mielke en assure
aujourd'hui la maintenance. La liste à jour de l'équipe, l'historique
des contributions et l'actualité du projet sont publiés sur le site
du projet : `brltty.app <http://brltty.app/>`_.
