Description des possibilités
============================


.. _routing:

Routine curseur
---------------

Lorsque vous déplacez la plage braille sur l'écran en examinant le texte dans
un éditeur, vous avez souvent besoin d'amener le curseur à un caractère en
particulier à l'intérieur de la plage braille.
Vous trouverez que c'est une tâche plutôt difficile pour un certain nombre de
raisons. L'une d'entre elles est qu'il se peut que vous ne sachiez pas où se
trouve le curseur, et vous avez peut-être perdu votre position en essayant de le
trouver.
Une autre est que le curseur peut se déplacer de façon imprévisible lorsque
vous appuyez sur les flèches de direction (certains éditeurs, par exemple,
n'autorisent pas le curseur à aller plus à droite que la fin de la ligne où il
se trouve).
La routine-curseur fournit cette possibilité en sachant où se trouve le curseur,
en simulant les appuis sur les flèches de direction que vous devriez
entrer manuellement, et en affichant l'évolution du curseur pendant qu'il se
déplace.

Certains afficheurs braille ont un bouton, appelé routine-curseur, au-dessus de
chaque cellule. Ces touches utilisent la commande
:ref:`ROUTE <command-ROUTE>` pour amener le curseur précisément à
l'emplacement désiré.

La routine-curseur, bien que très pratique et très efficace, n'est pas,
à proprement parler, totalement fiable. Une des raisons de cela est que sa
conception actuelle utilise des séquences d'échappement de touches du curseur
VT100.

