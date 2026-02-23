.. _tables:

Tables
======


.. _table-text:

Tables de texte
---------------

Les fichiers ayant un nom sous la forme ``*.ttb`` sont des tables de texte,
et ceux avec des noms de la forme ``*.tti`` sont des sous-tables de texte.
Elles sont utilisées par BRLTTY pour traduire les caractères à l'écran
dans les représentations brailles qui correspondent à l'informatique 8 points.

Au départ, BRLTTY est configuré pour utiliser la table de texte
:ref:`North American Braille Computer Code <nabcc>`
(NABCC) (code informatique braille nord-américain).
En plus de celle-ci par défaut, les alternatives suivantes sont fournies:


.. csv-table::
   :header-rows: 1
   :file: ../../text-table.csv


Voir l'option :ref:`-t <options-text-table>` en ligne de commande,
la ligne :ref:`text-table <configure-text-table>` du fichier de
configuration, et l'option de compilation
:ref:`--with-text-table <build-text-table>` pour des détails
concernant la façon d'utiliser et de changer de table de texte.

Format des tables de texte
~~~~~~~~~~~~~~~~~~~~~~~~~~

Une table de texte consiste en une séquence d'instructions, une par ligne,
qui définit comment chaque caractère doit être représenté en braille
Vous devez utiliser l'encodage ``UTF-8``.
Un blanc (espaces, tabs) tant au début de la ligne qu'aavant et/ou après
l'opérateur d'une instruction, est ignoré.

Les lignes ne contenant que des blancs sont ignorées.
Si le premier caractère non blanc d'une ligne est "#", cette ligne est
un commentaire et est ignorée.

Instructions des tables de texte
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Les instructions suivantes sont fournies:


``char`` *caractère* *points* # *commentaire*

    Utilise l'instruction ``char`` pour spécifier la façon dont un caractère
    Unicode sera représenté en braille. Les caractères définis par cette
    instruction peuvent également être saisies au clavier braille. Si plusieurs
    caractères ont la même représentation braille, vous ne devriez en définir
    qu'un avec la ligne ``char`` - vous devriez définir l'autre avec la ligne
    ``glyph`` (dont la syntaxe est identique). Si plus d'un caractère ayant la
    même représentation braille est défini avec l'instruction ``char``
    (ce qui est actuellement possible pour des questions de rétrocompatibilité),
    c'est la première qui est sélectionnée.


*caractère*

        Le caractère Unicode qui sera défini. Cela peut être:

          -
            Tout caractère différent d'un antislash ou d'un caractère blanc.

          -
            Un caractère spécial précédé d'un anti-slash.
            Ce sont:

            ``\b``
              Le caractère Effacement
            ``\f``
              Le caractère formfeed
            ``\n``
              Le caractère Nouvelle ligne.
            ``\o###``
              La représentation octale à 3 chiffres d'un caractère.
            ``\r``
              Le caractère retour chariot
            ``\s``
              Le caractère Espace
            ``\t``
              Le caractère Tab horizontal
            ``\u####``
              La représentation hexadécimale à quatre chiffres d'un caractère.
            ``\U########``
              La représentation hexadécimale à huit chiffres d'un caractère.
            ``\v``
              Le caractère tab vertical
            ``\x##``
              La représentation hexadécimale à deux chiffres d'un caractère.
            ``\X##``
              ... (la casse du X et des chiffres n'a pas de signification)
            \#
              Signe d'un nombre littéral.
            ``\``
              Le nom Unicode d'un caractère (utilisez _ pour l'espace).
            ``\\``
              Un antislash littéral.


*dots*

        La représentation braille du caractère Unicode. C'est une séquence d'un
        à huit nombres de points. Si la séquence du nombre de points est entourée
        de parenthèses, vous pouvez séparer les numéros des points l'un de l'autre
        par des blancs. Un numéro de point est un chiffre compris entre ``1``-``8``
        tels que définis par la
        :ref:`Standard Braille Dot Numbering Convention <dots>` (convention
        standard de numérotation de points brailles). Le numéro de point spécial ``0``
        est reconnu quand il n'est pas entouré de parenthèses, et il signifie aucun
        point; il ne peut être utilisé parallèlement à un autre numéro de point.


    Exemples:

      - ``char a 1``
      - ``char b (12)``
      - ``char c ( 4  1   )``
      - ``char \\ 12567``
      - ``char \s 0``
      - ``char \x20 ()``
      - ``char \<LATIN_SMALL_LETTER_D> 145``


``glyph`` *caractère* *dots* # *comment*

    Utilisez l'instruction ``glyph`` pour spécifier la façon dont doit être
    représenté en braille un caractère Unicode. Les caractères définis avec
    cette instruction peuvent uniquement être affichés. On ne peut pas les
    saisir au clavier braille. Voir la ligne ``char`` pour les détails sur
    la syntaxe et pour des exemples.

