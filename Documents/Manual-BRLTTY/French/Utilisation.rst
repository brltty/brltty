Utilisation de BRLTTY
=====================

Avant de démarrer BRLTTY, vous avez besoin d'installer votre afficheur
braille. Dans la plupart des cas, cela se fait simplement en le
connectant sur un port série disponible, puis en le mettant sous
tension. Après avoir installé votre afficheur, exécutez simplement
BRLTTY en tapant la commande BRLTTY sur une invite du shell (vous
devez faire cela en tant que root). Intéressez-vous à l'option :ref:`-d <options-braille-device>`
en ligne de commande, à la ligne :ref:`braille-device <configure-braille-device>` du fichier de
configuration, et à l'option de compilation :ref:`--with-braille-device <build-braille-device>`
pour d'autres moyens de dire à BRLTTY à quel périphérique votre
afficheur est connecté. Regardez l'option :ref:`-b <options-braille-driver>` en ligne de commande,
la ligne :ref:`braille-driver <configure-braille-driver>` du fichier de configuration, et l'option
de compilation :ref:`--with-braille-driver <configure-braille-driver>` pour d'autres possibilités
concernant la manière d'indiquer à BRLTTY quel type d'afficheur braille
vous avez. Observez l'option :ref:`-B <options-braille-parameters>` en ligne de commande, et la ligne
:ref:`braille-parameters <configure-braille-parameters>` du fichier de configuration pour
d'autres possibilités concernant la manière de passer les paramètres
au pilote de votre afficheur braille. Un message donnant le nom du
programme (BRLTTY) et son numéro de version apparaîtra brièvement
(voir l'option :ref:`-M <options-message-timeout>` en ligne de commande) sur l'afficheur braille.
L'afficheur montrera alors une petite zone de l'écran contenant le
curseur. Par défaut, le curseur est représenté par les points 7 et 8
superposés sur le caractère où il est.

Toute activité de l'écran sera affichée sur l'afficheur
braille. L'afficheur suivra aussi la progression du curseur sur
l'écran. Cette caractéristique est connue en tant que **poursuite du curseur**.

Toutefois, le fait de simplement taper sur le clavier et lire
l'afficheur ne suffit pas. Essayez d'entrer une commande qui
provoquera une erreur, et de taper ``entrée``. L'erreur
apparaît sur l'écran mais, sauf si vous avez un afficheur à plusieurs
lignes, il y a des chances qu'elle ne soit pas visible sur l'afficheur
braille. Vous n'y voyez que l'invite du shell. Vous avez donc besoin,
à présent, d'un moyen de déplacer la ``plage`` braille à
travers l'écran. Les touches de l'afficheur braille lui-même peuvent
être utilisées pour envoyer les commandes à BRLTTY qui, entre autres
choses, peut s'en charger.

.. _commands:

Commandes
---------

Malheureusement, les différents afficheurs braille n'offrent pas des
touches de contrôle standard. Certains ont des touches standard six
points, certains en ont huit, et d'autres n'en ont pas. Certains ont
des touches au niveau des pouces, mais il n'y en a pas beaucoup en
standard. Certains ont un bouton au-dessus de chaque cellule
braille. Certains ont des boutons qu'il faut pousser. D'autres ont une
barre facile d'accès fonctionnant beaucoup comme une manette de
jeu. La plupart ont des combinaisons variées des éléments ci-dessus. A
cause de la nature et de la disposition si différente entre chaque
afficheur, reportez-vous à la documentation de votre afficheur
personnel pour trouver ce que fait exactement chaque touche.

Les commandes de BRLTTY sont identifiées par leur nom dans ce
manuel. Si vous oubliez la/les touche(s) de votre afficheur braille à
utiliser pour une commande particulière, reportez-vous à la page
d'aide de son pilote. Par conséquent, la touche que vous devriez
mémoriser immédiatement est celle de la commande
:ref:`HELP <command-HELP>`. Utilisez les routines habituelles (comme
décrit ci-dessous) pour naviguer dans la page d'aide, puis pressez de
nouveau la touche d'aide pour quitter.

.. _vertical-motion:

Déplacement vertical
~~~~~~~~~~~~~~~~~~~~

Voir aussi les commandes de routines :ref:`PRINDENT/NXINDENT <command-PRINDENT-NXINDENT>` et :ref:`PRDIFCHAR/NXDIFCHAR <command-PRDIFCHAR-NXDIFCHAR>`.


.. _command-LNUP-LNDN:

LNUP/LNDN
    Monte/descend d'une ligne. Si le saut des lignes identiques a été
    activé (voir la commande :ref:`SKPIDLNS <command-SKPIDLNS>`), ces commandes, au lieu de
    se déplacer exactement d'une ligne à l'autre, sont des alias pour
    les commandes :ref:`PRDIFLN/NXDIFLN <command-PRDIFLN-NXDIFLN>`.

.. _command-WINUP-WINDN:

WINUP/WINDN
    Monte/descend d'un écran. Si l'écran n'est pas plus haut qu'une ligne,
    elle déplace alors de 5 lignes.

.. _command-PRDIFLN-NXDIFLN:

PRDIFLN/NXDIFLN
    Monte/descend à la ligne la plus proche ayant un contenu
    différent. Si le saut de lignes identiques a été activé (voir les
    commandes :ref:`SKPIDLNS <command-SKPIDLNS>`), ces commandes, plutôt que de sauter des
    lignes identiques, sont des alias pour les commandes :ref:`LNUP/LNDN <command-LNUP-LNDN>` commands.

.. _command-ATTRUP-ATTRDN:

ATTRUP/ATTRDN
    Monte/descend à la ligne la plus proche avec des attributs
    différents (mise en valeur de caractère).

.. _command-TOP-BOT:

TOP/BOT
    Va au début ou à la fin de la ligne.

.. _command-TOP_LEFT-BOT_LEFT:

TOP_LEFT/BOT_LEFT
    Va au coin en haut à gauche ou en bas à gauche.

.. _command-PRPGRPH-NXPGRPH:

PRPGRPH/NXPGRPH
    Va à la ligne la plus proche du paragraphe précédent/suivant (la
    première ligne non vide après la ligne vide la plus proche). La
    ligne courante est incluse dans la recherche de l'espace entre les
    paragraphes.

.. _command-PRPROMPT-NXPROMPT:

PRPROMPT/NXPROMPT
    Va à l'invite de commande suivant/précédent.

.. _command-PRSEARCH-NXSEARCH:

PRSEARCH/NXSEARCH
    Recherche avant/arrière de l'occurrence la plus proche de la
    chaîne de caractères du presse-papier collé (voir :ref:`Copier-coller <cut>`) qui
    n'est pas dans la plage braille. La recherche se fait à
    gauche/droite, et commence au caractère immédiatement à
    gauche/droite de la plage, et en englobant le bord de
    l'écran. La recherche ne tient pas compte de la casse.


.. _horizontal-motion:

Déplacement horizontal
~~~~~~~~~~~~~~~~~~~~~~

Voir aussi la commande de la routine :ref:`SETLEFT <command-SETLEFT>`.


.. _command-CHRLT-CHRRT:

CHRLT/CHRRT
    Va à gauche/droite d'un caractère.

.. _command-HWINLT-HWINRT:

HWINLT/HWINRT
    Va d'une demi-fenêtre à gauche/droite.

.. _command-FWINLT-FWINRT:

FWINLT/FWINRT
    Va d'une fenêtre à gauche/droite. Ces commandes sont
    particulièrement utiles parce qu'elles débordent automatiquement
    quand elles atteignent le bord de l'écran. Les autres
    caractéristiques, comme celles de sauter les fenêtres vides (voir
    la commande :ref:`SKPBLNKWINS <command-SKPBLNKWINS>`), accroissent
    leur maniabilité.

.. _command-FWINLTSKIP-FWINRTSKIP:

FWINLTSKIP/FWINRTSKIP
    Va à la fenêtre non-vide la plus proche à gauche/droite.

.. _command-LNBEG-LNEND:

LNBEG/LNEND
    Va au début/fin de la ligne.


.. _implicit-motion:

Déplacement implicite
~~~~~~~~~~~~~~~~~~~~~

Voir aussi la commande de routine :ref:`GOTOMARK <command-GOTOMARK>`.


.. _command-HOME:

HOME
    Va à l'endroit où se trouve le curseur d'écran.

.. _command-BACK:

BACK
    Revient là où la commande de déplacement la plus récente a mis la
    plage braille. C'est une façon facile de retourner à droite de là
    où vous lisiez, après un événement imprévu (comme une poursuite du
    curseur) qui déplace la plage braille à un moment inapproprié.

.. _command-RETURN:

RETURN
      -
        Si le déplacement le plus récent de la plage braille
        était automatique, comme le résultat d'une poursuite du
        curseur, revient là où la commande de déplacement la plus
        récente l'a mise (voir la commande :ref:`BACK <command-BACK>`).

      -
        Si le curseur n'est pas dans la plage braille, va où se
        trouve le curseur (voir la commande :ref:`HOME <command-HOME>`).


.. _feature-activation:

Activation de fonctionnalités
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Chacune de ces commandes a trois modes: **activée**
(déclenche la fonction), **désactivée** (annule la fonction),
et **à bascule** (si la fonction est inhibée, la déclenche, et
vis versa). Sauf si cela est explicitement dit, chacune de ces
fonctions est initialement à ``inactif``, et, quand elle est
**activée**, cela agit sur l'exécution de BRLTTY tout entier. Le
paramètre initial de certaines des fonctionnalités peut être changée
par le :ref:`menu des préférences <preferences-menu>`.


.. _command-FREEZE:

FREEZE
    Gèle l'image de l'écran. BRLTTY fait une copie de l'écran (contenu
    et attributs) au moment où l'image de l'écran est
    raffraîchie, et ignore dès lors toute mise à jour de l'écran
    jusqu'à ce qu'il soit réactualisé. Par exemple, cette
    fonctionnalité facilite la lecture de la sortie d'une application
    qui écrit beaucoup trop rapidement.