Une autre est que certaines applications font des choses non standards pour
réagir lorsqu'elles détectent l'appui sur les touches du curseur.
Un problème mineur trouvé à l'intérieur de certains éditeurs (comme ``vi``),
comme mentionné ci-dessus, est qu'ils se précipitent dans un déplacement
vertical imprévisible lorsque vous demandez un déplacement vertical, car elles
n'autorisent pas le curseur à se placer à droite de la fin d'une ligne.
Un problème majeur trouvé dans certains navigateurs web (comme
``lynx``), est que les flèches haut et bas sont utilisées pour se déplacer
parmi les liens (ce qui peut sauter des lignes et/ou déplacer horizontalement
le curseur mais rarement déplacer le curseur d'une ligne dans la direction
désirée), et que les flèches gauche et droite sont utilisées pour sélectionner
les liens (ce qui n'a absolument rien à voir avec une quelconque forme de
déplacement de curseur, et qui change même totalement le contenu de l'écran).

Il se peut que la routine curseur ne fonctionne pas très bien sur les systèmes
lourds à se charger, et elle ne fonctionnera certainement pas très bien
lorsqu'elle tournera sous un vieux système ayant des liaisons lentes. Il en est
ainsi car tous les tests qui doivent être faits pendant le processus de façon à
traiter les mouvements imprévisibles du curseur et afin de s'assurer
qu'une erreur a au moins une chance d'être corrigée.
Bien que BRLTTY s'efforce d'être assez intelligent, il doit attendre encore
de voir ce qu'il se passe après chaque appui simulé sur une flèche de direction.

Une fois qu'une demande de routine-curseur a été faite, BRLTTY essaie d'amener
le curseur à la position désirée durant un certain délai avant
que le curseur n'atteigne cette position, le curseur semblant se déplacer dans la
mauvaise direction, ou vous passez à un terminal différent.
En premier, un effort est fait pour utiliser le déplacement vertical pour
amener le curseur à la bonne ligne, puis, uniquement si cela réussit, un effort
est fait pour utiliser le déplacement horizontal pour amener le curseur à la
bonne colonne. Si vous faites une autre demande alors qu'une routine est en train de
s'effectuer, la première est annulée et la deuxième est démarrée.

Une commande de routine curseur plus sûre mais moins puissante,
:ref:`CSRJMP_VERT <command-CSRJMP_VERT>`, utilise le déplacement
vertical pour amener le curseur n'importe où sur la première ligne de la
plage braille. Elle est surtout utile, jointe à certaines applications
(comme ``lynx``), dans lesquelles le déplacement du curseur à l'horizontal
ne doit jamais être tenté.

.. _cut:

Copier-coller
-------------

Cette possibilité vous permet d'extraire un texte qui est déjà sur l'écran
et de le réentrer à la position courante du curseur. Son utilisation vous fait
gagner du temps et permet d'éviter les erreurs lorsque vous avez besoin de copier un
texte long et/ou compliqué, et même quand vous avez besoin de copier plusieurs
fois le même texte court et simple. Elle est particulièrement utile pour des
choses telles que les noms de fichier longs, les lignes de commande compliquées,
les adresses mail, et les URLS. Copier et coller du texte passe par trois
étapes simples:

  #.
    Marquer le coin en haut à gauche de la zone rectangulaire ou le début de la
    zone linéaire sur l'écran, qui sera extraite (copiée). Si votre afficheur a
    des routines-curseur, déplacez la plage braille de façon à ce que le
    premier caractère à copier apparaisse quelque part à l'intérieur, puis:

      -
        appelez la commande :ref:`CUTBEGIN <command-CUTBEGIN>` pour

	démarrer un nouveau tampon
      -
        appelez la commande :ref:`CUTAPPEND <command-CUTAPPEND>` pour

	marquer le tampon copié existant.

    en pressant la/les touche(s) qui y sont associée(s), puis en appuyant sur la
    routine-curseur associée au caractère

  #.
    Marquer le coin en bas à droite de la zone rectangulaire ou la fin de
    la zone linéaire sur l'écran qui doit être extraite (copiée).
    Si votre afficheur a des routines-curseur, déplacez la plage braille
    de telle sorte que le dernier caractère à copier apparaisse quelque part
    à l'intérieur, puis

      -
        appelez la commande :ref:`CUTRECT <command-CUTRECT>` pour

	copier une zone rectangulaire.
      -
        appelez la commande :ref:`CUTLINE <command-CUTLINE>` pour

	copier une zone linéaire.

    en pressant les/la touche(s) associée(s) puis en pressant la routine-curseur
    associée au caractère. Le marquage
    de la fin de la région copiée dépose le contenu d'un écran copié dans le
    presse-papier.
    Les espaces superflus sont supprimés en fin de chaque ligne dans le
    presse-papier de façon à ce que les espaces non désirés qui en découlent ne
    soient pas collés.
    Les caractères de contrôle sont remplacés par des espaces.

  #.
    Insérez (coller) le texte là où vous avez besoin. Placez le curseur après le
    caractère où le texte doit être inséré, et appelez la commande
    :ref:`PASTE <command-PASTE>`. Vous pouvez copier le même texte
    autant de fois que vous le voulez sans le copier à nouveau.
    Cette description suppose que vous êtes déjà dans un mode d'entrée. Si vous
    collez alors que vous êtes dans un autre type de mode (comme le
    mode commande de ``vi``), vous devez alors bien être conscient de ce que
    les caractères du presse-papier pourront faire.


Le tampon copié est utilisé aussi par les commandes
:ref:`PRSEARCH/NXSEARCH <command-PRSEARCH-NXSEARCH>`.

.. _gpm:

Support du pointeur (souris) via GPM
------------------------------------

Si BRLTTY est configuré avec l'option de compilation
:ref:`--enable-gpm <gpm>` sur un système où l'application
``gpm`` a été installée, il réagira au pointeur (souris).

Le fait de bouger la plage braille déplace le pointeur (voir la préférence
:ref:`Le pointeur suit la fenêtre <preference-pointer-follows-window>`).
Le déplacement de la plage braille (manuel, par recherche du curseur etc.),
est appliqué non seulement quand il se déplace en réponse à un mouvement du pointeur, mais il
déplace aussi le pointeur au caractère à l'écran qui correspond au coin en haut à
gauche de la plage braille. Cela permet à une personne voyante de voir où se
trouve la plage braille et donc, de savoir ce que l'utilisateur brailliste
est en train de lire. Cela récupère aussi le pointeur dans la plage
braille de sorte que vous pouvez le
trouver facilement et que le périphérique du pointeur peut toujours être
utilisé comme un autre moyen de déplacer la plage braille.

Le déplacement du pointeur emmène la plage braille (voir la préférence
:ref:`La fenêtre suit le pointeur <preference-window-follows-pointer>`).
Chaque fois que vous déplacez le pointeur au-delà du bord de la plage
braille, la plage braille est emmenée tout au long du déplacement (un
caractère à la fois).
Cela donne à l'utilisateur braille une autre manière en deux dimensions
d'inspecter le contenu de l'écran ou de déplacer rapidement la plage braille
à un endroit désiré. Cela donne aussi à l'utilisateur voyant une façon
simple de déplacer la plage braille sur quelque chose qu'il aimerait que
l'utilisateur brailliste lise.

``gpm`` utilise la vidéo inversée pour montrer où se trouve le pointeur.
Le soulignement des caractères en surbrillance devrait donc être activé (voir
la commande :ref:`ATTRVIS <command-ATTRVIS>` pour des  détails) que
quand l'utilisateur brailliste veut utiliser le pointeur.

Cette fonctionnalité donne aussi à l'utilisateur brailliste accès à la
fonction copier-coller de ``gpm``. Bien que vous devriez lire la documentation
spécifique de ``gpm``, voici quelques remarques sur son fonctionnement.

  -
    Copiez le caractère courant dans le presse-papier par un simple clic sur le
    bouton gauche.

  -
    Copiez le mot courant (délimité par un espace) dans le presse-papier en
    cliquant deux fois sur le bouton gauche.

  -
    Copiez la ligne courante dans le presse papier en cliquant trois fois sur
    le bouton gauche.

  -
    Copiez une région linéaire vers le presse-papier comme suit:

      #.
        Placez le curseur sur le premier caractère de la région.

      #.
        Pressez (et maintenez) le bouton gauche.

      #.
        Déplacez le curseur au dernier caractère de la région (tous les

	caractères sélectionnés sont maintenant en surbrillance).
      #.
        Lâchez le bouton gauche.

  -
    Collez (insérez) le contenu courant du presse papier en cliquant sur le
    bouton central d'une souris à trois boutons, ou en cliquant sur le bouton
    droit d'une souris à deux boutons.

  -
    Marquez le presse-papier en utilisant le bouton droit d'une souris à trois
    boutons.


.. _tunes:

Sons d'avertissement
--------------------

BRLTTY vous avertit de l'exécution d'événements significatifs en jouant un son
bref prédéfini. Cette fonctionalité peut être activée et désactivée avec la
commande
:ref:`TUNES <command-TUNES>` ou la préférence
:ref:`Sons d`avertissement <preference-alert-tunes>`.
Les sons sont joués par le synthétiseur interne par défaut, mais vous pouvez
sélectionner d'autres alternatives avec la préférence
:ref:`Périphérique pour les sons <preference-tune-device>`.

Chaque événement significatif est associé, de la priorité la plus haute à la
plus faible, à un ou plusieurs des éléments suivants:


un son

    Si vous avez associé un nom à l'événement, si la préférence
    :ref:`Sons d`avertissement <preference-alert-tunes>` (voir aussi la
    commande :ref:`TUNES <command-TUNES>`) est active, et si le
    périphérique de son sélectionné (voir la préférence :ref:`Périphérique de son <preference-tune-device>`)
    peut être ouvert, le son est joué.

un schéma de points

    Si un type de signe a été associé à un événement, et si la préférence
    :ref:`Points d`avertissement <preference-alert-dots>` est active, le signe
    est brièvement affiché sur chaque cellule braille.
    Certains afficheurs braille ne réagissent pas assez vite pour que ce
    système fonctionne efficacement.

un message

    Si un message a été associé à l'événement, et si la préférence
    :ref:`Messages d`avertissement <preference-alert-messages>` est active,
    il est affiché pendant quelques secondes (voir l'option :ref:`-M <options-message-timeout>`
    en ligne de commande.


Ces événements incluent:

  -
    Le moment où le pilote de l'afficheur braille démarre ou s'arrête.

  -
    Le moment où une commande longue est terminée.

  -
    Le moment où une commande peut être exécutée.

  -
    Le moment où un repère est posé.

  -
    Le moment où le début ou la fin du presse-papier est posé.

  -
    Le moment où une caractéristique est activée ou désactivée.

  -
    Le moment où la poursuite du curseur est activé ou désactivé.

  -
    Le moment où l'image de l'écran est gelée ou rafraîchie.

  -
    Le moment où la plage braille entraîne la plage braille en bas au début de la
    ligne suivante, ou en haut à la fin de la ligne précédente.

  -
    Le moment où des lignes identiques sont sautées.

  -
    Le moment où un déplacement demandé ne peut pas être effectué.

  -
    Le moment où la routine-curseur commence, s'achève ou échoue.


.. _preferences:

Paramètres de préférence
------------------------

Quand BRLTTY démarre, il charge un fichier qui contient vos paramètres de
préférence. Il n'est pas indispensable que le fichier existe, et il est créé la
première fois que les paramètres sont sauvegardés avec la commande
:ref:`PREFSAVE <command-PREFSAVE>`.
Les paramètres sauvegardés le plus récemment peuvent être restaurés n'importe
quand par la commande :ref:`PREFLOAD <command-PREFLOAD>`.

Le nom de ce fichier est ``/etc/brltty-`` *pilote*\ ``.prefs``.
où *pilote* correspond aux deux lettres du
:ref:`code d`identification de pilote <drivers>`.

.. _preferences-menu:

Le menu préférences
~~~~~~~~~~~~~~~~~~~

Les paramètres de préférence sont sauvegardés sous forme de données binaires
que vous ne pouvez donc pas éditer à la main.
Cependant, BRLTTY a un menu simple à partir duquel vous pouvez facilement les
changer.

le menu est activé par la commande :ref:`PREFMENU <command-PREFMENU>`.
L'afficheur braille affiche brièvement (voir l'option
:ref:`-M <options-message-timeout>` en ligne de commande) le titre du
menu, puis présente l'item du paramètre actuel.

Navigation dans le menu
^^^^^^^^^^^^^^^^^^^^^^^

Voir :ref:`Commandes de navigation dans le menu <menu-navigation>` pour la
liste des commandes qui vous permettent de sélectionner l'élément, et de changer
la valeur dans le menu.
Par souci de compatibilité avec les vieux pilotes, les commandes
de déplacement dans la fenêtre, qui ont changé de sens dans ce contexte,
peuvent être aussi utilisées.


``Début``/``Fin``, ``Haut-Gauche``/``Bas-Gauche``, ``PAGE_PRECEDENTE``/``PAGE_SUIVANTE``

    Va au premier/dernier élément du menu (comme
    :ref:`MENU_FIRST_ITEM/MENU_LAST_ITEM <command-MENU_FIRST_ITEM-MENU_LAST_ITEM>`).

``FH``/``FB``, ``LIGNPRECEDENTE``/``LNSUIV``, ``CURSEUR_HAUT``/``CURSEUR_BAS``

    Va à l'élément précédent/suivant du menu (comme
    :ref:`MENU_PREV_ITEM/MENU_NEXT_ITEM <command-MENU_PREV_ITEM-MENU_NEXT_ITEM>`).

``FENETRE_PRECEDENTE``/``FENSUIV``, ``CARGAUCH``/``CADROITE``, ``CURSEUR_GAUCHE``/``CURSEUR_DROITE``, ``DEBUT``/``HOME``

    Déroule ou "enroule" la valeur de l'élément actuel dans le menu (comme
    :ref:`MENU_PREV_SETTING/MENU_NEXT_SETTING <command-MENU_PREV_SETTING-MENU_NEXT_SETTING>`).


Remarques:

  -
    Vous pouvez aussi utiliser les routine-curseur pour sélectionner une valeur
    pour l'élément actuel. Si un élément a des valeurs numériques, toute la ligne
    de routines-curseur agit comme une barre de défilement qui couvre toute la
    gamme des valeurs valides. Si un élément a une valeur nominale,
    les routines-curseur correspondent normalement aux valeurs.

  -
    Utilisez la commande ``PREFLOAD`` pour annuler tous les changements
    qui ont été faits depuis que vous êtes dans le menu.

  -
    Utilisez la commande ``PREFMENU`` (encore) pour donner effet
    aux nouvelles valeurs, sortir du menu et faire des
    opérations normales. De plus, si vous avez sélectionné l'option ``Enregistrer en quittant``, les
    nouvelles valeurs sont sauvegardées dans le fichier des préférences.
    Toute commande non reconnue par le système du menu fait la même chose.


Les éléments du menu
^^^^^^^^^^^^^^^^^^^^


.. _preference-save-on-exit:

Enregistrer en quittant
      Lors de la sortie du menu de préférences:


Non

        Ne sauvegarde pas automatiquement les paramètres de préférence.

Oui

        Sauvegarde automatiquement les paramètres de préférence.

    Le paramètre par défaut est ``Non``.

.. _preference-text-style:

Apparence du texte
    Lors de l'affichage du contenu de l'écran (voir la commande
    :ref:`DISPMD <command-DISPMD>`), montre les caractères:


8 points

        Avec les huit points.

6 points

        Avec 6 points seulement.

	Si vous avez sélectionné une table de braille abrégé (voir l'option
	:ref:`-c <options-contraction-table>` en ligne de commande et
	la ligne :ref:`contraction-table <configure-contraction-table>`
	du fichier de configuration), elle est utilisée.

    Vous pouvez aussi changer ce paramètre par la commande
    :ref:`SIXDOTS <command-SIXDOTS>`.

.. _preference-skip-identical-lines:

Passer les lignes identiques
    Quand vous vous déplacez de ligne en ligne en haut ou en bas avec les
    commandes :ref:`LNUP/LNDN <command-LNUP-LNDN>`,
    et lors de la fonction de défilement de lignes des commandes
    :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` et
    :ref:`FWINLTSKIP/FWINRTSKIP <command-FWINLTSKIP-FWINRTSKIP>`:


Non

        Ne passe pas les lignes qui ont le même contenu que la ligne actuelle.

Oui

        Passe les lignes déjà vues qui ont le même contenu que la ligne actuelle.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`SKPIDLNS <command-SKPIDLNS>`.

.. _preference-skip-blank-windows:

Passer les lignes vierges
    Lors d'un déplacement à gauche ou à droite avec les commandes
    :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>`:


Non

        Ne passe pas les fenêtres vides déjà lues.

Oui

        Passe les fenêtres vides.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`SKPBLNKWINS <command-SKPBLNKWINS>`.

.. _preference-which-blank-windows:

Quelles fenêtres vierges
    Si les fenêtres vides doivent être sautées:


Toutes

        Les passe toutes.

Fin de ligne

        Ne passe que celles qui sont à la fin (sur le côté droit) de l'écran.

Reste de la ligne

        Ne passe que celles qui sont à la fin (sur le côté droit) d'une ligne lors d'une lecture

	vers l'avant, et au début (sur le côté gauche) d'une ligne lors d'une
	lecture à reculons.


.. _preference-sliding-window:

Faire défiler la fenêtre
    Si le curseur est poursuivi (voir la commande
    :ref:`CSRTRK <command-CSRTRK>`)
    et que le curseur est trop enfermé (ou trop à l'extérieur) à la fin d'une plage braille:


Non

        Repositionne la plage horizontalement de sorte que son bord gauche

	soit un multiple de sa largeur à partir du bord gauche de l'écran.

Oui

        Repositionne la plage horizontalement de façon à ce que le curseur,

	tout en restant sur ce côté de la plage, soit plus proche du centre.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`SLIDEWIN <command-SLIDEWIN>`.

.. _preference-eager-sliding-window:

Eager Sliding Window
    Si la plage braille doit glisser:


Non

        La repositionne chaque fois que le curseur va au-delà de la fin.

Oui

        La repositionne chaque fois que le curseur va trop à l'intérieur près

	de la fin.

    Le paramètre initial est à ``non``.

.. _preference-window-overlap:

Chevauchement de fenêtre
    Lors d'un déplacement à gauche ou à droite avec les commandes
    :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>`, ce paramètre
    spécifie de combien de caractères adjacents horizontalement doit
    se couvrir la plage braille.
    Le paramètre initial est ``0``.

.. _preference-autorepeat:

Répétition automatique
    Tandis que la touche (la combinaison) d'une commande reste appuyée:


Non

        Ne répète pas automatiquement la commande.

Oui

        Répète automatiquement la commande selon un intervalle régulier après

	un délai initial.

    Les commandes suivantes peuvent être répétées automatiquement:

      -
        Les commandes :ref:`LNUP/LNDN <command-LNUP-LNDN>`.

      -
        Les commandes :ref:`PRDIFLN/NXDIFLN <command-PRDIFLN-NXDIFLN>`.

      -
        Les commandes :ref:`CHRLT/CHRRT <command-CHRLT-CHRRT>`.

      -
        Opérations de défilement automatique de la plage braille (voir
        la préférence "Répétition automatique du défilement").

      -
        Les opérations Page Précédente Page suivante.

      -
        Les opérations curseur sur ligne précédente et ligne suivante.

      -
        Les opérations curseur à gauche et curseur à droite.

      -
        Les opérations Effacer et Supprimer.

      -
        L'entrée d'un caractère.

    Seuls certains pilotes supportent cette fonctionnalité, la première
    limite étant que beaucoup d'afficheurs braille ne signalent pas
    les appuis sur une touche et les fonctions d'une touche comme des
    événements distinctement séparés.
    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`AUTOREPEAT <command-AUTOREPEAT>`.
    Le paramètre initial est ``oui``.

.. _preference-autorepeat-delay:

Répétition automatique du défilement
    Quand la préférence "Répétition automatique" est activée:


Non

        Ne répète pas automatiquement les opérations de défilement  de la
        plage braille.

Oui

        Répète automatiquement les opérations de défilement de la plage
        braille.


    Ce paramètre modifie le comportement des commandes "FWINLT/FWINRT". Le
    paramètre initial est ``non``.


Délai de la répétition automatique
    Lorsqu'un caractère doit être répété automatiquement, ce paramètre spécifie la valeur
    de le délai (voir la remarque sur :ref:`paramètres de temps <time-settings>` ci-dessous)
    qui doit s'écouler avant de commencer la répétition automatique.
    Le paramètre initial est ``50``.

.. _preference-autorepeat-interval:

Intervalle de la répétition automatique
    Lorsqu'un caractère doit être répété automatiquement, ce paramètre spécifie la valeur
    de temps (voir la remarque à propos de :ref:`paramètres de temps <time-settings>` ci-dessous)
    entre chaque réexécution.
    La valeur initiale est ``10``.

.. _preference-show-cursor:

Afficher le curseur
    Lors de l'affichage du contenu de l'écran (voir la commande
    :ref:`DISPMD <command-DISPMD>`):


Non

        N'affiche pas le curseur.

Oui

        Affiche le curseur.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`CSRVIS <command-CSRVIS>`.
    La valeur initiale est ``Oui``.

.. _preference-cursor-style:

Apparence du curseur
    Lorsque le curseur est affiché, il faut le représenter:


Souligné

        (souligné) Avec les points 7 et 8.

Pavé

      (un pavé) Avec les huit points.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`CSRSIZE <command-CSRSIZE>`.

.. _preference-blinking-cursor:

Clignotement du curseur
    Lorsque le curseur doit être affiché:


Non

        Le rend visible tout le temps.

YOui

        Le rend alternativement visible et invisible selon un intervalle

	prédéfini.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`CSRBLINK <command-CSRBLINK>`.

.. _preference-cursor-visible-time:

Durée de visibilité du curseur
    Quand le curseur doit clignoter, ce paramètre spécifie la durée (voir
    la remarque à propos des :ref:`paramètres de temps <time-settings>`
    ci-dessous) pendant laquelle il est visibile pendant chaque cycle.
    La valeur par défaut est ``40``.

.. _preference-cursor-invisible-time:

Durée d'invisibilité du curseur
    Lorsque le curseur doit clignoter, ce paramètre spécifie la durée (voir
    la remarque à propos des :ref:`paramètres de temps <time-settings>`
    ci-dessous) pendant laquelle il est invisibile pendant chaque cycle.
    La valeur initiale est ``40``.

.. _preference-show-attributes:

Afficher les attributs
    Lors de l'affichage du contenu de l'écran (voir la commande
    :ref:`DISPMD <command-DISPMD>`):


Non

        Ne souligne pas les caractères en surbrillance.

Oui

        Souligne les caractères en surbrillance.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`ATTRVIS <command-ATTRVIS>`.

.. _preference-blinking-attributes:

Clignottement des attributs
    Lorsque les caractères en surbrillance doivent clignoter:


Non

        Laisse l'indicateur visible tout le temps.

Oui

        Rend l'indicateur alternativement visible et invisible selon un

	intervalle prédéfini.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`ATTRBLINK <command-ATTRBLINK>`.

.. _preference-attributes-visible-time:

Durée de visibilité des attributs
    Quand le soulignement des caractères en surbrillance doit clignoter,
    ce paramètre spécifie la durée (voir la remarque à propos des
    :ref:`paramètres de temps <time-settings>` ci-dessous) pendant
    laquelle il est visibile pendant chaque cycle.
    La valeur initiale est ``20``.

.. _preference-attributes-invisible-time:

Durée d'invisibilité des attributs
    Quand le soulignement des caractères en surbrillance doit clignoter,
    ce paramètre spécifie la durée (voir la remarque à propos des
    :ref:`paramètres de temps <time-settings>` ci-dessous) pendant
    laquelle il est invisibile pendant chaque cycle.
    La valeur initiale est ``60``.

.. _preference-blinking-capitals:

Clignottement des majuscules
    Lors de l'affichage du contenu de l'écran (voir la commande
    :ref:`DISPMD <command-DISPMD>`):


Non

        Laisse les lettres en majuscule visibles tout le temps.

Oui

        Rend les lettres en majuscule alternativement visibles et invisibles

	selon un intervalle prédéfini.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`CAPBLINK <command-CAPBLINK>`.

.. _preference-capitals-visible-time:

Durée de visibilité des majuscules
    Lorsque les lettres en majuscule doivent clignoter, ce paramètre
    spécifie la durée (voir la remarque à propos des
    :ref:`paramètres de temps <time-settings>` ci-dessous) pendant
    laquelle elles sont visibile pendant chaque cycle.
    La valeur par défaut est ``60``.

.. _preference-capitals-invisible-time:

Durée d'invisibilité des majuscules
    Lorsque les lettres en majuscule doivent clignoter, ce paramètre
    spécifie la durée (voir la remarque à propos des
    :ref:`paramètres de temps <time-settings>` ci-dessous) pendant
    laquelle elles sont invisibiles pendant chaque cycle.
    La valeur par défaut est ``20``.

.. _preference-braille-firmness:

Rugosité du braille
    Règle la rugosité (ou la rigidité) des points braille.
    Elle peut être réglée à:

      - Maximum
      - Forte
      - Moyenne
      - Faible
      - Minimum

    Cette préférence n'est disponible que si vous utilisez un pilote qui
    la supporte. La valeur initiale est ``Moyenne``.

Sensibilité du braille
    Règle la sensibilité des points braille au teucher.
    Elle peut être réglée à:

      - Maximum
      - Haute
      - Moyenne
      - Faible
      - Minimum

    Cette préférence n'est disponible que si vous utilisez un pilote qui
    la supporte. La valeur initiale est ``Moyenne``.


.. _preference-window-follows-pointer:

La fenêtre suit le pointeur
    Lors du déplacement du pointeur (souris):


Non

        N'emmène pas la plage braille.

Oui

        Emmène la plage braille.

    Cette préférence n'est présentée que si l'option de compilation
    :ref:`--enable-gpm <gpm>` a été spécifiée.

.. _preference-pointer-follows-window:

Surlignement de la fenêtre
    Lors du déplacement de la plage braille:


Non

        Ne met pas en surbrillance la nouvelle zone de l'ùcran.

Oui

        Cette option active un marqueur visible montrant où se situe la
        plage braille, et, par conséquent, permettant de savoir ce que
        l'utilisateur brailliste est en train de lire. Tout mouvement de la
        plage braille (manuel, poursuite du curseur, etc.) autre que ceux
        répondant au mouvement du pointeur (voir la préférence
        :ref:`La fenêtre suit le pointeur <preference-window-follows-pointer>`)
        a pour conséquence que la zone de l'écran correspondant
        au nouvel endroit où se trouve la  plage braille est mise en
        surbrillance. Si la préférence "Afficher les attributs" est activée, seul le
        caractère correspondant au coin en haut à gauche de la plage braille
        est mis en surbrillance.

    Cette préférence n'est présentée que si l'option de compilation
    :ref:`--enable-gpm <gpm>` a été spécifiée.

.. _preference-alert-tunes:

Sons d'avertissement
    Chaque fois qu'un événement significatif avec un son associé se produit,
    (voir :ref:`Sons d`avertissement <tunes>`):


Non

        Ne joue pas le son.

Oui

        Joue le son.

    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`TUNES <command-TUNES>`.
    La valeur initiale est ``oui``.

.. _preference-tune-device:

Périphérique de son
    Joue les son d'avertissement via:


Beeper

        Le beeper interne (générateur de sons de la console).

	Cette valeur est supportée sur Linux, OpenBSD, FreeBSD, et
	NetBSD. Elle est toujours sûre dans son utilisation, bien qu'elle
	soit peut-être quelque peu rustique. Ce périphérique n'est pas
	disponible si vous avez spécifié l'option de compilation
        :ref:`--disable-beeper-support <build-beeper-support>`.

PCM

        L'interface audio de la carte son.
        Cette valeur est supportée sous Linux (via  ``/dev/dsp``),

	Solaris (via ``/dev/audio``), OpenBSD (via ``/dev/audio0``),
        FreeBSD (via ``/dev/dsp``), et NetBSD

	(via ``/dev/audio0``).
	Ne fonctionne pas quand ce périphérique doit déjà être utilisé par
	une autre application.
	Ce périphérique n'est pas disponible si vous avez spécifié l'option de
	compilation :ref:`--disable-pcm-support <build-pcm-support>`.

MIDI

        L'interface MIDI de la carte son. Cette valeur

	est supportée sous Linux (via ``/dev/sequencer``).
	Ne fonctionne pas quand ce périphérique est déjà utilisé par une
	autre application.
        Ce périphérique n'est pas disponible si vous avez spécifié l'option de

	compilation :ref:`--disable-midi-support <build-midi-support>`.

FM

        La synthèse FM sur une carte son AdLib, OPL3, Sound Blaster, ou

	équivalente. Cette valeur est supportée sous Linux. Elle fonctionne
	même si une synthèse FM est déjà utilisée par une autre application.
	Les résultats sont imprévisibles, et peuvent ne pas être bons, si elle
	est utilisée avec une carte son ne supportant pas cette caractéristique.
        Ce périphérique n'est pas disponible si vous avez spécifié l'option de

	compilation :ref:`--disable-fm-support <build-fm-support>`.

    La valeur initiale est ``Beeper`` sur les plateformes supportant cela,
    et ``PCM`` sur les autres.

.. _preference-pcm-instrument:

Volume PCM
    Si vous utilisez l'interface audio numérique de votre carte son pour
    jouer les sons d'avertissement, ce paramètre spécifie le volume (sous la forme d'un
    pourcentage du maximum) auquel ils sont joués.

Volume MIDI
    Si vous utilisez la Musical Instrument Digital Interface (MIDI, interface
    numérique d'instruments de musique) de votre carte son
    pour jouer les sons d'avertissement, ce paramètre spécifie le volume (sous la
    forme d'un pourcentage du maximum) auquel ils sont joués.
    Le paramètre initial est 70.

.. _preference-midi-instrument:

Instrument MIDI
    Si l'interface MIDI de la carte son est
    utilisée pour jouer les sons d'avertissement, ce paramètre spécifie l'instrument
    qui doit être utilisé (voir :ref:`Table d`instruments MIDI <operand-midi-device>`).
    La valeur initiale est ``Grand piano acoustique``.

Volume FM
    Si vous utilisez le synthétiseur FM de votre carte son
    pour jouer les sons d'avertissement, ce paramètre spécifie le volume (sous la
    forme d'un pourcentage du maximum) auquel ils sont joués.

.. _preference-alert-dots:

Points d'avertissement
    Chaque fois qu'un événement avec un type de point associé se produit (voir
    :ref:`Sons d`avertissement <tunes>`):


Non

        N'affiche pas les points.

Oui

        Affiche brièvement les points.

    Si les sons d'avertissement doivent être joués (voir la commande
    :ref:`TUNES <command-TUNES>` et la préférence
    :ref:`Sons d`avertissement <preference-alert-tunes>`), si un son a été
    associé à l'événement, et si le périphérique de son sélectionné peut être
    ouvert, alors sans s'occuper de la valeur de cette préférence, les points ne sont pas affichés.

.. _preference-alert-messages:

Messages d'avertissement
    Chaque fois qu'un événement significatif avec un message associé se produit
    (voir :ref:`Sons d`avertissement <tunes>`):


Non

        N'affiche pas le message.

Oui

        Affiche le message.

    Si des sons d'avertissement doivent être joués (voir la commande
    :ref:`TUNES <command-TUNES>`, et la préférence
    :ref:`Sons d`avertissement <preference-alert-tunes>`), si un son a été
    associé à l'événement, et si le périphérique de son sélectionné peut être
    ouvert, ou si des points d'avertissement doivent être affichés (voir la préférence
    :ref:`Points d`avertissement <preference-alert-dots>`) et si des points ont
    été associés à l'événement, sans se soucier de la valeur de cette préférence,
    le message n'est pas affiché.

.. _preference-sayline-mode:

Mode Dire la ligne
    Lors de l'utilisation de la commande
    :ref:`SAY_LINE <command-SAY_LINE>`:


Immédiat

        Suspend la parole.

En file

        Ne suspend pas la parole.

    La valeur initiale est ``Immédiat``.

.. _preference-autospeak:

Parole automatique
Inactif

        Ne parle que quand vous le demandez explicitement.

Actif

        Dit automatiquement:

          - la nouvelle ligne lorsque vous déplacez la plage braille

	  verticalement.
          - les caractères qui sont entrés ou effacés.
          - le caractère sur lequel vous déplacez le curseur.


    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`AUTOSPEAK <command-AUTOSPEAK>`.
    La valeur initiale est ``inactif``.

.. _preference-speech-rate:

Vitesse de la synthèse
    Ajuste le débit de parole (``0`` est le plus lent, ``20`` est le plus
    rapide). Cette préférence n'est disponible que si vous utilisez un pilote
    qui la supporte.
    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`SAY_SLOWER/SAY_FASTER <command-SAY_SLOWER-SAY_FASTER>`.
    La valeur initiale est ``10``.

.. _preference-speech-volume:

Volume de la synthèse
    Ajuste le volume de la synthèse (``0`` est le plus bas, ``20`` le plus
    fort). Cette préférence n'est disponible que si vous utilisez un pilote
    qui la supporte.
    Vous pouvez aussi changer ce paramètre avec la commande
    :ref:`SAY_SOFTER/SAY_LOUDER <command-SAY_SOFTER-SAY_LOUDER>`.
    La valeur initiale est ``10``.

.. _preference-speech-pitch:

Ton de la voix
    Ajuste le volume de la synthèse ((``0`` est le plus bas, ``20`` est
    le plus élevé). Cette préférence n'est disponible que si vous utilisez
    un pilote qui la supporte. Le réglage initial est ``10``.

.. _preference-speech-punctuation:

Ponctuation pour la synthèse
    Ajuste la quantité de ponctuation parlée. Elle peut être initialisée à:

      - Aucune
      - Quelques
      - Toutes


    Cette préférence n'est disponible que si vous utilisez
    un pilote qui la supporte. Le réglage initial est ``Quelques``.

.. _preference-status-style:

Apparence de l'état
    Ce paramètre spécifie la façon dont les cellules d'état doivent être
    utilisées. Normalement, vous ne devriez pas avoir besoin de jouer avec ça.
    Cela permet aux développeurs de BRLTTY de tester les configurations des
    cellules d'état pour les afficheurs braille qu'ils n'ont pas avec eux.


Aucune

        N'utilise pas les cellules de statut.

	Cette valeur est toujours sûre, mais elle est aussi totalement inutile.

Alva

        Les cellules d'état contiennent:


1

            La place du curseur (voir ci-dessous).

2

            La place du coin en haut à gauche de la plage braille (voir

	    ci-dessous).

3

            Une lettre indiquant l'état de BRLTTY.
            Dans l'ordre de rangement:


a

	        Les attributs de l'écran sont affichés (voir la commande

		:ref:`DISPMD <command-DISPMD>`).

f

	        L'image de l'écran est gelée (voir la commande

		:ref:`FREEZE <command-FREEZE>`).

f

	        Le curseur est poursuivi (voir la commande

		:ref:`CSRTRK <command-CSRTRK>`).

*vierge*

                Rien de spécial.


	Les emplacements du curseur et de la plage braille sont présentés de
	façon intéressante. Les points 1 à 6 représentent le numéro de la ligne
	avec une lettre de ``a`` (pour 1) à ``y`` (pour 25).
	Les points 7 et 8 (les deux points supplémentaires tout en bas)
	représentent le numéro de la plage braille horrizontale comme suit:


Aucun points
La première fenêtre (la plus à gauche).

Point 7
La seconde fenêtre.

Point 8
La troisième fenêtre.

Points 7 et 8
La quatrième fenêtre.

	Dans les deux cas, les indicateurs incluent:
	la ligne 26 est représentée par la lettre ``a``, et la cinquième
	plage braille horizontale est représentée avec aucun point tout en bas.

Tieman

        Les cellules d'état contiennent:


1-2

	    Les colonnes (en partant de un) du curseur (montrées dans la
	    moitié supérieure des cellules et du coin haut à gauche de la
	    plage braille (affiché dans la partie en bas des cellules).

3-4

	    Les lignes (en partant de un) du curseur affichées dans la
	    moitié supérieure des cellules et du coin haut à gauche de la
	    plage braille (affiché dans la partie basse des cellules).

5

	    Chaque point indique si une caractéristique est active comme suit:


point 1

                L'image de l'écran est gelée (voir la commande

		:ref:`FREEZE <command-FREEZE>`).

Point 2

                Les attributs de l'écran sont affichés (voir la commande
                :ref:`DISPMD <command-DISPMD>`).

Point 3

                Les sons d'avertissement sont joués (voir la commande
                :ref:`TUNES <command-TUNES>`).

Point 4

                Le curseur est affiché (voir la commande
                :ref:`CSRVIS <command-CSRVIS>`).

Point 5

                Le curseur est un pavé (voir la commande
                :ref:`CSRSIZE <command-CSRSIZE>`).

Point 6

                Le curseur est masqué (voir la commande
                :ref:`CSRBLINK <command-CSRBLINK>`).

Point 7

                Le curseur est poursuivi (voir la commande
                :ref:`CSRTRK <command-CSRTRK>`).

Point 8

                La plage braille défilera (voir la commande
                :ref:`SLIDEWIN <command-SLIDEWIN>`).


PowerBraille 80

        Les cellules d'état contiennent:


1

	    La ligne (en partant de 1) correspondant au haut de la plage
	    braille. La partie des dizaines est montrée dans la moitié supérieure
	    de la cellule, et celle des unités est montrée dans la moitié
	    inférieure de la cellule.


Générique

        Ce paramètre transmet beaucoup d'informations au pilote braille, et le

	pilote lui-même décide comment les présenter.

MDV

        Les cellules de statut contiennent:


1-2

	    L'emplacement du coin en haut à gauche de la plage braille.
	    La ligne (en partant de 1) est affichée dans la moitié supérieure
	    des cellules, et la colonne (en partant de 1) est montrée dans la

    moitié inférieure des cellules.


Voyager

        Les cellules d'état contiennent:


1

	    La ligne (en partant de 0) correspondant au haut de la plage
	    braille (voir ci-dessous).

2

            La ligne (en partant de 1) sur laquelle se trouve le curseur (voir

	    ci-dessous).

3

	    Si l'écran est gelé (voir la commande
	    :ref:`FREEZE <command-FREEZE>`), la lettre ``F``.
            Sinon, la colonne (en partant de 1) dans laquelle se trouve le

	    curseur (voir ci-dessous).

	Les numéros de ligne et de colonne sont montrés comme deux cases dans
	une seule cellule. Les dizaine sont affichées dans la moitié haute de la
	cellule, et les unités sont affichées dans la moitié inférieure de la
	cellule.

    La valeur initiale dépend du pilote de l'afficheur braille.

.. _preference-text-table:

Table de texte
    Sélectionne la table de texte. Voir la section
    :ref:`Table de texte <table-text>` pour des détails.
    Voir l'option :ref:`-t <options-text-table>` en ligne de
    commande pour la valeur initiale.
    Cette préférence n'est pas sauvegardée.

.. _preference-attributes-table:

Table d'attributs
    Sélectionne la table d'attributs. Voir la section
    :ref:`Tables d`attributs <table-attributes>` pour
    des détails. Voir l'option :ref:`-a <options-attributes-table>`
    en ligne de commande pour la valeur initiale.
    Cette préférence n'est pas sauvegardée.

.. _preference-contraction-table:

Table de braille abrégé
    Sélectionne la table de braille abrégé. Voir la section
    :ref:`Tables d'abrégé <table-contraction>` pour des détails. Voir l'option
    :ref:`-c <options-contraction-table>` en ligne de commande
    pour la valeur initiale.
    Cette préférence n'est pas sauvegardée.

.. _preference-key-table:

Tabe de touches
    Sélection de la table de touches.
    Voir la section :ref:`Tables de touches <table-key>` pour plus de détails.
    Voir l'option :ref:`-k <options-key-table>` en ligne de commande
    pour le réglage initial.
    Cette préférence n'est pas sauvegardée.


Remarques:

  -

.. _time-settings:

    Tous les paramètres de temps sont en centièmes de seconde. Ce sont des
    multiples de 4 compris entre 1 et 100.


.. _status:

L'affichage des états
---------------------

L'affichage des états est un résumé de l'état courant de BRLTTY qui s'adapte
totalement à l'intérieur de la plage braille. Certains afficheurs braille ont
un type de cellules d'état qui sont utilisées pour afficher en permanence
certaines de ces informations de la même façon (voir la documentation du
pilote de votre afficheur).
Les données présentées par cet affichage ne sont pas statiques et peuvent changer
à n'importe quel moment, en réaction aux mises à jour de l'écran et/ou aux
commandes BRLTTY.

Utilisez la commande :ref:`INFO <command-INFO>` pour aller à
l'affichage des états, et utilisez-la de nouveau pour revenir à l'écran.
La présentation des informations qu'il contient dépend de la taille de
la plage braille.

Afficheurs de 21 cellules ou plus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

De courtes symboliques ont été utilisées, bien qu'elles s'apparentent à un code
chiffré, de façon à afficher la présentation en colonne précise.

.. parsed-literal::

   *wx*:*wy* *cx*:*cy* *vt* *tcmfdu*


*wx*\ ``:`` *wy*

    La colonne et la ligne (en partant de 1) sur l'écran correspondant au
    coin en haut à gauche de la plage braille.

*cx*\ ``:`` *cy*

    La colonne et la ligne (en partant de 1) sur l'écran correspondant à la
    position du curseur.

*vt*

    Le numéro (en partant de 1) de la console virtuelle courante.

*t*

    L'état de la fonction de poursuite du curseur (voir la commande
    :ref:`CSRTRK <command-CSRTRK>` command).


