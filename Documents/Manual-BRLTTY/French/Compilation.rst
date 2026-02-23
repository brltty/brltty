La procédure de compilation
===========================

.. _tar:

On peut télécharger BRLTTY depuis son site Web (voir la section :ref:`Contacts <contact>` pour son adresse).  Toutes les versions
sont fournies en :ref:`archives tar <tar>` compressés. Les
versions récentes sont aussi fournies en fichiers :ref:`RPM <rpm>` (RedHat paquet Manager).

Ces information éparses ont probablement piqué votre
curiosité et vous êtes impatient de démarrer.
Cependant, nous vous recommandons de vous familiariser d'abord
avec les fichiers qui seront finalement installés.

.. _hierachy:

.. _hierarchy:

Hiérarchie des fichiers installés
---------------------------------

La procédure de compilation devrait aboutir à l'installation des
fichiers suivants:


/bin/


brltty

        Le programme BRLTTY.

:ref:`brltty-install <utility-brltty-install>`

        Un outil pour la copie de la :ref:`hiérarchie des fichiers installés <hierarchy>` de BRLTTY d'un emplacement à
        un autre...

:ref:`brltty-config <utility-brltty-config>`

        Un outil qui assigne un certain nombre de variables
        d'environnement à des valeurs reflétant l'installation
        courante de BRLTTY.


/lib/brltty/rw/

    Fichiers créés lors de l'exécution, comme ceux nécessaires mais absents
    du système.

/etc/


brltty.conf

        Paramètres système par défaut pour BRLTTY.

brlapi.key

        Clés d'accès pour BrlAPI.


/etc/brltty/

    Il se peut que votre installation de BRLTTY n'ait pas tous les
    types de fichiers suivants. Ils ne sont créés qu'en fonction des
    besoins déduits des options de compilation que vous sélectionnez

.. _build:
.. _build-features:

    (voir :ref:`Options de compilation <build>`).


\*.conf

        Base de configuration spécifique au pilote. Leur nom ressemble
        plus ou moins à ``brltty-`` *pilote*\ ``.conf``, où
        *pilote* correspond aux deux lettres du :ref:`code d`identification du pilote <drivers>`.

\*.atb

.. _build-attributes-table:

        Tables d'attributs
        (voir la section :ref:`Tables d'attributs <table-attributes>`

.. list-table::
   :header-rows: 1

   * - Plateforme
     - Interface
     - Description

   * - Linux
     - oss
     - Open Sound System

   * -
     - alsa
     - Advanced Linux Sound Architecture


.. _build-midi-support:

``--disable-midi-support``
    Réduit la taille du programme en excluant le support pour
    l'interface numérique d'instruments de musique sur la carte son.

``--enable-midi-support=`` *interface*

    Si une plateforme propose plus d'une interface numérique d'instrument de
    musique, vous pouvez spécifier celle qui sera utilisée.

.. list-table::
   :header-rows: 1

   * - Platte-forme
     - Interface
     - Description

   * - Linux
     - oss
     - Open Sound System

   * -
     - alsa
     - Advanced Linux Sound Architecture


.. _build-fm-support:

``--disable-fm-support``
    Réduit la taille du programme en excluant le support pour le
    synthétiseur FM sur une carte son AdLib, OPL3, Sound Blaster, ou
    équivalent.

.. _build-pm-configfile:

``--disable-pm-configfile``
    Inclut une interface avec l'application ``gpm`` de telle sorte que,
        sur les systèmes qui le supportent, BRLTTY interagisse avec le
        pilote du pointeur (souris) (voir la section :ref:`Support du pointeur (souris) via GPM <gpm>`).

.. _build-api:

``--disable-gpm``
    Réduit la taille du programme en excluant l'interface avec l'application gpm
    qui permet à BRLTTY d'interagir avec le périphérique du pointeur (la souris)
    (voir Support du pointeur (souris)  par GPM).

``--disable-api``
    Réduit la taille du programme en excluant l'interface de programmation de l'application.

.. _build-api-parameters:

``--with-api-parameters=`` *name*\ ``=`` *valeur*\ ``,``...
    Spécifie les paramètres par défaut pour l'interface de programmation
    de l'application. Si le même paramètre est spécifié plus d'une
    fois, sa valeur la plus à droite est utilisée. Pour une
    description des paramètres acceptés par l'interface, voir le
    manuel de référence de **BrlAPI**. Voir la ligne
    :ref:`api-parameters <configure-api-parameters>` du
    fichier de configuration et l'option :ref:`-A <options-api-parameters>`
    en ligne de commande pour la sélection à l'exécution.

``--disable-caml-bindings=`` *nom*\ ``=`` *valeur*\ ``,``...
    Ne compile pas les bindings Caml (interfaces de programmation) pour l'interface de programmation de l'application.

``--disable-java-bindings=`` *nom*\ ``=`` *valeur*\ ``,``...
    Ne compile pas les bindings Java (interfaces de programmation)  pour l'interface de programmation de l'application.

``--disable-lisp-bindings=`` *nom*\ ``=`` *valeur*\ ``,``...
    Ne compile pas les bindings Lisp (interfaces de programmation) pour l'interface de programmation de l'application.

``--disable-python-bindings=`` *nom*\ ``=`` *valeur*\ ``,``...
    Ne compile pas les bindings Python (interfaces de programmation) pour l'interface de programmation de l'application.

``--disable-tcl-bindings=`` *nom*\ ``=`` *valeur*\ ``,``...
    Ne compile pas les bindings Tcl (interfaces de programmation) pour l'interface de programmation de l'application.

``--with-tcl-config=`` *chemin*
    Spécifie l'endroit où se trouve le script de configuration Tcl
    (tclConfig.sh). Vous pouvez fournir le chemin, soit vers le script lui-même,
    soit vers le répertoire où il se trouve. Un des mots suivants peut être
    aussi utilisé comme opérateur pour l'option:

(``-``),

      no

        Utilise d'autres moyens pour savoir si Tcl est disponible et, s'il
        l'est, où il a été installé. Cela revient à spécifier
        ``--without-tcl-config.``.

yes

        Cherche le script dans quelques répertoires couramment utilisés.
        Cela revient à spécifier
        ``--with-tcl-config``.


.. _build-miscellaneous:

Options diverses
~~~~~~~~~~~~~~~~


.. _build-init-path:

``--with-init-path=`` *path*
    Spécifie le chemin du programme réel de démarrage pour le
    système. Vous devriez fournir le chemin absolu. Si vous ne
    spécifiez pas cette option:

      #.
        Vous devriez déplacer le programme ``init`` dans un nouvel
        emplacement.

      #.
        Vous devriez déplacer ``brltty`` dans l'emplacement original
        du programme.

      #.
        Finalement, quand le système exécute ``init`` au démarrage,
        ``brltty`` est exécuté. Il se met automatiquement en
        arrière-plan
        et exécute le ``init`` réel au premier plan. C'est
        l'une des manières quelque peu tordue d'avoir droit au braille dès le
        début. C'est surtout utile pour certains disques
        d'installation/sauvegarde.

    Si vous ne spécifiez pas cette option, cette possibilité n'est pas
    activée. Elle vise tout particulièrement la compilation d'une image pour
    un installeur en braille.

``--with-stderr-path==`` *chemin*
    Spécifie le chemin du fichier ou du périphérique où la sortie standard
    des erreurs sera écrite. Vous devriez fournir un chemin absolu. Si vous
    ne spécifiez pas cette option, cette possibilité n'est pas activée.
    Cette option vise tout particulièrement la compilation d'une image pour

un installeur en braille.


.. _make:

Préparer les cibles de fichier
------------------------------

Une fois que BRLTTY a été configuré, les étapes suivantes consistent
en la compilation et l'installation de ce dernier. Elles sont
effectuées en entrant la commande make du système dans le fichier make
principal de BRLTTY (``Makefile`` dans le répertoire principal). Le
fichier make de BRLTTY supporte la plupart des cibles de maintenance
d'application courants. Ils incluent:


make

    Un raccourci pour tout préparer.

.. _make-all:

make all
    Compile et fait l'édition de liens pour l'exécutable BRLTTY, ses pilotes et leurs pages
    de manuel, ses programmes de texte, et quelques autres petits
    outils.

.. _make-install:

make install
    Complète la compilation et la phase d'édition de liens (voir :ref:`make all <make-all>`), et
    installe alors l'exécutable BRLTTY, ses fichiers de données
    (data), pilotes et pages d'aide, aux emplacements corrects et avec
    les bonnes permissions.

.. _make-uninstall:

make uninstall
    Enlève du système l'exécutable BRLTTY, ses fichiers de données,
    pilotes et pages de manuel.

.. _make-clean:

make clean
    Garantit que la compilation à venir et l'édition de liens (voir see :ref:`make all <make-all>`)
    se feront à vide en enlevant les résultats de la
    compilation précédente, en liant et en testant depuis la structure du
    répertoire source. Cela comprend la suppression des fichiers
    objets, des exécutables, des objets dynamiques, des listes de
    pilote, des pages de manuel, des fichiers d'en-tête temporaires,
    et des fichiers liés.

.. _make-distclean:

make distclean
    Au-delà de la suppression des résultats de la compilation précédente et de
    l'edition de liens (voir :ref:`make clean <make-clean>`):

      -

        Supprime les résultats de la configuration de BRLTTY (voir
        :ref:`Options de compilation <build>`).  Cela inclut la suppression de
        ``config.mk``, ``config.h``,
        ``config.cache``, ``config.status``, et ``config.log``.

      -
        Supprime les autres fichiers de la structure du répertoire
        source, qui prennent beaucoup de place mais qui ne leur
        appartient pas. Cela inclut la suppression de fichiers éditeur de sauvegarde (backup), résultats de test, les reliquats du patch, et copies de fichiers source originaux.


Tester BRLTTY
-------------

Après la compilation, l'édition de liens, et l'installation de BRLTTY,
c'est peut-être une bonne idée de faire un petit test avant de
l'activer en permanence. Pour cela, appelez-le avec:


.. parsed-literal::

   brltty -b*pilote* -d*périphérique*