.. _command-DISPMD:

DISPMD
    Montre les mises en relief (attributs) de chaque caractère dans la
    plage braille, plutôt que les caractères eux-mêmes (le
    contenu). Par exemple, cette fonctionnalité est utile si vous
    devez mettre en valeur un item. Quand elle montre le contenu de
    l'écran, la table de texte est utilisée (voir
    l'option :ref:`-t <options-text-table>` en ligne de commande, la ligne :ref:`text-table <configure-text-table>` du
    fichier de configuration et l'option de compilation
    :ref:`--with-text-table <build-text-table>`).  Quand elle montre les attributs de
    l'écran, la table d'attributs est utilisée (voir
    l'option ``-a`` en ligne de commande, la ligne
    :ref:`attributes-table <build-attributes-table>` du fichier de configuration, et l'option de
    compilation ``--with-attributes-table``).  Cette fonctionnalité ne
    s'applique qu'au terminal virtuel courant.

.. _command-SIXDOTS:

SIXDOTS
    Montre les caractères brailles sous la forme 6-points et non
    8-points. Les points 7 et 8 sont encore utilisés par d'autres
    fonctionnalités telles que la représentation du curseur et le
    soulignement des caractères mis en relief. Si une table de braille
    abrégé a été sélectionnée (voir l'option :ref:`-c <options-contraction-table>` en ligne de
    commande, et la ligne :ref:`contraction-table <configure-contraction-table>` du fichier de
    configuration), elle est utilisée. Vous pouvez aussi changer ce
    paramètre avec la préférence :ref:`Apparence
    du texte <preference-text-style>`.

.. _command-SLIDEWIN:

SLIDEWIN
    Si la poursuite du curseur (voir la commande :ref:`CSRTRK <command-CSRTRK>`) est  **on**, chaque fois que
    le curseur se déplace de façon trop limitée (ou pas assez) au-delà
    de la fin de la plage braille, elle repositionne
    horizontalement la plage braille, de sorte que le curseur,
    tant qu'il reste de ce côté, soit plus proche du centre. Si cette
    caractéristique est **inactive**, la plage braille est toujours
    positionnée de façon à ce que sont côté gauche soit un multiple de
    sa largeur depuis le bord gauche de l'écran.

    Vous pouvez aussi modifier ce paramètre avec la préférence
    :ref:`Faire défiler la fenêtre <preference-sliding-window>`.

.. _command-SKPIDLNS:

SKPIDLNS
    Au lieu de déplacer exactement d'une ligne en haut ou en bas,
    saute les lignes déjà lues qui ont le même contenu que la ligne
    courante. Cette caractéristiques influence les commandes :ref:`LNUP/LNDN <command-LNUP-LNDN>`,
    ainsi que le défilement de la ligne exécutée par les commandes

  :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` et :ref:`FWINLTSKIP/FWINRTSKIP <command-FWINLTSKIP-FWINRTSKIP>`.
    Vous pouvez aussi changer ce paramètre avec la préférence
    :ref:`Sauter les lignes identiques <preference-skip-identical-lines>`.

.. _command-SKPBLNKWINS:

SKPBLNKWINS
    Saute les fenêtres vides déjà passées lors d'une lecture en avant ou
    en arrière. Cette caractéristique influence les commandes :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>`.
    Vous pouvez aussi changer ce paramètre avec la préférence

:ref:`Sauter les fenêtres vierges <preference-skip-blank-windows>`.

.. _command-CSRVIS:

CSRVIS
    Montre le curseur en superposant un modèle de point (voir la
    commande :ref:`CSRSIZE <command-CSRSIZE>`) au-dessus du
    caractère où il se trouve. A l'origine, cette caractéristique est **activée**.
    Vous pouvez aussi changer ce paramètre avec la préférence
    :ref:`Afficher le curseur <preference-show-cursor>`.

.. _command-CSRHIDE:

CSRHIDE
    Rend le curseur invisible (voir la commande :ref:`CSRVIS <command-CSRVIS>` ) de façon à lire précisément le
    caractère sous lui. Cette caractéristique n'affecte que la console

  virtuelle courante.

.. _command-CSRTRK:

CSRTRK
    Poursuite du curseur. Si le curseur se déplace à un
    endroit qui n'est pas à l'intérieur de la plage braille, il
    déplace automatiquement la plage braille vers la nouvelle
    position du curseur. Vous voudrez en principe que cette
    caractéristique soit active, puisque cela minimise les effets du défilement
    de l'écran, et puisque, lorsque vous entrez quelque chose, la
    région à l'intérieur de laquelle vous écrivez actuellement est
    toujours visible.
    Si cette option fait sauter la plage braille à un moment
    inadéquat, utilisez la commande :ref:`BACK <command-BACK>`
    pour revenir là où vous étiez en train de lire.
    Il se peut que vous deviez
    désactiver cette option lorsque vous utilisez une application qui
    raffraîchit en permanence l'écran tout en maintenant une présentation fixe
    des données. A l'origine, cette caractéristique est **active**.

.. _command-CSRSIZE:

CSRSIZE
    Représente le curseur avec les huit points (un bloc complet), au
    lieu de n'afficher que les points 7 et 8 (un soulignement). Vous

  pouvez aussi changer ce paramètre avec la préférence
  :ref:`Apparence du curseur <preference-cursor-style>`.

.. _command-CSRBLINK:

CSRBLINK
    Cache (active et désactive en fonction d'un intervalle
    prédéfini) le symbole représentant le curseur (voir la commande :ref:`CSRVIS <command-CSRVIS>`).
    Vous pouvez changer ce paramètre avec la préférence
    :ref:`Curseur clignotant <preference-blinking-cursor>`.

.. _command-ATTRVIS:

ATTRVIS
    Souligne (avec la combinaison des points 7 et 8) les caractères à
    mettre en évidence.


Non souligné

        Blanc sur noir (normal),
        Gris sur noir,
        Blanc sur bleu,
        Noir sur cyan.

points 7 et 8

        Noir sur blanc (vidéo inversée).

point 8

        Tout le reste.

    Vous pouvez aussi changer ce paramètre avec la préférence
    :ref:`Afficher les attributes <preference-show-attributes>`.

.. _command-ATTRBLINK:

ATTRBLINK
    Masque (active ou désactive selon un intervalle prédéfini)
    l'attribut souligné (voir la commande :ref:`ATTRVIS <command-ATTRVIS>` command).
    A l'origine, cette caractéristique est **activée**.
    Vous pouvez aussi changer ce paramètre avec la préférence
    :ref:`Attributs clignotants <preference-blinking-attributes>`.

.. _command-CAPBLINK:

CAPBLINK
    Masque (active ou désactive selon un intervalle prédéfini) les
    lettres en majuscule. Vous pouvez aussi changer ce paramètre avec
    la préférence :ref:`Majuscules
    clignotantes <preference-blinking-capitals>`.

.. _command-TUNES:

TUNES
    Joue un son court prédéfini (voir :ref:`Sons d`avertissement <tunes>`)
    chaque fois qu'un événement significatif se produit. A
    l'origine, cette caractéristique est **activée**.
    Vous pouvez aussi changer ce paramètre avec la préférence
    :ref:`Sons d`avertissement <preference-alert-tunes>`.

.. _command-AUTOREPEAT:

AUTOREPEAT
    Répète automatiquement une commande à un intervalle régulier après
    un délai initial tant que sa touche (combinaison) reste
    appuyée. Seuls certains pilotes supportent cette fonctionnalité,
    la principale limite étant que la plupart des afficheurs brailles
    ne signale pas en même temps les touches pressées et les touches
    exécutées comme des événements distinctement séparés. A l'origine,
    cette caractéristique est **activée**.
    Vous pouvez aussi changer ce paramètre avec la préférence
    :ref:`Répétition automatique <preference-autorepeat>`.

.. _command-AUTOSPEAK:

AUTOSPEAK
    Dit automatiquement:

      - la nouvelle ligne quand la plage braille est déplacée verticalement
      - les caractères que vous entrez ou que vous effacez.
      - le caractère vers lequel vous déplacez le curseur.

    A l'origine, cette caractéristique est **inactive**.
    Vous pouvez aussi changer ce paramètre avec la préférence
    :ref:`Parole automatique <preference-autospeak>`.


.. _mode-selection:

Sélection de mode
~~~~~~~~~~~~~~~~~


.. _command-HELP:

HELP
    Se déplace à la page d'aide du pilote d'un afficheur braille. C'est
    là que vous pouvez trouver un résumé en ligne de choses telles que
    ce que font les touches de votre afficheur braille, et comment
    interpréter ses cellules d'état.
    Utilisez les commandes :ref:`vertical <vertical-motion>`
    et :ref:`horizontal <horizontal-motion>` pour naviguer
    dans la page d'aide. Appelez la commande ``help`` de nouveau pour
    revenir à l'écran.

.. _command-INFO:

INFO
    Va à l'affichage des états (voir la section :ref:`Affichage de l`état <status>` pour des détails complets). Cela
    présente un résumé comprenant la position du curseur, la position
    de la plage braille, et l'état d'un certain nombre de
    caractéristiques de BRLTTY. Appelez de nouveau
    cette commande pour revenir à l'écran.

.. _command-LEARN:

LEARN
    Entre dans le mode d'apprentissage de commande (voir la section
    :ref:`Mode Apprentissage des commandes <learn>` pour des détails complets). C'est ainsi
    que vous pouvez apprendre de façon interactive ce que font les
    touches de votre afficheur braille. Rappelez cette commande pour
    revenir à l'écran. Cette commande n'est pas disponible si l'option
    de compilation :ref:`--disable-learn-mode <build-learn-mode>` a été spécifiée.


.. _preference-maintenance:

Maintenance des préférences
~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. _command-PREFMENU:

PREFMENU
    Accès au menu des préférences (voir :ref:`Le
    Menu des préférences <preferences-menu>` pour des détails complets). Rappelez cette
    commande pour revenir à l'écran.

.. _command-PREFSAVE:

PREFSAVE
    Enregistre les paramètres de préférence courants (voir :ref:`Preferences <preferences>` pour des détails complets).

.. _command-PREFLOAD:

PREFLOAD
    Recharge les derniers paramètres de préférence sauvegardés
    (voir :ref:`Preferences <preferences>` pour des
    détails complets).


.. _menu-navigation:

Navigation dans le menu
~~~~~~~~~~~~~~~~~~~~~~~


.. _command-MENU_FIRST_ITEM-MENU_LAST_ITEM:

MENU_FIRST_ITEM/MENU_LAST_ITEM
    Va au premier/dernier élément du menu.

.. _command-MENU_PREV_ITEM-MENU_NEXT_ITEM:

MENU_PREV_ITEMMENU_NEXT_ITEM/
    Va à l'élément précédent/suivant du menu.

.. _command-MENU_PREV_SETTING-MENU_NEXT_SETTING:

MENU_PREV_SETTING/MENU_NEXT_SETTING
    Remonte/descend le paramètre des éléments du menu courant.


.. _speech-controls:

Contrôles de la verbosité
~~~~~~~~~~~~~~~~~~~~~~~~~


.. _command-SAY_LINE:

SAY_LINE
    Dit la ligne courante. La préférence :ref:`Mode Dire la ligne <preference-sayline-mode>` détermine si le
    message en attente est proposé en premier.

.. _command-SAY_ABOVE:

SAY_ABOVE
    Dit la portion supérieure de l'écran (qui s'arrête à la ligne actuelle).

.. _command-SAY_BELOW:

SAY_BELOW
    Dit la portion inférieure de l'écran (qui commence à la ligne courante).

.. _command-MUTE:

MUTE
    Arrête immédiatement la parole.

.. _command-SPKHOME:

SPKHOME
    Va où se trouve le curseur parlant.

.. _command-SAY_SLOWER-SAY_FASTER:

SAY_SLOWER/SAY_FASTER
    Diminue/accélère le débit de parole (voir aussi la préférence
    :ref:`Vitesse de la synthèse <preference-speech-rate>`).
    Cette commande n'est disponible que si un pilote qui la supporte est utilisé.

.. _command-SAY_SOFTER-SAY_LOUDER:

SAY_SOFTER/SAY_LOUDER
    Diminue/augmente le volume de la parole (voir aussi la préférence
    :ref:`Volume de la synthèse <preference-speech-volume>`).
    Cette commande n'est disponible que si un pilote qui la supporte est utilisé.


.. _speech-navigation:

Verbosigé dans la navigation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~


SPEAK_CURR_CHAR
a