``byte`` *byte* *points* # *commentaire*

    Utilisez l'instruction ``byte`` pour spécifier comment un caractère en encodage local
    doit être représenté en braille. Il a été retenu pour des raisons de
    compatibilité mais ne devrait pas être utilisé.

    Les caractères Unicode devraient être définis
    (via l'instruction ``char``) de telle sorte que la table de texte demeure
    valide par rapport à l'encodage local.


*byte*

        Le caractère local défini. Il peut être spécifié de la même manière que
        l'opérateur *caractère* de l'instruction ``char`` sauf que les formes
        spécifiques à l'Unicode (\u, \U, \<) ne peuvent pas être utilisées.

*points*

        La représentation braille du caractère local.
        Il peut être spécifié de la même manière que
        l'opérateur *points* de l'instruction ``char``.


``include`` *fichier* # *commentaire*

    Utilisez l'instruction ``include`` pour inclure le contenu d'une sous-table de
    texte. Elle est récursive, ce qui signifie que toute sous-table de texte peut
    inclure elle-même une autre sous-table de texte. Prenez soin de vous
    assurer de ne pas créer une "inclusion en boucle".


*fichier*

        Le fichier à inclure. Cela peut être un chemin relatif ou absolu. Si c'est
        relatif, il est ancré au répertoire contenant le fichier qui inclut.


.. _table-attributes:

Tables d'attributs
------------------

Les fichiers aux noms sous la forme ``*.atb`` sont des tables d'attributs et ceux
aux noms sous la forme ``*.ati`` sont des sous-tables d'attributs. Ils sont
utilisés quand BRLTTY affiche les
attributs de l'écran au lieu du contenu de l'écran (voir la commande
:ref:`DISPMD <command-DISPMD>`).
Chacun des huit points braille représente l'un des huit bits d'attributs
``VGA``.
Les tables d'attributs suivantes sont fournies:


left_right

    La colonne à gauche représente les couleurs de premier plan:


Point 1
Bleu

Point 2
Vert

Point 3
Rouge

Point 7
Brillant

    La colonne à droite représente les couleurs de fond:


Point 4
Bleu

Point 5
Vert

Point 6
Rouge

Point 8
Clignotant

    Un point est affiché quand son bit d'attribut correspondant est actif.
    C'est la table d'attributs par défaut car c'est la plus
    intuitive. Cependant, l'un de ses problèmes est qu'il est difficile de
    distinguer la différence entre la vidéo normale (noir sur blanc) et inversée
    (blanc sur noir).

invleft_right

    La colonne à gauche représente les couleurs de premier plan:


Point 1
Bleu

Point 2
Vert

Point 3
Rouge

Point 7
Brillant

    La colonne à droite représente les couleurs de fond:


Point 4
Bleu

Point 5
Vert

Point 6
Rouge

Point 8
Clignotant

    Un bit de fond est actif pour générer ses points correspondant, tandis
    qu'un bit de premier plan est inactif pour générer son point correspondant.
    Cette logique non intuitive facilite en fait la lecture de la plupart des
    combinaisons d'attributs communément utilisées.

upper_lower

    Le carré supérieur représente les couleurs de premier plan:


Point 1
Rouge

Point 4
Vert

Point 2
Bleu

Point 5
Brillant

    Le carré inférieur représente les couleurs d'arrière-plan:


Point 3
Rouge

Point 6
Vert

Point 7
Bleu

Point 8
Clignotant

    Un point s'affiche quand le bit de l'attribut qui y correspond est actif.

Voir l'option :ref:`-a <options-attributes-table>` en ligne de
commande, la ligne :ref:`attributes-table <configure-attributes-table>`
du fichier de configuration, et l'option de compilation
:ref:`--with-attributes-table <build-attributes-table>` pour des
détails concernant l'utilisation et le changement de table
d'attributs.

Format des tables d'attributs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Une table d'attributs est une séquence de lignes de commande, avec une
commande par ligne, qui définit comment doit être représenté en braille les
combinaisons des attributs ``VGA``. Vous devez utiliser un encodage de caractères
``UTF-8``.
Les espaces blancs (les vides, tabulations) au début d'une ligne, ou avant
et/ou après l'opérateur d'une ligne de commande, sont ignorés.
Les lignes ne contenant que des espaces sont ignorées.
Si le premier caractère non-blanc d'une ligne est "#" cette ligne est un
commentaire et est ignorée.

Lignes de commande des tables d'attributs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Les lignes de commande suivantes sont fournies:


``dot`` *point* *etat* # *commentaire*

    Utilisez l'instruction ``dot`` pour spécifier ce que représente un point
    particulier.


*point*

        Le point qui est défini. C'est une seule case allant de
        ``1`` à ``8`` comme défini par la

	:ref:`Convention standard du nombre de points brailles <dots>`.

*état*

        Ce que représente le point. Il peut s'agir:


``on``
D'un point élevé si l'attribut nommé est actif.

``off``
D'un point enfoncé si l'attribut nommé est inactif.


        Les noms des bits des attributs sont:


0X01
``bleu premier plan``

0X02
``vert premier plan``/

0X04
``rouge premier plan``/

0X08
``brillant premier plan``/

0X10
``fond bleu``

0X20
``fond vert``

0X40
``fond rouge``

0X80
``fond clignotant``


    Exemples:

      - ``dot 1 =fg-red``
      - ``dot 2 ~bg-blue``


``include`` *fichier* # *commentaire*

    Utilisez la ligne  ``include`` pour inclure le contenu d'une sous-table d'attributs.
    Il est récursif, ce qui signifie que toute sous-table d'attributs peut
    inclure elle-même une autre sous-table d'attributs. Prenez soin de vous
    assurer de ne pas créer une "inclusion en boucle".


*fichier*

        Le fichier à inclure. Cela peut être un chemin relatif ou absolu. Si c'est
        relatif, il est ancré au répertoire contenant le fichier qui inclut.


.. _table-contraction:

Tables de braille abrégé
------------------------

Les fichiers aux noms sous la forme ``*.ctb`` sont des tables de braille abrégé
et ceux aux noms sous la forme ``*.cti`` sont des sous-tables de braille abrégé.
Ils sont utilisés par BRLTTY pour traduire des séquences de caractères à l'écran
en leurs représentations correspondantes en braille abrégé.

BRLTTY présente du braille abrégé si:

  -
    Une table de braille abrégé a été sélectionnée.
    Voir l'option :ref:`-c <options-contraction-table>` en ligne
    de commande et la ligne
    :ref:`contraction-table <configure-contraction-table>` du
    fichier de configuration pour des détails.

  -
    La fonction braille 6 points a été activée. Voir la commande
    :ref:`SIXDOTS <command-SIXDOTS>` et la préférence
    :ref:`Text Style <preference-text-style>` pour des détails.

Cette possibilité n'est pas disponible si vous avez spécifié l'option de
compilation
:ref:`--disable-contracted-braille <build-contracted-braille>`.

Les tables d'abrégé suivantes sont fournies:


.. csv-table::
   :header-rows: 1
   :file: ../../contraction-table.csv

Voir l'option :ref:`-c <options-contraction-table>` en ligne de commande
et la ligne :ref:`contraction-table <configure-contraction-table>` du fichier de
configuration pour des détails sur la façon d'utiliser une table d'abrégé.

Format des tables de braille abrégé
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Une table de braille abrégé est une séquence de lignes de commande, avec une
commande par ligne, qui définit comment les séquences de caractères vont être
représentées en braille. Vous devez utiliser un encodage de caractères
``UTF-8``.

Les espaces blancs (les vides, tabulations) au début d'une ligne, ou avant
et/ou après l'opérateur d'une ligne de commande, sont ignorés.
Les lignes ne contenant qu'à des espaces sont ignorées.
Si le premier caractère non-blanc d'une ligne est "#" cette ligne est un
commentaire et est ignorée.

Le format d'une entrée de table de braille abrégé est:

.. parsed-literal::

   *directive* *opérateur* ... [*commentaire*]

Chaque ligne a un nombre d'opérateurs spécifique.
Tout texte au-delà du dernier opérateur d'une ligne est interprété comme un commentaire.
L'ordre des entrées à l'intérieur de la table de braille abrégé est, en général, selon la
convenance de son/ses mainteneur(s).
Une entrée qui définit une entité, comme ``class``,
doit précéder toutes les références de cette entité.

Les entrées qui correspondent à des séquences de caractères
sont automatiquement réorganisées de la plus longue à la plus courte
afin que des correspondances plus longues soient toujours préférées.
Si plus d'une entrée correspond à la même séquence de caractères,
leur organisation d'origine dans la table est maintenue.
Ainsi, la même séquence peut être traduite différemment dans des circonstances
différentes.

Opérateurs des tables de braille abrégé
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


*characters*

    Le premier opérateur d'une séquence de caractères correspondant à une ligne
    est la séquence de caractères à laquelle elle doit correspondre.
    Chaque caractère dans la séquence peut être:

      -
            Tout caractère différent d'un antislash (barre oblique inversée) ou d'un caractère blanc.

          -
            Un caractère spécial précédé d'une barre oblique inversée.
            Ce sont:

            ``\b``
              Le caractère Effacement
            ``\f``
              Le caractère formfeed
            ``\n``
              Le caractère Nouvelle ligne.
            ``\o###``
              La représentation octale à 3 chiffres d'un caractère.
            ``\r``
              Le caractère retour chariot
            ``\s``
              Le caractère Espace
            ``\t``
              Le caractère Tab horizontale
            ``\u####``
              La représentation hexadécimale à quatre chiffres d'un caractère.
            ``\U########``
              La représentation hexadécimale à huit chiffres d'un caractère.
            ``\v``
              Le caractère tab verticale
            ``\x##``
              La représentation hexadécimale à deux chiffres d'un caractère.
            ``\X##``
              ... (la casse du X et des chiffres n'a pas de signification)
            \#
              Signe d'un nombre littéral.
            ``\``
              Le nom Unicode d'un caractère (utilisez _ pour l'espace).
            ``\\``
              Un antislash littéral.


*representation*

    Le second opérateur de ces lignes correspondant à la séquence de caractères
    qui en a une est la représentation braille de la séquence.
    Chaque cellule braille est spécifiée comme une séquence d'un à huit numéros
    de points. Un numéro de point est un chiffre compris entre ``1``-``8``
    tels que définis par la
    :ref:`Standard Braille Dot Numbering Convention <dots>` (convention standard
    de numérotation de points brailles). Le numéro de point spécial ``0``
    est reconnu quand il n'est pas entouré de parenthèses, et il signifie aucun
    point; il ne peut pas être utilisé parallèlement à un autre numéro de point.


.. _contraction-opcodes:

Opcodes
^^^^^^^

Un opcode est un mot-clé qui dit au traducteur comment interpréter les
opérateurs. Les opcodes sont groupés ici par leur fonction.

.. _contraction-opcodes-administration:

Administration de la table
^^^^^^^^^^^^^^^^^^^^^^^^^^

Ces opérateurs facilitent l'écriture des tables de braille abrégé.
Ils n'ont pas d'effet direct sur la traduction de caractère.


.. _contraction-opcode-include:

``include`` *chemin*
    Inclut le contenu d'un autre fichier.
    L'inclusion peut se faire à n'importe quel niveau.
    Les chemins relatifs sont déterminés par rapport au répertoire du fichier inclu.

.. _contraction-opcode-locale:

``locale`` *locale*
    Définit la locale pour l'interprétation d'un caractère (minuscule,
    majuscule, numérique, etc). La locale peut être définie comme:


*langue*\ [``_``\ *pays*\ ][``.``\ *charset*\ ][``@``\ *modifier*\ ]

        La composante *langue* est requise et devrait être un code de

	langue à deux lettres ``ISO-639``.
        La composante *pays* est facultative et devrait être un code de

	pays à deux lettres ``ISO-3166``.
        La composante *charset* est optionnelle et devrait être le nom

	d'une table de caractères, comme ``ISO-8859-1``.

C

        7-bit ASCII.

-

        Aucune locale.

    La dernière spécification de locale s'applique à toute la table. Si
    vous n'utilisez pas cet opcode, la locale ``C`` est utilisée.


.. _contraction-opcodes-symbols:

Définition d'un symbole spécial
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Ces opcodes définissent les caractères spéciaux qui doivent être insérés dans
le texte braille afin de le rendre plus clair.


.. _contraction-opcode-capsign:

``capsign`` *points*
    Le symbole qui met en majuscule une seule lettre.

.. _contraction-opcode-begcaps:

``begcaps`` *points*
    Le symbole qui commence un bloc de lettres en majuscule à l'intérieur d'un
    mot.

.. _contraction-opcode-endcaps:

``endcaps`` *points*
    Le symbole qui termine un bloc de lettres en majuscules à l'intérieur
    d'un mot.

.. _contraction-opcode-letsign:

``letsign`` *points*
    Le symbole qui désigne une lettre ne faisant pas partie du mot.


.. _contraction-opcode-numsign:

``numsign`` *points*
    Le symbole marquant le début d'un nombre.

``lastlargesign`` *points*
    Traduit les caractères quel que soit l'endroit où ils apparaissent.
    Supprime les espaces qui les précède si le mot précédent a été marqué
    par le code "largesign".


.. _contraction-opcodes-translation:

Traduction de caractère
^^^^^^^^^^^^^^^^^^^^^^^

Ces opcodes définissent les représentations braille des séquences de caractères.
Chacun d'eux définit une entrée à l'intérieur de la table de braille abrégé.
Ces entrées peuvent être définies dans n'importe quel ordre, sauf, comme
remarqué ci-dessous, lorsqu'elles définissent des représentations
alternatives de la même séquence de caractères.

Chacun de ces opcodes a un opérateurs *caractères* (qui doit être
spécifié comme une *chaîne*), et une condition de configuration dirigeant
son utilisation.
Le texte est traduit strictement de la gauche vers la droite, caractère par
caractère, avec l'entrée la plus acceptable pour chaque position utilisée.
S'il y a plus d'une entrée acceptable pour une position donnée, celle ayant la
chaîne de caractères la plus longue est utilisée. S'il y a plus d'une
entrée acceptable pour la même chaîne de caractères, celle définie le plus
au début de la table est utilisée (c'est la seule dépendance de l'ordre).

Beaucoup de ces opcodes ont un opérateur *points* qui définit la
représentation braille de son opérateur *caractères*. Il peut
être aussi spécifié comme un signe égal (``=``), au quel cas il
signifie l'une des deux choses. Si l'entrée est pour un seul caractère,
cela signifie que la représentation du braille informatique sélectionnée
(voir l'option :ref:`-t <options-text-table>` en ligne de
commande et la ligne :ref:`text-table <configure-text-table>`
du fichier de configuration) de ce caractère doit être utilisée. Si
c'est pour une séquence multi-caractères, la représentation par défaut
de chaque caractère (voir :ref:`always <contraction-opcode-always>`) dans une séquence doit être utilisée.

Certains termes spéciaux sont utilisés à l'intérieur des descriptions de ces
opcodes.


word

    Une séquence maximale d'une ou plusieurs lettres à la suite.


Enfin, voici maintenant la description des opcodes eux-mêmes:


.. _contraction-opcode-literal:

``literal`` *caractères*
    Traduit ce qui est lié à l'espace et qui contient une séquence de
    caractères en braille informatique (voir l'option
    :ref:`-t <options-text-table>` en ligne de commande et la ligne
    :ref:`text-table <configure-text-table>` du fichier de
    configuration).

.. _contraction-opcode-replace:

``replace`` *caractères* *caractères*
    Remplace la première valeur des caractères, quel que soit l'endroit où ils
    apparaissent, par la seconde. Les caractères remplacés ne sont pas
    réinsérés.

.. _contraction-opcode-always:

``always`` *caractères* *points*
    Traduit les caractères quel que soit l'endroit où ils apparaissent.
    S'il n'y a qu'un caractère, alors, en plus, définit la représentation par
    défaut de ce caractère.

.. _contraction-opcode-repeated:

``repeatable`` *caractères* *points*
    Traduit les caractères quel que soit l'endroit où ils apparaissent. Ignore toute
    répétition immédiate de la même séquence.

.. _contraction-opcode-largesign:

``largesign`` *caractères* *points*
    Traduit les caractères quel que soit l'endroit où ils apparaissent. Supprime les
    espaces entre les mots qui se suivent et qui sont gérés par cet opcode.

.. _contraction-opcode-word:

``word`` *caractères* *points*
    Traduit les caractères s'ils forment un mot.

.. _contraction-opcode-joinword:

``joinword`` *caractères* *points*
    Traduit les caractères s'ils forment un mot. Supprime l'espace suivant si
    le premier caractère qui le suit est une lettre.

.. _contraction-opcode-lowword:

``lowword`` *caractères* *points*
    Traduit les caractères s'ils forment un mot lié à un espace.

.. _contraction-opcode-contraction:

``contraction`` *caractères*
    Fait précéder les caractères d'un signe-lettre (voir
    :ref:`letsign <contraction-opcode-letsign>`) s'ils forment un mot.

