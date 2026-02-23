Introduction
============

BRLTTY donne à un utilisateur brailliste un accès aux consoles texte
d'un système Linux/Unix. Il exécute un processus en arrière-plan
(démon) qui fait fonctionner l'afficheur braille, et
peut être démarré très tôt dans la séquence de démarrage du
système. Ainsi, il permet à un utilisateur brailliste, de prendre en main
facilement et de façon indépendante des aspects de l'administration du
système, comme l'entrée en mode mono-utilisateur, la restauration de systèmes de fichiers, et
l'analyse de problèmes de démarrage. Il facilite aussi beaucoup des
opérations de bases telles que l'identification.

BRLTTY reproduit une portion rectangulaire de l'écran (appelée
dans ce document 'la fenêtre') sous forme de texte braille sur l'afficheur.
Vous pouvez utiliser des contrôles de l'affichage pour déplacer la fenêtre sur
l'écran, pour activer et désactiver des options de revue
variées, et pour exécuter des fonctions spéciales.

Résumé des possibilités
-----------------------

BRLTTY donne les possibilités suivantes:

  -
    Totale mise en oeuvre des facilités de revue d'écran habituelles.

  -
    Choix d'un curseur en forme de ``bloc``, ``souligné``, ou
    ``aucun``.
  -
    ``Soulignement`` possible pour indiquer un texte particulièrement
    en surbrillance.

  -
    Utilisation possible du ``clignotement`` (fréquences réglables
    individuellement) pour le curseur, le soulignement des caractères en
    surbrillance, et/ou les lettres en majuscule.

  -
    Gel de l'écran pour en faire une relecture plus lente.

  -
    Routine-curseur intelligente, permettant un rapide
    rapatriment du curseur dans les éditeurs de texte, les navigateurs
    web, etc., sans bouger les mains de l'afficheur braille.

  -
    Une fonction copier-coller (linéaire ou rectangulaire) qui est
    particulièrement utile pour la copie de longs noms de fichier, de
    texte entre des terminaux virtuels, la saisie de commandes
    compliquées, etc.

  -
    Gestion des tables de braille abrégé (fournie en anglais et en français).

  -
    Support pour de multiples codes braille.

  -
    Possibilité d'identifier un caractère inconnu.

  -
    Possibilité d'inspecter un caractère en surbrillance.

  -
    Une facilité dans l'aide en ligne pour les commandes de
    l'afficheur braille.

  -
    Un menu préférences.

  -
    Support de synthèses basiques.

  -
    Une conception en modules permettant d'ajouter relativement
    facilement d'autres afficheurs braille et d'autres synthèses
    vocales.

  -
    Une Interface de programmation de l'Application.


Système requis
--------------

Actuellement, BRLTTY fonctionne sur Linux, Solaris, OpenBSD, FreeBSD,
NetBSD et Windows. Les portages sur d'autres systèmes d'exploitation
dérivés de Unix ne sont pas encore prévues, nous apprécierions vraiment
tout intérêt pour de tels projets.


Linux

    Ce logiciel a été testé sur un grand nombre de systèmes Linux:

      -
        Ordinateurs de bureau, portables, et quelques PDAs.

      -
        Des processeurs 386SX20 à Pentium.

      -
        Une large échelle de capacité de mémoire.

      -
        Plusieurs distributions dont Debian, Red Hat, Slackware et SuSE.

      -
        La plupart des noyaux, dont les 1.2.13, 2.0, 2.2, et 2.4.


Solaris

    Ce logiciel a été testé sur les systèmes Solaris suivants:

      -
        L'architecture Sparc (versions 7, 8, et 9).

      -
        L'architecture Intel (version 9).


OpenBSD

    Ce logiciel a été testé sur les systèmes OpenBSD suivants:

      -
        L'architecture Intel (version 3.4).


FreeBSD

    Ce logiciel a été testé sur les systèmes FreeBSD suivants:

      -
        L'architecture Intel (version 5.1).


NetBSD

    Ce logiciel a été testé sur les systèmes NetBSD suivants:

      -
        L'architecture Intel (version 1.6).


Windows

    Ce logiciel a été testé sur Windows 95, 98 et XP.


Sur Linux, BRLTTY peut inspecter le contenu de l'écran de façon
totalement indépendante de l'utilisateur. Cela est possible
grâce à l'utilisation d'un périphérique spécial offrant un accès
facile aux contenus de la console virtuelle courante. Ce périphérique
a été ajouté à la version 1.1.92 du noyau Linux, et s'appelle
normalement ``/dev/vcsa`` ou ``/dev/vcsa0`` (sur les systèmes
avec ``devfs``, il s'appelle ``/dev/vcc/a``). C'est pourquoi le noyau
Linux 1.1.92 ou supérieur est nécessaire si BRLTTY est utilisé de
cette façon. Cette possibilité:

  -
    Permet à BRLTTY d'être démarré très tôt dans la séquence de
    démarrage du système.

  -
    Active l'afficheur braille pour qu'il soit totalement opérationnel
    à l'invite de logging.

  -
    Facilite fortement pour un utilisateur brailliste
    des tâches d'administration lors du démarrage.


Un correctif pour le programme ``d'écran`` est fourni (voir le sous-répertoire
``Patches``). Il permet à BRLTTY d'accéder à l'image d'un écran
via une mémoire partagée, et, ainsi, permet à BRLTTY d'être utilisé
beaucoup plus efficacement sur des plateformes qui n'ont pas leurs
propres facilité d'inspection du contenu de leur écran. La faiblesse
principale de cette approche de l'écran est que BRLTTY ne peut être
démarré tant que l'utilisateur n'est pas connecté.

BRLTTY ne fonctionne qu'avec des consoles et des applications basées
sur du texte. Il peut être utilisé avec les applications basées sur
``curses``, mais pas avec une application utilisant des caractéristiques
spéciales VGA ou qui requièrent une console graphique (comme le système X
Window).

Bien entendu, vous devez aussi posséder un afficheur braille supporté
(voir la section :ref:`Afficheurs braille
supportés <displays>` pour la liste complète). Nous espérons que des afficheurs
supplémentaires seront supportés dans le futur, donc si vous disposez de quelques
vagues informations de programmation techniques pour un pilote que
vous aimeriez voir supporté, faites-le nous savoir (voir la section
:ref:`Contacts <contact>`).

Enfin, vous avez besoin d'outils pour compiler l'exécutable depuis le
source, ``make``, les compilateurs ``C`` et ``C++``, ``yacc``,
``awk``, etc. Les outils de développement fournis avec les
distributions Unix standards devraient suffire. Si vous rencontrez des
problèmes, contactez-nous et nous vous compilerons un exécutable.