Pour *pilote*, spécifiez les deux lettres du :ref:`code d`identification de pilote <drivers>` correspondant à votre
afficheur braille. Pour *périphérique*, spécifiez le chemin complet
du périphérique auquel votre afficheur braille est connecté.

Si vous ne voulez pas identifier explicitement le pilote et le
périphérique à chaque fois que vous démarrez BRLTTY, vous pouvez
procéder de deux façons. Vous pouvez établir des paramètres système
par défaut via les lignes :ref:`braille-driver <configure-braille-driver>` et :ref:`braille-device <configure-braille-device>` du
fichier de configuration, et/ou compiler tout ce dont vous avez besoin

.. _build-braille-driver:

dans BRLTTY via les options de compilation :ref:`--with-braille-driver <build-braille-driver>`

.. _build-braille-device:

et :ref:`--with-braille-device <build-braille-device>`.

Si tout va bien, le message d'identification de BRLTTY devrait
apparaître sur l'afficheur braille pendant quelques secondes (voir
l'option :ref:`-M <options-message-timeout>` de la ligne de commande). Après qu'il ait disparu, (ce
que vous pouvez accélérer en appuyant sur n'importe quelle touche de
l'afficheur), la zone de l'écran où le curseur se situe devrait
apparaître. Cela signifie que vous devriez vous attendre à voir
s'afficher les commande du shell. Alors, comme vous avez entré votre
commande précédente, chaque caractère devrait apparaître sur
l'afficheur dès qu'il est tapé sur le clavier.

Si les choses se passent ainsi, arrêtez l'exécution de BRLTTY, et
réjouissez-vous. Sinon, il peut être nécessaire de tester chaque pilote
séparément de façon à isoler la source du problème. Vous pouvez tester
le pilote de l'écran par :ref:`scrtest <utility-scrtest>`, et celui de l'afficheur braille
par :ref:`brltest <utility-brltest>`.

Si vous rencontrez un problème qui nécessite de farfouiller,
vous voudrez peut-être exécuter les options suivantes de la ligne de commande de
``brltty``:

  -
    ... :ref:`-ldebug <options-log-level>`
    pour mettre dans un journal beaucoup de messages de diagnostique.

  -
    :ref:`-n <options-no-daemon>`
    pour mettre BRLTTY au premier plan.

  -
    :ref:`-e <options-standard-error>`
    pour diriger les messages de diagnostique vers l'erreur standard et
    non dans le journal système.


Démarrer BRLTTY
---------------


Quand BRLTTY est correctement installé, on l'appelle par la simple
commande ``brltty``. Vous pouvez créer un fichier de configuration
(voir la section :ref:`le fichier de configuration <configure>` pour plus de détails)
afin de d'établir des paramètres système par défaut pour des choses
telles que l'emplacement du fichier de préférences, le pilote
d'afficheur braille à utiliser, le périphérique auquel l'afficheur
braille est connecté, et la table de texte à utiliser.
Beaucoup d'options (voir la section :ref:`Options en ligne de commande <options>`
pour plus de détails) permettent de spécifier lors de l'exécution des
choses telles que le fichier de configuration, les paramètres par
défaut fixés dans le fichier de configuration, et quelques
caractéristiques qui ont des paramètres par défaut raisonnables mais
avec lesquelles seuls ceux qui pensent savoir ce qu'ils font peuvent
souhaiter modifier. L'option :ref:`-h <options-help>` affiche un résumé de toutes les
options. L'option :ref:`-V <options-version>` affiche la version courante du programme,
de l'API et des pilotes sélectionnés. L'option ``-v`` affiche les valeurs
des options après que toutes les sources aient été examinées.

C'est probablement mieux d'avoir BRLTTY démarré automatiquement par le
système, dès la séquence d'amorçage, de façon à ce
que l'afficheur braille soit déjà prêt à fonctionner quand l'invite du
logging apparaît. La plupart (probablement toutes) des distributions
fournissent un script dans lequel les applications fournies par
l'utilisateur peuvent être démarrées en sécurité, presqu'à la fin de
la séquence de boot. Le nom de ce script dépend de votre
distribution. Voici celles que nous connaissons:


Red Hat

    ``/etc/rc.d/rc.local``


C'est une bonne idée que de démarrer BRLTTY depuis ce script (surtout
pour les nouveaux utilisateurs). Ajustez simplement des lignes du
type:

::

   if [ -x /bin/brltty -a -f /etc/brltty.conf ]
   then
      /bin/brltty

   fi

Normalement, cela peut s'abréger en une forme un peu moins
lisible du type:

::

   [ -x /bin/brltty -a -f /etc/brltty.conf ] && /bin/brltty

N'ajoutez pas ces lignes avant la première ligne (qui ressemble
généralement à ``#!/bin/sh``).

Si l'afficheur braille doit être utilisé par un administrateur système, il
devrait probablement être démarré le plus tôt possible, pendant la
séquence de boot (comme par exemple avant que les systèmes de fichier
ne soient vérifiés) afin que l'afficheur soit utilisable si quelque
chose ne va pas dans ces tests et que le système bascule en mode
mono-utilisateur. De nouveau, là où il est l'idéal de faire cela
dépend de la distribution. Voici les emplacements que nous connaissons:


Debian

    ``/etc/init.d/boot`` (pour les vieilles versions)

    ``/etc/init.d/`` (pour les versions récentes)

    Un paquet ``brltty`` est fourni
    (voir `http://paquets.debian.org/brltty <http://paquets.debian.org/brltty>`_)
    en tant que version ``3.0`` (``Woody``).
    Comme ce paquet prend soin du démarrage de BRLTTY, il n'y a pas
    besoin pour l'utilisateur de fournir quoique ce soit, s'il est
    installé. Si vous avez besoin que le démon se lance avec des options en ligne
    de commande, vous pouvez modifier le contenu entre guillemets du fichier
    ``/etc/default/brltty``. Si vous avez besoin que le démon se lance avec
    des options en ligne de commande, vous pouvez modifier le contenu entre
    guillemets de la ligne ``ARGUMENTS`` du fichier ``/etc/default/brltty``.

RedHat

    ``/etc/rc.d/rc.sysinit``

Sachez que les versions récentes, afin de supporter une procédure
    d'initialisation du système plus orientée vers les utilisateurs,
    font rappeler ce script par lui-même de telle sorte qu'il soit
    sous le contrôle de ``initlog``. Recherchez des lignes comme celles-ci:

::

       # Rerun ourselves through initlog
       if [ -z "$IN_INITLOG" ]; then
         [ -f /sbin/initlog ] && exec /sbin/initlog $INITLOG_ARGS -r /etc/rc.sysinit

       fi

    Démarrer BRLTTY avant ce rappel donne deux processus BRLTTY en
    même temps, et cela vous causera des foules de problèmes. Si votre
    version de ce script a cette caractéristique, assurez-vous de
    démarrer BRLTTY après les lignes qui le mettent en action.

Slackware

    ``/etc/rc.d/rc.S``

SuSE

    ``/sbin/init.d/boot``


Une autre solution est de démarrer BRLTTY depuis /etc/inittab. Si vous
choisissez cette solution, vous avez deux possibilités.

  -
    Si vous voulez qu'il démarre vraiment très tôt, mais que vous
    n'avez pas besoin qu'il soit redémarré automatiquement s'il
    échoue, ajoutez une ligne comme la suivante avant la première
    ligne ``:sysinit:`` qui est déjà dans le fichier.

  -
    Une autre solution est de démarrer BRLTTY depuis /etc/inittab. Si
    vous choisissez cette solution, vous avez deux possibilités. Si
    vous voulez qu'il démarre vraiment très tôt, mais que vous n'avez
    pas besoin qu'il soit redémarré automatiquement en cas d'échec,
    ajoutez une ligne comme la suivante avant la première ligne
    :sysinit: qui est déjà dans le fichier.