.. _contraction-opcode-sufword:

``sufword`` *caractères* *points*
    Traduit les caractères s'ils forment soit un mot, soit s'ils sont au
    début d'un mot.

.. _contraction-opcode-prfword:

``prfword`` *caractères* *points*
    Traduit les caractères s'ils forment soit un mot, soit s'ils sont à la
        fin d'un mot.

.. _contraction-opcode-begword:

``begword`` *caractères* *points*
    Traduit les caractères s'ils sont au début d'un mot.

.. _contraction-opcode-begmidword:

``begmidword`` *caractères* *points*
    Traduit les caractères s'ils sont au début ou au milieu d'un mot.

.. _contraction-opcode-midword:

``midword`` *caractères* *points*
    Traduit les caractères s'ils sont au milieu d'un mot.

.. _contraction-opcode-midendword:

``midendword`` *caractères* *points*
    Traduit les caractères s'ils sont au milieu ou à la fin d'un mot.

.. _contraction-opcode-endword:

``endword`` *caractères* *points*
    Traduit les caractères s'ils sont à la fin d'un mot.

.. _contraction-opcode-prepunc:

``prepunc`` *caractères* *points*
    Traduit les caractères s'ils font partie de la ponctuation au début d'un
    mot.

.. _contraction-opcode-postpunc:

``postpunc`` *caractères* *points*
    Traduit les caractères s'ils font partie de la ponctuation à la fin d'un
    mot.

.. _contraction-opcode-begnum:

``begnum`` *caractères* *points*
    Traduit les caractères s'ils sont au début d'un nombre.

.. _contraction-opcode-midnum:

``midnum`` *caractères* *points*
    Traduit les caractères s'ils sont au milieu d'un nombre.

.. _contraction-opcode-endnum:

``endnum`` *caractères* *points*
    Traduit les caractères s'ils sont à la fin d'un nombre.


.. _contraction-opcodes-classes:

Classes de caractère
^^^^^^^^^^^^^^^^^^^^

Ces opcodes définissent et utilisent des classes de caractères. Une classe de
caractères associe un type de caractère à un nom. Le nom se réfère alors à
n'importe quel caractère à l'intérieur de la classe. Un caractère peut
appartenir à plus d'une classe.

Les classes de caractère suivantes sont automatiquement prédéfinies, basées sur
la locale sélectionnée.


digit

    Caractères numériques.

letter

    Les caractères alphabétiques majuscule ou minuscule.
    Certaines locales ont des lettres supplémentaires qui ne sont ni en majuscule
    ni en minuscule.

lowercase

    Les caractères alphabétiques minuscules.