vide
Le suivi du curseur est inactif.

``t``
Le suivi du curseur est actif.


*c*

    L'état des caractéristiques de visibilité du curseur (voir les commandes
    :ref:`CSRVIS <command-CSRVIS>` et
    :ref:`CSRBLINK <command-CSRBLINK>`).


vide
  Le curseur n'est pas visible et ne clignotera pas quand il
  sera visible.

``b``
  Le curseur n'est pas visible, et clignotera lorsqu'il
  sera visible.

``v``
  Le curseur est visible et non clignotant.

``B``
  Le curseur est visible et clignotant.


*m*

    Le mode d'affichage actuel (voir la commande
    :ref:`DISPMD <command-DISPMD>`).


``t``
Le contenu de l'écran (texte) est affiché.

``a``
La surbrillance à l'écran (les attributs) est affichée.


*f*

    L'état de la fonction de gel de l'écran (voir la commande
    :ref:`FREEZE <command-FREEZE>`).


vide
L'écran n'est pas gelé.

``f``
L'écran est gelé.


*d*

    Le nombre de points braille utilisés pour afficher chaque caractère (voir
    la commande :ref:`SIXDOTS <command-SIXDOTS>`).


``8``
Les huit points sont utilisés.

``6``
Seuls 6 points sont utilisés.