Aller à un terminal virtuel
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Voir aussi la commande de touche de chemin :ref:`SWITCHVT <command-SWITCHVT>`.


.. _command-SWITCHVT_PREV-SWITCHVT_NEXT:

SWITCHVT_PREV/SWITCHVT_NEXT
    Va à la console précédente/suivante.


Autres Commandes
~~~~~~~~~~~~~~~~


.. _command-CSRJMP_VERT:

CSRJMP_VERT
    Amène le curseur n'importe où sur la première ligne de la plage
    braille (voir :ref:`Déplacement du Curseur <routing>` pour des détails complets).
    Le curseur se déplace grâce à la simulation de l'appui sur les touches de direction verticales.
    Cette commande ne fonctionne pas toujours car certaines applications,
    soit déplacent le curseur sans vraiment avertir, soit utilisent
    les flèches de direction à d'autres fins que le déplacement du curseur.
    C'est toutefois un peu plus sûr que les autres commandes de déplacement du
    curseur car cette commande ne tente pas de simuler les flèches gauche et
    droite.

.. _command-PASTE:

PASTE
    Insère les caractères du presse-papier à l'emplacement actuel du curseur.
    (voir :ref:`Copier-coller <cut>` pour des détails complets).

.. _command-RESTARTBRL:

RESTARTBRL
    Arrête puis relance le pilote de l'afficheur braille.

.. _command-RESTARTSPEECH:

RESTARTSPEECH
    Arrête puis relance le pilote de la synthèse vocale.


.. _commands-characters:

Commandes de caractères
~~~~~~~~~~~~~~~~~~~~~~~


.. _command-ROUTE:

ROUTE
    Déplace le curseur au caractère associé à la routine
    (voir :ref:`Déplacement du Curseur <routing>` pour des
    détails complets).
    Le curseur se déplace grâce à la simulation de l'appui sur les touches de direction verticales.
    Cette commande ne fonctionne pas toujours car certaines applications,
    soit déplacent le curseur sans vraiment avertir, soit utilisent
    les flèches de direction à d'autres fins que le déplacement du curseur.

.. _command-CUTBEGIN:

CUTBEGIN
    Positionne le début du bloc à copier au caractère associé à la routine-curseur.
    (voir :ref:`Copier-coller <cut>` pour des détails complets).
    Cette commande vide le presse-papier.

.. _command-CUTAPPEND:

CUTAPPEND
    Positionne le début du bloc à copier au caractère associé à la routine
    (voir :ref:`Copier-coller <cut>` pour des détails complets).
    Cette commande ne vide pas le presse-papier.

.. _command-CUTRECT:

CUTRECT
    Fixe la fin du bloc à copier au caractère associé à la touche de direction,
    et met la fenêtre rectangulaire dans le presse-papier (voir
    :ref:`Copier/Coller <cut>` pour des détails complets).

.. _command-CUTLINE:

CUTLINE
    Fixe la fin du bloc à copier au caractère associé à la touche de
    direction, et met la fenêtre linéaire dans le presse-papier (voir
    :ref:`Cut and Paste <cut>` pour des détails complets).

.. _command-COPYCHARS:

COPYCHARS
    Copie le bloc de caractères marqué par les deux touches de routine
    dans le presse-papier
    (voir :ref:`Copier et coller <cut>` pour tous les détails).

.. _command-APNDCHARS:

APNDCHARS
    Insère le bloc de caractères marqués par les deux touches de routine
    dans le presse-papier
    (voir :ref:`Copier et coller <cut>` pour tous les détails).

.. _command-PRINDENT-NXINDENT:

PRINDENT/NXINDENT
    Monte/descend à la ligne la plus proche qui n'est pas indentée plus
    que la colonne associée à la routine.

.. _command-DESCCHAR:

DESCCHAR
    Affiche momentanément (voir l'option :ref:`-M <options-message-timeout>`
    en ligne de commande) un message décrivant le caractère associé à
    la routine. Cela montre les valeurs décimales et
    hexadécimales du caractère, les couleurs de fond et de premier
    plan, et, lorsqu'ils sont présents, les attributs spéciaux
    (``bright`` et ``blink``). Le message ressemble à cela:

::

   char 65 (0x41): white on black bright blink


.. _command-SETLEFT:

SETLEFT
    Replace horrizontalement la plage braille de façon à ce que son
    côté gauche soit à la colonne associée à la touche de
    déplacement. Cette caractéristique rend très facile l'action de
    mettre la plage précisément là où il faut et, donc, pour les
    afficheurs qui ont des touches routines, élimine presque la
    nécessité pour beaucoup de déplacements élémentaires de la plage
    (comme les commandes :ref:`CHRLT/CHRRT <command-CHRLT-CHRRT>` et :ref:`HWINLT/HWINRT <command-HWINLT-HWINRT>`).

.. _command-PRDIFCHAR-NXDIFCHAR:

PRDIFCHAR/NXDIFCHAR
    Monte/descend à la ligne la plus proche contenant un caractère différent sur la
    colonne associée à la routine curseur.


.. _commands-base:

Commandes de base
~~~~~~~~~~~~~~~~~


.. _command-SWITCHVT:

SWITCHVT
    Bascule vers le terminal virtuel dont le numéro (en commençant à 1)
    correspond à la routine curseur.
    Voir aussi les commandes de basculement entre terminaux virtuels
    :ref:`SWITCHVT_PREV/SWITCHVT_NEXT <command-SWITCHVT_PREV-SWITCHVT_NEXT>`.

.. _command-SETMARK:

SETMARK
    Marque la position courante de la plage braille dans
    une mémoire associée à la routine. Voir la commande :ref:`GOTOMARK <command-GOTOMARK>`.
    Cette caractéristique n'affecte que le terminal virtuel courant.

.. _command-GOTOMARK:

GOTOMARK
    Déplace la plage braille à la position précédemment marquée
    (voir la commande :ref:`SETMARK <command-SETMARK>`) avec
    la même routine.
    Cette fonction n'affecte que le terminal virtuel courant.


.. _configure:

Le fichier de configuration
---------------------------


Des valeurs système par défaut pour certains paramètres peuvent être
établies dans un fichier de configuration. Le nom par défaut de ce
fichier est ``/etc/brltty.conf``, même s'il peut être contourné
par l'option en ligne de commande :ref:`-f <options-configuration-file>`.
Son existence n'est pas indispensable.
Vous pouvez trouver un fichier type dans le sous-répertoire
``Documents``

Les lignes vides sont ignorées.
Un commentaire commence par un signe nombre (``#``), et continue jusqu'à
la fin de la ligne.
Les lignes de commande suivantes sont reconnues:


.. _configure-api-parameters:

``api-parameters`` *name*\ ``=`` *valeur*\ ``,``...
    Spécifie les paramètres pour l'API.
    Si vous spécifiez le même paramètre plus d'une fois, sa
    valeur située le plus à droite est utilisée. Pour une description
    des paramètres acceptés par l'interface, voyez le manuel de
    référence **BrlAPI**. Voir l'option de compilation :ref:`--with-api-parameters <build-api-parameters>`
    pour les paramètres par défaut établis lors de la procédure de
    compilation. Cette ligne de commande peut être évitée avec
    l'option en ligne de commande :ref:`-A <options-api-parameters>`.

.. _configure-attributes-table:

``attributes-table`` *file*
    Spécifie la table d'attributs (voir la section
    :ref:`Tables d`attributs <table-attributes>` pour des détails).
    Si vous fournissez un chemin relatif, il est déterminé par rapport à ``/etc/brltty``
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>`
    pour plus de détails).
    L'extension ``.atb`` est facultative.
    Le comportement par défaut est d'utiliser la table compilée en dur (voir l'option de
    compilation :ref:`--with-attributes-table <build-attributes-table>`).
    Vous pouvez vous passer de cette ligne de commande avec l'option
    en ligne de commande :ref:`-a <options-attributes-table>`.