punctuation

    Caractères imprimables qui ne sont ni des espaces ni alphanumériques.

space

    Caractères d'espacement.
    Dans la locale par défaut, il s'agit de: espace, tabulation horizontale, tabulation
    verticale, retour chariot, nouvelle ligne, saut de page.

uppercase

    Caractères alphabétiques en majuscules.


Les opcodes qui définissent et utilisent des classes de caractères sont:


.. _contraction-opcode-class:

``class`` *nom* *caractères*
    Définit une nouvelle classe de caractère. L'opérateur *caractères*
    doit être spécifié comme une *chaîne*. Une classe de caractère ne peut
    pas être utilisée tant qu'elle n'est pas définie.

.. _contraction-opcode-after:

``after`` *class* *opcode* ...
    L'opcode spécifié est assez contraint dans le sens où la séquence de
    caractères adéquat doit être immédiatement précédée par un caractère
    appartenant à la classe spécifiée.
    Si vous utilisez plus d'une fois cet opcode sur la même ligne, l'union des
    caractères de toute la classe est utilisée.

.. _contraction-opcode-before:

``before`` *class* *opcode* ...
    L'opcode spécifié est assez contraint dans le sens où la séquence de
    caractères adéquat doit être immédiatement suivie par un caractère
    appartenant à la classe spécifiée.
    Si vous utilisez plus d'une fois cet opcode sur la même ligne, l'union des
    caractères de toute la classe est utilisée.


.. _table-key:

Tables de touches
-----------------

Les fichiers aux noms ayant la forme ``*.ktb`` sont des tables de touches, et ceux
aux noms ayant la forme ``*.kti`` sont des sous-tables de touches.
Ils sont utilisés par BRLTTY pour associer des combinaisons de touches
de l'afficheur braille et du clavier à des commandes BRLTTY.

