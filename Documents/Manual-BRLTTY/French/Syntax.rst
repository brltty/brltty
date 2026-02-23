Syntaxe des opérateurs
======================


.. _operand-driver:

Spécification de pilote
-----------------------

Vous devez spécifier un pilote pour un afficheur braille ou une synthèse vocale
via les deux lettres de son
:ref:`Code d`identification de pilote <drivers>`.

Vous pouvez spécifier une liste de pilotes délimités par des virgules. Dans
ce cas, une détection automatique s'effectue en utilisant chaque pilote listé dans la
séquence.
Il se peut que vous soyez obligé de faire des essais afin de déterminer l'ordre le plus
fiable, étant donné que certains pilotes se détectent mieux automatiquement que d'autres.

Si vous ne spécifiez que le mot ``auto``, la détection automatique s'effectue
en n'utilisant que les pilotes connus pour leur fiabilité dans le but
recherché.

.. _operand-braille-device:

Spécification du périphérique braille
-------------------------------------

La forme générale de la spécification d'un périphérique braille (voir l'option
:ref:`-d <options-braille-device>` en ligne de commande, la ligne
:ref:`braille-device <configure-braille-device>` du fichier de
configuration, et l'option de compilation
:ref:`--with-braille-device <build-braille-device>`) est
``qualificateur:`` *donnée*.
Par compatibilité entre d'anciennes versions et les plus récentes, si vous ommettez
le qualificateur c'est ``serial:`` qui est utilisé.

Les types de périphérique suivants sont supportés:


Bluetooth

    Pour un périphérique bluetooth, spécifiez ``bluetooth:`` *addresse*.
    L'adresse doit se composer de six nombres hexadécimaux à deux chiffres
    séparés par des "deux-points", par exemple ``01:23:45:67:89:AB``.

Série

    Pour un périphérique en port série, spécifiez
    ``serial:`` */chemin/vers/peripherique*. Le qualificateur ``serial:``
    est facultatif (pour compatibilité). Si vous donnez un chemin
    relatif, il est déterminé par rapport à ``/dev`` (l'emplacement habituel
    où les périphériques sont définis sur un système de type Unix).
    Les spécifications de périphérique suivantes se réfèrent toutes au port
    série 1 sur Linux:

      - ``serial:/dev/ttyS0``
      - ``serial:ttyS0``
      - ``/dev/ttyS0``
      - ``ttyS0``


USB

    Pour un périphérique USB, spécifiez ``usb:``. BRLTTY cherchera le premier
    périphérique USB qui entraîne l'utilisation du pilote d'afficheur braille.
    Par exemple, si vous avez plus d'un afficheur braille USB nécessitant le
    même pilote, vous pouvez affiner la spécification de pilote en y affectant
    le numéro de série de l'afficheur, comme par exemple ``usb:12345``.
    N.B.: La possibilité "identification par le numéro de série" ne
    fonctionne pas avec certains modèles car certains fabricants, soit
    n'indiquent pas la description du numéro de série, soit l'indiquent
    mais pas en une valeur unique.


Vous pouvez spécifier une liste de pilotes délimités par des virgules. Dans
ce cas, une détection automatique s'effectue en utilisant chaque pilote listé dans la
séquence. Cette possibilité est particulièrement utile si vous avez un
afficheur braille à plusieurs interfaces, par exemple un port série et un USB.
Dans ce cas, il est en général préférable de lister d'abord le port USB, comme
par exemple ``usb:,serial:/dev/ttyS0``, étant donné que l'ancien a
tendance à être mieux détecté que le plus récent.

.. _operand-pcm-device:

Spécification d'un périphérique PCM
-----------------------------------

Dans la plupart des cas, le périphériqve PCM est le chemin complet vers un
périphérique du système approprié. Les exceptions sont:


ALSA

    Le nom et ses arguments pour le périphérique logique ou physique, comme
    *nom*\ [``:``\ *argument*\ ``,``\ ...].


Le périphérique PCM par défaut est:

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

Spécification de périphérique MIDI
----------------------------------

Dans la plupart des cas, le périphérique MIDI est le chemin complet vers un
périphérique du système approprié. Les exceptions sont:


ALSA

    Le client et le port séparés par "deux-points" tel que
    *client*\ ``:`` *port*.
    Vous pouvez spécifier chacun soit comme un nombre soit comme une
    sous-chaîne sensible à la casse de son nom.


Le périphérique MIDI par défaut est:

.. list-table::
   :header-rows: 1

   * - Plateforme
     - Périphérique

   * - Linux/ALSA
     - le premier port de sortie MIDI disponible

   * - Linux/OSS
     - /dev/sequencer