.. _configure-braille-device:

``braille-device`` *device*\ ``,``...
    Spécifie le périphérique auquel l'afficheur braille est connecté
    (voir la section :ref:`Spécification du périphérique braille <operand-braille-device>`).
    Voir l'option de compilation :ref:`--with-braille-device <build-braille-device>`
    pour les paramètres par défaut établis durant la procédure de compilation.
    Vous pouvez vous passer de cette ligne de commande avec
    l'option :ref:`-d <options-braille-device>`.

.. _configure-braille-driver:

``braille-driver`` *pilote*\ ``,``...\|\ ``auto``
    Spécifie le pilote de l'afficheur braille (voir la section
    :ref:`Spécification du pilote <operand-driver>`). Par défaut, c'est
    une  autodétection  qui est effectuée.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-b <options-braille-driver>`.

.. _configure-braille-parameters:

``braille-parameters`` [*pilote*\ ``:``]*nom*\ ``=`` *valeur*\ ``,``...
    Spécifie les paramètres pour les pilotes de l'afficheur
    braille. Si vous spécifiez le même paramètre plus d'une fois,
    c'est sa valeur placée le plus à droite qui est utilisée. Si le
    nom d'un paramètre est qualifié par celui d'un pilote (voir la
    section :ref:`Codes d`identification de pilote <drivers>`)
    ce paramètre ne s'applique qu'à ce pilote; sinon il s'applique à
    tous les pilotes. Par défaut, c'est une  autodétection  qui est effectuée.
    Pour une description des paramètres acceptés par un pilote en
    particulier, voyez la documentation de ce pilote.
    Voir l'option de compilation :ref:`--with-braille-parameters <build-braille-parameters>`
    pour les paramètres par défaut établis lors de la procédure de
    compilation.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-B <options-braille-parameters>`.

.. _configure-contraction-table:

``contraction-table`` *fichier*
    Spécifie la table de braille abrégé (voir la section :ref:`Braille abrégé <table-contraction>` pour des détails).
    Si vous fournissez un chemin relatif, il est déterminé par rapport à ``/etc/brltty``
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>` pour
    plus de détails.
    L'extension ``.ctb`` est facultative.
    La table de braille abrégé est utilisée quand la fonction de braille
    6 points est activée (voir la commande :ref:`SIXDOTS <command-SIXDOTS>`
    et la préférence :ref:`Apparence du texte <preference-text-style>`).
    Par défaut, l'affichage est en braille 6 points intégral.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-c <options-contraction-table>`.
    Elle n'est pas disponible si l'option de compilation :ref:`--disable-contracted-braille <build-contracted-braille>`
    a été spécifiée.

.. _configure-key-table:

``key-table`` *file*\ \\|\ ``auto``
    Spécifie la table de touches
    (voir la section :ref:`Tables de touches <table-key>` pour plus de détails).
    Si vous fournissez un chemin relatif, il est ancré sur ``/etc/brltty``
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>` pour plus de
    détails).
    L'extension ``.ktb`` est facultative.
    Par défaut, aucune table de touches n'est utilisée.
    L'effet de cette instruction peut être annulé par l'option en ligne de
    commande
    :ref:`-k <options-key-table>`.

.. _configure-keyboard-properties:

``keyboard-properties`` *name*\ ``=`` *value*\ ``,``...
    Spécifie les propriétés du/des claviers(s) qui vont être surveillés (monitored).
    Si vous spécifiez plus d'une fois la même propriété, sa valeur la plus à
    droite est utilisée.
    Voir la section :ref:`Propriétés du clavier <keyboard-properties>`
    pour une liste des propriétés que vous pouvez spécifier. Par défaut, on gère
    tous les claviers. Vous pouvez annuler l'effet de cette instruction avec
    l'option en ligne de commande
    :ref:`-K <options-keyboard-properties>`.


.. _configure-midi-device:

``midi-device`` *device*
    Spécifie le périphérique à utiliser pour l'interface MIDI
    (voir la section :ref:`Spécification du périphérique MIDI <operand-midi-device>`).
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-m <options-midi-device>`.
    Elle n'est pas disponible si vous avez spécifié l'option de
    compilation :ref:`--disable-midi-support <build-midi-support>`.

.. _configure-pcm-device:

``pcm-device`` *device*
    Spécifie le périphérique à utiliser pour le son (voir la
    section :ref:`Spécification du
    périphérique PCM <operand-pcm-device>`).
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-p <options-pcm-device>`.
    Elle n'est pas disponible si vous avez spécifié l'option de
    compilation :ref:`--disable-pcm-support <build-pcm-support>`.

.. _configure-screen-parameters:

``screen-parameters`` *name*\ ``=`` *value*\ ``,``...
    Spécifie les paramètres pour le pilote d'écran.
    Si le même paramètre est spécifié plus d'une fois, sa valeur la
    plus à droite est utilisée. Pour une description des paramètres
    acceptés par un pilote en particulier, voyez la documentation de
    ce pilote. Voir l'option de compilation :ref:`--with-screen-parameters <build-screen-parameters>`
    pour les paramètres par défaut établis pendant la procédure de
    compilation.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-X <options-screen-parameters>`.

.. _configure-release-device:

``release-device`` *boolean*
    Crée ou non le périphérique auquel l'afficheur braille est connecté lorsque
    l'écran ou la fenêtre courant ne peuvent pas être lus.


on
Créer le périphérique.

off
Ne pas créer le périphérique.

    Par défaut, le réglage est ``on`` sur les plateformes Windows et
    ``off`` on sur les autres plateformes. Vous pouvez annuler l'effet de
    cette instruction avec l'option en ligne de commande
    :ref:`-r <options-release-device>`.

.. _configure-screen-driver:

``screen-driver`` *driver*
    Voir l'option de compilation :ref:`--with-screen-driver <build-braille-device>`.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-x <options-screen-driver>`.

``screen-parameters`` *pilote**nom*\ ``=`` *valeur*\ ``,``...
    Spécifie les paramètres pour les pilotes d'écran.
    Si le même paramètre est spéciié plus d'une fois, sa valeur la
    plus à droite est utilisée. Si un nom de paramètre est affecté à un pilote
    (voir section Pilotes d'écran supportés), ce paramètre ne s'applique qu'au
    pilote; sinon il s'applique à tous les pilotes. Pour une description des paramètres
    acceptés par un pilote en particulier, voir la documentation de
    ce pilote. Voir l'option de compilation :ref:`--with-screen-parameters <build-screen-parameters>`
    pour les paramètres par défaut établis pendant la procédure de
    compilation.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-X <options-screen-parameters>`.


.. _configure-speech-driver:

``speech-driver`` *driver*\ ``,``...\|\ ``auto``
    Spécifie le pilote de synthèse vocales (voir la section
    :ref:`Spécification du pilote de synthèse <operand-driver>`). Par
    défaut, une autodétection est effectuée.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-s <options-speech-driver>`.
    Elle n'est pas disponible si vous avez spécifié l'option de
    compilation :ref:`--disable-speech-support <build-speech-support>`.

.. _configure-speech-input:

``speech-input`` *file*
    Spécifie le nom de l'objet du système de fichiers (FIFO, tuyau (pipe)
    nommé, socket nommée, etc) que d'autres applications peuvent utiliser pour
    convertir du texte en parole avec le pilote de synthèse de BRLTTY.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-F <options-speech-input>`.
    Elle n'est pas disponible si vous avez spécifié :ref:`--disable-speech-support <build-speech-support>`.

.. _configure-speech-parameters:

``speech-parameters`` [*driver*\ ``:``]*name*\ ``=`` *value*\ ``,``...
    Spécifie les paramètres pour les pilotes de synthèse vocale. Si
    vous spécifiez plus d'une fois le paramètre, c'est sa valeur le
    plus à droite qui est utilisée.
    Si le nom d'un paramètre est qualifié par le nom d'un pilote (voir
    la section :ref:`Codes d`identification de pilote <drivers>`)
    ce paramètre s'applique seulement à ce pilote; sinon il s'applique
    pour tous les pilotes.
    Pour une description des paramètres acceptés par un pilote en
    particulier, voyez la documentation de ce pilote. Voir l'option de
    compilation :ref:`--with-speech-parameters <build-speech-parameters>`
    pour les paramètres par défaut établis durant la procédure de compilation.
    Vous pouvez éviter cette ligne avec l'option en ligne de commande
    :ref:`-S <options-speech-parameters>`.