::

   brl:12345:respawn:/bin/brltty -n


    L'option :ref:`-n <options-no-daemon>` (--nodaemon) quand BRLTTY est exécuté avec la
    facilité respawn de l'inittab. Si vous oubliez de la
    spécifier, vous allez vous retrouver avec des centaines de
    processus BRLTTY, tous exécutés en même temps.

Vérifiez que l'identificateur (``brl`` dans ces exemples) n'est pas déjà
utilisé par une autre entrée, et, si c'est le cas, choisissez-en un
autre.

Remarquez qu'une commande telle que ``kill -TERM`` suffit pour arrêter
BRLTTY dans ses opérations. Si, par exemple, il meurt lors de
l'entrée en mode mono-utilisateur, il se pourrait bien que cela vienne
d'un problème de cette nature.

Certains systèmes rencontrent des problèmes si une application tente
d'utiliser le sous-système son du noyau avant qu'il ait été
initialisé. Si votre système en fait partie, vous aurez peut-être
besoin de désactiver le démarrage automatique du pilote de la synthèse
vocale avec l'option :ref:`-N <options-no-speech>`.

Certains systèmes, dans une étape de leur séquence de boot, testent
(probe) les ports série (normalement afin de détecter automatiquement
la souris et déduire son type). Si votre afficheur braille utilise un
port série, ce genre de détection peut suffir à le gêner. Si cela vous
arrive, essayez de redémarrer le pilote braille (voir la commande
:ref:`RESTARTBRL <command-RESTARTBRL>`). Ou mieux, désactiver le test du port série. Voici ce
que nous savons sur la manière de réaliser cela:


Red Hat
    Le test se fait par un service appelé ``kudzu``. Utilisez la commande::

        chkconfig --list kudzu

    pour voir s'il a été activé.
    Utilisez la commande::

        chkconfig kudzu off

    pour le désactiver.
    Les dernières versions permettent de laisser ``kudzu`` s'exécuter sans
    tester les ports série.
    Pour cela, éditez le fichier ``/etc/sysconfig/kudzu``, et réglez ``SAFE`` à ``yes``.


Si vous voulez démarrer BRLTTY avant que les systèmes de fichiers ne
soient montés, assurez-vous que tous leurs composants sont installés
dans le fichier racine du système. Voir les options de compilation

.. _build-execute-root:

:ref:`--with-execute-root <build-execute-root>`,
:ref:`--bindir <build-program-directory>`,

.. _build-data-directory:

:ref:`--with-data-directory <build-data-directory>`,
et :ref:`--libdir <build-library-directory>`  et .

Considérations sur la sécurité
------------------------------

L'exécution de BRLTTY nécessite les privilèges root parce que le
programme a besoin, lorsqu'il s'exécute, des droits d'accès de lecture
et d'écriture pour le port auquel l'afficheur braille est connecté,
les droits d'accès en lecture à /dev/vcsa ou équivalent (pour avoir
les dimensions de l'écran et la position du curseur, et pour revoir le
contenu et les choses mises en valeur sur l'écran courant), ainsi que
les accès en lecture et écriture à la console système (pour l'entrée
des flèches de direction pendant le déplacement du curseur) pour
l'insertion de caractères lors du Coller, pour la simulation de
touches spéciales en utilisant celles de l'afficheur braille, pour la
recherche de traduction des caractères en sortie et les tables faisant
la correspondance des polices de l'écran, et pour l'activation
du beeper interne). Vous pouvez, bien entendu, autoriser les
utilisateurs ordinaires à accéder aux périphériques nécessaires, en
changeant les permissions du fichier associé au
périphérique. Toutefois, le simple accès à la console ne suffit pas
car l'activation du beeper interne et des fonctions de
simulation du clavier requiert les privilèges root. Ainsi, si vous
voulez arrêter le déplacement du curseur, utiliser Copier/Coller les
beep et tout cela, vous pouvez exécuter BRLTTY sans les privilèges
root.

Restrictions applicables à la compilation et à l'exécution
----------------------------------------------------------


.. _restriction-tunes:

les beeps d'Alerte

    Certaines plateformes ne supportent pas tous les périphériques
    sonores. Voir :ref:`Périphérique pour le son <preference-tune-device>` pour plus de détails.

.. _restrictions-flite:

Pilote pour la synthèse FestivalLite

    Le pilote pour le moteur FestivalLite n'est compilé
    que si ce paquet a été installé.

    Ce pilote et celui pour le moteur Theta (voir

.. _build-theta:

    l'option de compilation ``--with-theta``) ne peuvent être liés
    tous les deux à l'exécutable BRLTTY (voir l'option de compilation

.. _build-speech-driver:

    :ref:`--with-speech-driver <build-theta>`) car les bibliothèques nécessaires à leur
    exécution contiennent des éléments conflictuels.

.. _restrictions-mikropuhe:

Pilote pour l'afficheur braille Libbraille

    Le pilote pour le paquet libbraille n'est compilé que si le paquet a
    été installé.


Pilote pour la synthèse Mikropuhe

    Le pilote pour le moteur Mikropuhe n'est compilé
    que si ce paquet a été installé.

    Vous ne pouvez inclure ce pilote si l'exécutable BRLTTY est lié
    statiquement (voir l'option de compilation

.. _build-standalone-programs:

    :ref:`--enable-standalone-programs <build-standalone-programs>`) car le paquet n'inclut pas
    de bibliothèque statique.

.. _restrictions-theta:

Le pilote pour la synthèse Theta

    Le pilote pour le moteur Theta n'est compilé que si
    ce paquet a été installé.

    Ce pilote et celui pour le moteur FestivalLite

