.. |product name| replace:: Raspbian Canute Controller
.. |document subject| replace:: |product name| Software

.. _BRLTTY: https://brltty.app/
.. _source repository: https://github.com/brltty/brltty
.. _bbt-canute branch: https://github.com/brltty/brltty/tree/bbt-canute

======================
The |document subject|
======================

The |document subject| is Copyright ©
2022
by Bristol Braille Technology CIC.

.. contents::

Installation and Upgrading
==========================

The source for the |document subject| is on
the `bbt-canute branch`_ of `BRLTTY`_'s `source repository`_.  

Files pertaining to the most recent stable release can be found at `<https://brltty.app/archive/Canute>`_.
Its commit identifier is in `<git-commit.txt>`_.
An archive of its build can be downloaded in the following formats:

  =====  ==============================
  bzip2  `<canute-controller.tar.bz2>`_
  gzip   `<canute-controller.tar.gz>`_
  xz     `<canute-controller.tar.xz>`_
  =====  ==============================

To install it for the first time,
download the `<canute-install>`_ script and run it as **root**.

After it has been installed, you can upgrade it by
running the `canute-upgrade`_ script as **root**.

Commands
========

The |document subject| provides several commands.
Each of them has a **-h** (help) option which shows its usage summary.

What follows is a lsit of these commands along with their associated usage summaries.

User Sessions
-------------

.. include:: canute-startx.help.rst
.. include:: canute-screen.help.rst
.. include:: canute-ttysize.help.rst

System Administration
---------------------

.. include:: canute-upgrade.help.rst
.. include:: canute-status.help.rst
.. include:: canute-start.help.rst
.. include:: canute-stop.help.rst
.. include:: canute-enable.help.rst
.. include:: canute-disable.help.rst

BrlAPI Parameter Management
---------------------------

.. include:: canute-table.help.rst
.. include:: canute-parameter.help.rst

BrlAPI Session Establishment
----------------------------

.. include:: canute-client.help.rst
.. include:: canute-java.help.rst
.. include:: canute-apitool.help.rst

Miscellaneous Commands
----------------------

.. include:: weather-current.help.rst