.. _configure-text-table:

``text-table`` *file*\ \\|\ ``auto``
    Spécifie la table de texte (voir la section :ref:`Tables de texte <table-text>` pour les détails).
    Si vous fournissez un chemin relatif, il est déterminé par rapport à ``/etc/brltty``
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>`.
    pour plus de détails).
    L'extension ``.ttb`` est facultative.
    Pour un nom de fichier simple, le préfixe ``text.`` est facultatif.
    Par défaut, on réalise une auto-sélection à partir de la locale
    avec utilisation de la table compilée en dur en cas de problème
    (voir l'option de compilation :ref:`--with-text-table <build-text-table>`).
    L'effet de cette instruction peut être annulé avec l'option en ligne de commande
    :ref:`-t <options-text-table>`.


.. _options:

Options en ligne de commande
----------------------------

Un grand nombre de paramètres peuvent être spécifiés explicitement
lorsque vous appelez BRLTTY. La commande ``brltty`` accepte les options
suivantes:


.. _options-attributes-table:

``-a`` *file* ``--attributes-table=`` *file*
    Spécifie la table d'attributs (voir la section
    :ref:`Tables d`attributs <table-attributes>` pour les détails).
    Si vous fournissez un chemin relatif, il est déterminé par rapport à ``/etc/brltty``
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>`
    pour plus de détails).
    L'extension ``.atb`` est facultative. Si vous ne spécifiez pas cette
    option, on suppose que c'est ``left_right``. Passez-la à ``invleft_right``
    si vous souhaitez l'ancien comportement.
    Voir la ligne :ref:`attributes-table <configure-attributes-table>`
    du fichier de configuration pour la sélection des paramètres par défaut
    lors de l'exécution.
    Vous pouvez changer ce paramètre avec la préférence :ref:`Table d`attributs <preference-attributes-table>`.

.. _options-braille-driver:

``-b`` *driver*\ ``,``...\|\ ``auto`` ``--braille-driver=`` *driver*\ ``,``...\|\ ``auto``
    Spécifie le pilote de l'afficheur braille (voir la section
    :ref:`Spécification du pilote <operand-driver>`).
    Voir la ligne :ref:`braille-driver <configure-braille-driver>`
    du fichier de configuration pour les paramètres par défaut
    à l'exécution.

.. _options-contraction-table:

``-c`` *fichier* ``--contraction-table=`` *fichier*
    Spécifie la table de braille abrégé (voir la section :ref:`Braille abrégé <table-contraction>`
    pour les détails).
    Si vous fournissez un chemin relatif, il est rattaché à ``/etc/brltty``.
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>` pour plus de
    détails).
    L'extension ``.ctb`` est facultative.
    La table de braille abrégé est utilisée lorsque la possibilité braille 6
    points est activée (voir la commande :ref:`SIXDOTS <command-SIXDOTS>`
    et la préférence :ref:`Apparence du texte <preference-text-style>`).
    Voir la ligne :ref:`contraction-table <configure-contraction-table>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.
    Vous pouvez changer ce paramètre avec
    la préférence :ref:`Table de braille abrégé <preference-contraction-table>`.
    Cette option n'est pas disponible si l'option de compilation
    :ref:`--disable-contracted-braille <build-contracted-braille>`
    a été spécifiée.

.. _options-braille-device:

``-d`` *peripherique*\ ``,``... ``--braille-device=`` *device*\ ``,``...
    Spécifie le périphérique auquel l'afficheur braille est connecté (voir la
    section :ref:`Spécification du périphérique braille <operand-braille-device>`).
    Voir la ligne :ref:`braille-device <configure-braille-device>`
    du fichier de configuration pour les paramètres par défaut à
    l'exécution.

.. _options-standard-error:

``-e`` ``--standard-error``
    Ecrit les messages de diagnostic sur la console d'erreur standard (stderr).
    Par défaut, ils s'enregistrent dans ``syslog``.

.. _options-configuration-file:

``-f`` *fichier* ``--configuration-file=`` *file*
    Spécifie l'emplacement du :ref:`fichier de configuration <configure>`
    qui doit être utilisé pour l'établissement des paramètres par défaut à l'exécution.

.. _options-help:

``-h`` ``--help``
    Affiche un résumé des options en ligne de commande acceptées par BRLTTY,
    puis quitte.

.. _options-speech-input:

``-i`` *nom* ``--speech-input=`` *nom*
    Spécifie le nom de l'objet du système de fichiers (FIFO, tuyau (pipe) nommé,
    socket nommée, etc) que les applications peuvent utiliser pour convertir du
    texte en parole avec le pilote de synthèse de BRLTTY. Si elle n'est pas
    spécifiée, l'objet du système de fichiers n'est pas créé.
    Voir la ligne :ref:`speech-input <configure-speech-input>` du
    fichier de configuration pour le réglage par défaut au moment de l'exécution.
    Cette option n'est pas disponible si vous avez spécifié l'option de
    compilation :ref:`--disable-speech-support <build-speech-support>`.

.. _options-key-table:

``-k`` *file* ``--key-table=`` *file*
    Spécifie la table de touches
    (voir la section :ref:`Key Tables <table-key>` pour plus de détails).
    Si vous fournissez un chemin relatif, il est ancré à ``/etc/brltty``
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>` pour plus de détails).
    L'extension ``.ktb`` est facultative. Voir l'instruction
    :ref:`key-table <configure-key-table>` du fichier de
    configuration pour le paramètre d'exécution par défaut. Vous pouvez modifier
    ce paramètre avec la préférence
    :ref:`Table de touches <preference-key-table>`.


.. _options-log-level:

``-l`` *niveau* ``--log-level=`` *level*
    Spécifie le niveau de sécurité pour l'émission des messages de diagnostique.
    Les niveaux suivants sont reconnus.


0
emergency (urgence)

1
alert (avertissement)

2
critical (critique)

3
error (erreur)

4
warning (attention)

5
notice (note)

6
information

7
debug (débogage)

    Vous pouvez fournir soit le numéro soit le nom, et vous pouvez abréger le nom.
    Si vous ne spécifiez pas cela, c'est ``information`` qui est utilisé
    (voir l'option :ref:`-q <options-quiet>` pour plus de détails).

.. _options-midi-device:

``-m`` *périphérique* ``--midi-device=`` *device*
    Spécifie le pilote à utiliser pour l'interface MIDI.
    (voir la section :ref:`Spécification du périphérique MIDI <operand-midi-device>`).
    Voir la ligne :ref:`midi-device <configure-midi-device>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.
    Cette option n'est pas disponible si vous avez spécifié l'option de
    compilation :ref:`--disable-midi-support <build-midi-support>`.

