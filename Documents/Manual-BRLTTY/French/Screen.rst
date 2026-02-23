.. _screen:

Pilotes d'écran supportés
=========================

BRLTTY supporte les pilotes d'écran suivants:


as

    AT-SPI

hd

    Ce pilote fournit un accès direct à l'écran d'une console Hurd.
    Il n'est  sélectionnable et par défaut oque sur les systèmes Hurd.

lx

    Ce pilote fournit un accès direct à l'écran d'une console Linux
    Il n'est  sélectionnable et par défaut oque sur les systèmes Linux.

sc

    Ce pilote fournit un accès direct au programme ``screen``.
    Vous pouvez le sélectionner sur tous les systèmes, et il l'est par défaut
    si aucun pilote d'écran d'origine n'est disponible. Vous devez appliquer le
    correctif de ``screen`` que nous fournissons (voir le sous-répertoire
    ``Patches`` ). Du fait que screen doit être exécuté simultanément,
    l'utilisation de ce pilote rend BRLTTY opérationnel uniquement après que
    l'utilisateur s'est identifié.

wn

    Ce pilote fournit un accès direct à l'écran d'une console Windows.
    Il n'est  sélectionnable et par défaut que sur les systèmes Windows/Cygwin.