*u*

    L'état des fonctions d'affichage des lettres majuscules (voir la
    commande :ref:`CAPBLINK <command-CAPBLINK>`).


vide
Les lettres en majuscule ne clignotent pas.

``B``
Les lettres en majuscule clignotent.


Afficheurs à 20 cellules ou moins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

De courtes symboliques ont été utilisées, bien qu'elles s'apparentent à un code
chiffré, de façon à montrer la présentation en colonne précise.

.. parsed-literal::

   *xx**yy**s* *vt* *tcmfdu*


*xx*

    Les colonnes (en partant de 1) sur l'écran correspondant à la position du
    curseur (affiché dans la moitié supérieure des cellules) et au coin en
    haut à gauche de l'afficheur braille (affiché dans la moitié inférieure des
    cellules).

*yy*

    Les lignes (en partant de 1) sur l'écran correspondant à la position du
    curseur (affichée dans la moitié supérieure des cellules) et au coin en    haut à gauche de l'afficheur braille (montré dans la moitié inférieure des
    cellules).

*s*

    Les valeurs de certaines fonctions de BRLTTY.
    Une fonctionalité est active si le point lui correspondant est élevé.


Point 1

        L'image de l'écran gelée (voir la commande

	:ref:`FREEZE <command-FREEZE>`).