.. _build-flite:

    (voir l'option de compilation :ref:`--with-flite <build-flite>`) ne peuvent être
    liés tous les deux à l'exécutable BRLTTY (voir l'option de
    compilation :ref:`--with-speech-driver <build-speech-driver>`) car les bibliothèques
    nécessaires à leur exécution contiennent des éléments conflictuels.

    Si ce pilote est compilé comme objet dynamique, vous devez
    rajouter ``$THETA_HOME/lib`` à la variable d'environnement
    ``LD_LIBRARY_PATH`` avant que BRLTTY ne soit appelé, car les objets à
    l'intérieur du paquet ne contiennent pas les chemins de recherche
    pour l'exécution et leurs dépendances.

.. _restrictions-viavoice:

ViaVoice Speech Synthesizer Driver

    Le pilote pour le moteur ViaVoice n'est compilé que
    si ce paquet a été installé.

    Vous ne pouvez inclure ce pilote si l'exécutable BRLTTY est lié
    statiquement (voir l'option de compilation
    the :ref:`--enable-standalone-programs <build-standalone-programs>`) car le paquet n'inclut pas
    de bibliothèque statique.

.. _restrictions-videobraille:

Pilote pour l'afficheur braille VideoBraille

    Le pilote pour l'afficheur braille VideoBraille est compilé sur
    tous les systèmes, mais ne fonctionne que sur Linux.


.. _rpm:

Installation à partir d'un fichier RPM
--------------------------------------

Pour installer BRLTTY à partir d'un fichier RPM (RedHat paquet
Manager), procéder comme suit:

  #.
    Téléchargez le paquet exécutable correspondant à votre
    matériel. Ce sera un fichier nommé
    ``brltty-`` *version*\ ``-`` *version*\ ``.`` *architecture*\ ``.rpm``,
    par exemple, ``brltty-3.0-1.i386.rpm``.

  #.
    Installez le paquet.

.. parsed-literal::

   rpm -Uvh brltty-*version*-*version*.*architecture*.rpm

    Vous devez faire cela en tant que **root**.

    A proprement parler, l'option ``-U`` (Update) est la seule
    nécessaire. L'option ``-v`` (verbose) affiche le nom du paquet
    lorsqu'il va être installé. L'option ``-h`` (hashes) affiche une barre
    de progression (utilisant des hachures).

Pour les gens courageux, nous fournissons le fichier RPM source
(``.src.rpm``) mais cela dépasse l'objectif de ce document.

Pour désinstaller BRLTTY, faites:

::

   rpm -e brltty


Autres outils
-------------

La compilation de BRLTTY donne aussi celle de quelques petits
outils d'aide et de diagnostic.

.. _utility-brltty-config:

brltty-config
~~~~~~~~~~~~~

Cet outil affecte un certain nombre de variables d'environnement
à des valeurs reflétant l'installation courante de BRLTTY (voir les
:ref:`options de compilation <build>`). Vous devriez l'exécuter dans un
environnement shell existant, donc ce n'est pas une véritable commande,
et seuls les scripts supportant la syntaxe ``Bourne Shell``
peuvent l'utiliser.

::

   . brltty-config


Les variables d'environnement suivantes sont affectées:


BRLTTY_VERSION

    Le numéro de version du paquet BRLTTY.

BRLTTY_EXECUTE_ROOT

    La racine de l'exécution du paquet installé. Configurée par
    l'option de compilation :ref:`--with-execute-root <build-execute-root>`.

BRLTTY_PROGRAM_DIRECTORY

.. _build-portable-root:

    Répertoire des programmes exécutables (binaires
    exécutables). Configuré par l'option de compilation :ref:`--with-program-directory <build-program-directory>`.

BRLTTY_LIBRARY_DIRECTORY

.. _build-library-directory:

    Répertoire des pilotes. Configuré par l'option de compilation :ref:`--with-library-directory <build-library-directory>`.

BRLTTY_WRITABLE_DIRECTORY

    Répertoire dans lequel il est possible d'écrire. Configuré par l'option
    de compilation :ref:`--with-writable-directory <build-program-directory>`.

BRLTTY_DATA_DIRECTORY

    Répertoire des tables et des pages de manuel. Configuré par
    l'option de compilation :ref:`--with-data-directory <build-data-directory>`.

BRLTTY_MANPAGE_DIRECTORY

    Répertoire des pages de manuel. Configuré par l'option de
    compilation :ref:`--with-manpage-directory <build-manpage-directory>`.

BRLTTY_INCLUDE_DIRECTORY

    Répertoire pour les fichiers d'en-tête C de BrlAPI. Configuré
    par l'option de compilation :ref:`--with-include-directory <build-include-directory>`.

BRLAPI_VERSION

    Le numéro de version de BrlAPI (BRLTTY's Application
    Programming Interface).

BRLAPI_RELEASE

    Le numéro de version complet de BrlAPI.

BRLAPI_AUTH

    Le nom du fichier de clés de BrlAPI.


En plus, les variables d'environnement standard ``autoconf``
suivantes sont aussi assignées:


prefix

    Sous-répertoire pour les fichiers indépendants de l'architecture.
    Configuré par l'option de compilation :ref:`--prefix <build-portable-root>`.

exec_prefix

    Sous-répertoire pour les fichiers dépendants de l'architecture.

.. _build-architecture-root:

    Configuré par l'option de compilation :ref:`--exec-prefix <build-architecture-root>`.

bindir

    Emplacement par défaut du :ref:`répertoire du programme <build-program-directory>`.

.. _build-program-directory:

    Configuré par l'option de compilation ``--bindir``.

.. _build-api-directory:

libdir

    Répertoire pour les objets dynamiques et statiques de
    BrlAPI, la localisation par défaut pour le :ref:`répertoire des
    bibliothèques <build-library-directory>`. Configuré par l'option de compilation :ref:`--libdir <build-api-directory>`.

.. _build-configuration-directory:

sysconfdir

    Répertoire des fichiers de configuration, emplacement par défaut du :ref:`répertoire de données <build-data-directory>`. Configuré par
    l'option de compilation :ref:`--sysconfdir <build-configuration-directory>`.

mandir

    Emplacement par défaut pour le :ref:`répertoire des pages de manuel <build-manpage-directory>`.

.. _build-manpage-directory:

    Configuré par l'option de compilation ``--mandir``.

includedir

    Emplacement par défaut du :ref:`répertoire des fichiers d`en-tête <build-include-directory>`.

.. _build-include-directory:

    Configuré par l'option de compilation ``--includedir``.


.. _utility-brltty-install:

brltty-install
~~~~~~~~~~~~~~

Cet outil copie la :ref:`hiérarchie des fichiers installés <hierarchy>` de
BRLTTY d'un emplacement à un autre.

.. parsed-literal::

   brltty-install *destination* [*origine*]


*destination*

    L'emplacement où la :ref:`hiérarchie des fichiers installés <hierarchy>`
    sera copiée. Cela doit être un répertoire existant.

*from*

    L'emplacement à partir duquel la :ref:`hiérarchie des fichiers installés <hierarchy>`
    sera copiée. S'il est spécifié, le répertoire doit
    exister. S'il n'est pas spécifié, l'emplacement utilisé pour la
    compilation est utilisé.

Par exemple, vous pouvez utiliser cette outil pour copier BRLTTY
à partir d'un disque racine. Si une disquette racine est montée dans
``/mnt``, et que BRLTTY est installé sur le système principal, taper
::

   brltty-install /mnt

copie BRLTTY, en entier avec ses fichiers de données et ses bibliothèques, sur
la disquette racine.

Quelques problèmes ont été rencontrés en copiant BRLTTY entre des
systèmes avec différentes versions de la bibliothèque C. Si vous avez
des difficultés, cela vaut la peine d'enquêter.

.. _utility-brltest:

brltest
~~~~~~~

Cet outil teste un pilote d'afficheur braille, et fournit presque
une façon interactive d'apprendre ce que font les touches de
l'afficheur braille. Vous devriez l'exécuter en tant que root.

.. parsed-literal::

   brltest -*option* ... [*pilote* [*nom*=*valeur* ...]]


*pilote*

    Le pilote de l'afficheur braille. Doit être les deux lettres du
    :ref:`code d'identification du pilote <drivers>`. S'il n'est pas spécifié, le
    premier pilote configuré par l'option de compilation
    :ref:`--with-braille-driver <build-braille-driver>` est utilisé.

*nom*\ ``=`` *valeur*

.. _build-braille-parameters:

    Affecte un paramètre d'afficheur braille. Pour une description des
    paramètres acceptés par un pilote spécifique, voir la
    documentation de ce pilote.

``-d`` *device* ``--device=`` *périphérique*

    Le chemin absolu pour le périphérique auquel l'afficheur braille
    est connecté. S'il n'est pas spécifié, c'est le périphérique
    configuré par l'option de compilation :ref:`--with-braille-device <build-braille-device>` qui est
    utilisé.

``-D`` *directory* ``--data-directory=`` *répertoire*

    Vous devriez fournir un chemin absolu pour le répertoire où les
    fichiers de données des pilotes sont placés. S'il n'est pas spécifié,
    c'est le répertoire configuré par l'option de compilation
    :ref:`--with-data-directory <build-data-directory>` qui est utilisé.

``-L`` *directory* ``--library-directory=`` *repertoire*

    Le chemin absolu pour le répertoire dans lequel sont situés les
    pilotes. S'il n'est pas spécifié, c'est le répertoire configuré par
    l'option de compilation :ref:`--libdir <build-library-directory>` qui est utilisé.

``-W`` *répertoire* ``--writable-directory=`` *repertoire*

    Le chemin absolu vers un répertoire où il est possible d'écrire. S'il
    n'est pas spécifié, le répertoire configuré avec l'option >e compilation --with-writable-directory
    est utilisé.

``-h`` ``--help``

    Affiche un résumé des options de la ligne de commande, puis quitte.


.. _build-learn-mode:

Cet outil utilise le ``mode apprentissage des commandes`` de
BRLTTY. Le délai d'appui sur une touche (après lequel cet outil
quitte) est de 10 secondes. Le temps d'affichage du message (utilisé
pour les segments non-finaux de longs messages) est de ``4`` secondes.

.. _utility-spktest:

spktest
~~~~~~~

.. _build-speech-support:

Cet outil teste un pilote de synthèse vocale. Il se peut qu'il
doive être exécuté en tant que root.

.. parsed-literal::

   spktest -*option* ... [*pilote* [*nom*=*value* ...]]


*pilote*


    Le pilote pour la synthèse vocale. Doit être les deux lettres
    du :ref:`code d'identification de pilote <drivers>`. S'il n'est pas spécifié,
    c'est le premier pilote spécifié par l'option de compilation
    :ref:`--with-speech-driver <build-speech-driver>` qui est utilisé.

*nom*\ ``=`` *valeur*

.. _build-speech-parameters:

    Règle le paramètre du pilote de la synthèse vocale. Pour une
    description des paramètres acceptés par un pilote spécifique,
    voir la documentation de ce pilote.

``-t`` *string* ``--text-string=`` *string*

    Le texte qui sera dit. S'il n'est pas spécifié, c'est l'entrée
    standard (stdin) qui est lue.

``-D`` *repertoire* ``--data-directory=`` *repertoire*

    Le chemin absolu pour le répertoire dans lequel se situent les
    fichiers de données du pilote. S'il n'est pas spécifié, c'est le
    répertoire configuré par l'option de compilation
    :ref:`--with-data-directory <build-data-directory>` qui est utilisé.

``-L`` *repertoire* ``--library-directory=`` *repertoire*

    Le chemin absolu du répertoire où se situe les pilotes. S'il n'est
    pas spécifié, c'est le répertoire configuré par l'option de
    compilation :ref:`--libdir <build-library-directory>` qui est utilisé.