.. _options-no-daemon:

``-n`` ``--no-daemon``
    Indique à BRLTTY qu'il doit rester au premier plan.
    Si cette option n'est pas spécifiée, BRLTTY devient un processus en
    tâche de fond (démon) après s'être initialisé mais avant de démarrer tous
    les pilotes sélectionnés.

.. _options-pcm-device:

``-p`` *périphérique* ``--pcm-device=`` *device*
    Spécifie le périphérique à utiliser pour le son (voir la section
    (:ref:`Spécification du périphérique PCM <operand-pcm-device>`).
    Voir la ligne :ref:`pcm-device <configure-pcm-device>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.
    Cette option n'est pas disponible si vous avez spécifié l'option de
    compilation :ref:`--disable-pcm-support <build-pcm-support>`.

.. _options-quiet:

``-q`` ``--quiet``
    Met moins d'informations dans le journal. Cette option passe le niveau de
    journalisation (voir l'option :ref:`-l <options-log-level>`
    à ``notice`` si vous avez spécifié l'option :ref:`-v <options-verify>`
    ou :ref:`-V <options-version>`,
    et, sinon, à ``warning``.

.. _options-release-device:

``-r`` ``--release-device=``...\|\ ``auto``
    Prend en compte le périphérique auquel est connecté l'afficheur braille
    lorsque  l'écran courant ou la fenêtre  ne peuvent pas être lus.
    Voir la ligne :ref:`release-device <configure-speech-driver>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.

.. _options-speech-driver:

``-s`` *périphérique*\ ``,``...\|\ ``auto`` ``--speech-driver=`` *pilote*\ ``,``...\|\ ``auto``
    Spécifie le pilote de la synthèse vocale (voir la section
    :ref:`Spécification de pilote <operand-driver>`).
    Voir la ligne :ref:`speech-driver <configure-speech-driver>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.
    Cette option n'est pas disponible si vous avez spécifié l'option de compilation
    :ref:`--disable-speech-support <build-speech-support>`.

.. _options-text-table:

``-t`` *file* ``--text-table=`` *file*
    Spécifie la table de texte (voir la section
    :ref:`Tables de texte <table-text>` pour les détails).
    Si vous fournissez un chemin relatif, il est rattaché à ``/etc/brltty``
    (voir les options de compilation :ref:`--with-data-directory <build-data-directory>`
    et :ref:`--with-execute-root <build-execute-root>` pour plus de
    détails).
    L'extension ``.ttb`` est facultative.
    Pour un nom de fichier simple, le péfixe ``text.`` est facultatif.
    Voir la ligne :ref:`text-table <configure-text-table>`
    du fichier de configureation pour les paramètres par défaut à l'exécution.
    Vous pouvez changer ce paramètre avec la préférence :ref:`Table de texte <preference-text-table>`.

.. _options-verify:

``-v`` ``--verify``
    Affiche les versions courantes de BRLTTY, du côté serveur de son
    API, et des pilotes de synthèse
    et de braille sélectionnés, puis quitte.
    Si l'option :ref:`-q <options-quiet>` n'est pas spéciifiée,
    affiche aussi les valeurs des
    options après que toutes les sources aient été examinées.
    Si vous avez spécifié plus d'un pilote braille (voir l'option en ligne de
    commande :ref:`-b <options-braille-driver>`) et/ou plus d'un
    périphérique braille (voir l'option en ligne de commande :ref:`-d <options-braille-device>`),
    une autodétection de l'afficheur braille est entreprise.
    Si vous avez spécifié plus d'un pilote de synthèse (voir l'option en ligne
    de commande :ref:`-s <options-speech-driver>`), une autodétection
    de la synthèse vocale est entreprise.

.. _options-screen-driver:

``-x`` *pilote* ``--screen-driver=`` *pilote*...
    Spécifie le pilote d'écran (voir la section Pilotes d'écran supportés).
    Voir la ligne :ref:`screen-driver <configure-braille-device>`
    du fichier de configuration pour les paramètres par défaut à
    l'exécution.

.. _options-api-parameters:

``-A`` *nom*\ ``=`` *valeur*\ ``,``... ``--api-parameters=`` *name*\ ``=`` *value*\ ``,``...
    Spécifie les paramètres pour l'API.
    Si vous spécifiez le même paramètre plus d'une fois, c'est sa valeur la
    plus à droite qui est utilisée. Pour une description des paramètres acceptés
    par l'interface, reportez-vous au manuel de référence **BrlAPI**.
    Voir la ligne :ref:`api-parameters <configure-api-parameters>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.

.. _options-braille-parameters:

``-B``\ [*pilote*\ ``:``]*nom*\ ``=`` *valeur*\ ``...`` ``--braille-parameters=``\ [*pilote*\ ``:``]*nom*\ ``=`` *valeur*\ ``,``...
    Spécifie les paramètres pour les pilotes des afficheurs braille.
    Si vous spécifiez le même paramètre plus d'une fois, c'est sa valeur le
    plus à droite qui est utilisée. Si vous qualifiez le nom d'un paramètre par un pilote, (voir
    la section :ref:`Codes d`identification des pilotes <drivers>`) ce paramètre
    ne s'applique qu'à ce pilote; sinon il s'applique à tous les pilotes.
    Pour une description des paramètres acceptés par un pilote en particulier,
    reportez-vous à la documentation de ce pilote.
    Voir la ligne :ref:`braille-parameters <configure-braille-parameters>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.

.. _options-environment-variables:

``-E`` ``--environment-variables``
    Reconnaît les variables d'environnement quand les paramètres par défaut
    pour des options en ligne de commandes non spécifiées sont déterminées
    (voir la section :ref:`Options en ligne de commande <options>`).
    Si vous spécifiez cette option, et si vous définissez une variable
    d'environnement associée à une option non spécifiée, c'est la valeur de cette variable d'environnement qui est
    utilisée. Les noms de ces variables d'environnement sont basés sur les noms
    longs des options auxquelles elles correspondent:

      - Toutes les lettres sont en majuscule.
      - Les soulignements (``_``) sont utilisés au lieu du signe moins
        (``-``).

      - Le préfixe ``BRLTTY_`` est ajouté.

    Cette option est particulièrement utile sur les systèmes d'exploitation
    Linux dans le sens où elle permet de passer des paramètres par défaut à BRLTTY
    via les paramètres du boot. Les variables d'environnement suivantes sont
    supportées:


``BRLTTY_API_PARAMETERS``

        Paramètres pour l'API (voir l'option
        en ligne de commande :ref:`-A <options-api-parameters>`).

``BRLTTY_ATTRIBUTES_TABLE``

        La table d'attributs (voir l'option en ligne de
        commande :ref:`-a <options-attributes-table>`).

``BRLTTY_BRAILLE_DEVICE``

        Le périphérique de l'afficheur braille (voir l'option en ligne de commande
        :ref:`-d <options-braille-device>`).

``BRLTTY_BRAILLE_DRIVER``

        Le pilote de l'afficheur braille (voir l'option en ligne de commande
        :ref:`-b <options-braille-driver>`).

``BRLTTY_BRAILLE_PARAMETERS``

        Paramètres pour le pilote de l'afficheur braille (voir l'option en ligne
        de commande :ref:`-B <options-braille-parameters>`).