Point 2

        Affichage des attributs (voir la commande

	:ref:`DISPMD <command-DISPMD>`).

Point 3

        Les sons d'avertissement (voir la commande

	:ref:`TUNES <command-TUNES>`).

Point 4

        Curseur visible (voir la commande

	:ref:`CSRVIS <command-CSRVIS>`).

Point 5

        Curseur en pavé (voir la commande

	:ref:`CSRSIZE <command-CSRSIZE>`).

Point 6

        Clignotement du curseur (voir la commande

	:ref:`CSRBLINK <command-CSRBLINK>`).

Point 7

        Poursuite du curseur (voir la commande

	:ref:`CSRTRK <command-CSRTRK>`).

Point 8

        Glissement de la plage (voir la commande

	:ref:`SLIDEWIN <command-SLIDEWIN>`).


*vt*

    Le numéro (en partant de 1) de la console virtuelle actuelle.

*t*

    L'état de la fonction de poursuite du curseur (voir la commande
    :ref:`CSRTRK <command-CSRTRK>` command).


vide
Le suivi du curseur est inactif.

``t``
Le suivi du curseur est actif.


*c*

    L'état des fonctions de visibilité du curseur (voir les commandes
    :ref:`CSRVIS <command-CSRVIS>` et
    :ref:`CSRBLINK <command-CSRBLINK>`).