``-h`` ``--help``

    Affiche un résumé des options de la ligne de commande, puis quitte.


.. _utility-scrtest:

scrtest
~~~~~~~

Cet outil teste le pilote d'écran. Il doit être exécuté en tant
que root.

.. parsed-literal::

   scrtest -*option* ... [*nom*=*valeur* ...]


.. _build-screen-parameters:

*nom*\ ``=`` *valeur*

    Règle le paramètre du pilote de l'écran. Pour une
    description des paramètres acceptés par un pilote spécifique
    voir la documentation de ce pilote.

``-l`` *colonne* ``--left=`` *colonne*

    Spécifie la colonne du début (à gauche) de l'écran (origine à
    zéro). Si vous ne fournissez pas cette valeur, une valeur par
    défaut, basée sur la largeur spécifiée, est sélectionnée, de telle
    sorte que la fenêtre soit centrée à l'horizontal, est utilisée.

``-c`` *coompte* ``--columns=`` *compte*

    Spécifie la largeur de la fenêtre (en colonnes). Si vous ne
    fournissez pas cette valeur, une valeur par défaut, basée sur la
    colonne de début, est sélectionnée, de telle sorte que la fenêtre
    soit centrée à l'horrizontal.

``-t`` *ligne* ``--top=`` *ligne*

    Spécifie la ligne de début (en haut) de l'écran (origine à
    zéro). Si vous ne fournissez pas cette valeur, une valeur par
    défaut, basée sur la hauteur spécifiée, est sélectionnée, de telle
    sorte que la fenêtre soit centrée à la verticale.

``-r`` *compte* ``--rows=`` *compte*

    Spécifie la hauteur de la fenêtre (en lignes). Si vous ne
    fournissez pas cette valeur, une valeur par défaut, basée sur la
    rangée de début spécifiée, est sélectionnée, de telle sorte que la
    fenêtre soit centrée à la verticale.

``-h`` ``--help``

    Affiche un résumé des options de la ligne de commande, puis
        quitte.


Remarques:

  -
    Si vous ne spécifiez ni la colonne de début, ni la largeur de la
    fenêtre, la région est centrée horizontalement et commence à la
    colonne 5.

  -
    Si vous ne spécifiez ni la rangée de début, ni la hauteur de la
    fenêtre, la région est centrée verticalement et commence à la ligne
    5.


Les éléments suivants sont écrits sur la sortie standard (stdout):

  #.
    Une ligne détaillant les dimensions de l'écran.