``BRLTTY_CONFIGURATION_FILE``

        Le fichier de configuration (voir l'option
        :ref:`-f <options-configuration-file>` de la ligne de commande).

``BRLTTY_KEY_TABLE``

        La table de touches
        (voir l'option en ligne de commande :ref:`-k <options-key-table>`).

``BRLTTY_KEYBOARD_PROPERTIES``

        Les propriétés du clavier
        (voir l'option en ligne de commande :ref:`-K <options-keyboard-properties>`).

``BRLTTY_CONTRACTION_TABLE``

        La table de braille abrégé (voir l'option :ref:`-c <options-contraction-table>`

       en ligne de commande).

``BRLTTY_MIDI_DEVICE``

        Le périphérique pour l'interface MIDI (voir
        l'option :ref:`-m <options-midi-device>` en ligne de commande).

``BRLTTY_PCM_DEVICE``

        Le périphérique audio (voir l'option :ref:`-p <options-pcm-device>`
        en ligne de commande).

``BRLTTY_PREFERENCES_FILE``

        L'emplacement du fichier qui doit être utilisé pour sauvegarder et charger
        les préférences de l'utilisateur.(voir l'option
        :ref:`-F <options-preferences-file>` en ligne de commande).

``BRLTTY_RELEASE_DEVICE``

        Crée ou non le périphérique auquel l'afficheur est connecté quand
        l'écran ou la fenêtre suivant ne peuvent pas être lus
        (voir l'option :ref:`-r <options-release-device>` de la ligne de commande).

``BRLTTY_SCREEN_PARAMETERS``

        Paramètres pour le pilote d'écran (voir l'option :ref:`-X <options-screen-parameters>`
        en ligne de commande).

``BRLTTY_SPEECH_DRIVER``

        Le pilote de la synthèse vocale (voir l'option :ref:`-s <options-speech-driver>`
        en ligne de commande).

``BRLTTY_SPEECH_INPUT``

        Le nom de l'objet du système de fichiers que d'autres applications
        peuvent utiliser pour convertir du texte en parole avec le pilote de
        synthèse de BRLTTY (voir l'option :ref:`-i <options-speech-input>`
        en ligne de commande).

``BRLTTY_SPEECH_PARAMETERS``

        Paramètres pour le pilote de la synthèse vocale (voir l'option
        :ref:`-S <options-speech-parameters>` en ligne de commande).

``BRLTTY_TEXT_TABLE``

        La table de texte (voir l'option
        :ref:`-a <options-text-table>` en ligne de commande).


.. _options-preferences-file:

``-F`` *file* ``--preferences-file=`` *file*
    Spécifie l'emplacement du fichier qui doit être utilisé pour sauvegarder et
    charger les les préférences de l'utilisateur. Si vous spécifiez un chemin
    relatif, il est encré sur ``/var/lib/brltty``. Voir la ligne
    :ref:`preferences-file <configure-preferences-file>` du fichier de
    configuration pour le réglage par défaut au moment de l'exécution.
    qui doit être utilisé par les autres applications souhaitant avoir
    accès au pilote de synthèse de BRLTTY. Si ce n'est pas spécifié,
    aucun FIFO n'est créé. Voir la ligne
    :ref:`speech-fifo <configure-speech-input>` du fichier de
    configuration pour les paramètres par défaut à l'exécution.
    Cette option n'est pas disponible si vous avez spécifié l'option
    de compilation
    :ref:`--disable-speech-support <build-speech-support>`.

.. _options-install-service:

``-I`` ``--install-service``
    Installe BRLTTY et le service BrlAPI.
    Cela signifie que:

      - BRLTTY sera lancé automatiquement quand le système démarrera.
      - Les applications peuvent savoir qu'un serveur BrlAPI est lancé.

    Cette option n'est supportée que sur la plateforme Windows.

.. _options-keyboard-properties:

``-K`` *name*\ ``=`` *value*\ ``,``... ``--keyboard-properties=`` *name*\ ``=`` *value*\ ``,``...
    Spécifie les propriétés du/des clavier(s) à prendre en charge. Si vous spécifiez
    la même propriété plus d'une fois, la valeur la plus à droite est utilisée.
    Voir la section :ref:`Propriétés du clavier <keyboard-properties>`
    pour une liste des propriétés que vous pouvez spécifier.
    Voir l'instruction :ref:`keyboard-properties <configure-keyboard-properties>`
    du fichier de configuration pour les paramètres d'exécution par défaut.

.. _options-message-timeout:

``-M`` *csecs* ``--message-timeout=`` *csecs*
    Spécifie le temps (en centièmes de seconde) que prend BRLTTY pour
    afficher ses propres messages internes sur l'afficheur braille. Si
    ce n'est pas spécifié, c'est ``400`` (4 secondes) qui est utlisé.

.. _options-no-speech:

``-N`` ``--no-api``
    Désactive l'interface de programmation de l'application.

.. _options-pid-file:

``-P`` *file* ``--pid-file=`` *file*
    Spécifie le fichier à l'intérieur duquel BRLTTY doit écrire ses
    identifiants de processus (pid). Si cela n'est pas spécifié,
    BRLTTY écrit ses identifiants de processus nulle part.

.. _options-remove-service:

``-R`` ``--remove-service``
    Supprime le service BrlAPI.
    Cela signifie que:

      - BRLTTY ne sera pas lancé automatiquement quand le système démarrera.
      - Les applications peuvent savoir qu'aucun serveur BrlAPI n'est lancé.

    Cette option n'est supportée que sur la plateforme Windows.

``-S``\ [*driver*\ ``:``]*name*\ ``=`` *value*\ ``,``...
   ``--speech-parameters=``\ [*driver*\ ``:``]*name*\ ``=`` *value*\ ``,``...

.. _options-speech-parameters:

    Spécifie les paramètres pour les pilotes de synthèse vocale.
    Si vous spécifiez plus d'une fois le même paramètre, c'est sa
    valeur la plus à droite qui est utilisée. Si un nom de paramètre
    est qualifié par un pilote (voir la section
    :ref:`Code d`identification de pilote <drivers>`)
    ce paramètre ne s'applique qu'à ce pilote; sinon il s'applique à
    tous les pilotes. Pour une description des paramètres acceptés par
    un pilote en particulier, voyez la documentation de ce pilote.
    Voir la ligne :ref:`speech-parameters <configure-speech-parameters>`
    du fichier de configuration pour les paramètres par défaut à l'exécution.

.. _options-version:

``-V`` ``--version``
    Affiche les versions courantes de BRLTTY lui-même, de la partie
    serveur de son API, et des
    pilotes qui ont été liés à son binaire, puis quitte. Si vous
    ne spécifiez pas l'option :ref:`-q <options-quiet>`,
    affiche aussi les informations légales.

.. _options-screen-parameters:

``-X`` *nom*\ ``=`` *valeur*\ ``,``... ``--screen-parameters=`` *pilote*\ ``=`` *nom*\ ``valeur``\ ...
    Spécifie les paramètres pour les pilotes d'écran.
    Si vous spécifiez plus d'une fois le même paramètre, c'est sa
    valeur la plus à droite qui est utilisée. Si un paramètre est réservé
    à un pilote (voir la section Pilotes d'écran supportés), ce paramètre
    ne s'applique qu'à ce pilote; sinon il s'applique à tous les pilotes.
    Pour une description des paramètres acceptés par un pilote en particulier,
    reportez-vous à sa documentation. Voir la ligne :ref:`screen-parameters <configure-screen-parameters>`
    du fichier de configuration pour la sélection des paramètres par
    défaut à l'exécution.
