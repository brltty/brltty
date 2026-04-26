Sujets avancés
==============


.. _multiple:

Installation de plusieurs versions
----------------------------------

Il est facile d'avoir plus d'une version de BRLTTY installée sur  le même
système en même temps. Cette possibilité vous permet de tester une nouvelle
version avant de supprimer l'ancienne.

L'option de compilation :ref:`--with-execute-root <build-execute-root>`
vous permet d'installer la :ref:`hiérarchie des fichiers installés <hierarchy>` complète là où
vous le voulez de sorte qu'elle soit totalement autonome dans son contenu.
Tout en vous rappelant qu'il vaut mieux prendre tous les composants de
BRLTTY dans le système de fichier racine, vous pouvez le compiler ainsi:

::

   ./configure --with-execute-root=/brltty-3.1
   make install

Vous pouvez alors l'exécuter ainsi:

::

   /brltty-3.1/bin/brltty

Quand vous avez fait la version 3.2, installez-la simplement dans un
emplacement différent et exécutez le nouvel exécutable à partir de là.

::

   ./configure --with-execute-root=/brltty-3.2
   make install
   /brltty-3.2/bin/brltty


Jusque-là, tout cela est quelque peu maladroit pour au moins deux raisons.
La première est que ces longs chemins sont trop difficiles à taper, et l'autre
est que vous ne voulez pas bricoler avec
la séquence de démarrage de votre système chaque fois que vous souhaitez aller
à une version différente de BRLTTY.
Ces problèmes se résolvent facilement en ajoutant un lien symbolique pour
l'exécutable.

::

   ln -s /brltty-3.1/bin/brltty /bin/brltty

Quand il est temps d'utiliser la version la plus récente, pointez de nouveau
simplement le lien symbolique.

::

   ln -s /brltty-3.2/bin/brltty /bin/brltty


Si vous aimez vraiment faire de la fantaisie, introduisez un autre niveau
de redirection de façon à rendre tous les fichiers de chaque version de BRLTTY
tels qu'ils sont dans tous les emplacements standards.
D'abord, créez un lien symbolique à travers un emplacement où on peut
repointer aisément à partir de chacun des emplacements standards de BRLTTY.

::

   ln -s /brltty/bin/brltty /bin/brltty
   ln -s /brltty/etc/brltty /etc/brltty
   ln -s /brltty/lib/brltty /lib/brltty

Ensuite, tout ce que vous devez faire est de pointer ``/brltty`` vers la
version désirée.

::

   ln -s /brltty-3.1 /brltty


Disques racines d'Installation/Secours pour Linux
-------------------------------------------------

BRLTTY peut s'exécuter comme un seul exécutable. Tout ce qu'il a besoin de
savoir peut être configuré explicitement lors de la compilation (voir
:ref:`Options de compilation <build>`).
Si le répertoire de données (voir les options de compilation
:ref:`--with-data-directory <build-data-directory>` et
:ref:`--with-execute-root <build-execute-root>`) n'existe pas, BRLTTY
cherche dans ``/etc`` les fichiers dont il a besoin.
Même si un de ces fichiers n'existe pas, BRLTTY fonctionne encore!

Si, pour une raison quelconque, vous avez déjà créé le répertoire de données
(habituellement, ``/etc/brltty``) à la main, il est important de régler
ses permissions de telle sorte que seul le super-utilisateur puisse y créer des
fichiers.

::

   chmod 755 /etc/brltty


Le périphérique d'inspection du contenu de l'écran (habituellement
``/dev/vcsa``) est nécessaire. Il devrait déjà exister, à moins que votre
distribution de Linux ne soit trop vieille. Si nécessaire, vous pouvez le créer
avec:

::

   mknod /dev/vcsa c 7 128
   chmod 660 /dev/vcsa
   chown root.tty /dev/vcsa


Un problème souvent rencontré lorsqu'on essaie d'utiliser BRLTTY dans un
environnement instable, comme un disque racine ou un système incomplet, est
qu'il pourrait ne pas trouver les bibliothèques partagées (ou des parties
de celles-ci) dont il a besoin.
Les disques racines utilisent souvent des versions des bibliothèques
sous-paramétrées et/ou non à jour qui peuvent être inadéquates. La solution est de configurer BRLTTY avec l'optfon de
compilation :ref:`--enable-standalone-programs <build-standalone-programs>`.
Cela supprime toutes les dépendances des bibliothèques partagées, mais
malheureusement, crée aussi un exécutable plus gros.
Il y a un certain nombre d'options de compilation que vous pouvez utiliser
pour supprimer de façon sélective les possibilités de BRLTTY dont vous n'avez pas
besoin de façon à atténuer ce problème (voir la section
:ref:`possibilitéés de la compilation <build-features>`).

L'exécutable est nettoyé pendant le
:ref:`make install <make-install>`. Cela réduit de façon
significative sa taille en supprimant sa table de symboles.
Vous obtiendrez un exécutable beaucoup plus petit, puis copiez-le de son emplacement
d'installation.
Cependant, si vous le copiez depuis le répertoire de compilation, n'oubliez pas de le nettoyer.

::

   strip brltty


Avancées futures
----------------

Outre la réparation de bugs et le support de plus de types d'afficheurs
brailles, nous espérons, si le temps nous le permet, travailler sur:


Meilleure prise en charge des attributs


      - Poursuite des attributs.
      - Mode mixte texte et attributs.


Poursuite du défilement

     Alignement de la plage braille sur une ligne alors qu'elle défile à
     l'écran.

Meilleur support de voix


      - Braille et voix mélangés pour une lecture plus rapide du texte.
      - Meilleur navigation vocale.
      - Plus de synthèses vocales.


Sous-régions sur l'écran

    Ignorer le déplacement du curseur hors de la région, et rendre fluide les
    limites de navigation aux bords de la région.

Bogues connus
-------------

A l'heure où nous écrivons (décembre 2001), les problèmes suivants sont
connus.

La routine-curseur est considérée comme un sous-processus en boucle qui s'exécute
avec une priorité réduite pour éviter de trop utiliser le temps du processeur.
Les chargements de systèmes différents nécessitent des réglages différents de
ses paramètres. Ceux par défaut fonctionnent très bien dans un éditeur Unix
classique sur un système chargé normalement, mais très mal dans d'autres
situations, par exemple sur une liaison série lente vers un nom de machine
supprimé.