.. parsed-literal::

   Screen: *largeur*x*hauteur*

  #.
    Une ligne détaillant la position (à l'origine zéro) du curseur.

.. parsed-literal::

   Cursor: [*colonne*,*ligne*]

  #.
    Une ligne détaillant les dimensions de la zone d'écran
    sélectionnée, et la position (à l'origine zéro) de son coin en
    haut à gauche.

.. parsed-literal::

   Region: *largeur*x*hauteur*@[*colonne*,*ligne*]

  #.
    Le contenu de la région d'écran sélectionnée. Les caractères
    non-imprimables sont représentés par des espaces.


.. _utility-ttbtest:

ttbtest
~~~~~~~

Cet outil teste une table de texte (section :ref:`Tables de
texte <table-text>`).

.. parsed-literal::

   ttbtest -*option* ... *input-table* *output-table*


*table-en-entrée*

    Le chemin du fichier vers la table de texte en entrée du test. S'il
    est relatif, il est ancré au répertoire configuré avec l'option de
    compilation --with-data-directory.


*table-en-sortie*

    Le chemin du fichier vers la table de texte en sortie du test. S'il
    est relatif, il est ancré au répertoire de travail  courant. Si vous ne
    fournissez pas ce paramètre, aucune table en sortie ne  sera écrite.

``-i`` ``--input-format=`` *format*

    Spécifie le format  de la table d'entrée. S'il vous ne fournissez pas cette
    option, le  format de la table en entrée est déduit de l'extension de son
    fichier.

``-o`` ``--output-format=`` *format*

    Spécifie le format  de la table en sortie. S'il vous ne fournissez pas cette
    option, le  format de la table en entrée est déduit de l'extension de son
    fichier.

``-c`` *charset* ``--charset=`` *charset*

    Spécifie le nom de l'encodage 8-bit à utiliser lors de l'interprétation des
    tables. Si vous ne fournissez pas cette option, le codage de l'hôte est
    utilisé.

``-e`` ``--edit``

    Appelle l'éditeur de tables de texte. Si vous spécifiez table de sortie,
    les changements seront écrits dessus. Sinon, la table d'entrée et réécrite.

``-h`` ``--help``

    Affiche un résumé des options en ligne de commande, et quitte.


Si vous ne demandez aucune action en particulier, la table de sortie est
facultative. Si vous ne la spécifiez pas, la table d'entrée est vérifiée.
Si vous la spécifiez, la table d'entrée est convertie.
Les formats de table suivants sont supportés:


ttb
BRLTTY

sbl
SuSE Blinux

a2b
Gnopernicus

gnb
Braille Gnome


.. _utility-ctbtest:

ctbtest
~~~~~~~

.. _build-contracted-braille:

Cet outil teste une table de braille abrégé (section :ref:`Tables
de braille abrégé <table-contraction>`). Le texte lu à partir de l'entrée standard (stdin) est réécrit sur
la sortie standard (stdout) en braille abrégé.

.. parsed-literal::

   ctbtest -*fichier-en-entrée*


*fichier-en-entrée*

    La liste des fichiers à traiter. Vous pouvez spécifier n'importe quel
    nombre de fichiers. Ils sont traités de la gauche vers la droite. Le  nom
    de fichier spécial - est interprété comme l'entrée standard (stdin). Si
    vous ne spécifiez aucun fichier, l'entrée standard est traitée.

``-c`` *fichier* ``--contraction-table=`` *fichier*

    Le chemin vers le fichier de la table de braille abrégé. S'il est relatif, il
    est ancré au répertoire configuré par l'option de compilation
    :ref:`--with-data-directory <build-data-directory>`.
    L'extension .ctb est facultative. Si vous ne fournissez pas cette option,
    en-us-g2 est supposé.

.. _build-text-table:

``-t`` *file*\ \\|\ ``auto`` ``--text-table=`` *file*\ \\|\ ``auto``

    Spécifie la table de texte (voir
    la section :ref:`Tables de texte <table-text>` pour les détails).
    Si vous fournissez un chemin relatif, il est ancré à
    ``/etc/brltty`` (voir les options de compilation :ref:`--with-data-directory <build-data-directory>` et
    :ref:`--with-execute-root <build-execute-root>` pour plus de détails).
    L'extension ``.ttb`` est facultative.
    Voir la ligne :ref:`text-table <configure-text-table>`
    du fichier de configuration pour le paramétrage par défaut au moment de l'exécution. Vous pouvez modifier
    ce paramètre avec la préférence
    :ref:`Text Table <preference-text-table>`.

``-w`` *colonnes* ``--output-width==`` *colonnes*

    La longueur  maximale d'une ligne en sortie. Chaque ligne d'entrée en
    braille abrégé est développée sur  autant de lignes que nécessaire. Si
    vous ne spécifiez  pas cette option,  il n'y a  pas de limites et s'opère
    une correspondance  ligne par  ligne entre les  lignes en entrée et celles  en
    sortie.

``-h`` ``--help``

    Affiche un résumé des options en ligne de commande, et quitte.


La table de texte est utilisée:

  -
    Pour définir l'encodage de sortie de manière à ce que le
    braille abrégé soit correctement affiché. Vous devriez spécifier
    la même table devant être utilisée par BRLTTY lorsque la sortie sera lue.

  -
    Pour définir les représentations braille de ces caractères
    définies dans le braille abrégé (voir la section
    :ref:`Traduction de caractère <contraction-opcodes-translation>`).


C'est la table de traduction de texte ``text.brf.ttb`` qui est
fournie pour l'utilisation de cet outil. Il définit le format
utilisé dans les fichiers .brf. C'est aussi le format que préfèrent
utiliser les imprimantes braille et les documents braille
électroniques. Cette table permet à cet outil d'être réellement
utilisé en tant que traducteur de braille en texte.

.. _utility-tunetest:

tunetest
~~~~~~~~

Cet outil teste la facilité des sons d'avertissement, et fournit aussi
un moyen facile de créer de nouveaux sons. Il se peut que vous
soyez obligés de l'exécuter en tant que root.

.. parsed-literal::

   tunetest -*option* ... {*note* *durée*} ...


*note*

    Un numéro de note MIDI standard. Il doit être un entier de ``1`` à ``127``,
    avec 60 représentant la valeur moyenne. Chaque valeur représente un
    demi-ton chromatique standard, donc des notes les
    plus basses aux notes les plus hautes. La valeur la plus faible (``1``)
    représente le cinquième sous Middle C, et la valeur la plus haute
    (``127``) représente le sixième G au-dessus de Middle C (notation anglosaxonne).

*durée*

    La durée de la note est en millisecondes. Elle doit être un
    entier de ``1`` à ``255``..

``-d`` *périphérique* ``--device=`` *périphérique*

    Le périphérique sur lequel jouer le son.


.. _build-beeper-support:

beeper

        Le beeper interne (générateur de sons en console).

pcm

        L'interface digital audio sur la carte son.

midi

        L'interface numérique d'instrument de musique sur la carte
        son.

fm

        Le synthétiseur FM sur une carte son AdLib, OPL3, Sound
        Blaster, ou équivalente.

    Vous pouvez abréger le nom du périphérique. Voir la préférence
    :ref:`Tune Device <preference-tune-device>` pour plus de détails concernant le périphérique
    par défaut et les restrictions de la plateforme.

``-v`` *volume* ``--volume=`` *volume*

    Spécifie le volume à la sortie (intensité) sous la forme d'un
    pourcentage du maximum. Le volume de sortie par défaut est de ``50``.

``-p`` *device* ``--pcm-device=`` *device*

    Spécifie le périphérique à utiliser pour le son (voir la
    section :ref:`spécification du périphérique PCM <operand-pcm-device>`). Cette option ne
    fonctionne pas si vous avez spécifié l'option de compilation

.. _build-pcm-support:

    :ref:`--disable-pcm-support <build-pcm-support>`.

.. _configure-preferences-file:

``preferences-file`` *file*
    Spécifie l'emplacement du fichier qui doit être utilisé pour sauvegarder
    et charger les préférences de l'utilisateur. Si vous fournissez un chemin
    relatif, il est ancré sur ``/var/lib/brltty``. Le réglage par défaut
    consiste à utiliser ``brltty.prefs``. Vous pouvez outre-passer cette
    ligne avec l'option :ref:`-F <options-preferences-file>` en
    ligne de commande.

``-m`` *peripherique* ``--midi-device=`` *peripherique*

    Spécifie le périphérique à utiliser pour l'interface numérique d'instrument
    de musique (voir la section :ref:`spécification du
    périphérique MIDI <operand-midi-device>`). Cette option ne fonctionne pas si vous avez
    spécifié l'option de compilation :ref:`--disable-midi-support <build-midi-support>`.

``-i`` *instrument* ``--instrument=`` *instrument*

    L'instrument à utiliser si le périphérique sélectionné est
    midi. Pour la liste complète des instruments, voir la :ref:`Table des
    instrument MIDI <operand-midi-device>`. L'instrument par défaut est un ``piano grand
    accoustique``. Les mots comportant le nom de l'instrument doivent
    être séparés les uns des autres par un simple signe moins plutôt
    que par des espaces, et chacun des mots peut être abrégé.

``-h`` ``--help``

    Affiche un résumé des options de la ligne de commande.