vide
  Le curseur n'est pas visible et ne cligontera pas quand il
  sera visible.

``b``
  Le curseur n'est pas visible, et clignotera lorsqu'il
  sera visible.

``v``
  Le curseur est visible et non clignotant.

``B``
  Le curseur est visible et clignotant.


*m*

    Le mode d'affichage actuel (voir la commande
    :ref:`DISPMD <command-DISPMD>`).


``t``
Le contenu de l'écran (texte) est affiché.

``a``
La surbrillance à l'écran (les attributs) est affichée.


*f*

    L'état de la fonction de gel de l'écran (voir la commande
    :ref:`FREEZE <command-FREEZE>`).


vide
L'écran n'est pas gelé.

``f``
L'écran est gelé.


*d*

    Le nombre de points braille utilisés pour afficher chaque caractère (voir
    la commande :ref:`SIXDOTS <command-SIXDOTS>`).


``8``
Les huit points sont utilisés.

``6``
Seuls 6 points sont utilisés.


*u*

    L'état des fonctions d'affichage des lettres majuscules (voir la
    commande :ref:`CAPBLINK <command-CAPBLINK>`).


vide
Les lettres en majuscule ne clignotent pas.

``B``
Les lettres en majuscule clignotent.


.. _learn:

Mode Apprentissage des commandes
--------------------------------