Les noms de fichier de table de touches de l'afficheur braille commencent par
``brl-`` *xx*\ ``-``", où *xx* représente le
:ref:`code d`identification de pilote <drivers>` à deux lettres. Le reste
du nom identifie le(s) modèle(s) pour le(s)quel(s) la table de touches est
utilisée.

Les noms de fichier de table de touches du clavier commencent par ``kbd-``.
Le reste du nom décrit le type de clavier pour lequel a été conçue la table
de touches.

Les tables de touches suivantes sont fournies:


braille
associations pour les claviers braille

desktop
associations pour les claviers complets

keypad
associations pour la navigation à partir du pavé numérique

laptop
associations pour les claviers sans pavé numérique

sun_type6
associations pour les claviers Sun Type 6

    Voir l'option :ref:`-k <options-key-table>` en ligne de commande
    et la ligne
    :ref:`key-table <configure-key-table>` du fichier de
    configuration pour plus de détails concernant la manière de sélectionner
    une table de touches de clavier.

Format des tables de touches
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Une table de touches consiste en une séquence d'instructions, une par ligne,
qui définit comment les touches et les combinaisons de touches seront interprétées.
Vous devez utiliser l'encodage ``UTF-8``.
Un blanc (espaces, tabs) tant au début de la ligne qu'aavant et/ou après n'importe quel opérateur,
est ignoré.

Les lignes ne contenant que des blancs sont ignorées.
Si le premier caractère non blanc d'une ligne est un nombre ("#"), cette ligne est
un commentaire et est ignorée.

L'ordre de résolution de chaque appui de touche/événement qui se produit
est le suivant:

  #.
    Un appui sur une touche de raccourci ou une action définie dans le contexte actuel.
    Voir la ligne
    :ref:`hotkey <key-table-hotkey>`
    pour des détails.

  #.
    Une combinaison de touches définie dans le contexte actuel.
    Voir la ligne
    :ref:`bind <key-table-bind>`
    pour des détails.

  #.
    Une commande du clavier braille définie dans le contexte actuel.
    Voir les lignes
    :ref:`map <key-table-map>`
    et
    :ref:`superimpose <key-table-superimpose>` d
    pour des détails.

  #.
    Une combinaison de touches définie dans le contexte par défaut.
    Voir la ligne
    :ref:`bind <key-table-bind>`
    pour des détails.


      Les lignes suivantes sont fournies:

.. _key-table-assign:

La ligne Assign
^^^^^^^^^^^^^^^

Crée ou met à jour une variable associée au niveau include actuel.
La variable est visible aux niveaux include actuel et inférieur, mais pas aux
niveaux include supérieurs.

``assign`` *variable* [*valeur*]


*variable*

    Le nom de la variable.
    Si la variable n'existe pas déjà au niveau include actuel, elle sera créée.

*valeur*

    La valeur qui sera associée à la variable.
    Si on ne la fournit pas, une valeur zéro (null) est affectée.


La séquence d'échappement \{variable} est remplacée par la valeur de la
variable nommée dans les accolades. La variable doit être définie au niveau
include actuel ou supérieur.

Exemples:

  - ``assign nullValue``
  - ``assign ReturnKey Key1``
  - ``bind \{ReturnKey} RETURN``


.. _key-table-bind:

La ligne Bind
^^^^^^^^^^^^^

Définit la commande qui est BRLTTY est exécutée quand on appuie sur une ou plusieurs
combinaisons de touches particulières.
L'association est définie dans le contexte actuel.

        ``bind`` *touches* *commande*


*touches*

                La combinaison de touches à associer.

               C'est une séquence d'un ou plusieurs noms de touches séparés par
               des signes plus (``+``). Le nom de touche à la fin (ou seulement)
               peut être éventuellement précédé d'un point d'exclamation
               (``!``). Vous pouvez appuyer sur les touches dans
               n'importe quel ordre sauf si le nom de la touche à la fin est
               précédé d'un point d'exclamation, alors on doit appuyer dessus
               en dernier.
               Le préfixe du point d'exclamation signifie que la commande est
               exécutée dès qu'on appuie sur cette touche. S'il n'est pas
               utilisé, la commande est exécutée dès qu'on effectue
               une des touches.

*commande*

                Le nom d'une commande BRLTTY. Un ou plusieurs modificateurs
                peuvent éventuellement être associé au nom de la commande en
                utilisant un signe plus (``+``) comme séparateur.

      -
        Pour les commandes qui activent/désactivent une fonctionnalité:

           -
            Si vous spécifiez le modificateur ``+on``, la fonctionnalité est
            alors activée.

          -
            Si vous spécifiez le modificateur ``+off``, la fonctionnalité est
            alors désactivée.

          -
            Si vous ne spécifiez ni ``+on`` ni ``+off``, l'état de la fonctionnalité
            est activable/désactivable en bascule.

      -
        Pour les commandes qui déplacent la plage braille:

          -
            Si vous spécifiez le modificateur ``+route``, si nécessaire,
            le curseur est automatiquement routé afin d'être toujours visible
            sur l'afficheur braille.

      -
        Pour les commandes qui déplacent la plage braille vers une ligne
        spécifique de l'écran:

          -
            Si vous spécifiez le modificateur ``+toleft``,
            la plage braille est alors également déplacée au début de cette
            ligne.

          -
            Si vous spécifiez le modificateur ``+scaled``, l'ensemble de
            touches associé à la commande est interprété comme si c'était une
            barre de défilement. Si vous ne le spécifiez pas, il y a une
            correspondance une à une entre les touches et les lignes.

      -
        Pour les commandes qui demandent un complément (offset):

          -
            Vous pouvez spécifier le modificateur +*offset*, où
            *offset* est un entier non négatif.
            Si vous ne le fournissez pas, ``+0`` est supposé.


Exemples:

  - ``bind Key1 CSRTRK``
  - ``bind Key1+Key2 CSRTRK+off``
  - ``bind Key1+Key3 CSRTRK+on``
  - ``bind Key4 TOP``
  - ``bind Key5 TOP+route``
  - ``bind VerticalSensor GOTOLINE+toleft+scaled``
  - ``bind Key6 CONTEXT+1``


.. _key-table-context:

La ligne Context
^^^^^^^^^^^^^^^^

Définit des façons alternatives d'interpréter certains événements et/ou
combinaisons de touches. Un contexte contient des définitions créées avec les
lignes
:ref:`bind <key-table-bind>`,
:ref:`hotkey <key-table-hotkey>`,
:ref:`map <key-table-map>`,
et
:ref:`superimpose <key-table-superimpose>`.

``context`` *identificateur* [*titre*]


*identificateur*

    À l'intérieur du contexte sous-jacent dans lequel doivent être créées les définitions.
    Cela peut être:


      -
        Un de ces noms spéciaux:


default

            Le contexte par défaut. Si une combinaison de touches n'a pas été définie
            dans le contexte actuel, c'est alors sa définition dans le contexte
            par défaut qui sera utilisée. Cela ne s'applique qu'aux définitions
            créées par la ligne
            :ref:`bind <key-table-bind>`.

menu

            Ce contexte est utilisé quand on est à l'intérieur du menu des
            préférences de BRLTTY.

      -
        Un entier compris entre ``0`` et ``252``.
        Le contexte ``0`` est une façon alternative de se référer au contexte
        par défaut. Vous devriez éviter les numéros de contexte supérieurs car
        le numéro le plus élevé autorisé est susceptible de changer sans signalement,
        si, par exemple, on ajoute davantage de contextes nommés.


*titre*

    Une description lisible par un humain du  contexte.
    Il peut contenir des espaces et vous devriez utiliser les conventions de
    mise en majuscules standards.
    Cet opérande est facultatif. Si on le fournit lors de la sélection d'un
    contexte ayant déjà un titre, les deux doivent correspondre.
    Les contextes nommés ont déjà des titres internes attribués.

    Les contextes numériques sont créés au départ sans titres.


Un contexte est créé la première fois qu'il est sélectionné.

Il peut être ensuite sélectionné autant de fois que nécessaire.

Toutes les définitions sous-jacentes jusqu'à la prochaine ligne
:ref:`context <key-table-context>` ou à la fin du niveau include
actuel sont créées à l'intérieur du contexte sélectionné.

Le contexte initial du premier niveau de la table de touches est ``default``.
Le contexte initial d'une sous-table de touches incluse est le contexte qui
a été sélectionné lorsqu'il a été inclu.

Les changements de contexte dans les sous-tables de touches incluses n'affectent
pas le contexte de la table de touches qui inclut ou de la sous-table.

Si un contexte a un titre (tous les contextes nommés et les contextes numériques
pour lesquels on a fourni l'opérande *title*), il demeure en place.

Quand un événement de touches entraîne l'activation d'un contexte permanent,
ce contexte reste actuel jusqu'à ce qu'un événement de touches subséquent
entraîne l'activation d'un contexte permanent différent.

Si un contexte n'a pas de titre (les contextes numériques pour lesquels on n'a
pas fourni l'opérande *title*), il est temporaire.

Quand un événement de touche provoque l'activation d'un contexte temporaire,
ce contexte n'est utilisé que pour interpréter le tout prochain événement de
touche.

Exemples:

  - ``context menu``
  - ``context 1 Braille Input``
  - ``context 2``


.. _key-table-hide:

La ligne Hide
^^^^^^^^^^^^^

Spécifie si des définitions (voir les lignes
:ref:`bind <key-table-bind>`,
:ref:`hotkey <key-table-hotkey>`,
:ref:`map <key-table-map>`,
et
:ref:`superimpose <key-table-superimpose>`) et les remarques (voir
la ligne :ref:`note <key-table-note>`) sont incluses ou pas
dans le texte d'aide de la table de touches.

``hide`` *state*


*state*

    Un de ces mots-clés:


on
Elles sont exclues.

off
Elles sont incluses.


L'état spécifié s'applique à toutes les définitions et les notes qui en découlent
jusqu'à la prochaine ligne ``hide`` ou jusqu'à la fin du niveau include actuel.
L'état initial du premier niveau de la table de touches est ``off``.
L'état initial d'une sous-table de touches incluse est l'état qui a été
sélectionné quand elle a été incluse.

Les changements d'état à l'intérieur des sous-tables de touches incluses
n'affectent pas l'état de la table de touche ou de la sous-table qui inclut.

Exemples:

  - ``hide on``


.. _key-table-hotkey:

La ligne Hotkey
^^^^^^^^^^^^^^^

Associe l'appui ou la survenance d'un événement d'une touche spécifique
à deux commandes BRLTTY distinctes.

Les associations sont définies dans le contexte actuel.

``hotkey`` *touche* *appui* *effectuer*


*touche*

    Le nom de la touche qui sera associée.

*appui*

    Le nom de la commande BRLTTY qui sera exécutée à chaque fois qu'on appuiera
    sur la touche.


*Effectuer*

    Le nom de la commande BRLTTY qui sera exécutée à chaque fois qu'on effectuera
    la touche.


On peut coller des modificateurs aux noms de commande.

Voir l'opérande *command* de la ligne
:ref:`bind <key-table-bind>` pour des
détails.

Spécifiez ``NOOP`` si aucune commande ne sera exécutée.
Spécifier ``NOOP`` pour deux commandes désactive la touche dans les faits.

Exemples:

  - ``hotkey Key1 CSRVIS+off CSRVIS+on``
  - ``hotkey Key2 NOOP NOOP``


.. _key-table-ifkey:

La ligne IfKey
^^^^^^^^^^^^^^

Applique une ligne de la table de touches à la condition que
le périphérique ait une touche particulière.

``ifkey`` *touche* *ligne*


*touche*

    Le nom de la touche dont la disponibilité doit être testée.

*ligne*

    La ligne de la table de touches qui doit être appliquée sous condition.


Exemples:

  - ``ifkey Key1 ifkey Key2 bind Key1+Key2 HOME``


.. _key-table-include:

La ligne Include
^^^^^^^^^^^^^^^^

Exécute les lignes à l'intérieur d'une sous-table de touches.

Cela est récursif, ce qui signifie que n'importe quelle table de touches
peut s'inclure elle-même dans une autre sous-table.

Il faut faire attention à bien s'assurer qu'une "boucle d'inclusion" ne soit
pas créée.

``include`` *fichier*


*file*

    La sous-table de touche qui doit être incluse.
    Il peut s'agir d'un chemin soit relatif soit absolu.
    S'il est relatif, il est ancré au répertoire contenant la table de touches
    qui inclut ou la sous-table.


Exemples:

  - ``include common.kti``
  - ``include /chemin/vers/mes/touches.kti``


.. _key-table-map:

La ligne Map
^^^^^^^^^^^^

Fait correspondre une touche à une fonction de clavier braille.
La correspondance est définie à l'intérieur du contexte actuel.

``map`` *touche* *fonction*


*touche*

    Le nom de la touche qui doit être associée. Vous pouvez associer plus d'une
    touche à la même fonction de clavier braille.

*fonction*

    Le nom de la fonction de clavier braille. Cela peut être un des
    mots-clés suivants:


DOT1
Le point braille standard en haut à gauche.

DOT2
Le point braille standard au milieu à gauche

DOT3
Le point braille standard en bas à gauche.

DOT4
Le point braille standard en haut à droite.

DOT5
Le point braille standard au milieu à droite.

DOT6
Le point braille standard en bas à droite.

DOT7
Le point braille informatique en bas à gauche.

DOT8
Le point braille informatique en bas à droite.

SPACE
La barre d'espace.

SHIFT
La touche shift.

UPPERCASE

        Si on doit entrer une lettre minuscule, la traduit alors dans son
        équivalent en majuscule.

CONTROL
La touche contrôle.

META
La touche alt gauche.


Si une combinaison de touches ne consiste qu'en des touches qui ont été
associées à des fonctions du clavier braille, et si ces fonctions, lorsqu'elles
sont combinées, constituent une commande de clavier braille valide, la commande
est alors exécutée dès que les touches sont utilisées.
Une commande de clavier braille valide doit inclure soit n'importe quelle
combinaison de points, soit la barre d'espace (mais pas les deux).
Si on inclut au moins un point braille, les fonctions du clavier braille
spécifiée par les lignes
:ref:`superimpose <key-table-superimpose>`
dans le même contexte sont aussi incluses implicitement.

Exemples:

  - ``map Key1 DOT1``


.. _key-table-note:

La ligne Note
^^^^^^^^^^^^^


Ajoute une explication lisible par un humain au texte d'aide de la table de
touches.

Les remarques sont utilisées en général, par exemple, pour décrire la place,
les tailles et les formes des touches d'un périphérique.

``note`` *texte*


*texte*

    L'explication qui doit être ajoutée.
    Elle peut contenir des espaces et devrait être grammaticalement correcte.


Chaque remarque comporte exactement une ligne de texte d'explication.
Le grand espace est ignoré donc on ne peut spécifier l'indentation.

Il n'y a pas de limite au nombre de remarques que vous pouvez spécifier.
Toutes sont regroupées et présentées dans un seul bloc au début du texte d'aide
de la table de touches.

Exemples:

  - ``note Key1 est la touche ronde tout à gauche de la partie frontale.``


.. _key-table-superimpose:

La ligne Superimpose
^^^^^^^^^^^^^^^^^^^^

Inclut implicitement une fonction de clavier braille à chaque fois qu'une
commande de clavier braille d'au moins un point esst exécutée.

L'inclusion implicite est définie dans le contexte actuel.

Vous pouvez spécifier n'importe quel numéro parmi elles.

``superimpose`` *fonction*


*fonction*

    Le nom de la fonction de clavier braille. Voir l'opérande *function*
    de la ligne :ref:`map <key-table-map>` pour des détails.


Exemples:

  - ``superimpose DOT7``


.. _key-table-title:

La ligne Title
^^^^^^^^^^^^^^

Fournit un résumé lisible par un humain de l'objectif de la table de touches.

``title`` *texte*


*texte*

   Un résumé d'une ligne de la raison pour laquelle est utilisée la table de
   touches. Il peut contenir des espaces et vous devriez utiliser les
   conventions de mise en majuscules standards.


Le titre d'une table de touches ne peut être spécifié qu'une fois.

Exemples:

  - ``title Bindings for Keypad-based Navigation``


.. _keyboard-properties:

Propriétés du clavier
~~~~~~~~~~~~~~~~~~~~~

Par défaut, tous les claviers sont pris en charge.
Un sous-paramètre des claviers peut être sélectionné en spécifiant une ou plusieurs des
propriétés suivantes (voir l'option :ref:`-K <options-keyboard-properties>`
en ligne de commande et la ligne :ref:`keyboard-properties <configure-keyboard-properties>` du fichier de configuration):


type

Le type de bus, spécifié en tant que mots-clés  parmi ceux suivants:
``any``,
``ps2``,
``usb``,
``bluetooth``.

vendor

L'identificateur du vendeur, spécifié comme une entier non-signé 16-bit.

product

L'identificateur du produit, spécifié comme une entier non signé 16 bits.


Les identificateurs du vendeur et du produit peuvent être spécifiés en décimal
(pas de préfixe), octal (préfixé par ``0``), ou hexacécimal (préfixé par ``0x``).
La spécification de ``0`` signifie que cela correspond à toute valeur (comme si
la propriété n'était pas spécifiée).