Le Mode Apprentissage des commandes est une façon interactive d'apprendre
ce que les touches de l'afficheur braille font. Vous pouvez y accéder soit
par la commande :ref:`LEARN <command-LEARN>` ou via l'utilitaire
:ref:`brltest <utility-brltest>`.
Cette caractéristique n'est pas disponible si vous avez spécifié l'option de
compilation :ref:`--disable-learn-mode <build-learn-mode>`.

Lorsque vous êtes entré dans ce mode, le message ``Mode apprentissage des commandes``
est écrit sur l'afficheur braille.
Alors, dès que vous pressez une touche (ou une combinaison de touches) de
l'afficheur, un court message décrivant sa fonction dans BRLTTY est écrit.
Vous quitterez immédiatement ce mode si vous pressez la touche (ou la
combinaison de touches) pour la commande :ref:`LEARN <command-LEARN>`.
Vous sortez automatiquement, et le message ``done`` s'inscrit, si un
de temps de 10 secondes s'écoule sans qu'une touche de l'afficheur ne soit
pressée.
Remarquez que certains afficheurs ne se signalent pas au pilote et/ou certains
afficheurs ne se signalent pas à BRLTTY jusqu'à ce que toutes les touches soient
faites.

Si un message est plus long que la largeur de l'afficheur braille, il est
affiché en segments. La longueur d'un segment est de un soustrait à
la largeur de l'afficheur braillle, avec le caractère le plus à droite sur
l'afficheur qui affiche le signe moins.
Chaque segment reste sur l'afficheur, soit pendant quelques secondes (voir
l'option :ref:`-M <options-message-timeout>` en ligne de commande)
soit jusqu'à ce qu'une touche de l'afficheur soit pressée.
